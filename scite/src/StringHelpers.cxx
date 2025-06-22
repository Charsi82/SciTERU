// SciTE - Scintilla based Text Editor
/** @file StringHelpers.cxx
 ** Implementation of widely useful string functions.
 **/
// Copyright 2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <stdexcept>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <algorithm>
#include <functional>
#include <chrono>

#include "GUI.h"
#include "StringHelpers.h"
#include "UnicodeConstants.h"

namespace {

/**
 * Is the character an octal digit?
 */
constexpr bool IsOctalDigit(char ch) noexcept {
	return ch >= '0' && ch <= '7';
}

constexpr int octalBase = 8;
constexpr int decimalBase = 10;

}

bool Contains(std::string const &s, char ch) noexcept {
	return s.find(ch) != std::string::npos;
}

bool isprefix(const char *target, const char *prefix) noexcept {
	while (*target && *prefix) {
		if (*target != *prefix)
			return false;
		target++;
		prefix++;
	}
	return !*prefix;
}

int Substitute(std::wstring &s, std::wstring_view sFind, std::wstring_view sReplace) {
	int c = 0;
	const size_t lenFind = sFind.size();
	const size_t lenReplace = sReplace.size();
	size_t posFound = s.find(sFind);
	while (posFound != std::wstring::npos) {
		s.replace(posFound, lenFind, sReplace);
		posFound = s.find(sFind, posFound + lenReplace);
		c++;
	}
	return c;
}

int Substitute(std::string &s, std::string_view sFind, std::string_view sReplace) {
	int c = 0;
	const size_t lenFind = sFind.size();
	const size_t lenReplace = sReplace.size();
	size_t posFound = s.find(sFind);
	while (posFound != std::string::npos) {
		s.replace(posFound, lenFind, sReplace);
		posFound = s.find(sFind, posFound + lenReplace);
		c++;
	}
	return c;
}

bool RemoveStringOnce(std::string &s, const char *marker) {
	const size_t modText = s.find(marker);
	if (modText != std::string::npos) {
		s.erase(modText, strlen(marker));
		return true;
	}
	return false;
}

void Trim(std::string &s) {
	size_t prefixLength = 0;
	while (prefixLength < s.length() && IsSpaceOrTab(s[prefixLength])) {
		prefixLength++;
	}
	s.erase(0, prefixLength);
	while (!s.empty() && IsSpaceOrTab(s.back())) {
		s.pop_back();
	}
}

// Remove terminating \r, \n, or \r\n when present.
void StripEOL(std::string &s) {
	if (s.ends_with("\r\n")) {
		s.erase(s.length() - 2);
	} else if (s.ends_with('\r') || s.ends_with('\n')) {
		s.erase(s.length() - 1);
	}
}

std::string StdStringFromInteger(int i) {
	return std::to_string(i);
}

std::string StdStringFromSizeT(size_t i) {
	return std::to_string(i);
}

std::string StdStringFromDouble(double d, int precision) {
	constexpr size_t maxDigits = 32;
	char number[maxDigits]{};
	snprintf(number, std::size(number), "%.*f", precision, d);
	return { number };
}

int IntegerFromString(const std::string &val, int defaultValue) {
	try {
		if (!val.empty()) {
			return std::stoi(val);
		}
	} catch (std::logic_error &) {
		// Ignore bad values, either non-numeric or out of range numeric
	}
	return defaultValue;
}

intptr_t IntPtrFromString(const std::string &val, intptr_t defaultValue) {
	try {
		if (!val.empty()) {
			return static_cast<intptr_t>(std::stoll(val));
		}
	} catch (std::logic_error &) {
		// Ignore bad values, either non-numeric or out of range numeric
	}
	return defaultValue;
}

long long LongLongFromString(const std::string &val, long long defaultValue) {
	try {
		if (!val.empty()) {
			return std::stoll(val);
		}
	} catch (std::logic_error &) {
		// Ignore bad values, either non-numeric or out of range numeric
	}
	return defaultValue;
}

int IntFromString(std::u32string_view s) noexcept {
	if (s.empty()) {
		return 0;
	}
	const bool negate = s.front() == '-';
	if (negate) {
		s.remove_prefix(1);
	}
	int value = 0;
	while (!s.empty()) {
		value = value * decimalBase + s.front() - '0';
		s.remove_prefix(1);
	}
	return negate ? -value : value;
}

