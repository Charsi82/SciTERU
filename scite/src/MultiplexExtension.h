// SciTE - Scintilla based Text Editor
/** @file MultiplexExtension.h
 ** Extension that manages / dispatches messages to multiple extensions.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef MULTIPLEXEXTENSION_H
#define MULTIPLEXEXTENSION_H

#include "Extender.h"

// MultiplexExtension manages multiple Extension objects, similar to
// what is proposed in the SciTE Extension documentation.  Each
// message is sent to each contained extension object in turn until
// one indicates that the message has been handled and does not need
// to be processed further.  Certain messages (Initialise, Finalise
// Clear, and SendProperty) are sent to all contained extensions
// regardless of return code.
//
// The Director extension incorrectly returns true for all messages,
// meaning that other extensions will never see the message if
// DirectorExtension comes before them in the list.  This has been
// fixed at source.
//
// Extensions are added to the multiplexer by calling RegisterExtension.
// The extensions are prioritized with the first one added having the
// highest priority.  If more flexibility is needed in order to support
// dynamic discovery of extensions and assignment of priority, that will
// be added later.  If the ability to remove extensions becomes important,
// that can be added as well (later).
//
// The multiplexer does not manage the lifetime of the extension objects
// that are registered with it.  If that functionality later turns out
// to be needed, it will be added at that time.  (Broken record?  Do the
// simplest thing...)  However, the option to "not" manage the lifecycle
// is a valid one, since it often makes sense to implement extensions as
// singletons.

class MultiplexExtension: public Extension {
public:
	MultiplexExtension();
	// Deleted so MultiplexExtension objects can not be copied.
	MultiplexExtension(const MultiplexExtension &) = delete;
	MultiplexExtension(MultiplexExtension &&) = delete;
	MultiplexExtension &operator=(const MultiplexExtension &) = delete;
	MultiplexExtension &operator=(MultiplexExtension &&) = delete;
	~MultiplexExtension() override;

	bool RegisterExtension(Extension &ext_);

	bool Initialise(ExtensionAPI *host_) override;
	bool Finalise() noexcept override;
	bool Clear() override;
	bool Load(const char *filename) override;

	bool InitBuffer(int) override;
	bool ActivateBuffer(int) override;
	bool RemoveBuffer(int) override;

	bool OnOpen(const char *) override;
	bool OnSwitchFile(const char *) override;
	bool OnBeforeSave(const char *) override;
	bool OnSave(const char *) override;
	bool OnChar(char) override;
	bool OnExecute(const char *) override;
	bool OnSavePointReached() override;
	bool OnSavePointLeft() override;
	bool OnStyle(Scintilla::Position, Scintilla::Position, int, StyleWriter *) override;

#ifdef RB_ODBCLK
	bool OnDoubleClick(int) override; //!-change-[OnDoubleClick]
#else
	bool OnDoubleClick() override;
#endif // RB_ODBCLK

#ifdef RB_ONCLICK
	bool OnClick(int) override; //!-add-[OnClick]
#endif // RB_ONCLICK

#ifdef RB_OMBU
	bool OnMouseButtonUp(int) override; //!-add-[OnMouseButtonUp]
#endif // RB_OMBU

#ifdef RB_OHSC
	bool OnHotSpotReleaseClick(int) override; //!-add-[OnHotSpotReleaseClick]
#endif // RB_OHSC

	bool OnUpdateUI() override;
	bool OnMarginClick() override;
	bool OnMacro(const char *, const char *) override;

#ifdef RB_ULID
	bool OnUserListSelection(int, const char*, Scintilla::Position) override; //!-change-[UserListItemID]
#else
	bool OnUserListSelection(int, const char *) override;
#endif // RB_ULID


	bool SendProperty(const char *) override;

#ifdef RB_ONKEY
	virtual bool OnKey(int, int, char); //!-change-[OnKey]
#else
	bool OnKey(int, int) override;
#endif // RB_ONKEY

	bool OnDwellStart(Scintilla::Position, const char *) override;
	bool OnClose(const char *) override;

#ifdef RB_OMC
	bool OnMenuCommand(int, int) override; //!-add-[OnMenuCommand]
#endif // RB_OMC

#ifdef RB_OnSendEditor
	const char* OnSendEditor(Scintilla::Message, uintptr_t, const char*) override; //!-add-[OnSendEditor]
	const char* OnSendEditor(Scintilla::Message, uintptr_t, long) override; //!-add-[OnSendEditor]
#endif

#ifdef RB_ONTABMOVE
	void OnTabMove(int, int) override;
#endif //RB_ONTABMOVE

	bool OnUserStrip(int control, int change) override;
	bool NeedsOnClose() override;

private:
	std::vector<Extension *> extensions;
	ExtensionAPI *host;
};

#endif
