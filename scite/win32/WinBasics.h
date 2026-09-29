// SciTE - Scintilla based Text Editor
/** @file WinBasics.h
 ** Utilities for calling the Win32 API.
 **/
// Copyright 2013-2026 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WINBASICS_H
#define WINBASICS_H

// Type conversion helpers avoid reinterpret_cast
void *PtrFrom(intptr_t val) noexcept;
template<typename T>
T PtrParam(intptr_t val) noexcept {
	return static_cast<T>(PtrFrom(val));
}
intptr_t FromPtr(void *p) noexcept;

LRESULT SendPointer(HWND hWnd, UINT Msg, WPARAM wParam, void *p) noexcept;
LRESULT SendPointer(HWND hWnd, UINT Msg, WPARAM wParam, const void *p) noexcept;

void *PointerFromWindow(HWND hWnd) noexcept;
void *SetWindowPointerFromCreate(HWND hWnd, LPARAM lParam) noexcept;

GUI::Point PointOfCursor() noexcept;
GUI::Point ClientFromScreen(HWND hWnd, GUI::Point ptScreen) noexcept;

inline bool IsKeyDown(int key) noexcept {
	constexpr int keyDownFlag = 0x8000'0000;
	return (::GetKeyState(key) & keyDownFlag) != 0;
}

constexpr GUI::Point PointFromLong(LPARAM lPoint) noexcept {
	// static_cast<short> needed for negative coordinates
	return GUI::Point(static_cast<short>(LOWORD(lPoint)), static_cast<short>(HIWORD(lPoint)));
}

constexpr int ControlIDOfWParam(WPARAM wParam) noexcept {
	constexpr WPARAM lowMask = 0xffff;
	return wParam & lowMask;
}

inline HWND HwndOf(const GUI::Window &w) noexcept {
	return static_cast<HWND>(w.GetID());
}

inline HMENU HmenuID(size_t id) noexcept {
	return static_cast<HMENU>(PtrFrom(id));
}

GUI::gui_string TextOfWindow(HWND hWnd);
GUI::gui_string ClassNameOfWindow(HWND hWnd);

std::optional<DWORD> RegistryGetDWORD(HKEY hKeyBase, LPCWSTR lpSubKey, LPCWSTR valueName) noexcept;

#endif
