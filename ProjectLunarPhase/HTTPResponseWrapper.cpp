
#include "HTTPResponseWrapper.hpp"
#include "StringParser.hpp"
#include "HTTPParser.hpp"

HTTPStringData httpdata;

namespace {
	string frameFileContents;
	string frameBodyFlag;

	string readNextWord(const string& str, size_t& offset);
}


HTTPStringData::HTTPStringData() {
	//TODO All responses
	strings.insert(make_pair(200, "OK"));

	strings.insert(make_pair(301, "Moved Permanently"));
	strings.insert(make_pair(302, "Found"));
	strings.insert(make_pair(303, "See Other"));
	strings.insert(make_pair(307, "Temporary Redirect"));

	strings.insert(make_pair(400, "Bad Request"));
	strings.insert(make_pair(401, "Unauthorized"));
	strings.insert(make_pair(403, "Forbidden"));
	strings.insert(make_pair(404, "Not Found"));
	strings.insert(make_pair(408, "Request Timeout"));
	strings.insert(make_pair(411, "Length Required"));

	strings.insert(make_pair(500, "Internal Server Error"));
	strings.insert(make_pair(503, "Service Unavailable"));
	strings.insert(make_pair(505, "HTTP Version Not Supported"));

	// Load MIME types
	string mimefile = readFileBinary(L"mime.types"), word;
	size_t off = 0;
	while (!(word = readNextWord(mimefile, off)).empty()) {
		string type = word;
		while ((word = readNextWord(mimefile, off)).back() != ';')
			mimes.insert(make_pair(word, type));
		mimes.insert(make_pair(word.substr(0, word.size() - 1), type));
	}
}


void HTTPResponseShort::send(TcpSocket::Ptr socket) {
	SetHTTPVersion("HTTP/1.1");
	SetStatus("200 OK");
	SetHeaderValue("Server", completeServerName);
	SetHeaderValue("Content-Type", "text/html");
	SetHeaderValue("Connection", "keep-alive");

	if (hasFrame && !frameFileContents.empty())
		SetBody(StringParser::replaceSubString(frameFileContents, { { frameBodyFlag, GetBody() } }));
	if (!replaces.empty())
		SetBody(StringParser::replaceSubString(GetBody(), replaces));

	SetHeaderValue("Content-Length", to_string(GetBody().size()));
	SetBodyComplete();
	SetHeaderComplete();
	string data = ToString();
	socket->Send(data.data(), data.length());
}


void HTTPResponseFile::send(TcpSocket::Ptr socket) {
	file.open(filename, ifstream::in | ifstream::binary);

	// Return 404 Not Found if the stream was not opened
	if (!file) {
		file.close();
		HTTPResponseError(404).send(socket);
		return;
	}

	// Tell the size of the file
	file.ignore(numeric_limits<streamsize>::max());
	size_t fileSize = (size_t)file.gcount();
	file.seekg(0, ifstream::beg);
	mlog << "Filename: " << filename << ", Size: " << fileSize << dlog;

	SetHTTPVersion("HTTP/1.1");
	SetStatus("200 OK");
	SetHeaderValue("Server", completeServerName);
	SetHeaderValue("Content-Type", mimeType.empty() ? httpdata.getMIMEString(wstringToUtf8(filename.substr(filename.find_last_of(L'.') + 1))) : mimeType);
	SetHeaderValue("Content-Length", to_string(fileSize));
	SetHeaderValue("Connection", "keep-alive");
	SetBody("");
	SetBodyComplete();
	SetHeaderComplete();

	// Send header
	string data = ToString();
	socket->Send(data.data(), data.length());

	// Send the file
	constexpr size_t maxBufferSize = 1024 * 1024; // 1M buffer
	SetMaximumBlockSize(max(2.5*maxBufferSize, 2.2*fileSize));
	size_t bufferSize = min(maxBufferSize, fileSize);
	char* buffer = new char[bufferSize];

	while (!file.eof()) {
		file.read(buffer, bufferSize);
		if (file.gcount() > 0)
			while (!(socket->LocalHasShutdown() || socket->RemoteHasShutdown()) && !socket->Send(buffer, file.gcount()))
				this_thread::sleep_for(chrono::milliseconds(5));
	}

	delete[] buffer;

	file.close();
}

