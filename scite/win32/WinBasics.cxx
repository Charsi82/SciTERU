// SciTE - Scintilla based Text Editor
/** @file WinBasics.cxx
 ** Utilities for calling the Win32 API.
 **/
// Copyright 2013-2026 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string>
#include <string_view>
#include <array>
#include <chrono>

#include <windows.h>

#include "GUI.h"

#include "WinBasics.h"

void *PtrFrom(intptr_t val) noexcept {
	return reinterpret_cast<void *>(val);
}

intptr_t FromPtr(void *p) noexcept {
	return reinterpret_cast<intptr_t>(p);
}

LRESULT SendPointer(HWND hWnd, UINT Msg, WPARAM wParam, void *p) noexcept {
	return ::SendMessageW(hWnd, Msg, wParam, FromPtr(p));
}

LRESULT SendPointer(HWND hWnd, UINT Msg, WPARAM wParam, const void *p) noexcept {
	return ::SendMessageW(hWnd, Msg, wParam, reinterpret_cast<intptr_t>(p));
}

void *PointerFromWindow(HWND hWnd) noexcept {
	return PtrFrom(::GetWindowLongPtr(hWnd, 0));
}

void *SetWindowPointerFromCreate(HWND hWnd, LPARAM lParam) noexcept {
	LPCREATESTRUCT pcs = PtrParam<LPCREATESTRUCT>(lParam);
	void *ptr = pcs->lpCreateParams;
	::SetWindowLongPtr(hWnd, 0, FromPtr(ptr));
	return ptr;
}

GUI::Point PointOfCursor() noexcept {
	POINT ptCursor;
	::GetCursorPos(&ptCursor);
	return GUI::Point(ptCursor.x, ptCursor.y);
}

GUI::Point ClientFromScreen(HWND hWnd, GUI::Point ptScreen) noexcept {
	POINT ptClient = { ptScreen.x, ptScreen.y };
	::ScreenToClient(hWnd, &ptClient);
	return GUI::Point(ptClient.x, ptClient.y);
}

GUI::gui_string TextOfWindow(HWND hWnd) {
	const int len = ::GetWindowTextLengthW(hWnd);
	GUI::gui_string gsText(len, 0);
	if (::GetWindowTextW(hWnd, gsText.data(), len + 1)) {
		return gsText;
	}
	return {};
}

GUI::gui_string ClassNameOfWindow(HWND hWnd) {
	// In the documentation of WNDCLASS:
	// "The maximum length for lpszClassName is 256."
	constexpr int maxClassNameLength = 256+1;	// +1 for NUL
	std::array<GUI::gui_char, maxClassNameLength> className;
	if (::GetClassNameW(hWnd, className.data(), maxClassNameLength))
		return {className.data()};
	return {};
}

std::optional<DWORD> RegistryGetDWORD(HKEY hKeyBase, LPCWSTR lpSubKey, LPCWSTR valueName) noexcept {
	HKEY hKey {};
	const LSTATUS statusOpen = ::RegOpenKeyExW(hKeyBase, lpSubKey, 0, KEY_QUERY_VALUE, &hKey);
	if (statusOpen != ERROR_SUCCESS) {
		return {};
	}
	DWORD value = 0;
	DWORD type = REG_NONE;
	DWORD size = sizeof(DWORD);
	const LSTATUS status = ::RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(&value), &size);
	::RegCloseKey(hKey);
	if (status == ERROR_SUCCESS && type == REG_DWORD) {
		return value;
	}
	return {};
}
