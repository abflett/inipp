/*
MIT License

Copyright (c) 2017 Matthias C. M. Troffaes

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <cctype>
#include <sstream>

namespace inipp {

// trim functions based on http://stackoverflow.com/a/217605

template <class CharT>
static inline void ltrim(std::basic_string<CharT> & s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		std::not1(std::ptr_fun<int, int>(std::isspace))));
}

template <class CharT>
static inline void rtrim(std::basic_string<CharT> & s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// string replacement function based on http://stackoverflow.com/a/3418285

template <class CharT>
static inline void replace(std::basic_string<CharT> & str, const std::basic_string<CharT> & from, const std::basic_string<CharT> & to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::basic_string<CharT>::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

// template based string literals based on http://stackoverflow.com/a/32845111
// cannot declare constexpr due to MSVC limitations

template <typename CharT>
static inline std::basic_string<CharT> literal(const char *value)
{
	std::basic_string<CharT> result{};
	std::size_t length = std::strlen(value);
	result.reserve(length);
	for (int i=0; i < length; i++) result.push_back((CharT)value[i]);
	return result;
}

template <typename CharT, typename T>
inline bool extract(const std::basic_string<CharT> & value, T & dst) {
	CharT c;
	std::basic_istringstream<CharT> is{ value };
	return bool{ is >> std::boolalpha >> dst && !(is >> c) };
}

template <typename CharT>
inline bool extract(const std::basic_string<CharT> & value, std::basic_string<CharT> & dst) {
	dst = value;
	return true;
}

template<class CharT>
class Ini
{
public:
	typedef std::basic_string<CharT> String;
	typedef std::map<String, String> Section;
	typedef std::map<String, Section> Sections;

	Sections sections;
	std::list<String> errors;

	static const CharT char_section_start  = (CharT)'[';
	static const CharT char_section_end    = (CharT)']';
	static const CharT char_assign         = (CharT)'=';
	static const CharT char_comment        = (CharT)';';
	static const CharT char_interpol       = (CharT)'%';
	static const CharT char_interpol_start = (CharT)'(';
	static const CharT char_interpol_end   = (CharT)')';
	const std::basic_string<CharT> default_section_name;

	Ini()
		: sections(), errors()
		, default_section_name(literal<CharT>("DEFAULT")) {};

	void generate(std::basic_ostream<CharT> & os) {
		for (auto const & sec : sections) {
			os << char_section_start << sec.first << char_section_end << std::endl;
			for (auto const & val : sec.second) {
				os << val.first << char_assign << val.second << std::endl;
			}
		}
	}

	void parse(std::basic_istream<CharT> & is) {
		String line;
		String section;
		while (!is.eof()) {
			std::getline(is, line);
			ltrim(line);
			rtrim(line);
			auto length = line.length();
			if (length > 0) {
				const auto pos = line.find_first_of(char_assign);
				const auto & front = line.front();
				if (front == char_comment) {
					continue;
				}
				else if (front == char_section_start) {
					if (line.back() == char_section_end)
						section = line.substr(1, length - 2);
				}
				else if (pos != String::npos) {
					String variable(line.substr(0, pos));
					String value(line.substr(pos + 1, length));
					rtrim(variable);
					ltrim(value);
					sections[section][variable] = value;
				}
				else {
					errors.push_back(line);
				}
			}
		}
	}

	void interpolate(const Section & src, Section & dst) const {
		for (auto & srcval : src) {
			for (auto & val : dst) {
				if (val != srcval) {
					replace(val.second, char_interpol + (char_interpol_start + srcval.first + char_interpol_end), srcval.second);
				}
			}
		}
	}

	void interpolate() {
		auto defsec = sections.find(default_section_name);
		if (defsec != sections.end())
			interpolate(defsec->second, defsec->second);
		for (auto & sec : sections) {
			if (sec.first != default_section_name) {
				interpolate(sec.second, sec.second);
				if (defsec != sections.end())
					interpolate(defsec->second, sec.second);
			}
		}
	}

	void clear() {
		sections.clear();
		errors.clear();
	}
};

} // namespace inipp
