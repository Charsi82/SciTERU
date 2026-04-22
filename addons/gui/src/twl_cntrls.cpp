// TWL_CNTRLS.CPP
// Steve Donovan, 2003
// This is GPL'd software, and the usual disclaimers apply.
// See LICENCE
/////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <tuple>
#include <string>
#include <list>
#include <vector>
#include <memory>

#include "twl.hpp"
#include "twl_cntrls.hpp"
#include "twl_utils.hpp"

///////////////////////////////////
namespace
{
	HICON load_icon(const wchar_t* file, int idx = 0, bool small_icon = true)
	{
		HICON hIcon = NULL;
		ExtractIconEx(file, idx, small_icon ? NULL : (&hIcon), small_icon ? (&hIcon) : NULL, 1);
		return hIcon;
	}

	HBITMAP load_bitmap(const wchar_t* file)
	{
		return (HBITMAP)LoadImage(NULL, file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
	}
}

auto load_icons_from(const wchar_t* path, bool bSmallIcon)
{
	const int nIconsize = bSmallIcon ? 16 : 32;
	auto hImageList = ImageList_Create(nIconsize, nIconsize, ILC_COLOR32 | ILC_MASK, 0, 32);
	ImageList_SetBkColor(hImageList, CLR_NONE);
	const int icon_cnt = ExtractIconEx(path, -1, NULL, NULL, 1);
	if (!icon_cnt) return std::make_tuple(0, hImageList);
	std::vector<HICON> hIcon(icon_cnt);
	ExtractIconEx(path, 0, NULL, hIcon.data(), icon_cnt);
	for (auto item : hIcon)
	{
		ImageList_AddIcon(hImageList, item);
		DeleteObject(item);
	}
	return std::make_tuple(icon_cnt, hImageList);
}

int THasIconWin::load_icons(const wchar_t* path, bool bSmallIcon)
{
	auto [icons_loaded, hImageList] = load_icons_from(path, bSmallIcon);
	if (icons_loaded)
	{
		set_image_list(bSmallIcon, hImageList);
		_has_image = true;
	}
	return icons_loaded;
}

///////////////////////////////////
/// Windows controls - TControl ///

TControl::TControl(TEventWindow* parent, const wchar_t* classname, const wchar_t* caption, DWORD style)
	:TWin(parent, classname, caption, style)//, m_text_color(CLR_NONE)
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
	//if (auto dc = get_parent_win()->get_dc())
	//{
	//	std::wstring tmp = get_text();
	//	SIZE p = dc->get_text_extent(tmp.c_str());
	//	resize(static_cast<int>(1.05 * p.cx), static_cast<int>(1.05 * p.cy));
	//}
	TDC dc(get_parent_win());
	std::wstring tmp = get_text();
	SIZE p = dc.get_text_extent(tmp.c_str());
	resize(static_cast<int>(1.05 * p.cx), static_cast<int>(1.05 * p.cy));
}

//////////////////////////////
// EditControl
TEdit::TEdit(TEventWindow* parent, const wchar_t* caption, DWORD style)
	: TControl(parent, WC_EDIT, caption, style | WS_BORDER | ES_AUTOHSCROLL)
{
	resize(50, 18);
}

void TEdit::set_selection(int start, int finish)
{
	send_msg(EM_SETSEL, start, finish);
}

//////////////////////////////
// ProgressControl
TProgressControl::TProgressControl(TEventWindow* parent, bool vertical, bool hasborder, bool smooth, bool smoothrevers)
	: TControl(parent, PROGRESS_CLASS, NULL,
		WS_TABSTOP |
		(vertical ? PBS_VERTICAL : 0) |
		(hasborder ? WS_BORDER : 0) |
		(smooth ? PBS_SMOOTH : 0) |
		(smoothrevers ? PBS_SMOOTHREVERSE : 0)
	)
{}

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

void TProgressControl::get_range(int& low, int& hi)
{
	PBRANGE rng{};
	send_msg(PBM_GETRANGE, 1, (LPARAM)&rng);
	hi = rng.iHigh;
	low = rng.iLow;
}

void TProgressControl::set_pos(int to)
{
	send_msg(PBM_SETPOS, to);
}

