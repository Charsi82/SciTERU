// SciTE - Scintilla based Text Editor
// LuaExtension.h - Lua scripting extension
// Copyright 1998-2000 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LUAEXTENSION_H
#define LUAEXTENSION_H

class LuaExtension : public Extension {
private:
	LuaExtension() noexcept; // Singleton

public:
	static LuaExtension &Instance() noexcept;

	// Deleted so LuaExtension objects can not be copied.
	LuaExtension(const LuaExtension &) = delete;
	LuaExtension(LuaExtension &&) = delete;
	LuaExtension &operator=(const LuaExtension &) = delete;
	LuaExtension &operator=(LuaExtension &&) = delete;

	~LuaExtension() override;

	bool Initialise(ExtensionAPI *host_) override;
	bool Finalise() noexcept override;
	bool Clear() override;
	bool Load(const char *filename) override;

	bool InitBuffer(int) override;
	bool ActivateBuffer(int) override;
	bool RemoveBuffer(int) override;

	bool OnOpen(const char *filename) override;
	bool OnSwitchFile(const char *filename) override;
	bool OnBeforeSave(const char *filename) override;
	bool OnSave(const char *filename) override;
	bool OnChar(char ch) override;
	bool OnExecute(const char *s) override;
	bool OnSavePointReached() override;
	bool OnSavePointLeft() override;
	bool OnStyle(Scintilla::Position startPos, Scintilla::Position lengthDoc, int initStyle, StyleWriter *styler) override;

#ifdef RB_ODBCLK
	bool OnDoubleClick(int modifiers) override; //!-change-[OnDoubleClick]
#else
	bool OnDoubleClick() override;
#endif // RB_ODBCLK

#ifdef RB_ONCLICK
	bool OnClick(int modifiers) override; //!-add-[OnClick]
#endif // RB_ONCLICK

#ifdef RB_OMBU
	bool OnMouseButtonUp(int modifiers) override; //!-add-[OnMouseButtonUp]
#endif // RB_OMBU

#ifdef RB_OHSC
	bool OnHotSpotReleaseClick(int modifiers) override; //!-add-[OnHotSpotReleaseClick]
#endif // RB_OHSC

	bool OnUpdateUI() override;
	bool OnMarginClick() override;

#ifdef RB_ULID
	bool OnUserListSelection(int listType, const char* selection, Scintilla::Position id) override; //!-change-[UserListItemID]
#else
	bool OnUserListSelection(int listType, const char *selection) override;
#endif // RB_ULID

#ifdef RB_ONKEY
	bool OnKey(int keyval, int modifiers, char ch) override; //!-change-[OnKey]
#else
	bool OnKey(int keyval, int modifiers) override;
#endif // RB_ONKEY

	bool OnDwellStart(Scintilla::Position pos, const char *word) override;
	bool OnClose(const char *filename) override;
	bool OnUserStrip(int control, int change) override;
	bool NeedsOnClose() override;

#ifdef RB_MACRO
	bool OnMacro(const char* p, const char* q) override; //!-add-[macro]
#endif // RB_MACRO

#ifdef RB_OnSendEditor
	bool OnMenuCommand(int, int) override; //!-add-[OnMenuCommand]
	const char* OnSendEditor(Scintilla::Message, uintptr_t, const char*) override; //!-add-[OnSendEditor]
	const char* OnSendEditor(Scintilla::Message, uintptr_t, long) override; //!-add-[OnSendEditor]
#endif // RB_OnSendEditor

#ifdef RB_ONTABMOVE
	void OnTabMove(int idx_from, int idx_to) override;
#endif //RB_ONTABMOVE

};

#endif
