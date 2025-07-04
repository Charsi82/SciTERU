// SciTE - Scintilla based Text Editor
/** @file GUIWin.cxx
 ** Interface to platform GUI facilities.
 ** Split off from Scintilla's Platform.h to avoid SciTE depending on implementation of Scintilla.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <ctime>

#include <string>
#include <vector>
#include <chrono>
#include <sstream>

#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0A00
#include <windows.h>

#include <algorithm> //+ std::transform
#include "ScintillaTypes.h"
#include "GUI.h"

namespace GUI {

enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };

namespace {

unsigned int UTF8Length(const wchar_t *uptr, size_t tlen) noexcept {
	unsigned int len = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		const unsigned int uch = uptr[i];
		if (uch < 0x80) {
			len++;
		} else if (uch < 0x800) {
			len += 2;
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			len += 4;
			i++;
		} else {
			len += 3;
		}
		i++;
	}
	return len;
}

void UTF8FromUTF16(const wchar_t *uptr, size_t tlen, char *putf) noexcept {
	int k = 0;
	for (size_t i = 0; i < tlen && uptr[i];) {
		const unsigned int uch = uptr[i];
		if (uch < 0x80) {
			putf[k++] = static_cast<char>(uch);
		} else if (uch < 0x800) {
			putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		} else if ((uch >= SURROGATE_LEAD_FIRST) &&
			(uch <= SURROGATE_TRAIL_LAST)) {
			// Half a surrogate pair
			i++;
			const unsigned int xch = 0x10000 + ((uch & 0x3ff) << 10) + (uptr[i] & 0x3ff);
			putf[k++] = static_cast<char>(0xF0 | (xch >> 18));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 12) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | ((xch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (xch & 0x3f));
		} else {
			putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
			putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
		i++;
	}
}

size_t UTF16Length(const char *s, size_t len) noexcept {
	size_t ulen = 0;
	size_t charLen;
	for (size_t i = 0; i < len;) {
		const unsigned char ch = static_cast<unsigned char>(s[i]);
		if (ch < 0x80) {
			charLen = 1;
		} else if (ch < 0x80 + 0x40 + 0x20) {
			charLen = 2;
		} else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			charLen = 3;
		} else {
			charLen = 4;
			ulen++;
		}
		i += charLen;
		ulen++;
	}
	return ulen;
}

size_t UTF16FromUTF8(std::string_view s, gui_char *tbuf, size_t tlen) noexcept {
	size_t ui = 0;
	const unsigned char *us = reinterpret_cast<const unsigned char *>(s.data());
	size_t len = s.size();
	size_t i = 0;
	while ((i < len) && (ui < tlen)) {
		unsigned char ch = us[i++];
		if (ch < 0x80) {
			tbuf[ui] = ch;
		} else if (ch < 0x80 + 0x40 + 0x20) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		} else if (ch < 0x80 + 0x40 + 0x20 + 0x10) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		} else {
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

}

gui_string StringFromUTF8(const char *s) {
	if (!s || !*s) {
		return {};
	}
	const size_t wideLen = UTF16Length(s, strlen(s));
	gui_string us(wideLen, 0);
	UTF16FromUTF8(s, us.data(), wideLen);
	return us;
}

gui_string StringFromUTF8(const std::string &s) {
	if (s.empty()) {
		return {};
	}
	const size_t wideLen = UTF16Length(s.c_str(), s.length());
	gui_string us(wideLen, 0);
	UTF16FromUTF8(s, us.data(), wideLen);
	return us;
}

gui_string StringFromUTF8(std::string_view sv) {
	if (sv.empty()) {
		return {};
	}
	const size_t wideLen = UTF16Length(sv.data(), sv.length());
	gui_string us(wideLen, 0);
	UTF16FromUTF8(sv, us.data(), wideLen);
	return us;
}

std::string UTF8FromString(gui_string_view sv) {
	if (sv.empty()) {
		return {};
	}
	const size_t sLen = sv.size();
	const size_t narrowLen = UTF8Length(sv.data(), sLen);
	std::string us(narrowLen, 0);
	UTF8FromUTF16(sv.data(), sLen, us.data());
	return us;
}

gui_string StringFromInteger(long i) {
	return std::to_wstring(i);
}

gui_string StringFromLongLong(long long i) {
	return std::to_wstring(i);
}

	//!-start-[EncodingToLua]
	int CodePageFromName(std::string_view encodingName) noexcept {
		struct Encoding {
			const char* name;
			int codePage;
		};
		const Encoding	knownEncodings[] = {
			{ "ascii", CP_UTF8 },
			{ "utf-8", CP_UTF8 },
			{ "latin1", 1252 },
			{ "latin2", 28592 },
			{ "big5", 950 },
			{ "gbk", 936 },
			{ "shift_jis", 932 },
			{ "euc-kr", 949 },
			{ "cyrillic", 1251 },
			{ "iso-8859-5", 28595 },
			{ "iso8859-11", 874 },
			{ "1250", 1250 },
			{ "windows-1251", 1251 },
		};
		for (const Encoding& enc : knownEncodings) {
			if (encodingName == enc.name) {
				return enc.codePage;
			}
		}
		return CP_UTF8;
	}
	//!-end-[EncodingToLua]

#ifdef RB_ENCODING
//!-start-[FixEncoding]
// from ScintillaWin.cxx
	unsigned int CodePageFromCharSet(Scintilla::CharacterSet characterSet, unsigned int documentCodePage) {
		CHARSETINFO ci{};
		BOOL bci = ::TranslateCharsetInfo(reinterpret_cast<DWORD*>(static_cast<uintptr_t>(characterSet)),
			&ci, TCI_SRCCHARSET);

		UINT cp;
		if (bci)
			cp = ci.ciACP;
		else if (characterSet == Scintilla::CharacterSet::Oem866)
			cp = 866;
		else
			cp = documentCodePage;

		CPINFO cpi{};
		if (!::IsValidCodePage(cp) && !::GetCPInfo(cp, &cpi))
			cp = CP_ACP;

		return cp;
	}

	std::string ConvertFromUTF8(const std::string& s, int codePage) {
		if (codePage == CP_UTF8) {
			return s;
		}
		else {
			GUI::gui_string sWide = GUI::StringFromUTF8(s.c_str());
			const int cchMulti = ::WideCharToMultiByte(codePage, 0, sWide.c_str(), static_cast<int>(sWide.length()), NULL, 0, NULL, NULL);
			char* pszMulti = new char[static_cast<size_t>(cchMulti) + 1];
			::WideCharToMultiByte(codePage, 0, sWide.c_str(), static_cast<int>(sWide.length()), pszMulti, cchMulti + 1, NULL, NULL);
			pszMulti[cchMulti] = 0;
			std::string ret(pszMulti);
			delete[] pszMulti;
			return ret;
		}
	}

	std::string ConvertToUTF8(const std::string& s, int codePage) {
		if (codePage == CP_UTF8) {
			return s;
		}
		else {
			const char* original = s.c_str();
			int cchWide = ::MultiByteToWideChar(codePage, 0, original, -1, NULL, 0);
			wchar_t* pszWide = new wchar_t[static_cast<size_t>(cchWide) + 1];
			::MultiByteToWideChar(codePage, 0, original, -1, pszWide, cchWide + 1);
			GUI::gui_string sWide(pszWide);
			std::string ret = GUI::UTF8FromString(sWide);
			delete[] pszWide;
			return ret;
		}
	}

	std::string UpperCaseUTF8(std::string_view sv) {
		if (sv.empty()) {
			return std::string();
		}
		const std::string s(sv);
		const gui_string gs = StringFromUTF8(s);
		const int chars = ::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_UPPERCASE, gs.c_str(), static_cast<int>(gs.size()), nullptr, 0);
		gui_string lc(chars, L'\0');
		::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_UPPERCASE, gs.c_str(), static_cast<int>(gs.size()), lc.data(), chars);
		return UTF8FromString(lc);
	}

	//!-end-[FixEncoding]
#endif

gui_string HexStringFromInteger(long i) {
	char number[32];
	snprintf(number, std::size(number), "%0lx", i);
	gui_char gnumber[32] {};
	size_t n = 0;
	while (number[n]) {
		gnumber[n] = static_cast<gui_char>(number[n]);
		n++;
	}
	gnumber[n] = 0;
	return gui_string(gnumber);
}

std::string LowerCaseUTF8(std::string_view sv) {
	if (sv.empty()) {
		return {};
	}
	const std::string s(sv);
	const gui_string gs = StringFromUTF8(s);
	const int chars = ::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_LOWERCASE, gs.c_str(), static_cast<int>(gs.size()), nullptr, 0);
	gui_string lc(chars, L'\0');
	::LCMapString(LOCALE_SYSTEM_DEFAULT, LCMAP_LOWERCASE, gs.c_str(), static_cast<int>(gs.size()), lc.data(), chars);
	return UTF8FromString(lc);
}

void Window::Destroy() {
	if (wid)
		::DestroyWindow(static_cast<HWND>(wid));
	wid = {};
}

bool Window::HasFocus() const noexcept {
	return ::GetFocus() == wid;
}

Rectangle Window::GetPosition() {
	RECT rc;
	::GetWindowRect(static_cast<HWND>(wid), &rc);
	return Rectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::SetPosition(Rectangle rc) {
	::SetWindowPos(static_cast<HWND>(wid),
		       {}, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

Rectangle Window::GetClientPosition() {
	RECT rc {};
	if (wid)
		::GetClientRect(static_cast<HWND>(wid), &rc);
	return Rectangle(rc.left, rc.top, rc.right, rc.bottom);
}

void Window::Show(bool show) {
	if (show)
		::ShowWindow(static_cast<HWND>(wid), SW_SHOWNOACTIVATE);
	else
		::ShowWindow(static_cast<HWND>(wid), SW_HIDE);
}

void Window::InvalidateAll() {
	::InvalidateRect(static_cast<HWND>(wid), nullptr, FALSE);
}

void Window::SetTitle(const gui_char *s) {
	::SetWindowTextW(static_cast<HWND>(wid), s);
}

void Window::SetRedraw(bool redraw) {
	::SendMessage(static_cast<HWND>(GetID()), WM_SETREDRAW, redraw, 0);
	if (redraw) {
		::RedrawWindow(static_cast<HWND>(GetID()), nullptr, {},
			RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
	}
}

void Menu::CreatePopUp() {
	Destroy();
	mid = ::CreatePopupMenu();
}

void Menu::Destroy() noexcept {
	if (mid)
		::DestroyMenu(static_cast<HMENU>(mid));
	mid = {};
}

void Menu::Show(Point pt, Window &w) {
	::TrackPopupMenu(static_cast<HMENU>(mid),
			 TPM_RIGHTBUTTON, pt.x - 4, pt.y, 0,
			 static_cast<HWND>(w.GetID()), nullptr);
	Destroy();
}

intptr_t ScintillaPrimitive::Send(unsigned int msg, uintptr_t wParam, intptr_t lParam) {
	return ::SendMessage(static_cast<HWND>(GetID()), msg, wParam, lParam);
}

void SleepMilliseconds(int sleepTime) {
	::Sleep(sleepTime);
}

}
