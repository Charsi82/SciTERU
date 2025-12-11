// TNotifyWin implementation
#include "twl_notify.h"
#include "twl_tab.h"
#include "twl_listview.h"
#include "twl_treeview.h"
#include <Uxtheme.h> // SetWindowTheme
#include <commctrl.h>

/////////////////////
// utils
extern HINSTANCE hInst;

namespace
{
	struct id_generator
	{
		static int next() { return _id++; }
		static int _id;
	};

	int id_generator::_id{ 445560 };

	HWND create_common_control(TWin* form, const wchar_t* winclass, DWORD style, int height = -1)
	{
		int w = CW_USEDEFAULT, h = CW_USEDEFAULT;
		if (height != -1) { w = 100; h = height; }
		return CreateWindowEx(0L,				// No extended styles.
			winclass, nullptr, WS_CHILD | style,
			0, 0, w, h,
			form->handle(),               // Parent window of the control.
			reinterpret_cast<HMENU>(id_generator::next()),
			GetModuleHandle(NULL), //hInst,								// Current instance.
			NULL);
	}

	void set_explorer(HWND hWND, bool yes)
	{
		SetWindowTheme(hWND, yes ? L"Explorer" : L"Normal", NULL);
	}
}

//////////////////////
// TNotifyWin
TNotifyWin::TNotifyWin(TEventWindow* form) :TWin(form), m_hpopup_menu(NULL)
{
}

TNotifyWin::~TNotifyWin()
{
	DestroyMenu(m_hpopup_menu);
}

void TNotifyWin::set_popup_menu(HMENU menu)
{
	m_hpopup_menu = menu;
}