//////////////////////////////
// TComboBox
TComboBox::TComboBox(TEventWindow* parent, DWORD style)
	: TControl(parent, WC_COMBOBOX, nullptr, style | WS_VSCROLL | WS_TABSTOP)
{}

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
TButtonBase::TButtonBase(TEventWindow* parent, const wchar_t* caption, DWORD style)
	: TControl(parent, L"button", caption, style)
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
TButton::TButton(TEventWindow* parent, const wchar_t* caption, bool defpushbutton)
	:TButtonBase(parent, caption, defpushbutton ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON)
{}

void TButton::calc_size_imp()
{
	std::wstring tmp = get_text();
	TDC dc(get_parent_win());
	SIZE p = dc.get_text_extent(tmp.c_str());
	resize(p.cx + 10, p.cy + 10);
}

void TButton::set_icon(const wchar_t* path, int icon_idx)
{
	if (HICON hIcon = load_icon(path, icon_idx))
	{
		DWORD style = GetWindowLongPtr(handle(), GWL_STYLE);
		SetWindowLongPtr(handle(), GWL_STYLE, style | BS_ICON);
		send_msg(BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		DeleteObject(hIcon);
	}
}

void TButton::set_bitmap(const wchar_t* path)
{
	if (HBITMAP hBitmap = load_bitmap(path))
	{
		DWORD style = GetWindowLongPtr(handle(), GWL_STYLE);
		SetWindowLongPtr(handle(), GWL_STYLE, style | BS_ICON);
		send_msg(BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
		DeleteObject(hBitmap);
	}
}

//////////////////////////////
//  TCheckBox
TCheckBox::TCheckBox(TEventWindow* parent, const wchar_t* caption, bool is3state)
	: TButtonBase(parent, caption, is3state ? BS_AUTO3STATE : BS_AUTOCHECKBOX)
{
	if (caption && *caption)
		calc_size();
	else
		resize(20, 20);
}

void TButtonBase::calc_size_imp()
{
	// If the parent was a TComboBox, then it won't have a DC ready...
	//if (auto dc = get_parent_win()->get_dc())
	//{
	//	std::wstring tmp = get_text();
	//	SIZE p = dc->get_text_extent(tmp.c_str());
	//	resize(static_cast<int>(1.05 * p.cx + 30), static_cast<int>(1.05 * p.cy));
	//}
	TDC dc(get_parent_win());
	std::wstring tmp = get_text();
	SIZE p = dc.get_text_extent(tmp.c_str());
	resize(static_cast<int>(1.05 * p.cx + 30), static_cast<int>(1.05 * p.cy));
}

//////////////////////////////
//  TGroupBox
TGroupBox::TGroupBox(TEventWindow* parent, const wchar_t* caption, DWORD text_align)
	:TControl(parent, L"button", caption, BS_GROUPBOX |
		((text_align == 1) ? BS_CENTER : 0) | ((text_align == 2) ? BS_RIGHT : 0))
{}

//////////////////////////////
//  TRadioButton
TRadioButton::TRadioButton(TEventWindow* parent, const wchar_t* caption, bool stop_group)
	:TButtonBase(parent, caption, BS_AUTORADIOBUTTON | (stop_group ? WS_GROUP : 0))
{
	calc_size();
}

//////////////////////////////
// Label
TLabel::TLabel(TEventWindow* parent, DWORD style, const wchar_t* caption)
	: TControl(parent, WC_STATIC, NULL, style)
{
	set_text(caption);
}

void TLabel::set_icon(const wchar_t* path, int icon_idx)
{
	if (HICON hIcon = load_icon(path, icon_idx))
	{
		send_msg(STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		DeleteObject(hIcon);
	}
}

void TLabel::set_bitmap(const wchar_t* path)
{
	if (HBITMAP hBitmap = load_bitmap(path))
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
TListBox::TListBox(TEventWindow* parent, bool is_sorted)
	: TControl(parent, L"listbox", nullptr,
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
TTrackBar::TTrackBar(TEventWindow* parent, DWORD style)
	: TControl(parent, TRACKBAR_CLASS, nullptr, TBS_AUTOTICKS | TBS_TOOLTIPS | style), m_redraw(true)
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