intptr_t IntegerFromText(const char *s) noexcept {
	return static_cast<intptr_t>(atoll(s));
}

unsigned int IntFromHexDigit(int ch) noexcept {
	if ((ch >= '0') && (ch <= '9')) {
		return ch - '0';
	} else if (ch >= 'A' && ch <= 'F') {
		return ch - 'A' + decimalBase;
	} else if (ch >= 'a' && ch <= 'f') {
		return ch - 'a' + decimalBase;
	} else {
		return 0;
	}
}

bool AllBytesHex(std::string_view hexBytes) noexcept {
	for (const char ch : hexBytes) {
		if (!IsAHexDigit(ch)) {
			return false;
		}
	}
	return true;
}

int IntFromHexByte(std::string_view hexByte) noexcept {
	if (hexByte.length() >= 2) {
		return (IntFromHexDigit(hexByte[0]) * hexBase) + IntFromHexDigit(hexByte[1]);
	}
	return 0;
}

unsigned int IntFromHexBytes(std::string_view hexBytes) noexcept {
	unsigned int val = 0;
	while (!hexBytes.empty()) {
		val = val * hexBase + IntFromHexDigit(hexBytes[0]);
		hexBytes.remove_prefix(1);
	}
	return val;
}

std::set<std::string> SetFromString(std::string_view text, char separator) {
	std::set<std::string> result;

	while (!text.empty()) {
		const size_t after = text.find_first_of(separator);
		const std::string_view symbol(text.substr(0, after));
		if (!symbol.empty()) {
			result.emplace(symbol);
		}
		if (after == std::string_view::npos) {
			break;
		}
		text.remove_prefix(after + 1);
	}
	return result;
}