void TNotifyWin::show_popup()
{
	if (m_hpopup_menu)
	{
		POINT p;
		GetCursorPos(&p);
		HWND hwnd = static_cast<HWND>(get_parent_win()->handle());
		TrackPopupMenu(m_hpopup_menu, TPM_LEFTALIGN | TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
	}
}

//////////////////////////////
// TSysLink
TSysLink::TSysLink(TEventWindow* form, const wchar_t* text) : TNotifyWin(form)
{
	HWND pLink = create_common_control(form, WC_LINK, WS_TABSTOP | WS_VISIBLE);
	set(pLink);
	SetWindowText(pLink, text);
	send_msg(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
}

int TSysLink::handle_notify(void* p)
{
	LPNMHDR np = reinterpret_cast<LPNMHDR>(p);
	switch (np->code)
	{
	case NM_CLICK:
	case NM_RETURN:
	{
		PNMLINK pNMLink = reinterpret_cast<PNMLINK>(p);
		shell_execute(pNMLink->item.szUrl);
		send_msg(LM_SETITEM, 0, (LPARAM)(&pNMLink->item));
		break;
	}
	}
	return 0;
}

////////////////////////////////
// TMemo
TMemo::TMemo(TEventWindow* form, int id, bool do_scroll, bool plain) : TNotifyWin(form), m_pfmt(nullptr)
{
	DWORD style = WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | ES_LEFT;
	if (do_scroll) style |= WS_HSCROLL | WS_VSCROLL;
	set(create_common_control(form, plain ? WC_EDIT : L"RICHEDIT", style));
	if (!plain)
	{
		m_pfmt = new CHARFORMAT{};
		m_pfmt->cbSize = sizeof(CHARFORMAT);
		m_pfmt->dwMask = 0;
		m_pfmt->dwEffects = 0;
	}
	send_msg(EM_SETEVENTMASK, 0, ENM_KEYEVENTS | ENM_MOUSEEVENTS);
}

TMemo::~TMemo()
{
	if (m_pfmt) delete m_pfmt;
	m_pfmt = nullptr;
}

void TMemo::set_font(const wchar_t* facename, int size, int flags, bool selection)
{
	m_pfmt->dwMask = CFM_FACE | CFM_BOLD | CFM_ITALIC;
	wcscpy_s(m_pfmt->szFaceName, facename);
	m_pfmt->dwEffects = 0;
	if (flags & BOLD) m_pfmt->dwEffects = CFE_BOLD;
	if (flags & ITALIC) m_pfmt->dwEffects |= CFE_ITALIC;
	send_char_format();
	m_pfmt->dwMask = 0;
	m_pfmt->dwEffects = 0;
	send_msg(EM_SETMARGINS, EC_LEFTMARGIN | EC_USEFONTINFO, 5);
}

void TMemo::cut()
{
	send_msg(WM_CUT);
}

void TMemo::copy()
{
	send_msg(WM_COPY);
}

void TMemo::clear()
{
	send_msg(WM_CLEAR);
}

void TMemo::paste()
{
	send_msg(WM_PASTE);
}

void TMemo::undo()
{
	send_msg(EM_UNDO);
}

int TMemo::text_size()
{
	return send_msg(WM_GETTEXTLENGTH);
}

void TMemo::replace_selection(const wchar_t* str)
{
	send_msg(EM_REPLACESEL, TRUE, (LPARAM)str);
}

bool TMemo::modified()
{
	return send_msg(EM_GETMODIFY);
}

void TMemo::modified(bool yesno)
{
	send_msg(EM_SETMODIFY, yesno ? TRUE : FALSE);
}

int TMemo::line_count()
{
	return send_msg(EM_GETLINECOUNT);
}

int TMemo::line_offset(int line)
{
	return send_msg(EM_LINEINDEX, line);
}

int TMemo::line_size(int line)
{
	return send_msg(EM_LINELENGTH, line);
}

int TMemo::get_line_text(int line, char* buff, int sz)
{
	//*(short*)(void*)buff = (short)sz;
	size_t len = send_msg(EM_GETLINE, line, (LPARAM)buff);
	buff[len] = '\0';
	return len;
}

void TMemo::get_selection(int& start, int& finish)
{
	send_msg(EM_GETSEL, (WPARAM)&start, (LPARAM)&finish);
}

void TMemo::set_selection(int start, int finish)
{
	send_msg(EM_SETSEL, start, finish);
}

void TMemo::select_all()
{
	set_selection(0, text_size());
}

void TMemo::go_to_end()
{
	set_selection(text_size(), text_size());
}

void TMemo::scroll_line(int line)
{
	send_msg(EM_LINESCROLL, 0, line);
}

int TMemo::line_from_pos(int pos)
{
	return send_msg(EM_LINEFROMCHAR, pos, 0);
}

void TMemo::scroll_caret()
{
	send_msg(EM_SCROLLCARET);
}

void TMemo::auto_url_detect(bool yn)
{
	send_msg(EM_AUTOURLDETECT, (WPARAM)yn, 0);
}

void TMemo::send_char_format()
{
	send_msg(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)m_pfmt);
}

void TMemo::find_char_format()
{
	send_msg(EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)m_pfmt);
}

COLORREF TMemo::get_text_colour()
{
	m_pfmt->dwMask = CFM_COLOR;
	find_char_format();
	m_pfmt->dwMask = 0;
	m_pfmt->dwEffects = 0;
	return m_pfmt->crTextColor;
}

void TMemo::set_text_colour(COLORREF colour)
{
	m_pfmt->dwMask = CFM_COLOR;
	m_pfmt->crTextColor = colour;
	send_char_format();
	m_pfmt->dwMask ^= CFM_COLOR;
}

void TMemo::set_background_colour(COLORREF colour)
{
	send_msg(EM_SETBKGNDCOLOR, 0, colour);
}

void TMemo::go_to(int idx1, int idx2, int nscroll)
{
	if (idx2 == -1) idx2 = idx1;
	set_focus();
	set_selection(idx1, idx2);
	scroll_caret();
	scroll_line(nscroll); //*SJD* Should have an estimate of the page size!!
}

int TMemo::current_pos()
{
	int start = 0, finish = 0;
	get_selection(start, finish);
	return start;
}

int TMemo::current_line()
{
	return line_from_pos(current_pos()) + 1;
}

