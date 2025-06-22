// SciTE - Scintilla based Text Editor
/** @file GUI.h
 ** Interface to platform GUI facilities.
 ** Split off from Scintilla's Platform.h to avoid SciTE depending on implementation of Scintilla.
 ** Implementation in win32/GUIWin.cxx for Windows and gtk/GUIGTK.cxx for GTK.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef GUI_H
#define GUI_H

#include "SciTE_RB_defs.h"
#ifdef RB_ENCODING
#include <ScintillaTypes.h>
#endif

namespace GUI {

class Point {
public:
	int x;
	int y;

	explicit constexpr Point(int x_=0, int y_=0) noexcept : x(x_), y(y_) {
	}
};

class Rectangle {
public:
	int left;
	int top;
	int right;
	int bottom;

	explicit constexpr Rectangle(int left_=0, int top_=0, int right_=0, int bottom_=0) noexcept :
		left(left_), top(top_), right(right_), bottom(bottom_) {
	}
	bool Contains(Point pt) const noexcept {
		return (pt.x >= left) && (pt.x <= right) &&
		       (pt.y >= top) && (pt.y <= bottom);
	}
	int Width() const noexcept { return right - left; }
	int Height() const noexcept { return bottom - top; }
	constexpr bool operator==(const Rectangle &other) const noexcept = default;
};

#if defined(GTK) || defined(__APPLE__)

// On GTK and macOS use UTF-8 char strings

using gui_char = char;
using gui_string = std::string;
using gui_string_view = std::string_view;

#define GUI_TEXT(q) q

#else

// On Win32 use UTF-16 wide char strings

using gui_char = wchar_t;
using gui_string = std::wstring;
using gui_string_view = std::wstring_view;

#define GUI_TEXT(q) L##q

#endif

gui_string StringFromUTF8(const char *s);
gui_string StringFromUTF8(const std::string &s);
gui_string StringFromUTF8(std::string_view sv);
std::string UTF8FromString(gui_string_view sv);
gui_string StringFromInteger(long i);
gui_string StringFromLongLong(long long i);

std::string LowerCaseUTF8(std::string_view sv);

#ifdef RB_ENCODING
//!-start-[FixEncoding]
int CodePageFromName(std::string_view encodingName) noexcept;
unsigned int CodePageFromCharSet(Scintilla::CharacterSet characterSet, unsigned int documentCodePage);
std::string ConvertFromUTF8(const std::string& s, int codePage);
std::string ConvertToUTF8(const std::string& s, int codePage);
std::string UpperCaseUTF8(std::string_view sv);
//!-end-[FixEncoding]
#endif

using WindowID = void *;
class Window {
protected:
	WindowID wid {};
public:
	Window() noexcept {
	}
	Window(Window const &) = default;
	Window(Window &&) = default;
	Window &operator=(Window const &) = default;
	Window &operator=(Window &&) = default;
	Window &operator=(WindowID wid_) noexcept {
		wid = wid_;
		return *this;
	}
	virtual ~Window() = default;

	WindowID GetID() const noexcept {
		return wid;
	}
	void SetID(WindowID wid_) noexcept {
		wid = wid_;
	}
	bool Created() const noexcept {
		return !!wid;
	}
	void Destroy();
	bool HasFocus() const noexcept;
	Rectangle GetPosition();
	void SetPosition(Rectangle rc);
	Rectangle GetClientPosition();
	void Show(bool show=true);
	void InvalidateAll();
	void SetTitle(const gui_char *s);
	void SetRedraw(bool redraw);
};

using MenuID = void *;
class Menu {
	MenuID mid {};
public:
	Menu() noexcept {
	}
#ifdef RB_SUBMENU
	Menu(MenuID _id) : mid(_id) {} //!-add-[SubMenu]
#endif // RB_SUBMENU

	MenuID GetID() const noexcept {
		return mid;
	}
	void CreatePopUp();
	void Destroy() noexcept;
	void Show(Point pt, Window &w);
};

// Simplified access to high precision timing.
// Copied from Scintilla.
class ElapsedTime {
	std::chrono::high_resolution_clock::time_point tp;
public:
	/// Capture the moment
	ElapsedTime() noexcept : tp(std::chrono::high_resolution_clock::now()) {
	}
	/// Return duration as floating point seconds
	double Duration(bool reset=false) noexcept {
		const std::chrono::high_resolution_clock::time_point tpNow =
			std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double> stylingDuration =
			std::chrono::duration_cast<std::chrono::duration<double>>(tpNow - tp);
		if (reset) {
			tp = tpNow;
		}
		return stylingDuration.count();
	}
};

class ScintillaPrimitive : public Window {
public:
	// Send is the basic method and can be used between threads on Win32
	intptr_t Send(unsigned int msg, uintptr_t wParam=0, intptr_t lParam=0);
};

void SleepMilliseconds(int sleepTime);

}

#endif
