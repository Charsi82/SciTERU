// TWL_CNTRLS.H
#pragma once
#include "twl.h"
#include "twl_utils.h"

//////// Wrappers around Windows Controls //////

class TControl : public TWin
{
public:
	TControl(TEventWindow* parent, const wchar_t* classname, const wchar_t* text, int id = -1, DWORD style = 0);
	void calc_size();

protected:
	virtual void calc_size_imp();
};

class THasIconCtrl
{
public:
	virtual void set_icon(const wchar_t* mod, int icon_id) = 0;
	virtual void set_bitmap(const wchar_t* file) = 0;
};

class TLabel : public TControl, public THasIconCtrl
{
public:
	TLabel(TEventWindow* parent, DWORD style);
	void set_text(const wchar_t* caption) override;
	void set_icon(const wchar_t* file, int icon_idx) override;
	void set_bitmap(const wchar_t* file) override;
};

class TEdit : public TControl
{
public:
	TEdit(TEventWindow* parent, const wchar_t* text, int id = -1, DWORD style = 0);
	void set_selection(int start, int finish);
};

class TProgressControl : public TControl
{
public:
	TProgressControl(TEventWindow* parent, int id, bool vertical, bool hasborder, bool smooth, bool smoothrevers);
	void set_range(int from, int to);
	void set_pos(int to);
	void set_step(int step);
	void go();
	int get_range(int& hi);
};

class TComboBox : public TControl
{
public:
	TComboBox(TEventWindow* parent, int id, DWORD style);
	void reset();
	int add_string(const wchar_t* str);
	void insert_string(int id, const wchar_t* str);
	int remove_item(int id);
	int find_string(int id, const wchar_t* str);
	std::wstring get_item_text(int id);
	void set_data(int id, int data);
	int get_data(int id);
	void set_height(int nsize);
	void set_cursel(int id);
	int get_cursel();
	int count();
};

class TButtonBase : public TControl
{
public:
	// standard Button styles
	enum class ButtonStyle
	{
		PUSHBUTTON,
		DEFPUSHBUTTON,
		CHECKBOX,
		AUTOCHECKBOX,
		RADIOBUTTON,
		B3STATE,
		AUTO3STATE,
		GROUPBOX,
		USERBUTTON,
		AUTORADIOBUTTON,
		LEFTTEXT = 0x20,
		ICON = 0x40,
		BITMAP = 0x80,
	};
	TButtonBase(TEventWindow* parent, const wchar_t* caption, int id, DWORD style = (DWORD)ButtonStyle::PUSHBUTTON);
	void check(int state);
	int check() const;

protected:
	void calc_size_imp() override;
};

class TButton : public TButtonBase, public THasIconCtrl
{
public:
	TButton(TEventWindow* parent, int id, const wchar_t* caption, ButtonStyle style = ButtonStyle::PUSHBUTTON);
	void set_icon(const wchar_t* mod, int icon_id) override;
	void set_bitmap(const wchar_t* file) override;
private:
	void calc_size_imp() override;
};

class TCheckBox : public TButtonBase
{
public:
	TCheckBox(TEventWindow* parent, const wchar_t* caption, int id, bool is3state = false);
};

class TRadioButton : public TButtonBase
{
public:
	TRadioButton(TEventWindow* parent, const wchar_t* caption, int id, bool is_auto = false);
};

class TGroupBox : public TControl
{
public:
	TGroupBox(TEventWindow* parent, const wchar_t* caption, DWORD text_align = 0);
};

class TListBox : public TControl
{
public:
	TListBox(TEventWindow* parent, int id, bool is_sorted = false);
	void add(const wchar_t* str, int data = 0);
	void insert(int i, const wchar_t* str);
	void remove(int i);
	void clear();
	//void redraw(bool on);
	int  count();
	void selected(int idx);
	int  selected() const;
	std::wstring get_text(int idx);
	size_t get_textlen(int idx);
	void  set_data(int i, int data);
	int get_data(int i);

private:
	virtual void clear_impl() = 0;
};

class TTrackBar : public TControl
{
private:
	bool m_redraw;

public:
	TTrackBar(TEventWindow* parent, DWORD style, int id);
	void redraw(bool yes) { m_redraw = yes; }
	bool redraw() const { return m_redraw; }
	void selection(int lMin, int lMax);
	void sel_start(int lStart);
	int  sel_start() const; // returns starting pos of current selection
	int  sel_end() const; // returns end pos
	void sel_clear();
	int  pos();
	void pos(int lPos);
	void range(int lMin, int lMax);
};
