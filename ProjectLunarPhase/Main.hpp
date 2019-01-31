#pragma once

#pragma warning(disable: 4018 4244 4003)
// C4018: "binary-operator": signed/unsigned mismatch
// C4244: convertion from "type" to "type", possible loss of data
// C4003: persudo-function macro call "macro-name" no enough paramaters

#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <cctype>
#include <mutex>
#include <regex>
#include <chrono>
#include <random>
#include <map>
#include <atomic>
#include <list>
#include <climits>
#include "SFNUL/HTTP.hpp"
#include <SFML/System.hpp>
#include <SFML/Network.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace std;
using namespace sf;
using namespace sfn;

#if (defined LOCAL) || (defined USE_DEBUG)
#define DEBUG(...) printf(__VA_ARGS__)
#define PRINTARR(formatstr, arr, beginoff, size)				\
do{printf(#arr ":");											\
for (int __i = beginoff; __i <= beginoff + size - 1; __i++)		\
	printf("\t%d", __i);										\
printf("\n");													\
for (int __i = beginoff; __i <= beginoff + size - 1; __i++)		\
	printf("\t" formatstr, arr[__i]);							\
printf("\n"); }while(false)
#define PASS printf("Passing function \"%s\" on line %d\n", __func__, __LINE__)
#define ASSERT(expr) do{\
	if(!(expr)){\
		printf("Debug Assertation Failed on line %d, in function %s:\n  Expression: %s\n",__LINE__,__func__,#expr);\
		abort();\
	}\
}while(false)
#else
#define DEBUG(...)
#define PRINTARR(a, b, c, d)
#define PASS
#define ASSERT(expr)
#endif

// ifstream::open(const wstring& path, flags) is a Windows extension, not a standard
#if (defined _WIN32) || (defined WIN32)
#define OPEN_FSTREAM_WSTR(stream, wstrpath, flags) stream.open(wstrpath, flags);
#else
#define OPEN_FSTREAM_WSTR(stream, wstrpath, flags) stream.open(wstringToUtf8(wstrpath), flags);
#endif

#include "Version.hpp"

string wstringToUtf8(const wstring& source);
wstring utf8ToWstring(const string& source);
wstring ansiToWstring(const string& source);

string decodePercentEncoding(const string& source);
string encodePercent(const string& source, bool encodeSlash = false);
map<string, string> decodeFormUrlEncoded(string body);
string encodeCookieSequence(const vector<pair<string, string>>& cookies);
map<string, string> decodeCookieSequence(string body);

string readFileBinary(const wstring& filename);
string readFileBinaryCached(const wstring& filename);

string toUppercase(const string& str);

string generateCookie(int length = 24);

class NotImplementedException :public exception {
public:
	NotImplementedException() { contents = "NotImplementedException: an unknown function not implemented"; }
	NotImplementedException(const string& funcName) { contents = "NotImplementedException: function \"" + funcName + "\" not implemented"; }
	const char* what() const noexcept override { return contents.c_str(); }
private:
	string contents;
};

//#define DISABLE_ALL_LOGS
//#define USE_WCOUT_AS_LOG
#include "LogSystem.hpp"

#ifndef _WIN32

#include <signal.h>

#endif
