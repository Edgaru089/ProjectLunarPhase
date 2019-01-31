#pragma once

#include <iostream>
#include <string>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <functional>
#include <vector>
#include <mutex>
#include "StringParser.hpp"
using namespace std;

#define AUTOLOCK(a) lock_guard<mutex> __mutex_lock(a)

#ifndef DISABLE_ALL_LOGS

class Log {
public:

	const wstring logLevelName[6] = { L"DEBUG", L"INFO", L"EVENT", L"WARN", L"ERROR", L"FATAL ERROR" };

	enum LogLevel {
		Debug,
		Info,
		Event,
		Warning,
		Error,
		FatalError
	};

	Log() :ignoreLevel(-1) {}
	Log(wostream& output) :out({ &output }), ignoreLevel(-1) {}
	Log(wostream* output) :out({ output }), ignoreLevel(-1) {}

	void log(const wstring& content, LogLevel level = Info) {
		if (level <= ignoreLevel) return;
		time_t curtime = time(NULL);
		wchar_t buffer[64] = {};
		wcsftime(buffer, 63, L"[%T", localtime(&curtime));
		wstring final = wstring(buffer) + L" " + logLevelName[level] + L"] " + content + L'\n';
		lock.lock();
		buffers.push_back(final);
		for (wostream* i : out) {
			(*i) << final;
			i->flush();
			if (!(*i))
				i->clear();
		}
		for (const auto& i : outf)
			i(final);
		lock.unlock();
	}

	template<typename... Args>
	void logf(LogLevel level, wstring format, Args... args) {
		wchar_t buf[2560];
		wsprintf(buf, format.c_str(), args...);
		log(wstring(buf), level);
	}

	void operator() (const wstring& content, LogLevel level = Info) {
		log(content, level);
	}

	void addOutputStream(wostream& output) { lock.lock(); out.push_back(&output); lock.unlock(); }
	void addOutputStream(wostream* output) { lock.lock(); out.push_back(output); lock.unlock(); }
	void addOutputHandler(function<void(const wstring&)> output) { lock.lock(); outf.push_back(output); lock.unlock(); }

	// Lower and equal; use -1 to ignore nothing
	void ignore(int level) { ignoreLevel = level; }
	int getIgnoreLevel() { return ignoreLevel; }

	const vector<wstring>& getBuffers() { return buffers; }
	void clearBuffer() { AUTOLOCK(lock); buffers.clear(); }

private:
	vector<wostream*> out;
	vector<function<void(const wstring&)>> outf;
	mutex lock;
	int ignoreLevel;

	vector<wstring> buffers;
};

extern Log dlog;

class LogMessage {
public:

	LogMessage() :level(Log::Info) {}
	LogMessage(Log::LogLevel level) :level(level) {}

	LogMessage& operator <<(bool                data) { buffer += data ? L"true" : L"false"; return *this; }
	LogMessage& operator <<(char                data) { buffer += (data); return *this; }
	LogMessage& operator <<(unsigned char       data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(short               data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(unsigned short      data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(int                 data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(unsigned int        data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(long                data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(unsigned long       data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(long long           data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(unsigned long long  data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(float               data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(double              data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(long double         data) { buffer += to_wstring(data); return *this; }
	LogMessage& operator <<(const wchar_t*      data) { buffer += wstring(data); return *this; }
	LogMessage& operator <<(const char*         data) { buffer += utf8ToWstring(data); return *this; }
	LogMessage& operator <<(const wstring&      data) { buffer += data; return *this; }
	LogMessage& operator <<(const string&       data) { buffer += utf8ToWstring(data); return *this; }

	LogMessage& operator <<(Log::LogLevel      level) { this->level = level;  return *this; }
	LogMessage& operator <<(Log&                 log) { flush(log);  return *this; }

public:

	void setLevel(Log::LogLevel level) { this->level = level; }
	void flush(Log& log) { logout(log); clear(); }
	void logout(Log& log) { log(buffer, level); }
	void clear() { buffer.clear(); }

private:
	wstring buffer;
	Log::LogLevel level;
};

#define mlog LogMessage()
#define mloge LogMessage(Log::Event)
#define mlogd LogMessage(Log::Debug)

#else

class Log {
public:
	enum LogLevel {
		Debug,
		Info,
		Event,
		Warning,
		Error,
		FatalError
	};
	Log() {}
	Log(wostream& output) {}
	Log(wostream* output) {}
	void log(const wstring&, LogLevel) {}
	template<typename... Args>
	void logf(LogLevel, wstring, Args...) {}
	void operator() (const wstring&, LogLevel level) {}
	void addOutputStream(wostream&) {}
	void addOutputStream(wostream*) {}
	void addOutputHandler(function<void(const wstring&)>) {}
	void ignore(int) {}
	int getIgnoreLevel() { return -1; }
	const vector<wstring>& getBuffers() {}
	void clearBuffer() {}
};

class LogMessage {
public:
	LogMessage() {}
	LogMessage(Log::LogLevel level) {}
	LogMessage& operator <<(bool) { return *this; }
	LogMessage& operator <<(char) { return *this; }
	LogMessage& operator <<(unsigned char) { return *this; }
	LogMessage& operator <<(short) { return *this; }
	LogMessage& operator <<(unsigned short) { return *this; }
	LogMessage& operator <<(int) { return *this; }
	LogMessage& operator <<(unsigned int) { return *this; }
	LogMessage& operator <<(long long) { return *this; }
	LogMessage& operator <<(unsigned long long) { return *this; }
	LogMessage& operator <<(float) { return *this; }
	LogMessage& operator <<(double) { return *this; }
	LogMessage& operator <<(const wchar_t*) { return *this; }
	LogMessage& operator <<(const char*) { return *this; }
	LogMessage& operator <<(const wstring&) { return *this; }
	LogMessage& operator <<(const string&) { return *this; }
	LogMessage& operator <<(Log::LogLevel) { return *this; }
	LogMessage& operator <<(Log&) { return *this; }
public:
	void setLevel(Log::LogLevel) {}
	void flush(Log&) {}
	void logout(Log&) {}
	void clear() {}
};

#ifdef USE_WCOUT_AS_LOG

#define mlog (wcout << L"[INFO] ")
#define mloge (wcout << L"[EVENT] ")
#define mlogd (wcout << L"DEBUG ")
#define dlog endl

inline wostream& operator << (wostream& out, const string& str) { out << utf8ToWstring(str); return out; }

#else

extern Log dlog;

#define mlog LogMessage()
#define mloge LogMessage(Log::Event)
#define mlogd LogMessage(Log::Debug)

#endif

#endif
