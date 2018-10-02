#pragma once

#include <cstdio>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

class StringParser {
public:

	template<typename... Args>
	static const string toStringF(string format, Args... args) {
		char buffer[8192];
		sprintf(buffer, format.c_str(), args...);
		return string(buffer);
	}

	static const string replaceSubString(string source, const vector<pair<string, string>>& replace) {
		string dest;

		for (auto&&[target, replaced] : replace) {
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
