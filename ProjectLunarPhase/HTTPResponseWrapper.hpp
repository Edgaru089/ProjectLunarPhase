#pragma once

#include "Main.hpp"


class HTTPStringData {
public:
	HTTPStringData();

	string getResponseString(int code) {
		if (auto i = strings.find(code); i != strings.end())
			return i->second;
		else
			return empty;
	}

	string getMIMEString(const string& str) {
		if (auto i = mimes.find(str); i != mimes.end())
			return i->second;
		else
			return "application/octet-stream";
	}

private:
	map<int, string> strings;
	map<string, string> mimes;
	string empty;
};

extern HTTPStringData httpdata;



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
		:hasFrame(hasFrame), replaces(replaces) {
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

	string what() const override { return "HTTP ERROR " + to_string(code) + ' ' + httpdata.getResponseString(code); }

	void send(TcpSocket::Ptr socket) override;

private:
	int code;
};


class HTTPResponseRedirection :public HTTPResponseWrapper {
public:

	HTTPResponseRedirection() {}
	// Code is either 301 Moved Permanently or 302 Found
	HTTPResponseRedirection(const string& target, int code = 301, const vector<pair<string, string>>& cookies = {}) :
		target(target), cookies(cookies), code(code) {}

	string what() const override { return "HTTP Redirection " + to_string(code) + ' ' + httpdata.getResponseString(code) + "\r\nLocation: " + target; }

	void send(TcpSocket::Ptr socket) override;

private:
	string target;
	vector<pair<string, string>> cookies;
	int code;
};

void setFrameFile(const wstring& filename, const string& bodyFlag = "%BODY%");

inline HTTPResponseWrapper::Ptr htmltext(const wstring& str, bool hasFrame = true, const vector<pair<string, string>>& replaces = {})
{ return make_shared<HTTPResponseShort>(str, hasFrame, replaces); }
inline HTTPResponseWrapper::Ptr htmltext(const string& str, bool hasFrame = true, const vector<pair<string, string>>& replaces = {})
{ return make_shared<HTTPResponseShort>(str, hasFrame, replaces); }

inline HTTPResponseWrapper::Ptr file(const wstring& filename, const string& mimeType = string())
{ return make_shared<HTTPResponseFile>(filename, mimeType); }
inline HTTPResponseWrapper::Ptr file(const string& filename, const string& mimeType = string())
{ return make_shared<HTTPResponseFile>(utf8ToWstring(filename), mimeType); }

inline HTTPResponseWrapper::Ptr filetemplate(const wstring& filename, const vector<pair<string, string>>& replaces = {}, bool hasFrame = true, const string& mimeType = "text/html")
{ return make_shared<HTTPResponseTemplate>(filename, replaces, hasFrame, mimeType); }
inline HTTPResponseWrapper::Ptr filetemplate(const string& filename, const vector<pair<string, string>>& replaces = {}, bool hasFrame = true, const string& mimeType = "text/html")
{ return make_shared<HTTPResponseTemplate>(utf8ToWstring(filename), replaces, hasFrame, mimeType); }

inline HTTPResponseWrapper::Ptr error(int code) { return make_shared<HTTPResponseError>(code); }

// Code is one of 301 Moved Permanently, 302 Found, 303 See Other and 307 Temporary Redirect
// std::string sequence is encoded in UTF-8
// Note that when encoding an cookie, encodePercent is not used, i.e.,
// one cannot use any speical symbols and Unicode characters
inline HTTPResponseWrapper::Ptr redirect(const wstring& target, int code = 301, const vector<pair<string, string>>& cookies = {}) { return make_shared<HTTPResponseRedirection>(wstringToUtf8(target), code, cookies); }
inline HTTPResponseWrapper::Ptr redirect(const string& target, int code = 301, const vector<pair<string, string>>& cookies = {}) { return make_shared<HTTPResponseRedirection>(target, code, cookies); }

