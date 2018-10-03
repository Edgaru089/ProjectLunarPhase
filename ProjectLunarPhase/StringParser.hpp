#pragma once

#include <cstdio>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

class StringParser {
public:

	template<typename... Args>
	static const string toStringF(const string& format, Args... args) {
		char buffer[8192];
		sprintf(buffer, format.c_str(), args...);
		return string(buffer);
	}
	template<typename... Args>
	static const wstring toStringFW(const wstring& format, Args... args) {
		wchar_t buffer[8192];
		wsprintf(buffer, format.c_str(), args...);
		return wstring(buffer);
	}

	template<typename Char = char>
	static const basic_string<Char> replaceSubStringX(basic_string<Char> source, const vector<pair<basic_string<Char>, basic_string<Char>>>& replaces) {
		basic_string<Char> dest;

		for (auto&&[target, replaced] : replaces) {
			dest.clear();
			for (int j = 0; j < source.size();) {
				if (source.substr(j, target.size()) == target) {
					j += target.size();
					dest += replaced;
				}
				else {
					dest += source[j];
					j++;
				}
			}
			source = dest;
		}
		return source;
	}

	static const string replaceSubString(const string& source, const vector<pair<string, string>>& replaces) { return replaceSubStringX<char>(source, replaces); }
	static const wstring replaceSubStringW(const wstring& source, const vector<pair<wstring, wstring>>& replaces) { return replaceSubStringX<wchar_t>(source, replaces); }

	template<typename Type>
	const Type toValue(const string& data) { return Type(); }
	template<> const bool toValue<bool>(const string& data) { return toBool(data); }
	template<> const short toValue<short>(const string& data) { return toShort(data); }
	template<> const int toValue<int>(const string& data) { return toInt(data); }
	template<> const long long toValue<long long>(const string& data) { return toLongLong(data); }
	template<> const float toValue<float>(const string& data) { return toFloat(data); }
	template<> const double toValue<double>(const string& data) { return toDouble(data); }
	template<> const string toValue<string>(const string& data) { return data; }

	static const bool      toBool(const string&     data) { int x;       sscanf(data.c_str(), "%d", &x);   return x; }
	static const short     toShort(const string&    data) { int x;       sscanf(data.c_str(), "%d", &x);   return x; }
	static const int       toInt(const string&      data) { int x;       sscanf(data.c_str(), "%d", &x);   return x; }
	static const long long toLongLong(const string& data) { long long x; sscanf(data.c_str(), "%lld", &x); return x; }
	static const float     toFloat(const string&    data) { float x;     sscanf(data.c_str(), "%f", &x);   return x; }
	static const double    toDouble(const string&   data) { double x;    sscanf(data.c_str(), "%lf", &x);  return x; }

};
