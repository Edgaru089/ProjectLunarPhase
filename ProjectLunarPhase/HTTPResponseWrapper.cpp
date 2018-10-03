
#include "HTTPResponseWrapper.hpp"
#include "StringParser.hpp"
#include "HTTPParser.hpp"

HTTPResponseCodeStrings responses;

namespace {
	string frameFileContents;
	string frameBodyFlag;
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

	//TODO MIME Types
	SetHTTPVersion("HTTP/1.1");
	SetStatus("200 OK");
	SetHeaderValue("Server", completeServerName);
	SetHeaderValue("Content-Type", mimeType);
	SetHeaderValue("Content-Length", to_string(fileSize));
	SetHeaderValue("Connection", "keep-alive");
	SetBody("");
	SetBodyComplete();
	SetHeaderComplete();

	// Send header
	string data = ToString();
	socket->Send(data.data(), data.length());

	// Send the file
	constexpr size_t maxBufferSize = 512 * 1024; // 512K buffer
	SetMaximumBlockSize(1.2*maxBufferSize);
	size_t bufferSize = min(maxBufferSize, fileSize);
	char* buffer = new char[bufferSize];

	while (!file.eof()) {
		file.read(buffer, bufferSize);
		if (file.gcount() > 0)
			while (!(socket->LocalHasShutdown() || socket->RemoteHasShutdown()) && !socket->Send(buffer, file.gcount()))
				this_thread::sleep_for(chrono::milliseconds(50));
	}

	delete[] buffer;

	file.close();
}

void HTTPResponseTemplate::send(TcpSocket::Ptr socket) {
	file.open(filename, ifstream::in);

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
	mlog << "Template Filename: " << filename << ", Size: " << fileSize << dlog;

	// As a template, we have to read all of the file
	string body(fileSize, '\0');
	file.read(body.data(), fileSize);

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

	file.close();
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
	string realData = StringParser::replaceSubString(data, { { "%STR%", to_string(code) + ' ' + responses.get(code) } });

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
		{ "%STR%", to_string(code) + ' ' + responses.get(code) } });
	string realHeader = StringParser::replaceSubString(header, {
		{ "%STR%", to_string(code) + ' ' + responses.get(code) }
		, { "%LOC%", encodePercent(target) }, { "%LEN%", to_string(realBody.size()) }
	, { "%COOKIE%", cookies.empty() ? "" : ("Set-Cookie: " + encodeCookieSequence(cookies) + "\r\n") } });

	// Send the header and the body (without closing the connection)
	socket->Send(realHeader.data(), realHeader.length());
	socket->Send(realBody.data(), realBody.length());
}


void setFrameFile(const wstring& filename, const string& bodyFlag) {
	frameFileContents = readFileBinary(filename);
	frameBodyFlag = bodyFlag;
}