void TMemo::go_to_line(int line)
{
	int ofs = line_offset(line - 1);
	go_to(ofs, -1, current_line() > line ? -10 : +10);
}

COLORREF TMemo::get_line_colour(int l)
{
	int offs = line_offset(l);
	set_selection(offs, offs);
	return get_text_colour();
}

void TMemo::set_line_colour(int line, COLORREF colour)
{
	int old = current_pos();
	int ofs1 = line_offset(line - 1), ofs2 = line_offset(line);
	set_selection(ofs1, ofs2);
	set_text_colour(colour);
	set_selection(old, old);
}

int TMemo::handle_notify(void* p)
{
	LPNMHDR np = reinterpret_cast<LPNMHDR>(p);
	switch (np->code)
	{
	case EN_MSGFILTER:
		const MSGFILTER* msf = reinterpret_cast<MSGFILTER*>(p);
		switch (msf->msg)
		{
		case WM_RBUTTONDOWN:
			show_popup();
			break;

		case WM_KEYDOWN:
			handle_onkey(static_cast<int>(msf->wParam));
		}
	}
	return 0;
}

///////////////////////
// TTabControl
TTabControl::TTabControl(TEventWindow* form) :TNotifyWin(form), m_index(0), m_last_selected_idx(0)
{
	// Create the tab control.
	DWORD style = WS_CHILD;
	set(create_common_control(form, WC_TABCONTROL, style, 25));
	send_msg(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
}

TTabControl::~TTabControl()
{
	for (int idx = 0; idx < m_index; ++idx)
		if (idx != m_last_selected_idx)
			if (TWin* p = panels[idx]) delete p;
	panels.clear();
}

void TTabControl::add(wchar_t* caption, TWin* data, int image_idx /*= -1*/)
{
	TCITEM item{};
	item.mask = TCIF_TEXT | TCIF_PARAM;
	item.pszText = caption;
	//item.lParam = (LPARAM)data;
	if (has_image())
	{
		item.mask |= TCIF_IMAGE;
		item.iImage = image_idx;
	}
	panels.push_back(data);
	send_msg(TCM_INSERTITEM, m_index++, (LPARAM)&item);
}

void* TTabControl::get_data(int idx)
{
	if (idx == -1) idx = selected();
	//TCITEM item{};
	//item.mask = TCIF_PARAM;
	//send_msg(TCM_GETITEM, idx, (LPARAM)&item);
	//return (void*)item.lParam;
	if (idx >= panels.size()) return nullptr;
	return panels[idx];
}

void TTabControl::remove(int idx /*= -1*/)
{
	if (idx == -1)
	{
		TabCtrl_DeleteAllItems(handle());
		m_last_selected_idx = m_index = 0;
		for (TWin*& p : panels)
			delete p;
		panels.clear();
	}
	else
	{
		TabCtrl_DeleteItem(handle(), idx);
		delete panels[idx];
		panels.erase(std::remove(panels.begin(), panels.end(), panels[idx]));
		if (m_last_selected_idx && (idx <= m_last_selected_idx)) m_last_selected_idx--;
	}
}

int TTabControl::getRowCount() const
{
	return send_msg(TCM_GETROWCOUNT);
}

void TTabControl::selected(int idx)
{
	send_msg(TCM_SETCURSEL, idx);
	NMHDR nmh{};
	nmh.code = static_cast<UINT>(TCN_SELCHANGE);
	nmh.idFrom = GetDlgCtrlID(handle());
	nmh.hwndFrom = handle();
	SendMessage(GetParent(handle()), WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
}

int TTabControl::selected()
{
	return m_last_selected_idx = send_msg(TCM_GETCURSEL);
}

int TTabControl::fixedwidth()
{
	return (get_style() & TCS_FIXEDWIDTH) ? 1 : 0;
}

void TTabControl::fixedwidth(int flag)
{
	DWORD style = get_style();
	if (flag)
		style |= TCS_FIXEDWIDTH;
	else
		style &= ~TCS_FIXEDWIDTH;
	set_style(style);
}

void TTabControl::set_item_size(int w, int h)
{
	send_msg(TCM_SETITEMSIZE, 0, MAKELPARAM(w, h));
}

int TTabControl::handle_notify(void* p)
{
	LPNMHDR np = reinterpret_cast<LPNMHDR>(p);
	int id = selected();
	switch (np->code)
	{
	case TCN_SELCHANGE:
		handle_select(id);
		return 1;
	case NM_RCLICK:
		show_popup();
	}
	return 0;
}

void TTabControl::set_image_list(bool small_size)
{
	TabCtrl_SetImageList(handle(), get_image_list());
}

//////////////////////////
// TListView
TListView::TListView(TEventWindow* form, bool multiple_columns, bool single_select, bool large_icons) :TNotifyWin(form)
{
	DWORD style = WS_CHILD | LVS_SHOWSELALWAYS;
	if (large_icons)
	{
		style |= LVS_ICON | LVS_AUTOARRANGE;
	}
	else
	{
		style |= LVS_REPORT;
		if (single_select)
			style |= LVS_SINGLESEL;
		if (!multiple_columns)
			style |= LVS_NOCOLUMNHEADER;
	}

	// Create the list view control.
	set(create_common_control(form, WC_LISTVIEW, style));
	m_custom_paint = false;
	m_last_col = 0;
	m_last_row = -1;
	m_bg = 0;
	m_fg = 0;
	send_msg(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT); // Set style
}

void TListView::set_image_list(bool small_size)
{
	send_msg(LVM_SETIMAGELIST, small_size ? LVSIL_SMALL : LVSIL_NORMAL, (LPARAM)get_image_list());
}

void TListView::add_column(const wchar_t* label, int width)
{
	LVCOLUMN lvc{};
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;   // left-align, by default
	lvc.cx = width;
	lvc.pszText = const_cast<wchar_t*>(label);
	lvc.iSubItem = m_last_col;

	ListView_InsertColumn(m_hwnd, m_last_col, &lvc);
	m_last_col++;
}

void TListView::set_foreground(COLORREF colour)
{
	send_msg(LVM_SETTEXTCOLOR, 0, (LPARAM)colour);
	m_fg = colour;
}

void TListView::set_background(COLORREF colour)
{
	send_msg(LVM_SETBKCOLOR, 0, (LPARAM)colour);
	m_bg = colour;
	m_custom_paint = true;
}

void TListView::set_theme(bool explorer)
{
	set_explorer(m_hwnd, explorer);
}

unsigned int TListView::columns() const
{
	return m_last_col;
}

void TListView::autosize_column(int col, bool by_contents)
{
	ListView_SetColumnWidth(m_hwnd, col, by_contents ? LVSCW_AUTOSIZE : LVSCW_AUTOSIZE_USEHEADER);
}

void TListView::start_items()
{
	m_last_row = -1;
}

int TListView::add_item_at(int i, const wchar_t* text, int idx, int data)
{
	LVITEM lvi{};
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	if (has_image())
	{
		lvi.mask |= LVIF_IMAGE;
		lvi.iImage = idx;                // image list index
	}
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.pszText = const_cast<wchar_t*>(text);
	if (data)
	{
		lvi.mask |= LVIF_PARAM;
		lvi.lParam = (LPARAM)data;
	}
	lvi.iItem = i;
	lvi.iSubItem = 0;

	ListView_InsertItem(m_hwnd, &lvi);
	return i;
}

int TListView::add_item(const wchar_t* text, int idx, int data)
{
	m_last_row++;
	return add_item_at(m_last_row, text, idx, data);
}

void TListView::add_subitem(int i, wchar_t* text, int idx)
{
	ListView_SetItemText(m_hwnd, i, idx, text);
}

void TListView::remove_item(int i)
{
	remove_item_impl(i);
	ListView_DeleteItem(m_hwnd, i);
}

void TListView::select_item(int i)
{
	if (i != -1)
	{
		ListView_SetItemState(m_hwnd, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(m_hwnd, i, true);
	}
	else
		ListView_SetItemState(m_hwnd, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
}

void TListView::get_item_text(int i, wchar_t* buff, int buffsize)
{
	ListView_GetItemText(m_hwnd, i, 0, buff, buffsize);
}

int TListView::get_item_data(int i)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	lvi.iSubItem = 0;
	ListView_GetItem(m_hwnd, &lvi);
	return lvi.lParam;
}

int TListView::selected_id()
{
	return send_msg(LVM_GETNEXTITEM, (WPARAM)(-1), LVNI_FOCUSED);
}

int TListView::next_selected_id(int i)
{
	return send_msg(LVM_GETNEXTITEM, i, LVNI_SELECTED);
}

int TListView::count()
{
	return send_msg(LVM_GETITEMCOUNT);
}

int TListView::selected_count()
{
	return send_msg(LVM_GETSELECTEDCOUNT);
}

void TListView::clear()
{
	clear_impl();
	send_msg(LVM_DELETEALLITEMS);
	m_last_row = -1;
}

static int list_custom_draw(void* lParam, COLORREF fg, COLORREF bg)
{
	LPNMLVCUSTOMDRAW  lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

	if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
		// Request prepaint notifications for each item.
		return CDRF_NOTIFYITEMDRAW;

	if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
	{
		lplvcd->clrText = fg;
		lplvcd->clrTextBk = bg;
		return CDRF_NEWFONT;
	}
	return 0;
}

int TListView::handle_notify(void* lparam)
{
	LPNMHDR np = reinterpret_cast<LPNMHDR>(lparam);
	switch (np->code)
	{
	case LVN_ITEMCHANGED:
	{
		handle_select(selected_id());
		return 1;
	}

	case NM_DBLCLK:
	{
		LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lparam);
		LVHITTESTINFO pInfo;
		pInfo.pt = lpnmitem->ptAction;
		ListView_SubItemHitTest(handle(), &pInfo);

		int i = pInfo.iItem;
		int j = pInfo.iSubItem;
		wchar_t buffer[10]{};
		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = i;
		item.iSubItem = j;
		item.cchTextMax = sizeof(buffer) / sizeof(buffer[0]);
		item.pszText = buffer;
		ListView_GetItem(handle(), &item);
		if (i > -1)
			handle_double_click(i, j, UTF8FromString(std::wstring(buffer)).c_str());
		return 0;
	}

	case LVN_KEYDOWN:
		handle_onkey(((LPNMLVKEYDOWN)lparam)->wVKey);
		return 0;  // ignored, anyway

	case NM_RCLICK:
		show_popup();
		return 0;

	case NM_SETFOCUS:
		handle_onfocus(true);
		return 1;

	case NM_KILLFOCUS:
		handle_onfocus(false);
		return 1;

	case NM_CUSTOMDRAW:
		return m_custom_paint ? list_custom_draw(lparam, m_fg, m_bg) : 0;
	}
	return 0;
}

//////////////////////
// TTreeView
TTreeView::TTreeView(TEventWindow* form, DWORD tree_style) :TNotifyWin(form)
{
	//DWORD style = TVS_HASLINES | TVS_LINESATROOT;
	set(create_common_control(form, WC_TREEVIEW, tree_style));
	ins_mode = TVI_LAST;
}

void TTreeView::set_theme(bool explorer)
{
	set_explorer(handle(), explorer);
}

void TTreeView::expand(HANDLE itm)
{
	send_msg(TVM_EXPAND, TVE_EXPAND, (LPARAM)itm);
}

void TTreeView::collapse(HANDLE itm)
{
	send_msg(TVM_EXPAND, TVE_COLLAPSE, (LPARAM)itm);
}

void TTreeView::makeLabelEditable(bool toBeEnabled)
{
	DWORD dwNewStyle = GetWindowLongPtr(handle(), GWL_STYLE);
	if (toBeEnabled)
		dwNewStyle |= TVS_EDITLABELS;
	else
		dwNewStyle &= ~TVS_EDITLABELS;
	::SetWindowLongPtr(handle(), GWL_STYLE, dwNewStyle);
}

void TTreeView::set_image_list(bool normal)
{
	send_msg(TVM_SETIMAGELIST, normal ? TVSIL_NORMAL : TVSIL_STATE, (LPARAM)get_image_list());
}

void TTreeView::set_foreground(COLORREF clr)
{
	send_msg(TVM_SETTEXTCOLOR, 0, clr);
}

void TTreeView::set_background(COLORREF clr)
{
	send_msg(TVM_SETBKCOLOR, 0, clr);
}

HANDLE TTreeView::get_root()
{
	return TreeView_GetRoot(handle()); // send_msg(TVM_get, 0, clr);
}

HANDLE TTreeView::get_next(HANDLE itm)
{
	return TreeView_GetNextSibling(handle(), itm);
}

HANDLE TTreeView::get_prev(HANDLE itm)
{
	return TreeView_GetPrevSibling(handle(), itm);
}

HANDLE TTreeView::get_child(HANDLE itm)
{
	return TreeView_GetChild(handle(), itm);
}

void TTreeView::clean_subitems(HANDLE itm)
{
	for (HANDLE hItem = get_child(itm); hItem != NULL; hItem = get_next(hItem))
	{
		TVITEM tvItem;
		tvItem.hItem = (HTREEITEM)hItem;
		tvItem.mask = TVIF_PARAM;
		//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
		TreeView_GetItem(handle(), &tvItem);
		if (int data = tvItem.lParam)
			clean_data(data);
		clean_subitems(hItem);
	}
	TVITEM tvItem;
	tvItem.hItem = (HTREEITEM)itm;
	tvItem.mask = TVIF_PARAM;
	//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	TreeView_GetItem(handle(), &tvItem);
	if (int data = tvItem.lParam)
		clean_data(data);
}

const std::vector<int> TTreeView::iterate_item(HANDLE itm)
{
	std::vector<int> res;

	TVITEM tvItem{};
	tvItem.hItem = (HTREEITEM)itm;
	tvItem.mask = TVIF_PARAM;
	//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	TreeView_GetItem(handle(), &tvItem);
	res.push_back(tvItem.lParam);

	for (HANDLE hItem = get_child(itm); hItem != NULL; hItem = get_next(hItem))
	{
		//TVITEM tvItem;
		tvItem.hItem = (HTREEITEM)hItem;
		tvItem.mask = TVIF_PARAM;
		//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
		TreeView_GetItem(handle(), &tvItem);
		res.push_back(tvItem.lParam);
	}
	return res;
}

void TTreeView::iterate_childs(HANDLE itm)
{
	TVITEM tvItem{};
	tvItem.hItem = (HTREEITEM)itm;
	tvItem.mask = TVIF_PARAM;
	TreeView_GetItem(handle(), &tvItem);
	clean_data(tvItem.lParam);

	for (HANDLE tvProj = get_child(itm);
		tvProj != NULL;
		tvProj = get_next(tvProj))
	{
		//TVITEM item_p{};
		//item_p.mask = TVIF_PARAM;
		//item_p.hItem = (HTREEITEM)tvProj;
		//TreeView_GetItem(handle(), &item_p);
		//clean_data(item_p.lParam);

		iterate_childs(tvProj);
	}
}

void TTreeView::remove_item(HANDLE itm)
{
	iterate_childs(itm);
	TreeView_DeleteItem(handle(), itm);
}

void TTreeView::remove_childs(HANDLE itm)
{
	while (HANDLE tvProj = get_child(itm))
		TreeView_DeleteItem(handle(), tvProj);

	TVITEM item_p{};
	item_p.mask = TVIF_CHILDREN;
	item_p.hItem = (HTREEITEM)itm;
	item_p.cChildren = 0;
	TreeView_SetItem(handle(), &item_p);
}

void TTreeView::clear()
{
	iterate_childs(get_root());
	TreeView_DeleteAllItems(handle());
}

void TTreeView::insert_mode(HANDLE mode)
{
	ins_mode = mode;
}

void TTreeView::insert_mode(const char* mode)
{
	if (!strcmp(mode, "last"))
		ins_mode = TVI_LAST;
	else if (!strcmp(mode, "first"))
		ins_mode = TVI_FIRST;
	else if (!strcmp(mode, "sort"))
		ins_mode = TVI_SORT;
	else if (!strcmp(mode, "root"))
		ins_mode = TVI_ROOT;
	else
		ins_mode = TVI_LAST;
}

HANDLE TTreeView::add_item(const wchar_t* caption, HANDLE parent, int idx1, int idx2, int data)
{
	TVITEM item{};
	item.mask = TVIF_TEXT | TVIF_CHILDREN;
	if (has_image())
	{
		item.mask |= (TVIF_IMAGE | TVIF_SELECTEDIMAGE);
		if (idx2 == -1) idx2 = idx1;
		item.iImage = idx1;
		item.iSelectedImage = idx2;
	}
	item.pszText = const_cast<wchar_t*>(caption);
	item.cchTextMax = MAX_PATH;
	item.cChildren = 0;
	if (data)
	{
		item.mask |= TVIF_PARAM;
		item.lParam = data;
	}

	TVINSERTSTRUCT tvsi{};
	tvsi.item = item;
	tvsi.hInsertAfter = (HTREEITEM)ins_mode; //(HTREEITEM)parent;

	if (parent)
	{
		tvsi.hParent = (HTREEITEM)parent;

		// для родителького элемента указываем что он родительский 
		TVITEM item_p{};
		item_p.mask = TVIF_CHILDREN;
		item_p.hItem = (HTREEITEM)parent;
		item_p.cChildren = 1;
		TreeView_SetItem(handle(), &item_p);
	}
	else
	{
		tvsi.hParent = TVI_ROOT;
	}
	return TreeView_InsertItem(handle(), &tvsi);
}

int TTreeView::get_item_data(HANDLE pn)
{
	//if (pn == NULL) pn = selected();
	TVITEM item{};
	item.mask = TVIF_PARAM;
	item.hItem = (HTREEITEM)pn;
	item.lParam = 0;
	send_msg(TVM_GETITEM, 0, (LPARAM)&item);
	return (int)item.lParam;
}

void TTreeView::select(HANDLE p)
{
	send_msg(TVM_SELECTITEM, TVGN_CARET, (LPARAM)p);
}

HANDLE TTreeView::get_item_by_name(const wchar_t* caption, HANDLE parent_item)
{

	wchar_t wchLabel[MAX_PATH]{};
	TVITEM tvItem{};
	tvItem.hItem = (HTREEITEM)(parent_item ? get_child(parent_item) : get_root());
	tvItem.mask = TVIF_TEXT;
	tvItem.pszText = wchLabel;
	tvItem.cchTextMax = MAX_PATH;
	//int idx = 0;
	while (tvItem.hItem)
	{
		if (TreeView_GetItem(handle(), &tvItem))
		{
			//log_add("item[%d]", idx);
			//log_add( UTF8FromString(tvItem.pszText).c_str());
			if (!wcscmp(tvItem.pszText, caption))
				return tvItem.hItem;
		}
		tvItem.hItem = (HTREEITEM)get_next(tvItem.hItem);
	}
	return NULL;
}

HANDLE TTreeView::get_item_parent(HANDLE item)
{
	return TreeView_GetParent(handle(), item);
}

HANDLE TTreeView::get_selected()
{
	return TreeView_GetSelection(handle());
}

void TTreeView::set_item_text(void* itm, const wchar_t* str)
{
	TVITEM tvi{};
	tvi.pszText = (LPWSTR)str;
	tvi.cchTextMax = MAX_PATH;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = (HTREEITEM)itm;
	send_msg(TVM_SETITEM, 0, (LPARAM)&tvi);
}

std::wstring TTreeView::get_item_text(HANDLE itm)
{
	TCHAR buffer[MAX_PATH]{};
	TVITEM tvi{};
	tvi.pszText = buffer;
	tvi.cchTextMax = MAX_PATH;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = (HTREEITEM)itm;
	send_msg(TVM_GETITEM, 0, (LPARAM)&tvi);
	return buffer;
}

int TTreeView::handle_notify(void* p)
{
	LPNMTREEVIEW np = reinterpret_cast<LPNMTREEVIEW>(p);
	switch (np->hdr.code)
	{
	case TVN_KEYDOWN:
	{
		LPNMTVKEYDOWN ptvkd = reinterpret_cast<LPNMTVKEYDOWN>(p);
		handle_onkey(ptvkd->wVKey);
		break;
	}

	case NM_RCLICK:
	{
		Point ptCursor;
		GetCursorPos(&ptCursor);
		ScreenToClient(handle(), &ptCursor);
		TVHITTESTINFO hitTestInfo{};
		hitTestInfo.pt.x = ptCursor.x;
		hitTestInfo.pt.y = ptCursor.y;
		HTREEITEM targetItem = reinterpret_cast<HTREEITEM>(send_msg(TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTestInfo)));
		if (targetItem)
		{
			TreeView_Select(handle(), targetItem, TVGN_CARET);
			show_popup(); // show popup if cursor over item
		}
		return 0;
	}

	case NM_DBLCLK:
	{
		handle_dbclick(get_selected());
		break;
	}

	case TVN_SELCHANGED:
	{
		//if (GetActiveWindow() == handle()) return 0;
		handle_select(get_selected());
		break;
	}

	case TVN_GETINFOTIP:
	{
		LPNMTVGETINFOTIP lpGetInfoTip = (LPNMTVGETINFOTIP)p;
		static TCHAR tips[MAX_PATH]{};
		size_t len = handle_ontip(lpGetInfoTip->hItem, tips);
		if (!len)
		{
			const std::wstring tip = get_item_text(lpGetInfoTip->hItem);
			wcscpy_s(tips, MAX_PATH, tip.data());
		}
		lpGetInfoTip->pszText = tips;
		lpGetInfoTip->cchTextMax = MAX_PATH;
		break;
	}

	//case TVN_BEGINLABELEDIT:
	//{
	//	auto hEdit = TreeView_GetEditControl(handle());
	//	SetFocus(hEdit);
	//	break;
	//}

	case TVN_ENDLABELEDIT:
	{
		LPNMTVDISPINFO lp = reinterpret_cast<LPNMTVDISPINFO>(p);
		if (lp->item.pszText && lstrlen(lp->item.pszText))
			return 1;
		break;
	}
	}
	return 0;
}

////////////////////////////////
// TUpDownControl
TUpDownControl::TUpDownControl(TEventWindow* form, TWin* buddy, DWORD style) : TNotifyWin(form)
{
	HWND hUDN = CreateUpDownControl(WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | style,
		0, 0, 0, 0, form->handle(), -1 /*id*/, hInst, buddy->handle(), 10, -10, 5);
	set(hUDN);
}

void TUpDownControl::set_range(int nUpper, int nLower)
{
	send_msg(UDM_SETRANGE32, nUpper, nLower);
}

void TUpDownControl::set_current(int _pos)
{
	send_msg(UDM_SETPOS32, 0, _pos);
}

int TUpDownControl::get_current()
{
	return send_msg(UDM_GETPOS32);
}

int TUpDownControl::handle_notify(void* p)
{
	LPNMHDR np = reinterpret_cast<LPNMHDR>(p);
	switch (np->code)
	{
	case UDN_DELTAPOS:
	{
		LPNMUPDOWN pNMLink = reinterpret_cast<LPNMUPDOWN>(p);
		ud_clicked(pNMLink->iDelta);
		break;
	}
	}
	return 0;
}