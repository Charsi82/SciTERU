#pragma once
//#include "utf.h"

class TTreeView : public TNotifyWin, public THasIconWin
{
private:
	HANDLE ins_mode;
	HANDLE get_next(HANDLE itm);
	HANDLE get_child(HANDLE itm);
	HANDLE get_prev(HANDLE itm);
	void clean_subitems(HANDLE itm);
	void set_image_list(bool normal = true) override;

public:
	TTreeView(TEventWindow* form, DWORD tree_style);
	HANDLE add_item(const wchar_t* caption, HANDLE parent = NULL, int idx1 = 0, int idx2 = -1, int data = 0);
	void insert_mode(HANDLE mode);
	void insert_mode(const char* mode);
	int get_item_data(HANDLE pn);
	void select(HANDLE p);
	HANDLE get_item_parent(HANDLE item);
	HANDLE get_selected();
	HANDLE get_item_by_name(const wchar_t*, HANDLE);
	const std::vector<int> iterate_item(HANDLE itm);
	void clear();
	std::wstring get_item_text(HANDLE);
	void set_item_text(void* itm, const wchar_t* str);
	void set_foreground(COLORREF clr);
	void set_background(COLORREF clr);
	void makeLabelEditable(bool toBeEnabled);
	void set_theme(bool explorer);
	void expand(HANDLE itm);
	void collapse(HANDLE itm);
	HANDLE get_root();
	void iterate_childs(HANDLE itm);
	void remove_item(HANDLE itm);
	void remove_childs(HANDLE itm);
	int handle_notify(void* p) override;

private:
	virtual void clean_data(int) = 0;
	virtual void handle_select(HANDLE) = 0;
	virtual void handle_dbclick(HANDLE) = 0;
	virtual void handle_onkey(int id) = 0;
	virtual size_t handle_ontip(void* item, TCHAR* str) = 0;
};
