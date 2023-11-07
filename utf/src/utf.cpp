#include "utf.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// ==========================================================================
enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };

static size_t UTF8Length(const wchar_t* uptr, size_t tlen) {
	size_t len = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		unsigned int uch = uptr[i];
		if (uch < 0x80) {
			len++;
		}
		else if (uch < 0x800) {
			len += 2;
		}
		else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			len += 4;
			i++;
		}
		else {
			len += 3;
		}
		i++;
	}
	return len;
}
 
static void UTF8FromUTF16(const wchar_t* uptr, size_t tlen, char* putf, size_t len) {
	int k = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		unsigned int uch = uptr[i];
		if (uch < 0x80) {
			putf[k++] = static_cast<char>(uch);
		}
		else if (uch < 0x800) {
			putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			// Half a surrogate pair
			i++;
			unsigned int xch = 0x10000 + ((uch & 0x3ff) << 10) + (uptr[i] & 0x3ff);
			putf[k++] = static_cast<char>(0xF0 | (xch >> 18));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 12) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (xch & 0x3f));
		}
		else {
			putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
			putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		i++;
	}
	putf[len] = '\0';
}

static size_t UTF16Length(const char* s, size_t len) {
	size_t ulen = 0;
	size_t charLen;
	for (size_t i = 0; i < len;) {
		unsigned char ch = static_cast<unsigned char>(s[i]);
		if (ch < 0x80) {
			charLen = 1;
		}
		else if (ch < 0x80 + 0x40 + 0x20) {
			charLen = 2;
		}
		else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			charLen = 3;
		}
		else {
			charLen = 4;
			ulen++;
		}
		i += charLen;
		ulen++;
	}
	return ulen;
}

static size_t UTF16FromUTF8(const char* s, size_t len, wchar_t* tbuf, size_t tlen) {
	size_t ui = 0;
	const unsigned char* us = reinterpret_cast<const unsigned char*>(s);
	size_t i = 0;
	while ((i < len) && (ui < tlen)) {
		unsigned char ch = us[i++];
		if (ch < 0x80) {
			tbuf[ui] = ch;
		}
		else if (ch < 0x80 + 0x40 + 0x20) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		}
		else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		}
		else {
			// Outside the BMP so need two surrogates
			int val = (ch & 0x7) << 18;
			ch = us[i++];
			val += (ch & 0x3F) << 12;
			ch = us[i++];
			val += (ch & 0x3F) << 6;
			ch = us[i++];
			val += (ch & 0x3F);
			tbuf[ui] = static_cast<wchar_t>(((val - 0x10000) >> 10) + SURROGATE_LEAD_FIRST);
			ui++;
			tbuf[ui] = static_cast<wchar_t>((val & 0x3ff) + SURROGATE_TRAIL_FIRST);
		}
		ui++;
	}
	return ui;
}

std::wstring StringFromUTF8(const char* s)
{
	if (!s || !*s)
		return {};
	const size_t sLen = strlen(s);
	const size_t wideLen = UTF16Length(s, sLen);
	std::wstring us(wideLen, 0);
	UTF16FromUTF8(s, sLen, us.data(), wideLen);
	return us;
}

std::string UTF8FromString(const std::wstring& s)
{
	size_t sLen = s.size();
	size_t narrowLen = UTF8Length(s.c_str(), sLen);
	std::string res(narrowLen + 1, 0);
	UTF8FromUTF16(s.c_str(), sLen, res.data(), narrowLen);
	return res;
}

std::string ConvertFromUTF8(std::string_view s, int codePage) {
	if (codePage == CP_UTF8) {
		return s.data();
	}
	else {
		std::wstring sWide = StringFromUTF8(s.data());
		int len = static_cast<int>(sWide.length());
		int cchMulti = ::WideCharToMultiByte(codePage, 0, sWide.c_str(), len, NULL, 0, NULL, NULL);
		std::string ret(static_cast<size_t>(cchMulti) + 1, 0);
		::WideCharToMultiByte(codePage, 0, sWide.c_str(), len, ret.data(), cchMulti + 1, NULL, NULL);
		return ret;
	}
}

std::string ConvertToUTF8(std::string_view s, int codePage) {
	if (codePage == CP_UTF8) {
		return s.data();
	}
	else {
		const char* original = s.data();
		int cchWide = ::MultiByteToWideChar(codePage, 0, original, -1, NULL, 0);
		std::wstring sWide(static_cast<size_t>(cchWide) + 1, 0);
		::MultiByteToWideChar(codePage, 0, original, -1, sWide.data(), cchWide + 1);
		return UTF8FromString(sWide);
	}
}
