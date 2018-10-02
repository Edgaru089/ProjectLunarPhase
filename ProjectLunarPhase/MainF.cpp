
#include "Main.hpp"


namespace {

	char decodeHex2(char high, char low) {
		return (unsigned char)((((high >= 'A') ? (high - 'A' + 10) : (high - '0')) << 4) | ((low >= 'A') ? (low - 'A' + 10) : (low - '0')));
	}

	char encodeHex1(unsigned char c) {
		return (c >= 0xa) ? ('A' + c - 0xa) : ('0' + c);
	}

	pair<char, char> encodeHex2(unsigned char c) {
		return make_pair(encodeHex1((c & 0xf0) >> 4),
						 encodeHex1(c & 0xf));
	}

}

#ifdef _WIN32

string wstringToUtf8(const wstring& source, size_t bufferSize) {
	vector<char> buffer(bufferSize);
	buffer[WideCharToMultiByte(CP_UTF8, 0, source.data(), source.size(), buffer.data(), bufferSize, NULL, NULL)] = 0;
	return string(buffer.data());
}

wstring utf8ToWstring(const string& source, size_t bufferSize) {
	vector<wchar_t> buffer(bufferSize);
	buffer[MultiByteToWideChar(CP_UTF8, 0, source.data(), source.size(), buffer.data(), bufferSize)] = 0;
	return wstring(buffer.data());
}

wstring ansiToWstring(const string& source, size_t bufferSize) {
	vector<wchar_t> buffer(bufferSize);
	buffer[MultiByteToWideChar(CP_ACP, 0, source.data(), source.size(), buffer.data(), bufferSize)] = 0;
	return wstring(buffer.data());
}

#elif
#pragma error("I'm too lazy to implement a cross-platform UTF-8 <-> wchar_t convertion")
#endif

string decodePercentEncoding(const string& source) {
	string res;
	int i = 0;
	while (i < source.length()) {
		if (source[i++] == '%') {
			char c1 = source[i++];
			char c2 = source[i++];
			res.push_back(decodeHex2(c1, c2));
		}
		else if (source[i - 1] == '+')
			res.push_back(' ');
		else
			res.push_back(source[i - 1]);
	}
	return res;
}

string encodePercent(const string& source, bool encodeSlash) {
	string res;
	for (unsigned char i : source) {
		if (isalnum(i) || i == '.' || i == '-' || i == '_' || i == '~' || (!encodeSlash&&i == '/'))
			res.push_back(i);
		else {
			res.push_back('%');
			auto&&[c1, c2] = encodeHex2(i);
			res.push_back(c1);
			res.push_back(c2);
		}
	}
	return res;
}

vector<pair<string, string>> decodeFormUrlEncoded(string body) {
	vector<pair<string, string>> ans;

	int i = 0;
	while (i < body.size()) {
		pair<string, string> cur;
		while (body[i] == '&') i++;
		while (body[i] != '=')
			cur.first.push_back(body[i++]);
		while (body[i] == '=') i++;
		while (i < body.size() && body[i] != '&')
			cur.second.push_back(body[i++]);
		ans.push_back(make_pair(decodePercentEncoding(cur.first), decodePercentEncoding(cur.second)));
	}

	return ans;
}

string readFileBinary(const wstring& filename) {
	ifstream file(filename);
	if (!file)
		return string();
	file.ignore(numeric_limits<streamsize>::max());
	size_t fileSize = (size_t)file.gcount();
	file.seekg(0, ifstream::beg);
	string res(fileSize, '\0');
	file.read(res.data(), fileSize);
	return res;
}

string toUppercase(const string& str) {
	string ans;
	for (char c : str) {
		if (c > 0)
			ans.push_back(toupper(c));
		else
			ans.push_back(c);
	}
	return ans;
}

