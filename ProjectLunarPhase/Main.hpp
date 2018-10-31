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
#include <SFNUL.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace std;
using namespace sfn;

#include "Version.hpp"


#ifdef _WIN32
string wstringToUtf8(const wstring& source);
wstring utf8ToWstring(const string& source);
wstring ansiToWstring(const string& source);
#elif
#pragma error("I'm too lazy to implement a cross-platform UTF-8 <-> wchar_t convertion")
#endif

string decodePercentEncoding(const string& source);
string encodePercent(const string& source, bool encodeSlash = false);
map<string, string> decodeFormUrlEncoded(string body);
string encodeCookieSequence(const vector<pair<string, string>>& cookies);
map<string, string> decodeCookieSequence(string body);

string readFileBinary(const wstring& filename);
string readFileBinaryCached(const wstring& filename);

string toUppercase(const string& str);

string generateCookie(int length = 24);

//#define DISABLE_ALL_LOGS
//#define USE_WCOUT_AS_LOG
#include "LogSystem.hpp"