void HTTPResponseTemplate::send(TcpSocket::Ptr socket) {
	// As a template, we have to read all of the file
	string body = readFileBinaryCached(filename);

	// Return 404 Not Found if the file isn't valid
	if (body.empty()) {
		HTTPResponseError(404).send(socket);
		return;
	}

	// Process the tempate
	// TODO This is quite a dumb heuristic; try implementing a new one?
	if (hasFrame && !frameFileContents.empty())
		body = StringParser::replaceSubString(frameFileContents, { { frameBodyFlag, body } });
	body = StringParser::replaceSubString(body, replaces);

	SetHTTPVersion("HTTP/1.1");
	SetStatus("200 OK");
	SetHeaderValue("Server", completeServerName);
	SetHeaderValue("Content-Type", mimeType);
	SetHeaderValue("Content-Length", to_string(body.size()));
	SetHeaderValue("Connection", "keep-alive");
	SetBody(body);
	SetBodyComplete();
	SetHeaderComplete();

	// Send the header and body
	string data = ToString();
	socket->Send(data.data(), data.length());
}


void HTTPResponseError::send(TcpSocket::Ptr socket) {
	// Create a error response
	char data[] =
		"HTTP/1.1 %STR%\r\n"
		"Server: Project-LunarPhase\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n\r\n"
		"<html>\r\n"
		"<head><title>%STR%</title></head>\r\n"
		"<body bgcolor=\"white\">\r\n"
		"<center><h1>%STR%</h1></center>\r\n"
		"<hr><center>Project LunarPhase Test Server</center>\r\n"
		"</body>\r\n"
		"</html>\r\n";
	// Replace the error string
	string realData = StringParser::replaceSubString(data, { { "%STR%", to_string(code) + ' ' + httpdata.getResponseString(code) } });

	// Send it and shutdown the connection
	socket->Send(realData.data(), realData.length());
	socket->Shutdown();
}


void HTTPResponseRedirection::send(TcpSocket::Ptr socket) {
	// Create an analogous redirect response
	char header[] =
		"HTTP/1.1 %STR%\r\n"
		"Server: Project-LunarPhase\r\n"
		"Content-Type: text/html\r\n"
		"Connection: keep-alive\r\n"
		"Location: %LOC%\r\n"
		"%COOKIE%"
		"Content-Length: %LEN%\r\n\r\n",
		body[] =
		"<html>\r\n"
		"<head><title>%STR%</title></head>\r\n"
		"<body bgcolor=\"white\">\r\n"
		"<center><h1>%STR%</h1></center>\r\n"
		"<hr><center>Project LunarPhase Test Server</center>\r\n"
		"</body>\r\n"
		"</html>\r\n";
	// Replace the noififcation string
	string realBody = StringParser::replaceSubString(body, {
		{ "%STR%", to_string(code) + ' ' + httpdata.getResponseString(code) } });
	string realHeader = StringParser::replaceSubString(header, {
		{ "%STR%", to_string(code) + ' ' + httpdata.getResponseString(code) }
		, { "%LOC%", encodePercent(target) }, { "%LEN%", to_string(realBody.size()) }
	, { "%COOKIE%", cookies.empty() ? "" : ("Set-Cookie: " + encodeCookieSequence(cookies) + "\r\n") } });

	// Send the header and the body (without closing the connection)
	socket->Send(realHeader.data(), realHeader.length());
	socket->Send(realBody.data(), realBody.length());
}


void setFrameFile(const wstring& filename, const string& bodyFlag) {
	frameFileContents = readFileBinaryCached(filename);
	frameBodyFlag = bodyFlag;
}

