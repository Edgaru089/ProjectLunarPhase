#pragma once

#include "Main.hpp"


class HTTPResponseCodeStrings {
public:
	HTTPResponseCodeStrings() {
		//TODO All responses
		strings.insert(make_pair(200, "OK"));

		strings.insert(make_pair(301, "Moved Permanently"));
		strings.insert(make_pair(302, "Found"));

		strings.insert(make_pair(400, "Bad Request"));
		strings.insert(make_pair(401, "Unauthorized"));
		strings.insert(make_pair(403, "Forbidden"));
		strings.insert(make_pair(404, "Not Found"));
		strings.insert(make_pair(408, "Request Timeout"));
		strings.insert(make_pair(411, "Length Required"));

		strings.insert(make_pair(500, "Internal Server Error"));
		strings.insert(make_pair(503, "Service Unavailable"));
		strings.insert(make_pair(505, "HTTP Version Not Supported"));
	}

	string get(int code) {
		if (auto i = strings.find(code); i != strings.end())
			return i->second;
		else
			return empty;
	}

private:
	map<int, string> strings;
	string empty;
};

extern HTTPResponseCodeStrings responses;



class HTTPResponseWrapper :public HTTPResponse {
public:
	typedef shared_ptr<HTTPResponseWrapper> Ptr;
	virtual void send(TcpSocket::Ptr socket) = 0;
	virtual string what() const = 0;
};


class HTTPResponseShort :public HTTPResponseWrapper {
public:
	HTTPResponseShort() {}
	HTTPResponseShort(const wstring& body, bool hasFrame = true, const vector<pair<string, string>>& replaces = {})
		:hasFrame(hasFrame),replaces(replaces) {
		SetBody(wstringToUtf8(body));
	}
	HTTPResponseShort(const string& body, bool hasFrame = true, const vector<pair<string, string>>& replaces = {})
		:hasFrame(hasFrame), replaces(replaces) {
		SetBody(body);
	}

	string what() const override { return ToString(); }

	void send(TcpSocket::Ptr socket) override;
private:
	bool hasFrame;
	vector<pair<string, string>> replaces;
};


class HTTPResponseFile :public HTTPResponseWrapper {
public:

	HTTPResponseFile() {}
	HTTPResponseFile(const wstring& filename, const string& mimeType = "application/octet-stream") :
		filename(filename), mimeType(mimeType) {}

	bool isFileStreamOK() { return (bool)file; }

	string what() const override { return ToString() + "<File> Filename:" + wstringToUtf8(filename); }

	void send(TcpSocket::Ptr socket) override;

private:
	wstring filename;
	string mimeType;
	ifstream file;
};


class HTTPResponseTemplate :public HTTPResponseWrapper {
public:

	HTTPResponseTemplate() {}
	HTTPResponseTemplate(const wstring& filename, const vector<pair<string, string>>& replaces = {}, bool hasFrame = false, const string& mimeType = "text/html") :
		filename(filename), replaces(replaces), hasFrame(hasFrame), mimeType(mimeType) {}

	bool isFileStreamOK() { return (bool)file; }

	string what() const override { return ToString() + "\r\n>Filename:" + wstringToUtf8(filename); }

	void send(TcpSocket::Ptr socket) override;

private:
	bool hasFrame;
	wstring filename;
	vector<pair<string, string>> replaces;
	string mimeType;
	ifstream file;
};


class HTTPResponseError :public HTTPResponseWrapper {
public:

	HTTPResponseError() {}
	HTTPResponseError(int code) { this->code = code; }

	string what() const override { return "HTTP ERROR " + to_string(code) + ' ' + responses.get(code); }

	void send(TcpSocket::Ptr socket) override;

private:
	int code;
};


class HTTPResponseRedirection :public HTTPResponseWrapper {
public:

	HTTPResponseRedirection() {}
	// Code is either 301 Moved Permanently or 302 Found
	HTTPResponseRedirection(const string& target, int code = 301) { this->target = target; this->code = code; }

	string what() const override { return "HTTP Redirection " + to_string(code) + ' ' + responses.get(code) + "\r\nLocation: " + target; }

	void send(TcpSocket::Ptr socket) override;

private:
	string target;
	int code;
};

void setFrameFile(const wstring& filename, const string& bodyFlag = "%BODY%");

HTTPResponseWrapper::Ptr htmltext(const wstring& str, bool hasFrame = true, const vector<pair<string, string>>& replaces = {});
HTTPResponseWrapper::Ptr htmltext(const string& str, bool hasFrame = true, const vector<pair<string, string>>& replaces = {});

HTTPResponseWrapper::Ptr file(const wstring& filename, const string& mimeType = "application/octet-stream");
HTTPResponseWrapper::Ptr file(const string& filename, const string& mimeType = "application/octet-stream");

HTTPResponseWrapper::Ptr filetemplate(const wstring& filename, const vector<pair<string, string>>& replaces = {}, bool hasFrame = true, const string& mimeType = "text/html");
HTTPResponseWrapper::Ptr filetemplate(const string& filename, const vector<pair<string, string>>& replaces = {}, bool hasFrame = true, const string& mimeType = "text/html");

HTTPResponseWrapper::Ptr error(int code);

// Code is either 301 Moved Permanently or 302 Found
HTTPResponseWrapper::Ptr redirect(const wstring& target, int code = 301);
HTTPResponseWrapper::Ptr redirect(const string& target, int code = 301);


