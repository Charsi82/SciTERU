#pragma once
#include "Twl.h"
#include <richedit.h>
#include "twl_utils.h"

class TNotifyWin : public TWin
{
	HMENU m_hpopup_menu;
public:
	TNotifyWin(TEventWindow* form);
	~TNotifyWin();
	void set_popup_menu(HMENU menu);
	virtual int handle_notify(void* p) = 0;

protected:
	void show_popup();
};

class TSysLink : public TNotifyWin
{
public:
	TSysLink(TEventWindow* form, const wchar_t* text);
	int handle_notify(void* p) override;
};

class TMemo : public TNotifyWin
{
	enum { NORMAL, BOLD = 2, ITALIC = 4 };
protected:
	CHARFORMAT* m_pfmt;
public:
	TMemo(TEventWindow* form, int id, bool do_scroll = false, bool plain = false);
	TMemo(TMemo& rhs) = delete;
	TMemo& operator=(TMemo& rhs) = delete;
	~TMemo();
	void cut();
	void copy();
	void clear();
	void paste();
	//
	void undo();
	void select_all();
	int text_size();
	void replace_selection(const wchar_t* str);
	bool modified();
	void modified(bool yesno);
	void go_to_end();
	void scroll_line(int line);
	void scroll_caret();
	//
	int line_count();
	int line_offset(int line);
	int line_from_pos(int pos);
	int line_size(int line);
	int get_line_text(int line, char* buff, int sz);
	void get_selection(int& start, int& finish);
	void set_selection(int start, int finish);

	// Rich edit interface!
	void auto_url_detect(bool yn);
	void send_char_format();
	void find_char_format();
	COLORREF get_text_colour();
	void set_text_colour(COLORREF colour);
	void set_font(const wchar_t* facename, int size, int flags, bool selection = false);
	void go_to(int idx1, int idx2 = -1, int nscroll = 0);
	int current_pos();
	int current_line();
	void go_to_line(int line);
	COLORREF get_line_colour(int l);
	void set_line_colour(int line, COLORREF colour);
	void set_background_colour(COLORREF colour);
	int handle_notify(void* p) override;
private:
	virtual void handle_onkey(int id) = 0;
};

class TUpDownControl : public TNotifyWin
{
public:
	TUpDownControl(TEventWindow* form, TWin* buddy, DWORD style);
	int handle_notify(void* p) override;
	void set_range(int nUpper, int nLower);
	void set_current(int _pos);
	int get_current();
private:
	virtual void ud_clicked(int delta) = 0;
};