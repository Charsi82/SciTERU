// SciTE - Scintilla based Text Editor
/** @file MultiplexExtension.cxx
 ** Extension that manages / dispatches messages to multiple extensions.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdint>

#include <string>
#include <string_view>
#include <vector>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"

#include "MultiplexExtension.h"

MultiplexExtension::MultiplexExtension(): host(nullptr) {}

MultiplexExtension::~MultiplexExtension() {
}

bool MultiplexExtension::RegisterExtension(Extension &ext_) {
	for (const Extension *pexp : extensions)
		if (pexp == &ext_)
			return true;

	extensions.push_back(&ext_);

	if (host)
		ext_.Initialise(host);

	return true;
}


// Initialise, Finalise, Clear, and SetProperty get broadcast to all extensions,
// regardless of return code.  This does not strictly match the documentation, but
// seems like the right thing to do.  The others methods stop processing once one
// Extension returns true.
//
// Load will eventually be changed to be smarter, so that each Extension can have
// something different loaded into it.  OnExecute and OnMacro might also be made
// smarter with a syntax to indicate to which extension the command should be sent.

bool MultiplexExtension::Initialise(ExtensionAPI *host_) {
	if (host)
		Finalise(); // shouldn't happen.

	host = host_;
	for (Extension *pexp : extensions)
		pexp->Initialise(host_);

	return false;
}

bool MultiplexExtension::Finalise() noexcept {
	if (host) {
		for (int i = static_cast<int>(extensions.size()) - 1; i >= 0; --i)
			extensions[i]->Finalise();

		host = nullptr;
	}
	return false;
}

bool MultiplexExtension::Clear() {
	for (Extension *pexp : extensions)
		pexp->Clear();
	return false;
}

bool MultiplexExtension::Load(const char *filename) {
	for (Extension *pexp : extensions) {
		if (pexp->Load(filename)) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::InitBuffer(int index) {
	for (Extension *pexp : extensions)
		pexp->InitBuffer(index);
	return false;
}

bool MultiplexExtension::ActivateBuffer(int index) {
	for (Extension *pexp : extensions)
		pexp->ActivateBuffer(index);
	return false;
}

bool MultiplexExtension::RemoveBuffer(int index) {
	for (Extension *pexp : extensions)
		pexp->RemoveBuffer(index);
	return false;
}

bool MultiplexExtension::OnOpen(const char *filename) {
	for (Extension *pexp : extensions) {
		if (pexp->OnOpen(filename)) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnSwitchFile(const char *filename) {
	for (Extension *pexp : extensions) {
		if (pexp->OnSwitchFile(filename)) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnBeforeSave(const char *filename) {
	for (Extension *pexp : extensions) {
		if (pexp->OnBeforeSave(filename)) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnSave(const char *filename) {
	for (Extension *pexp : extensions) {
		if (pexp->OnSave(filename)) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnChar(char c) {
	for (Extension *pexp : extensions) {
		if (pexp->OnChar(c)) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnExecute(const char *cmd) {
	for (Extension *pexp : extensions) {
		if (pexp->OnExecute(cmd)) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnSavePointReached() {
	for (Extension *pexp : extensions) {
		if (pexp->OnSavePointReached()) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnSavePointLeft() {
	for (Extension *pexp : extensions) {
		if (pexp->OnSavePointLeft()) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnStyle(Scintilla::Position p, Scintilla::Position q, int r, StyleWriter *s) {
	for (Extension *pexp : extensions) {
		if (pexp->OnStyle(p, q, r, s)) {
			return true;
		}
	}
	return false;
}

#ifdef RB_ODBCLK
//!-start-[OnDoubleClick]
bool MultiplexExtension::OnDoubleClick(int modifiers) {
	for (Extension* pexp : extensions) {
		if (pexp->OnDoubleClick(modifiers)) {
			return true;
		}
	}
	return false;
}
//!-end-[OnDoubleClick]
#else
bool MultiplexExtension::OnDoubleClick() {
	for (Extension *pexp : extensions) {
		if (pexp->OnDoubleClick()) {
			return true;
		}
	}
	return false;
}
#endif // RB_ODBCLK

#ifdef RB_ONCLICK
//!-start-[OnClick]
bool MultiplexExtension::OnClick(int modifiers) {
	for (Extension* pexp : extensions) {
		if (pexp->OnClick(modifiers)) {
			return true;
		}
	}
	return false;
}
//!-end-[OnClick]
#endif // RB_ONCLICK

#ifdef RB_OMBU
//!-start-[OnMouseButtonUp]
bool MultiplexExtension::OnMouseButtonUp(int modifiers) {
	for (Extension* pexp : extensions) 
		if (pexp->OnMouseButtonUp(modifiers))
			return true;
	return false;
}
//!-end-[OnMouseButtonUp]
#endif

#ifdef RB_OHSC
//!-start-[OnHotSpotReleaseClick]
bool MultiplexExtension::OnHotSpotReleaseClick(int modifiers) {
	for (Extension* pexp : extensions) 
		if (pexp->OnHotSpotReleaseClick(modifiers))
			return true;
	return false;
}
//!-end-[OnHotSpotReleaseClick]
#endif // RB_OHSC

bool MultiplexExtension::OnUpdateUI() {
	for (Extension *pexp : extensions) {
		if (pexp->OnUpdateUI()) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnMarginClick() {
	for (Extension *pexp : extensions) {
		if (pexp->OnMarginClick()) {
			return true;
		}
	}
	return false;
}

bool MultiplexExtension::OnMacro(const char *p, const char *q) {
	for (Extension *pexp : extensions) {
		if (pexp->OnMacro(p, q)) {
			return true;
		}
	}
	return false;
}
#ifdef RB_ULID
bool MultiplexExtension::OnUserListSelection(int listType, const char* selection, Scintilla::Position id) { //!-change-[UserListItemID]
	for (Extension* pexp : extensions) {
		if (pexp->OnUserListSelection(listType, selection, id)) {
			return true;
		}
	}
	return false;
}
#else

bool MultiplexExtension::OnUserListSelection(int listType, const char *selection) {
	for (Extension *pexp : extensions) {
		if (pexp->OnUserListSelection(listType, selection)) {
			return true;
		}
	}
	return false;
}
#endif // RB_ULID

#ifdef RB_OMC
//!-start-[OnMenuCommand]
bool MultiplexExtension::OnMenuCommand(int cmd, int source) {
	for (Extension* pexp : extensions)
		if (pexp->OnMenuCommand(cmd, source))
			return true;
	return false;
}
//!-end-[OnMenuCommand]
#endif // RB_OMC

bool MultiplexExtension::SendProperty(const char *prop) {
	for (Extension *pexp : extensions)
		pexp->SendProperty(prop);
	return false;
}

#ifdef RB_ONKEY
//!-start-[OnKey]
bool MultiplexExtension::OnKey(int keyval, int modifiers, char ch) {
	for (Extension* pexp : extensions) {
		if (pexp->OnKey(keyval, modifiers, ch)) {
			return true;
		}
	}
	return false;
}
//!-end-[OnKey]
#else
bool MultiplexExtension::OnKey(int keyval, int modifiers) {
	for (Extension *pexp : extensions) {
		if (pexp->OnKey(keyval, modifiers)) {
			return true;
		}
	}
	return false;
}
#endif // RB_ONKEY

bool MultiplexExtension::OnDwellStart(Scintilla::Position pos, const char *word) {
	for (Extension *pexp : extensions)
		pexp->OnDwellStart(pos, word);
	return false;
}

bool MultiplexExtension::OnClose(const char *filename) {
	for (Extension *pexp : extensions)
		pexp->OnClose(filename);
	return false;
}

#ifdef RB_OnSendEditor
//!-start-[OnSendEditor]
const char* MultiplexExtension::OnSendEditor(Scintilla::Message msg, uintptr_t wp, const char* lp) {
	const char* result = NULL;
	for (Extension* pexp : extensions) {
		result = pexp->OnSendEditor(msg, wp, lp);
		if (result) break;
	}
	return result;
}

const char* MultiplexExtension::OnSendEditor(Scintilla::Message msg, uintptr_t wp, long lp) {
	const char* result = NULL;
	for (Extension* pexp : extensions) {
		result = pexp->OnSendEditor(msg, wp, lp);
		if (result) break;
	}
	return result;
}
//!-end-[OnSendEditor]
#endif

#ifdef RB_ONTABMOVE
void MultiplexExtension::OnTabMove(int idx_from, int idx_to)
{
	for (Extension *pexp : extensions)
		pexp->OnTabMove(idx_from, idx_to);
}
#endif //RB_ONTABMOVE

bool MultiplexExtension::OnUserStrip(int control, int change) {
	for (Extension *pexp : extensions)
		pexp->OnUserStrip(control, change);
	return false;
}

bool MultiplexExtension::NeedsOnClose() {
	for (Extension *pexp : extensions) {
		if (pexp->NeedsOnClose()) {
			return true;
		}
	}
	return false;
}
