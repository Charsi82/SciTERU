#pragma once
#include "utf.h"

class TTreeView : public TNotifyWin, public THasIconWin
{
private:
	Handle ins_mode;
	Handle get_next(Handle itm);
	Handle get_child(Handle itm);
	Handle get_prev(Handle itm);
	void clean_subitems(Handle itm);
	void set_image_list(bool normal = true) override;

public:
	TTreeView(TEventWindow* form, DWORD tree_style);
	Handle add_item(const wchar_t* caption, Handle parent = NULL, int idx1 = 0, int idx2 = -1, int data = 0);
	void insert_mode(Handle mode);
	void insert_mode(const char* mode);
	int get_item_data(Handle pn);
	void select(Handle p);
	Handle get_item_parent(Handle item);
	Handle get_selected();
	Handle get_item_by_name(const wchar_t*, Handle);
	const std::vector<int> iterate_item(Handle itm);
	void clear();
	std::wstring get_item_text(Handle);
	void set_item_text(void* itm, const wchar_t* str);
	void set_foreground(COLORREF clr);
	void set_background(COLORREF clr);
	void makeLabelEditable(bool toBeEnabled);
	void set_theme(bool explorer);
	void expand(Handle itm);
	void collapse(Handle itm);
	Handle get_root();
	void iterate_childs(Handle itm);
	void remove_item(Handle itm);
	void remove_childs(Handle itm);
	int handle_notify(void* p) override;

private:
	virtual void clean_data(int) = 0;
	virtual void handle_select(Handle) = 0;
	virtual void handle_dbclick(Handle) = 0;
	virtual void handle_onkey(int id) = 0;
	virtual size_t handle_ontip(void* item, TCHAR* str) = 0;
};
