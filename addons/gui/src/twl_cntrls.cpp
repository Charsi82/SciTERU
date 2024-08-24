// TWL_CNTRLS.CPP
// Steve Donovan, 2003
// This is GPL'd software, and the usual disclaimers apply.
// See LICENCE
/////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include "twl_cntrls.h"

///////////////////////////////////
/// Windows controls - TControl ///

TControl::TControl(TEventWindow* parent, const wchar_t* classname, const wchar_t* text, int id, DWORD style)
	:TWin(parent, classname, text, id, style)//, m_text_color(CLR_NONE)
{
	send_msg(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
	//SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LPARAM)this);
}

void TControl::calc_size()
{
	calc_size_imp();
}

void TControl::calc_size_imp()
{
	if (TDC* dc = get_parent_win()->get_dc())
	{
		std::wstring tmp;
		get_text(tmp);
		SIZE p = dc->get_text_extent(tmp.c_str());
		resize(static_cast<int>(1.05 * p.cx), static_cast<int>(1.05 * p.cy));
	}
}

//////////////////////////////
// EditControl
TEdit::TEdit(TEventWindow* parent, const wchar_t* text, int id, DWORD style)
//-----------------------------------------------
	: TControl(parent, WC_EDIT, text, id, style | WS_BORDER | ES_AUTOHSCROLL)
{ }

void TEdit::set_selection(int start, int finish)
{
	send_msg(EM_SETSEL, start, finish);
}

//////////////////////////////
// ProgressControl
TProgressControl::TProgressControl(TEventWindow* parent, int id, bool vertical, bool hasborder, bool smooth, bool smoothrevers)
	: TControl(parent, PROGRESS_CLASS, NULL, id,
		WS_TABSTOP |
		(vertical ? PBS_VERTICAL : 0) |
		(hasborder ? WS_BORDER : 0) |
		(smooth ? PBS_SMOOTH : 0) |
		(smoothrevers ? PBS_SMOOTHREVERSE : 0)
	)
{ }

void TProgressControl::set_range(int from, int to)
{
	send_msg(PBM_SETRANGE32, to, from);
}

void TProgressControl::set_step(int step)
{
	send_msg(PBM_SETSTEP, step);
}

void TProgressControl::go()
{
	send_msg(PBM_STEPIT);
}

int TProgressControl::get_range(int& hi)
{
	PBRANGE rng;
	send_msg(PBM_GETRANGE, 1, (LPARAM)&rng);
	hi = rng.iHigh;
	return rng.iLow;
}

void TProgressControl::set_pos(int to)
{
	send_msg(PBM_SETPOS, to);
}

//////////////////////////////
// TComboBox
TComboBox::TComboBox(TEventWindow* parent, int id, DWORD style)
	: TControl(parent, WC_COMBOBOX, nullptr, id, style | WS_VSCROLL | WS_TABSTOP)
{ }

void TComboBox::reset()
{
	send_msg(CB_RESETCONTENT);
}

int TComboBox::add_string(const wchar_t* str)
{
	return send_msg(CB_ADDSTRING, 0, (LPARAM)str);
}

void TComboBox::insert_string(int id, const wchar_t* str)
{
	send_msg(CB_INSERTSTRING, id, (LPARAM)str);
}

int TComboBox::remove_item(int id)
{
	return send_msg(CB_DELETESTRING, id);
}

int TComboBox::find_string(int id, const wchar_t* str)
{
	return send_msg(CB_FINDSTRINGEXACT, id, (LPARAM)str);
}

std::wstring TComboBox::get_item_text(int id)
{
	size_t len = send_msg(CB_GETLBTEXTLEN, id);
	std::wstring str(len + 1, 0);
	send_msg(CB_GETLBTEXT, id, (LPARAM)str.data());
	return str;
}

void TComboBox::set_data(int id, int data)
{
	send_msg(CB_SETITEMDATA, id, (LPARAM)data);
}

int TComboBox::get_data(int id)
{
	return send_msg(CB_GETITEMDATA, id);
}

void TComboBox::set_cursel(int id)
{
	send_msg(CB_SETCURSEL, id);
}

int TComboBox::get_cursel()
{
	return send_msg(CB_GETCURSEL);
}

void TComboBox::set_height(int nsize)
{
	send_msg(CB_SETITEMHEIGHT, 0, (LPARAM)nsize);
}

int TComboBox::count()
{
	return send_msg(CB_GETCOUNT);
}

//////////////////////////////////
// TButton
TButtonBase::TButtonBase(TEventWindow* parent, const wchar_t* caption, int id, DWORD style)
	: TControl(parent, L"button", caption, id, style)
{
	calc_size();
}

void TButtonBase::check(int state)
{
	switch (state)
	{
	case BST_CHECKED:
		send_msg(BM_SETCHECK, BST_CHECKED);
		break;

	case BST_INDETERMINATE:
		send_msg(BM_SETCHECK, BST_INDETERMINATE);
		break;

	default:
		send_msg(BM_SETCHECK, BST_UNCHECKED);
	}
}

int TButtonBase::check() const
{
	return send_msg(BM_GETCHECK);
}

/////////////////////////////////////
/// Button
TButton::TButton(TEventWindow* parent, int id, const wchar_t* caption, ButtonStyle style)
	:TButtonBase(parent, caption, id, static_cast<DWORD>(style))
{ }

void TButton::calc_size_imp()
{
	std::wstring tmp;
	get_text(tmp);
	SIZE p = get_parent_win()->get_dc()->get_text_extent(tmp.c_str());
	resize(p.cx + 10, p.cy + 10);
}

void TButton::set_icon(const wchar_t* mod, int icon_id)
{
	DWORD style = GetWindowLongPtr((HWND)handle(), GWL_STYLE);
	SetWindowLongPtr((HWND)handle(), GWL_STYLE, style | (DWORD)ButtonStyle::ICON);
	if (HICON hIcon = load_icon(mod, icon_id, true))
	{
		send_msg(BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		DeleteObject(hIcon);
	}
}

void TButton::set_bitmap(const wchar_t* file)
{
	DWORD style = GetWindowLongPtr((HWND)handle(), GWL_STYLE);
	SetWindowLongPtr((HWND)handle(), GWL_STYLE, style | (DWORD)ButtonStyle::ICON);
	if (HBITMAP hBitmap = load_bitmap(file))
	{
		send_msg(BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		DeleteObject(hBitmap);
	}
}

//////////////////////////////
//  TCheckBox
TCheckBox::TCheckBox(TEventWindow* parent, const wchar_t* caption, int id, bool is3state)
	: TButtonBase(parent, caption, id, DWORD(is3state ? ButtonStyle::AUTO3STATE : ButtonStyle::AUTOCHECKBOX))
{
	if (caption && *caption)
		calc_size();
	else
		resize(20, 20);
}

void TButtonBase::calc_size_imp()
{
	// If the parent was a TComboBox, then it won't have a DC ready...
	if (TDC* dc = get_parent_win()->get_dc())
	{
		std::wstring tmp;
		get_text(tmp);
		SIZE p = dc->get_text_extent(tmp.c_str());
		resize(static_cast<int>(1.05 * p.cx + 30), static_cast<int>(1.05 * p.cy));
	}
}

//////////////////////////////
//  TGroupBox
TGroupBox::TGroupBox(TEventWindow* parent, const wchar_t* caption, DWORD text_align)
	:TControl(parent, L"button", caption, -1, (DWORD)TButtonBase::ButtonStyle::GROUPBOX |
		((text_align == 1) ? BS_CENTER : 0) + ((text_align == 2) ? BS_RIGHT : 0))
{ }

//////////////////////////////
//  TRadioButton
TRadioButton::TRadioButton(TEventWindow* parent, const wchar_t* caption, int id, bool is_auto)
	:TButtonBase(parent, caption, id, DWORD(is_auto ? ButtonStyle::AUTORADIOBUTTON : ButtonStyle::RADIOBUTTON))
{
	calc_size();
}

//////////////////////////////
// Label
TLabel::TLabel(TEventWindow* parent, DWORD style)
	: TControl(parent, WC_STATIC, nullptr, -1, style)
{ }

void TLabel::set_icon(const wchar_t* file, int icon_idx)
{
	if (HICON hIcon = load_icon(file, icon_idx))
	{
		send_msg(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		DeleteObject(hIcon);
	}
}

void TLabel::set_bitmap(const wchar_t* file)
{
	if (HBITMAP hBitmap = load_bitmap(file))
	{
		send_msg(STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		DeleteObject(hBitmap);
	}
}

void TLabel::set_text(const wchar_t* caption)
{
	TWin::set_text(caption);
	calc_size();
	invalidate();
}

//////////////////////////////
//  TListBox
TListBox::TListBox(TEventWindow* parent, int id, bool is_sorted)
	: TControl(parent, L"listbox", nullptr, id,
		LBS_NOTIFY | WS_VSCROLL | WS_BORDER | (is_sorted ? LBS_SORT : 0))
{}

void TListBox::add(const wchar_t* str, int data)
{
	send_msg(LB_ADDSTRING, 0, (LPARAM)data);
	if (data) set_data(count() - 1, data);
}

void TListBox::set_data(int i, int data)
{
	send_msg(LB_SETITEMDATA, (WPARAM)i, (LPARAM)data);
}

int TListBox::get_data(int i)
{
	return send_msg(LB_GETITEMDATA, (WPARAM)i);
}

void TListBox::insert(int i, const wchar_t* str)
{
	send_msg(LB_INSERTSTRING, (WPARAM)i, (LPARAM)str);
}

void TListBox::remove(int i)
{
	send_msg(LB_DELETESTRING, (WPARAM)i);
}

void TListBox::clear()
{
	clear_impl();
	send_msg(LB_RESETCONTENT);
}

//void TListBox::redraw(bool on)
//{
//	send_msg(WM_SETREDRAW, (WPARAM)(on ? TRUE : FALSE));
//}

int  TListBox::count()
{
	return send_msg(LB_GETCOUNT);
}

void TListBox::selected(int idx)
{
	send_msg(LB_SETCURSEL, (WPARAM)idx);
}

int  TListBox::selected() const
{
	return send_msg(LB_GETCURSEL);
}

std::wstring TListBox::get_text(int idx)
{
	std::wstring res(get_textlen(idx), 0);
	send_msg(LB_GETTEXT, idx, (LPARAM)res.data());
	return res;
}

size_t TListBox::get_textlen(int idx)
{
	return send_msg(LB_GETTEXTLEN, idx);
}

//////////////////////////////////////////
// TTrackBar
TTrackBar::TTrackBar(TEventWindow* parent, DWORD style, int id)
	: TControl(parent, TRACKBAR_CLASS, nullptr, id, TBS_AUTOTICKS | TBS_TOOLTIPS | style), m_redraw(true)
{}

void TTrackBar::selection(int lMin, int lMax)
{
	send_msg(TBM_SETSEL, m_redraw, MAKELONG(lMin, lMax));
}

void TTrackBar::sel_start(int lStart)
{
	send_msg(TBM_SETSELSTART, m_redraw, lStart);
}

int TTrackBar::sel_start() const // returns starting pos of current selection
{
	return send_msg(TBM_GETSELSTART);
}

int TTrackBar::sel_end() const// returns end pos
{
	return send_msg(TBM_GETSELEND);
}

void TTrackBar::sel_clear()
{
	send_msg(TBM_CLEARSEL, m_redraw);
}

int TTrackBar::pos()
{
	return send_msg(TBM_GETPOS);
}

void TTrackBar::pos(int lPos)
{
	send_msg(TBM_SETPOS, TRUE, lPos);
}

void TTrackBar::range(int lMin, int lMax)
{
	send_msg(TBM_SETRANGE, m_redraw, MAKELONG(lMin, lMax));
}
