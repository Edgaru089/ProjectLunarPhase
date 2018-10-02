#pragma once

#include "Main.hpp"


namespace {

	string readNextWord(const string& str, size_t& offset) {
		string res;
		while (offset < str.length() && (isspace(str[offset]) || iscntrl(str[offset])))
			offset++;
		while (offset < str.length() && !(isspace(str[offset]) || iscntrl(str[offset])))
			res.push_back(str[offset++]);
		return res;
	}

	string readFromFirstCharToEndline(const string& str, size_t& offset) {
		string res;
		while (offset < str.length() && isspace(str[offset]) || iscntrl(str[offset]))
			offset++;
		while (offset < str.length() && (str[offset] != '\r'&&str[offset] != '\n'))
			res.push_back(str[offset++]);
		while (str[offset] == '\r' || str[offset] == '\n')
			offset++;
		return res;
	}

}



class HTTPParser {
public:

	class BadRequestException :public exception {
		char const* what() const override { return "HTTP/400 Bad Request"; }
	};

	static HTTPRequest parseRequest(string header) {
		HTTPRequest result;

		size_t headerlen = header.find("\r\n\r\n");
		if (headerlen == string::npos)
			throw BadRequestException();

		string body = header.substr(headerlen + 4);
		header.resize(headerlen);

		size_t off = 0;
		result.SetMethod(readNextWord(header, off));
		result.SetURI(readNextWord(header, off));
		if (readNextWord(header, off) != "HTTP/1.1")
			throw BadRequestException();

		string cur;
		while (!(cur = readNextWord(header, off)).empty()) {
			if (cur.back() == ':')
				result.SetHeaderValue(cur.substr(0, cur.length() - 1), readFromFirstCharToEndline(header, off));
			else
				throw BadRequestException();
		}

		return result;
	}

};

