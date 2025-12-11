#pragma once

class TListView : public TNotifyWin, public THasIconWin
{
	unsigned int m_last_col;
	int m_last_row;
	bool m_custom_paint;
	COLORREF m_fg, m_bg;
	void set_image_list(bool small_size) override;

public:
	TListView(TEventWindow* form, bool multiple_columns = false, bool single_select = true, bool large_icons = false);
	void add_column(const wchar_t* label, int width);
	void autosize_column(int col, bool by_contents);
	void start_items();
	int  add_item_at(int i, const wchar_t* text, int idx = 0, int data = 0);
	int  add_item(const wchar_t* text, int idx = 0, int data = 0);
	void add_subitem(int i, wchar_t* text, int idx);
	void select_item(int i);
	void get_item_text(int i, wchar_t* buff, int buffsize);
	int  get_item_data(int i);
	int  selected_id();
	int  next_selected_id(int i);
	int  count();
	int  selected_count();
	unsigned int columns() const;
	void set_foreground(COLORREF colour);
	void set_background(COLORREF colour);
	void set_theme(bool explorer);
	int handle_notify(void* lparam) override;
	void remove_item(int i);
	void clear();

private:
	virtual void handle_select(int id) = 0;
	virtual void handle_double_click(int id, int j, const char* s) = 0;
	virtual void handle_onkey(int id) = 0;
	virtual void handle_onfocus(bool yes) = 0;
	virtual void clear_impl() = 0;
	virtual void remove_item_impl(int i) = 0;
};