int CompareNoCase(const char *a, const char *b) noexcept {
	while (*a && *b) {
		if (*a != *b) {
			const char upperA = MakeUpperCase(*a);
			const char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
	}
	// Either *a or *b is nul
	return *a - *b;
}

bool EqualCaseInsensitive(const char *a, const char *b) noexcept {
	return 0 == CompareNoCase(a, b);
}

bool EqualCaseInsensitive(std::string_view a, std::string_view b) noexcept {
	if (a.length() != b.length()) {
		return false;
	}
	for (size_t i = 0; i < a.length(); i++) {
		if (MakeUpperCase(a[i]) != MakeUpperCase(b[i])) {
			return false;
		}
	}
	return true;
}

void LowerCaseAZ(std::string &s) {
	std::transform(s.begin(), s.end(), s.begin(), MakeLowerCase);
}

std::u32string UTF32FromUTF8(std::string_view s) {
	std::u32string ret;
	while (!s.empty()) {
		const unsigned char uc = static_cast<unsigned char>(s.front());
		const size_t lenChar = LengthFromLeadByte(uc);
		if (lenChar > s.length()) {
			// Character fragment
			for (size_t i = 0; i < s.length(); i++) {
				ret.push_back(static_cast<unsigned char>(s[i]));
			}
			break;
		}
		const char32_t ch32 = UTF32Character(s.data());
		ret.push_back(ch32);
		s.remove_prefix(lenChar);
	}
	return ret;
}

unsigned int UTF32Character(std::string_view utf8) noexcept {
	unsigned char ch = utf8[0];
	const size_t lenChar = LengthFromLeadByte(ch);
	if (lenChar > utf8.length()) {
		// Failure with character fragment at end
		return 0;
	}
	unsigned int u32Char = 0;
	switch (lenChar) {
	case 1:
		u32Char = ch;
		break;

	case 2:
		u32Char = (ch & leadBits2) << shiftByte2;
		ch = utf8[1];
		u32Char += TrailByteValue(ch);
		break;

	case 3:
		u32Char = (ch & leadBits3) << shiftByte3;
		ch = utf8[1];
		u32Char += TrailByteValue(ch) << shiftByte2;
		ch = utf8[2];
		u32Char += TrailByteValue(ch);
		break;

	case 4:
		u32Char = (ch & leadBits4) << shiftByte4;
		ch = utf8[1];
		u32Char += TrailByteValue(ch) << shiftByte3;
		ch = utf8[2];
		u32Char += TrailByteValue(ch) << shiftByte2;
		ch = utf8[3];
		u32Char += TrailByteValue(ch);
		break;

	default:
		// Impossible as LengthFromLeadByte can only return 1..4
		break;
	}
	return u32Char;
}

std::string UTF8FromUTF32(unsigned int uch) {
	std::string result;
	if (uch < first2Byte) {
		result.push_back(static_cast<char>(uch));
	} else if (uch < first3Byte) {
		result.push_back(SixBits(uch, 1, leadByte2));
		result.push_back(SixBits(uch, 0));
	} else if (uch < first4Byte) {
		result.push_back(SixBits(uch, 2, leadByte3));
		result.push_back(SixBits(uch, 1));
		result.push_back(SixBits(uch, 0));
	} else {
		result.push_back(SixBits(uch, 3, leadByte4));
		result.push_back(SixBits(uch, 2));
		result.push_back(SixBits(uch, 1));
		result.push_back(SixBits(uch, 0));
	}
	return result;
}

bool IsDBCSLeadByte(int codePage, char ch) noexcept {
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	const unsigned char uch = ch;
	switch (codePage) {
	case 932:
		// Shift_jis
		return ((uch >= 0x81) && (uch <= 0x9F)) ||
			((uch >= 0xE0) && (uch <= 0xFC));
		// Lead bytes F0 to FC may be a Microsoft addition.
	case 936:
		// GBK
		return (uch >= 0x81) && (uch <= 0xFE);
	case 949:
		// Korean Wansung KS C-5601-1987
		return (uch >= 0x81) && (uch <= 0xFE);
	case 950:
		// Big5
		return (uch >= 0x81) && (uch <= 0xFE);
	case 1361:
		// Korean Johab KS C-5601-1992
		return
			((uch >= 0x84) && (uch <= 0xD3)) ||
			((uch >= 0xD8) && (uch <= 0xDE)) ||
			((uch >= 0xE0) && (uch <= 0xF9));
	default:
		break;
	}
	return false;
}

/**
 * Convert a string into C string literal form using \a, \b, \f, \n, \r, \t, \v, and \ooo.
 */
std::string Slash(const std::string &s, bool quoteQuotes) {
	std::string oRet;
	for (const char ch : s) {
		if (ch == '\a') {
			oRet.append("\\a");
		} else if (ch == '\b') {
			oRet.append("\\b");
		} else if (ch == '\f') {
			oRet.append("\\f");
		} else if (ch == '\n') {
			oRet.append("\\n");
		} else if (ch == '\r') {
			oRet.append("\\r");
		} else if (ch == '\t') {
			oRet.append("\\t");
		} else if (ch == '\v') {
			oRet.append("\\v");
		} else if (ch == '\\') {
			oRet.append("\\\\");
		} else if (quoteQuotes && (ch == '\'')) {
			oRet.append("\\\'");
		} else if (quoteQuotes && (ch == '\"')) {
			oRet.append("\\\"");
		} else if (IsASCII(ch) && (ch < ' ')) {
			oRet.push_back('\\');
			oRet.push_back(static_cast<char>((ch >> 6) + '0'));
			oRet.push_back(static_cast<char>((ch >> 3) + '0'));
			oRet.push_back(static_cast<char>((ch & 0x7) + '0'));
		} else {
			oRet.push_back(ch);
		}
	}
	return oRet;
}

/**
 * Convert C style \0oo into their indicated characters.
 * This is used to get control characters into the regular expression engine.
 * Result length is always less than or equal to input length.
 */
std::string UnSlashLowOctalString(std::string_view sv) {
	std::string result;
	while (!sv.empty()) {
		if ((sv.front() == '\\') && (sv.length() > 3) && (sv[1] == '0') && IsOctalDigit(sv[2]) && IsOctalDigit(sv[3])) {
			result.push_back(static_cast<char>((octalBase * (sv[2] - '0')) + (sv[3] - '0')));
			sv.remove_prefix(3);
		} else {
			result.push_back(sv.front());
		}
		sv.remove_prefix(1);
	}
	return result;
}

/**
 * Convert C style \a, \b, \f, \n, \r, \t, \v, \ooo and \xhh into their indicated characters.
 */
std::string UnSlashString(std::string_view sv) {
	std::string result;

	while (!sv.empty()) {
		if (sv.front() == '\\') {
			sv.remove_prefix(1);
			if (sv.empty()) {
				result.push_back('\\');
				break;
			}
			const char after = sv.front();
			char ch = after;
			if (after == 'a') {
				ch = '\a';
			} else if (after == 'b') {
				ch = '\b';
			} else if (after == 'f') {
				ch = '\f';
			} else if (after == 'n') {
				ch = '\n';
			} else if (after == 'r') {
				ch = '\r';
			} else if (after == 't') {
				ch = '\t';
			} else if (after == 'v') {
				ch = '\v';
			} else if (IsOctalDigit(after)) {
				int val = after - '0';
				if ((sv.length() > 1) && IsOctalDigit(sv[1])) {
					sv.remove_prefix(1);
					val *= octalBase;
					val += sv.front() - '0';
					if ((sv.length() > 1) && IsOctalDigit(sv[1])) {
						sv.remove_prefix(1);
						val *= octalBase;
						val += sv.front() - '0';
					}
				}
				ch = static_cast<char>(val);
			} else if (after == 'x') {
				int val = 0;
				if ((sv.length() > 1) && IsAHexDigit(sv[1])) {
					sv.remove_prefix(1);
					val = IntFromHexDigit(sv.front());
					if ((sv.length() > 1) && IsAHexDigit(sv[1])) {
						sv.remove_prefix(1);
						val *= hexBase;
						val += IntFromHexDigit(sv.front());
					}
				}
				ch = static_cast<char>(val);
			}
			result.push_back(ch);
		} else {
			result.push_back(sv.front());
		}
		sv.remove_prefix(1);
	}
	return result;
}

std::string UnicodeUnEscape(std::string_view s) {
	constexpr size_t lengthPrefix = 2;
	constexpr size_t hexShort = 2;
	constexpr size_t hexMedium = 4;
	constexpr size_t hexLong = 8;
	// Leave invalid escapes as they are.
	std::string result;
	while (!s.empty()) {
		if (s.length() > lengthPrefix && s[0] == '\\') {
			size_t lengthDigits = 0;
			if (s[1] == 'x') {
				// \xAB
				lengthDigits = hexShort;
			} else if (s[1] == 'u') {
				// \uABCD
				lengthDigits = hexMedium;
			} else if (s[1] == 'U') {
				// \UABCDDEF9
				lengthDigits = hexLong;
			}
			const size_t lengthEscape = lengthPrefix + lengthDigits;
			unsigned int val = 0;
			if (lengthDigits && (s.length() >= lengthEscape) && AllBytesHex(s.substr(lengthPrefix, lengthDigits))) {
				val = IntFromHexBytes(s.substr(lengthPrefix, lengthDigits));
				s.remove_prefix(lengthEscape);
			} else {
				val = '\\';
				s.remove_prefix(1);
			}
			result.append(UTF8FromUTF32(val));
		} else {
			result.push_back(s[0]);
			s.remove_prefix(1);
		}
	}
	return result;
}

ComboMemory::ComboMemory(size_t sz_) : sz(sz_) {
}

void ComboMemory::Insert(std::string_view item) {
	std::vector<std::string>::iterator match = std::ranges::find(entries, item);
	if (match != entries.end()) {
		entries.erase(match);
	}
	entries.insert(entries.begin(), std::string(item));
	if (entries.size() > sz) {
		entries.pop_back();
	}
}

// Insert item at front of list, replacing the current front if one is a prefix
// of the other. This prevents typing or backspacing adding a large number of
// incomplete values.
void ComboMemory::InsertDeletePrefix(std::string_view item) {
	if (!entries.empty()) {
		const std::string_view svFront = entries.front();
		if (item.starts_with(svFront) || svFront.starts_with(item)) {
			entries.erase(entries.begin());
		}
	}
	Insert(item);
}

bool ComboMemory::Present(const std::string_view sv) const noexcept {
	for (const std::string &e : entries) {
		if (e == sv) {
			return true;
		}
	}
	return false;
}

void ComboMemory::Append(std::string_view item) {
	if (!Present(item) && entries.size() < sz) {
		entries.emplace_back(item);
	}
}

size_t ComboMemory::Length() const noexcept {
	return entries.size();
}

std::string ComboMemory::At(size_t n) const {
	return entries[n];
}

std::vector<std::string> ComboMemory::AsVector() const {
	return entries;
}
