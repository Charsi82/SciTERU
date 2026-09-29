// SciTE - Scintilla based Text Editor
/** @file DirectorExtension.cxx
 ** Extension for communicating with a director program.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>

#include <compare>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <chrono>
#include <sstream>
#include <atomic>
#include <mutex>

#include <windows.h>

#include "ScintillaTypes.h"
#include "ScintillaCall.h"
#include "ILexer.h"
#include "GUI.h"
#include "ScintillaWindow.h"
#include "StringList.h"
#include "StringHelpers.h"
#include "FilePath.h"
#include "StyleDefinition.h"
#include "PropSetFile.h"
#include "Extender.h"
#include "SciTE.h"
#include "JobQueue.h"
#include "Cookie.h"
#include "Worker.h"
#include "MatchMarker.h"
#include "Searcher.h"
#include "SciTEBase.h"
#include "WinBasics.h"
#include "DirectorExtension.h"

namespace {

HWND wDirector{};
HWND wCorrespondent{};
HWND wReceiver{};
bool startedByDirector = false;
bool shuttingDown = false;
unsigned int SDI = 0;

bool HasConnection() noexcept {
	return wDirector || wCorrespondent;
}

void SendDirector(const char *verb, const char *arg = nullptr) {
	if (HasConnection()) {
		HWND wDestination = wCorrespondent;
		std::string addressedMessage;
		if (wDestination) {
			addressedMessage += ":";
			std::string address = StdStringFromSizeT(FromPtr(wDestination));
			addressedMessage += address;
			addressedMessage += ":";
		} else {
			wDestination = wDirector;
		}
		addressedMessage += verb;
		addressedMessage += ":";
		if (arg)
			addressedMessage += arg;
		std::string slashedMessage = Slash(addressedMessage, false);
		COPYDATASTRUCT cds{};
		cds.dwData = 0;
		cds.cbData = static_cast<DWORD>(slashedMessage.length());
		slashedMessage.append(1, '\0');	// Ensure NUL at end of string
		cds.lpData = slashedMessage.data();
		SendPointer(wDestination, WM_COPYDATA,
			      FromPtr(wReceiver),
			      &cds);
	}
}

void SendDirectorInteger(const char *verb, intptr_t arg) {
	std::string s = std::to_string(arg);
	::SendDirector(verb, s.c_str());
}

HWND HwndFromString(const std::string &s) noexcept {
	try {
		return reinterpret_cast<HWND>(static_cast<uintptr_t>(stoull(s)));
	} catch (...) {
		return {};
	}
}

void CheckEnvironment(ExtensionAPI *phost) {
	if (phost && !shuttingDown) {
		if (!wDirector) {
			std::string director = phost->Property("director.hwnd");
			if (!director.empty()) {
				startedByDirector = true;
				wDirector = HwndFromString(director);
				// Director is just seen so identify this to it
				::SendDirectorInteger("identity", FromPtr(wReceiver));
			}
		}
		std::string sReceiver = StdStringFromSizeT(FromPtr(wReceiver));
		phost->SetProperty("WindowID", sReceiver.c_str());
	}
}

WCHAR DirectorExtension_ClassName[] = L"DirectorExtension";

LRESULT HandleCopyData(LPARAM lParam) {
	const COPYDATASTRUCT *pcds = PtrParam<const COPYDATASTRUCT *>(lParam);
	// Copy into an temporary buffer to ensure \0 terminated
	if (pcds->lpData) {
		std::string dataCopy(static_cast<const char *>(pcds->lpData), pcds->cbData);
		DirectorExtension::Instance().HandleStringMessage(dataCopy.c_str());
	}
	return 0;
}

LRESULT CALLBACK DirectorExtension_WndProc(
	HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	if (iMessage == WM_COPYDATA) {
		return HandleCopyData(lParam);
	}
	if (iMessage == SDI) {
		return SDI;
	}
	return ::DefWindowProc(hWnd, iMessage, wParam, lParam);
}

void DirectorExtension_Register(HINSTANCE hInstance) noexcept {
	WNDCLASS wndclass{};
	wndclass.style = 0;
	wndclass.lpfnWndProc = DirectorExtension_WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = {};
	wndclass.hCursor = {};
	wndclass.hbrBackground = {};
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = DirectorExtension_ClassName;
	if (!::RegisterClassW(&wndclass))
		::exit(FALSE);
}

}

DirectorExtension &DirectorExtension::Instance() {
	static DirectorExtension singleton;
	return singleton;
}

bool DirectorExtension::Initialise(ExtensionAPI *host_) {
	host = host_;
	SDI = ::RegisterWindowMessageW(L"SciTEDirectorInterface");
	HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(
				      host->GetInstance());
	DirectorExtension_Register(hInstance);
	wReceiver = ::CreateWindowW(
			    DirectorExtension_ClassName,
			    DirectorExtension_ClassName,
			    0,
			    0, 0, 0, 0,
			    {},
			    {},
			    hInstance,
			    nullptr);
	if (!wReceiver)
		::exit(FALSE);
	// Make the frame window handle available so the director can activate it.
	const SciTEBase *hostSciTE = dynamic_cast<SciTEBase *>(host);
	if (!hostSciTE) {
		::exit(FALSE);
	}
	::SetWindowLongPtrW(wReceiver, GWLP_USERDATA,
			   FromPtr(hostSciTE->GetID()));
	CheckEnvironment(host);
	return true;
}

bool DirectorExtension::Finalise() noexcept {
	try {
		::SendDirector("closing");
	} catch (...) {
		// Shutting down so continue even if unable to send message to director.
	}
	if (wReceiver)
		::DestroyWindow(wReceiver);
	wReceiver = {};
	return true;
}

bool DirectorExtension::Clear() {
	return false;
}

bool DirectorExtension::Load(const char *) {
	return false;
}

bool DirectorExtension::OnOpen(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("opened", path);
	}
	return false;
}

bool DirectorExtension::OnSwitchFile(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("switched", path);
	}
	return false;
}

bool DirectorExtension::OnSave(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("saved", path);
	}
	return false;
}

bool DirectorExtension::OnClose(const char *path) {
	CheckEnvironment(host);
	if (*path) {
		::SendDirector("closed", path);
	}
	return false;
}

bool DirectorExtension::NeedsOnClose() {
	CheckEnvironment(host);
	return HasConnection();
}

bool DirectorExtension::OnChar(char) {
	return false;
}

bool DirectorExtension::OnExecute(const char *cmd) {
	CheckEnvironment(host);
	::SendDirector("macro:run", cmd);
	return false;
}

bool DirectorExtension::OnSavePointReached() {
	return false;
}

bool DirectorExtension::OnSavePointLeft() {
	return false;
}

bool DirectorExtension::OnStyle(SA::Position, SA::Position, int, StyleWriter *) {
	return false;
}

// These should probably have arguments
#ifdef RB_ONCLICK
//!-add-[OnClick]
bool DirectorExtension::OnClick(int) {
	return false;
}
#endif // RB_ONCLICK

#ifdef RB_OMBU
//!-add-[OnMouseButtonUp]
bool DirectorExtension::OnMouseButtonUp(int) {
	return false;
}
#endif // RB_OMBU

#ifdef RB_ODBCLK
bool DirectorExtension::OnDoubleClick(int) { //!-change-[OnDoubleClick]
	return false;
}
#else
bool DirectorExtension::OnDoubleClick() {
	return false;
}
#endif // RB_ODBCLK
#ifdef RB_OHSC
//!-add-[OnHotSpotReleaseClick]
bool DirectorExtension::OnHotSpotReleaseClick(int) {
	return false;
}
#endif // RB_OHSC

bool DirectorExtension::OnUpdateUI() {
	return false;
}

bool DirectorExtension::OnMarginClick() {
	return false;
}

bool DirectorExtension::OnMacro(const char *command, const char *params) {
	SendDirector(command, params);
	return false;
}

bool DirectorExtension::SendProperty(const char *prop) {
	CheckEnvironment(host);
	if (*prop) {
		::SendDirector("property", prop);
	}
	return false;
}

void DirectorExtension::HandleStringMessage(const char *message) {
	// Message may contain multiple commands separated by '\n'
	// Reentrance trouble - if this function is reentered, the wCorrespondent may
	// be set to zero before time.
	std::vector<std::string> wlMessage = StringSplit(std::string(message), '\n');
	for (std::string cmd : wlMessage) {
		// Message format is [:return address:]command:argument
		if (cmd.starts_with(':')) {
			// There is a return address
			const size_t colon = cmd.find(':', 1);
			if (colon != std::string::npos) {
				wCorrespondent = HwndFromString(cmd.substr(1, colon - 1));
				cmd.erase(0, colon + 1);
			}
		}
		if (cmd.starts_with("identity:")) {
			const size_t colon = cmd.find(':');
			if (colon != std::string::npos)
				wDirector = HwndFromString(cmd.substr(colon + 1));
		} else if (cmd.starts_with("closing:")) {
			wDirector = {};
			if (startedByDirector) {
				shuttingDown = true;
				if (host) {
					host->ShutDown();
				}
				shuttingDown = false;
			}
		} else if (host) {
			host->Perform(cmd.c_str());
		}
		wCorrespondent = {};
	}
}
