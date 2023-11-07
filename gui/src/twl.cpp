// compile@ gcc -c -D_WINDOWS_ $(FileNameExt)
// TWL.CPP
// Main classes of YAWL
// Steve Donovan, 2003
// This is GPL'd software, and the usual disclaimers apply.
// See LICENCE
// Based on the Tiny, Terminal or Toy Windows Library
//  Steve Donovan, 1998
//  based on SWL, 1995.
////////////////////////////////////////////////////////////

#define NO_STRICT
#include "twl_menu.h"
#include <commctrl.h>
//#include <stdlib.h>
#include <ctype.h>
#include <cassert> // for assert
#include "twl_cntrls.h"
#include "twl_notify.h"

constexpr TCHAR EW_CLASSNAME[] = L"EVNTWNDCLSS";

static HANDLE hModeless = NULL;
static Point g_mouse_pt_right{};

// Miscelaneous functions!!
COLORREF RGBF(float r, float g, float b)
{
	return RGB(byte(255 * r), byte(255 * g), byte(255 * b));
}

Rect::Rect(TEventWindow* pwin)
{
	pwin->get_client_rect(*this);
}

bool Rect::is_inside(Point p) const
{
	return PtInRect(static_cast<const RECT*>(this), p);
}

Point Rect::corner(Corner idx) const
{
	int x, y;
	switch (idx)
	{
	case Corner::TOP_LEFT:
		x = left;
		y = top;
		break;
	case Corner::TOP_RIGHT:
		x = right;
		y = top;
		break;
	case Corner::BOTTOM_RIGHT:
		x = right;
		y = bottom;
		break;
	case Corner::BOTTOM_LEFT:
		x = left;
		y = bottom;
		break;
	default:
		x = y = 0;
		break;
	}
	return Point(x, y);
}

int Rect::width() const
{
	return right - left;
}

int Rect::height() const
{
	return bottom - top;
}

void Rect::offset_by(int dx, int dy)
{
	OffsetRect(static_cast<RECT*>(this), dx, dy);
}

/// TDC ///////////////

//TDC::TDC(TWin* ptr) :m_hdc(NULL), m_pen(NULL), m_font(NULL), m_brush(NULL), m_twin(ptr)
TDC::TDC(TWin* ptr) :m_hdc(NULL), m_pen(NULL), m_twin(ptr)
{ }

void TDC::get(TWin* pw)

{
	if (!pw) pw = m_twin;
	m_hdc = GetDC(pw->handle());
}

void TDC::release(TWin* pw)
{
	if (!pw) pw = m_twin;
	ReleaseDC(pw->handle(), m_hdc);
}

void TDC::kill()
{
	DeleteDC(m_hdc);
}

Handle TDC::select(Handle obj)
{
	return SelectObject(m_hdc, obj);
}

void TDC::select_stock(int val)
{
	select(GetStockObject(val));
}

void TDC::xor_pen(bool on_off)
{
	SetROP2(m_hdc, !on_off ? R2_COPYPEN : R2_XORPEN);
}

// this changes both the _pen_ and the _text_ colour
void TDC::set_colour(float r, float g, float b)
{
	COLORREF rgb = RGBF(r, g, b);
	get();
	SetTextColor(m_hdc, rgb);
	if (m_pen) DeleteObject(m_pen);
	m_pen = CreatePen(PS_SOLID, 0, rgb);
	select(m_pen);
	release();
}

void TDC::set_text_align(int flags)
{
	get();
	SetTextAlign(m_hdc, TA_UPDATECP | flags);
	release();
}

SIZE TDC::get_text_extent(const wchar_t* text)
{
	SIZE sz {0,0};
	//HFONT oldfont = NULL;
	get();
	//if (font) oldfont = select(*font);
	GetTextExtentPoint32(m_hdc, text, wcslen(text), &sz);
	//if (font) select(oldfont);
	release();
	return sz;
}

// wrappers around common graphics calls
void TDC::draw_text(const wchar_t* msg)
{
	TextOut(m_hdc, 0, 0, msg, wcslen(msg));
}

void TDC::move_to(int x, int y)
{
	MoveToEx(m_hdc, x, y, NULL);
}

void TDC::line_to(int x, int y)
{
	LineTo(m_hdc, x, y/*,NULL*/);
}

void TDC::rectangle(const Rect& rt)
{
	Rectangle(m_hdc, rt.left, rt.top, rt.right, rt.bottom);
}

void TDC::polyline(Point* pts, int npoints)
{
	Polyline(m_hdc, pts, npoints);
}

void TDC::draw_focus_rect(const Rect& rt)
{
	DrawFocusRect(m_hdc, static_cast<const RECT*>(&rt));
}

void TDC::draw_line(const Point& p1, const Point& p2)
{
	POINT pts[] = { {p1.x,p1.y},{p2.x,p2.y} };
	Polyline(m_hdc, pts, 2);
}

////// TWin ///////////
TWin::TWin(TEventWindow* parent, const wchar_t* winclss, const wchar_t* text, int id, DWORD style)

{
	DWORD err;
	HWND hwndChild = CreateWindowEx(WS_EX_LEFT, winclss, text, WS_CHILD | WS_VISIBLE | style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent->m_hwnd, (HMENU)id, hInst, NULL);
	if (hwndChild == NULL)
	{
		err = GetLastError();
		log_add("TWin::TWin can't create window error:%d", err);
	}
	set(hwndChild);
	set_parent_win(parent);
}

TWin::TWin(TEventWindow* form) : m_form(form)
{
	set(NULL);
}

TWin::TWin(Handle hwnd /*= NULL*/)
{
	m_form = nullptr;
	set(hwnd);
}

void TWin::set(Handle hwnd)
{
	m_hwnd = hwnd;
	m_align = Alignment::alNone;
}

TWin::~TWin()
{
	//log_add("~TWin[0x%d]", this);
}

void TWin::update()
//--
{
	UpdateWindow(m_hwnd);
}

void TWin::invalidate(Rect* lprt)
{
	InvalidateRect(m_hwnd, (LPRECT)lprt, TRUE);
}

void TWin::align(Alignment a, int size)
{
	m_align = a;
	if (size > 0 && a != Alignment::alNone)
	{
		if (a == Alignment::alRight || a == Alignment::alLeft)
			resize(size, 0);
		else
			resize(0, size);
	}
}

Handle TWin::parent_handle()
{
	return GetParent(m_hwnd);
}

void TWin::get_client_rect(const Rect& rt) const
{
	GetClientRect(m_hwnd, (LPRECT)&rt);
}

void TWin::get_rect(Rect& rt, bool use_parent_client)
{
	GetWindowRect(m_hwnd, (LPRECT)&rt);
	if (use_parent_client)
	{
		HWND hp = GetParent(m_hwnd);
		MapWindowPoints(NULL, hp, (LPPOINT)&rt, 2);
	}
}

void TWin::map_points(Point* pt, int n, TWin* target_wnd)
{
	HWND hwndTo = (target_wnd) ? target_wnd->handle() : GetParent(m_hwnd);
	MapWindowPoints(m_hwnd, hwndTo, (LPPOINT)pt, n);
}

int TWin::width() const
{
	Rect rt;
	get_client_rect(rt);
	return rt.right - rt.left;
}

int TWin::height() const
{
	Rect rt;
	get_client_rect(rt);
	return rt.bottom - rt.top;
}

void TWin::set_text(const wchar_t* str)
{
	SetWindowText(m_hwnd, str);
}

void TWin::get_text(std::wstring& str)
{
	int len = GetWindowTextLength(m_hwnd);
	if (len)
	{
		++len;
		str.resize(len);
		GetWindowText(m_hwnd, &str[0], len);
	}
}

// These guys work with the specified _child_ of the window

void TWin::set_text(int id, const wchar_t* str)
{
	SetDlgItemText(m_hwnd, id, str);
}

void TWin::set_int(int id, int val)
{
	SetDlgItemInt(m_hwnd, id, val, TRUE);
}

void TWin::get_text(int id, std::wstring& str)
{
	int len = GetWindowTextLength(GetDlgItem(m_hwnd, id));
	if (len)
	{
		++len;
		str.resize(len);
		GetDlgItemText(m_hwnd, id, &str[0], len);
	}
}

int TWin::get_ctrl_id()
{
	return GetDlgCtrlID(m_hwnd);
}

int TWin::get_int(int id)
{
	BOOL success;
	return (int)GetDlgItemInt(m_hwnd, id, &success, TRUE);
}

std::unique_ptr<TWin> TWin::get_active_window()
{
	return std::make_unique<TWin>(GetActiveWindow());
}

std::unique_ptr<TWin> TWin::get_foreground_window()
{
	return std::make_unique<TWin>(GetForegroundWindow());
}

bool TWin::set_enable(bool state)
{
	return EnableWindow(m_hwnd, state);
}

void TWin::remove_transparent()
{
	SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, ::GetWindowLongPtr(m_hwnd, GWL_EXSTYLE) & ~(WS_EX_LAYERED));
}

void TWin::set_transparent(int percent)
{
	SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, ::GetWindowLongPtr(m_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
	SetLayeredWindowAttributes(m_hwnd, 0, static_cast<BYTE>(percent), LWA_ALPHA);
	update();
}

void TWin::to_foreground()
{
	SetForegroundWindow(m_hwnd);
}

void TWin::set_focus()
{
	SetFocus((HWND)m_hwnd);
}

void TWin::mouse_capture(bool do_grab)
{
	if (do_grab) SetCapture(m_hwnd);
	else ReleaseCapture();
}

void TWin::close()
{
	send_msg(WM_CLOSE);
}

int TWin::get_id()
{
	return GetWindowLongPtr(m_hwnd, GWL_ID);
}

void TWin::resize(int x0, int y0, int w, int h)
{
	MoveWindow(m_hwnd, x0, y0, w, h, TRUE);
}

void TWin::resize(const Rect& rt)
{
	MoveWindow(m_hwnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, TRUE);
}

void TWin::resize(int w, int h)
{
	SetWindowPos(handle(), NULL, 0, 0, w, h, SWP_NOMOVE);
}

void TWin::move(int x0, int y0)
{
	SetWindowPos(handle(), NULL, x0, y0, 0, 0, SWP_NOSIZE);
}

void TWin::show(int how)
{
	ShowWindow(m_hwnd, how);
}

void TWin::hide()
{
	ShowWindow(m_hwnd, SW_HIDE);
}

bool TWin::visible()
{
	return IsWindowVisible(m_hwnd);
}

void TWin::set_parent(TWin* w)
{
	SetParent(m_hwnd, w ? w->handle() : NULL);
}

void TEventWindow::set_statusbar(int parts, int* widths)
{
	HWND st_wnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE | SBARS_TOOLTIPS/*| SBARS_SIZEGRIP*/, nullptr, m_hwnd, 255);
	statusbar_id = GetDlgCtrlID(st_wnd);
	SendDlgItemMessage(m_hwnd, statusbar_id, SB_SETPARTS, parts, (LPARAM)widths);
}

void TEventWindow::set_statusbar_text(int part_id, const wchar_t* str)
{
	if (statusbar_id)
		SendDlgItemMessage(m_hwnd, statusbar_id, SB_SETTEXT, part_id, (LPARAM)str);
}

//void TEventWindow::set_tooltip(int id, const wchar_t* tiptext, bool balloon)
//{
//	TOOLINFO ti{};
//	DWORD style = WS_POPUP | TTS_ALWAYSTIP;
//	if (balloon) style |= TTS_BALLOON;
//	HWND hwTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, style,
//		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, handle(), NULL, hInst, NULL);
//	ti.cbSize = sizeof(TOOLINFO);
//	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
//	ti.hwnd = m_hwnd;
//	ti.uId = (UINT_PTR)GetDlgItem(m_hwnd, id);
//	ti.hinst = hInst;
//	ti.lpszText = (LPWSTR)tiptext;
//
//	SendMessage(hwTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
//
//	SendMessage(hwTooltip, TTM_ACTIVATE, TRUE, 0);
//	const DWORD ttl = 10000; // lifetime in 10 seconds
//	SendMessage(hwTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP | TTDT_AUTOMATIC, MAKELPARAM((ttl), (0)));
//}

DWORD TWin::get_style()
{
	return GetWindowLongPtr(m_hwnd, GWL_STYLE);
}

void TWin::set_style(DWORD s)
{
	SetWindowLongPtr(m_hwnd, GWL_STYLE, s);
}

LRESULT TWin::send_msg(UINT msg, WPARAM wparam, LPARAM lparam) const

{
	return SendMessage(m_hwnd, msg, wparam, lparam);
}

//std::unique_ptr<TWin> TWin::create_child(const wchar_t* winclss, const wchar_t* text, int id, DWORD styleEx)
//
//{
//	return std::make_unique<TWin>((TEventWindow*)this, winclss, text, id, styleEx);
//}

int TWin::message(const wchar_t* msg, int type)
{
	enum
	{
		MSG_WARNING = 1, MSG_ERROR, MSG_QUERY
	};
	int flags = MB_OK;
	const wchar_t* title = L"Message";
	switch (type)
	{
	case MSG_ERROR:
	{
		flags = MB_ICONERROR | MB_OK;
		title = L"Error";
	}
	break;

	case MSG_WARNING:
	{
		flags = MB_ICONEXCLAMATION | MB_OKCANCEL;
		title = L"Warning";
	}
	break;

	case MSG_QUERY:
	{
		flags = MB_YESNO;
		title = L"Query";
	}
	break;
	}
	const int retval = (type == MSG_QUERY) ? IDYES : IDOK;
	return MessageBox(m_hwnd, msg, title, flags) == retval;
}

void TWin::on_top()  // *add 0.9.4
{
	SetWindowPos(handle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

//-TEventWindow class member definitions--
TEventWindow::TEventWindow(const wchar_t* caption, TWin* parent, DWORD style_extra, bool is_child, DWORD style_override) :
	m_style_extra(style_extra), m_children{}, m_client(nullptr), statusbar_id(0), m_hACCEL(NULL)
{
	set_defaults();
	if (style_override != -1) m_style = style_override;
	create_window(caption, parent, is_child);
	//m_dc->set_text_align(0);
	enable_resize(true);
	m_old_cursor = LoadCursor(NULL, IDC_ARROW);
	cursor(CursorType::ARROW);
}

void TEventWindow::create_window(const wchar_t* caption, TWin* parent, bool is_child)
{
	HWND hParent = NULL;
	void* CreatParms[]{ this, nullptr };
	//CreatParms[0] = (void*)this;
	if (parent)
	{
		hParent = parent->handle();
		if (is_child) m_style = WS_CHILD;
	}
	m_hwnd = CreateWindowEx(m_style_extra, EW_CLASSNAME, caption, m_style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hParent, NULL, hInst, CreatParms);
	set_window();
}

void TEventWindow::set_defaults()
{
	m_style = WS_OVERLAPPEDWINDOW;
	m_bk_color = GetSysColor(COLOR_BTNFACE);
	m_bkgnd_brush = CreateSolidBrush(m_bk_color);
	m_hmenu = NULL;
	m_hpopup_menu = NULL;
	m_timer = 0;
	m_dispatcher = nullptr;
	m_child_messages = false;
}

void TEventWindow::add_handler(MessageHandler* m_hand)
{
	get_handler()->add_handler(m_hand);
}

MessageHandler* TEventWindow::get_handler()
{
	if (!m_dispatcher) m_dispatcher = std::make_unique<MessageHandler>();
	return m_dispatcher.get();
}

bool TEventWindow::cant_resize() const
{
	return !m_do_resize;
}

TDC* TEventWindow::get_dc()
{
	if (!m_dc) m_dc = std::make_unique<TDC>(this);
	return m_dc.get();
}

int TEventWindow::metrics(int ntype)
// Encapsulates what we need from GetSystemMetrics() and GetTextMetrics()
{
	if (ntype < TM_CAPTION_HEIGHT)
	{
		// text metrics
		TEXTMETRIC tm;
		GetTextMetrics(get_dc()->get_hdc(), &tm);
		if (ntype == TM_CHAR_HEIGHT) return tm.tmHeight;
		else if (ntype == TM_CHAR_WIDTH)  return tm.tmMaxCharWidth;
	}
	else
	{
		switch (ntype)
		{
		case TM_CAPTION_HEIGHT:
			return GetSystemMetrics(SM_CYMINIMIZED);

		case TM_MENU_HEIGHT:
			return GetSystemMetrics(SM_CYMENU);

		case TM_CLIENT_EXTRA:
			if (m_style_extra & WS_EX_PALETTEWINDOW)
				return GetSystemMetrics(SM_CYSMCAPTION);
			else
				return metrics(TM_CAPTION_HEIGHT) + ((m_hmenu != NULL) ? metrics(TM_MENU_HEIGHT) : 0);

		case TM_SCREEN_WIDTH:
			return GetSystemMetrics(SM_CXMAXIMIZED);

		case TM_SCREEN_HEIGHT:
			return GetSystemMetrics(SM_CYMAXIMIZED);

		default:
			return 0;
		}
	}
	return 0;
}

void TEventWindow::client_resize(int cwidth, int cheight)
{
	// *SJD* This allows for menus etc
	int sz = 0;
	if (!(m_style & WS_CHILD))
		sz = metrics(TM_CLIENT_EXTRA);
	resize(cwidth, cheight + sz);
}

void TEventWindow::enable_resize(bool do_resize, int w, int h)
{
	m_do_resize = do_resize;
	m_fixed_size.x = w;
	m_fixed_size.y = h;
}

POINT TEventWindow::fixed_size()
{
	return m_fixed_size;
}

//*1 new cursor types
void TEventWindow::cursor(CursorType curs)
{
	HCURSOR new_cursor = 0;
	if (curs == CursorType::RESTORE)
		new_cursor = m_old_cursor;
	else
	{
		m_old_cursor = GetCursor();
		switch (curs)
		{
		case CursorType::ARROW:
			new_cursor = LoadCursor(NULL, IDC_ARROW);
			break;
		case CursorType::HOURGLASS:
			new_cursor = LoadCursor(NULL, IDC_WAIT);
			break;
		case CursorType::SIZE_VERT:
			new_cursor = LoadCursor(NULL, IDC_SIZENS);
			break;
		case CursorType::SIZE_HORZ:
			new_cursor = LoadCursor(NULL, IDC_SIZEWE);
			break;
		case CursorType::CROSS:
			new_cursor = LoadCursor(NULL, IDC_CROSS);
			break;
			//case CursorType::HAND: new_cursor = LoadCursor(NULL,IDC_HAND); break;
		case CursorType::UPARROW:
			new_cursor = LoadCursor(NULL, IDC_UPARROW);
			break;
		}
	}
	SetCursor(new_cursor);
}

bool TEventWindow::check_notify(LPARAM lParam, int& ret)
{
	LPNMHDR ph = (LPNMHDR)lParam;
	for (TWin* win : m_children)
	{
		if (ph->hwndFrom == win->handle())
			if (TNotifyWin* pnw = dynamic_cast<TNotifyWin*>(win))
			{
				ret = pnw->handle_notify(ph);
				return ret;
			}
	}
	return false;
}

void TEventWindow::set_window()
{
	set(m_hwnd);
}

void TEventWindow::set_background(float r, float g, float b)
{
	COLORREF rgb = RGBF(r, g, b);
	m_bkgnd_brush = CreateSolidBrush(rgb);
	get_dc()->get(this);
	SetBkColor(get_dc()->get_hdc(), rgb);
	get_dc()->select(m_bkgnd_brush);
	m_bk_color = rgb;
	get_dc()->release(this);
	invalidate();
}

void TEventWindow::set_menu(const wchar_t* res)
{
	if (m_hmenu) DestroyMenu(m_hmenu);
	set_menu(LoadMenu(hInst, res));
}

void TEventWindow::set_menu(HMENU menu)
{
	m_hmenu = menu;
	SetMenu(m_hwnd, menu);
}

void TEventWindow::set_popup_menu(HANDLE menu)
{
	if (m_hpopup_menu) DestroyMenu(m_hpopup_menu);
	m_hpopup_menu = menu;
}

HMENU TEventWindow::get_popup_menu()
{
	return m_hpopup_menu;
}

void TEventWindow::last_mouse_pos(int& x, int& y)
{
	POINT pt(g_mouse_pt_right);
	ScreenToClient(m_hwnd, &pt);
	x = pt.x;
	y = pt.y;
}

void TEventWindow::check_menu(int id, bool check)
{
	CheckMenuItem(m_hmenu, id, MF_BYCOMMAND | (check ? MF_CHECKED : MF_UNCHECKED));
}

void TEventWindow::show(int how)
{
	// default:  use the 'nCmdShow' we were _originally_ passed
	if (how == 0) how = SW_SHOW;
	ShowWindow(m_hwnd, how);
}

enum { uIDtimer = 100 };

void TEventWindow::create_timer(int msec)
{
	if (m_timer) kill_timer();
	m_timer = SetTimer(m_hwnd, uIDtimer, msec, NULL);
}

void TEventWindow::kill_timer()
{
	KillTimer(m_hwnd, uIDtimer);
}

constexpr int WM_QUIT_LOOP = 0x999;

// Message Loop!
// *NB* this loop shd cover ALL cases, controlled by global
// variables like mdi_sysaccell,  accell, hModelessDlg, etc.
LRESULT TEventWindow::run()
{
	BOOL bRet;
	MSG msg;
	while (bRet = GetMessage(&msg, NULL, 0, 0))
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else
		{
			if (msg.message == WM_QUIT_LOOP)
				return msg.wParam;
			if (!GetAcceleratorTable() || !TranslateAccelerator(m_hwnd, GetAcceleratorTable(), &msg))
			{
				if (!hModeless || !IsDialogMessage(hModeless, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}
	return msg.wParam;
}

void TEventWindow::quit(int retcode)
{
	PostMessage(m_hwnd, WM_QUIT_LOOP, retcode, 0);
}

TEventWindow::~TEventWindow()
{
	//log_add("~TEventWindow 0x%p", this);
	DestroyAcceleratorTable(m_hACCEL);
	if (m_timer) kill_timer();
	//int i = 0;
	for (TWin* win : m_children)
	{
		//log_add("del list item %d [0x%p]", ++i, p);
		delete win;
	}
	m_children.clear();
	DestroyMenu(m_hpopup_menu);
	DestroyMenu(m_hmenu);
	DestroyWindow((HWND)m_hwnd);
}

void TEventWindow::set_icon(const wchar_t* file)
{
	HANDLE hIcon = LoadImage(hInst, file, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SetClassLongPtr(m_hwnd, GCLP_HICON, (LPARAM)hIcon);
}

void TEventWindow::set_icon_from_window(TWin* win)
{
	HICON hIcon = (HICON)GetClassLongPtr((HWND)win->handle(), GCLP_HICON);
	SetClassLongPtr(m_hwnd, GCLP_HICON, (LPARAM)hIcon);
}

// *change 0.6.0 support for VCL-style alignment of windows
void TEventWindow::size()
{
	if (statusbar_id)
		SendDlgItemMessage(m_hwnd, statusbar_id, WM_SIZE, 0, 0);

	size_t n = m_children.size();
	if (n == 0) return;
	Rect m;
	get_client_rect(m);
	//if (m_tool_bar) m.top += m_tool_bar->height();  //*6
	// we will only be resizing _visible_ windows with explicit alignments.
	for (TWin* win : m_children)
		if (win->align() == Alignment::alNone || !win->visible()) n--;
	if (n == 0) return;
	HDWP hdwp = BeginDeferWindowPos(n);
	for (TWin* win : m_children)
	{
		if (!win->visible()) continue; //*new
		Rect wrt;
		//win->get_client_rect(wrt);
		win->get_rect(wrt, true);
		int left = m.left, top = m.top, w = wrt.width(), h = wrt.height();
		const int width_m = m.width(), height_m = m.height();
		switch (win->align())
		{
		case Alignment::alTop:
			w = width_m;
			m.top += h;
			break;

		case Alignment::alBottom:
			m.bottom -= h;
			top = m.bottom;
			w = width_m;
			break;

		case Alignment::alLeft:
			h = height_m;
			m.left += w;
			break;

		case Alignment::alRight:
			m.right -= w;
			left = m.right;
			h = height_m;
			break;

		case Alignment::alClient:
			h = height_m;
			w = width_m;
			break;

		case Alignment::alNone:
			continue;  // don't try to resize anything w/ no alignment.
		} // switch(...)
		DeferWindowPos(hdwp, win->handle(), NULL, left, top, w, h, SWP_NOZORDER);
	} // for(...)
	EndDeferWindowPos(hdwp);
}

// *add 0.6.0 can add a control to the child list directly
void TEventWindow::add(TWin* win)
{
	if (m_client)
		m_children.push_front(win);
	else
		m_children.push_back(win);
	win->show();
}

void TEventWindow::remove(TWin* win)
{
	if (win == m_client) m_client = nullptr;
	m_children.remove(win);
	win->hide();
	size();
}

// *change 0.6.0 set_client(), focus() has moved up from TFrameWindow
//*5 Note the special way that m_client is managed w.r.t the child list.
//void TEventWindow::set_client(TWin* cli, bool do_subclass)
void TEventWindow::set_client(TWin* cli)
{
	if (m_client)
	{
		m_client->hide();
		m_client = nullptr;
	}
	if (cli)
	{
		if (!m_children.empty() && m_children.back()->align() == Alignment::alClient)
			m_children.pop_back();
		add(cli);
		m_client = cli;
		m_client->align(Alignment::alClient);
		focus();
	}
}

void TEventWindow::focus()
{
	if (m_client) m_client->set_focus();
}

bool TEventWindow::command(int, int) { return true; }
bool TEventWindow::sys_command(int) { return false; }
void TEventWindow::paint(TDC*) { }
void TEventWindow::ncpaint(TDC*) { }
void TEventWindow::mouse_down(Point&) { }
void TEventWindow::mouse_up(Point&) { }
void TEventWindow::right_mouse_down(Point&) { }
void TEventWindow::mouse_move(Point&) { }
void TEventWindow::keydown(int) { }
void TEventWindow::destroy() { }
void TEventWindow::timer() { }
int  TEventWindow::notify(int id, void* ph) { return 0; }
void TEventWindow::scroll(int code, int posn) { };
void TEventWindow::move() { };

LRESULT CALLBACK WndProc(Handle hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//----------------Window Registration----------------------

static bool been_registered = false;

void RegisterEventWindow(HANDLE hIcon = NULL, HANDLE hCurs = NULL)
{
	WNDCLASS    wndclass{};
	wndclass.style =
		CS_HREDRAW | //перерисовывать окно при изменении вертикальных размеров
		CS_VREDRAW | //перерисовывать всё окно при изменении ширины 
		CS_OWNDC | //у каждого окна уникальный контекст устройства
		DS_LOCALEDIT;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(void*);
	wndclass.hInstance = hInst;
	wndclass.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = hCurs ? hCurs : NULL;
	wndclass.hbrBackground = NULL; //GetStockObject(LTGRAY_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = EW_CLASSNAME;
	RegisterClass(&wndclass);
	been_registered = true;
}

void UnregisterEventWindow()
{
	if (been_registered)
	{
		UnregisterClass(EW_CLASSNAME, hInst);
		been_registered = false;
	}
}

extern "C"  // inhibit C++ name mangling
BOOL APIENTRY DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved   // reserved
)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		hInst = hinstDLL;
		RegisterEventWindow();
		break;

	case DLL_PROCESS_DETACH:
		UnregisterEventWindow();  // though it is important only on NT platform...
		break;
	};
	return TRUE;
}

LRESULT CALLBACK WndProc(Handle hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TEventWindow* This = reinterpret_cast<TEventWindow*>(GetWindowLongPtr(hwnd, 0));

	switch (msg)
	{
	case WM_CREATE:
	{
		LPCREATESTRUCT lpCreat = reinterpret_cast<LPCREATESTRUCT>(lParam);
		PVOID* lpUser = reinterpret_cast<PVOID*>(lpCreat->lpCreateParams);

		//..... 'This' pointer passed as first part of creation parms
		This = reinterpret_cast<TEventWindow*>(lpUser[0]);
		SetWindowLongPtr(hwnd, 0, reinterpret_cast<LONG_PTR>(This));
		return 0;
	}

	case WM_SIZE:
	{
		This->size();
		return 0;
	}

	case WM_MOVE:
	{
		This->move();
		break;
	}

	case WM_GETMINMAXINFO:
	{
		if (This && This->cant_resize())
		{
			LPMINMAXINFO pSizeInfo = reinterpret_cast<LPMINMAXINFO>(lParam);
			//pSizeInfo->ptMaxTrackSize = This->fixed_size();
			//pSizeInfo->ptMinTrackSize = This->fixed_size();
			pSizeInfo->ptMaxTrackSize = pSizeInfo->ptMinTrackSize = This->fixed_size();
		}
		return 0;
	}

	case WM_PARENTNOTIFY:
		if (LOWORD(wParam) != WM_RBUTTONDOWN) break;
		g_mouse_pt_right.set(LOWORD(lParam), HIWORD(lParam));
		ClientToScreen(hwnd, reinterpret_cast<POINT*>(&g_mouse_pt_right));
		lParam = 0;
		// pass through.....
		[[fallthrough]];
	case WM_CONTEXTMENU:
	{
		if (This->get_popup_menu() == NULL) break;
		if (lParam != 0)
		{
			g_mouse_pt_right.set(LOWORD(lParam), HIWORD(lParam));
		}
		HWND wnd = WindowFromPoint(g_mouse_pt_right);
		if (wnd != This->handle()) break;
		TrackPopupMenu(This->get_popup_menu(), TPM_LEFTALIGN | TPM_TOPALIGN,
			g_mouse_pt_right.x, g_mouse_pt_right.y, 0, hwnd, NULL);
		return 0;
	}

	case WM_NOTIFY:
	{
		int ret = 0;
		if (!This->check_notify(lParam, ret))
			return This->notify(static_cast<int>(wParam), reinterpret_cast<void*>(lParam));
		else
			return ret;
	}

	case WM_COMMAND:
	{
		if (MessageHandler* disp = This->get_handler())
		{
			if (disp->dispatch(LOWORD(wParam)))
				return 0;
		}
		if (This->command(LOWORD(wParam), HIWORD(wParam)))
			return 0;
		else
			break;
	}

	case WM_KEYDOWN:
		This->keydown(LOWORD(wParam));
		return 0;

	case WM_HSCROLL:
	case WM_VSCROLL:
	{
		const UINT ctrlID = static_cast<UINT>(GetWindowLongPtr((HWND)lParam, GWL_ID));
		if (MessageHandler* disp = This->get_handler())
		{
			disp->dispatch(ctrlID);
		}
		if (lParam)
		{
			switch (LOWORD(wParam))
			{
				//case SB_THUMBTRACK: // change position
			case SB_THUMBPOSITION: // stop changing
				This->scroll(ctrlID, HIWORD(wParam));
				break;

			case SB_ENDSCROLL: // end of scroll or click on scrollbar, wParam is 0
				This->scroll(ctrlID, SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
			}
		}
		return 0;
	}

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		TDC* dc = This->get_dc();
		dc->set_hdc(BeginPaint(hwnd, &ps));
		This->paint(dc);
		dc->set_hdc(NULL);
		EndPaint(hwnd, &ps);
		return 0;
	}

	// Mouse messages....
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_MOUSEMOVE:
	{
		//g_mouse_pt.set(LOWORD(lParam), HIWORD(lParam));
		Point mouse_pt{ LOWORD(lParam), HIWORD(lParam) };
		//pt.to_logical(*This);
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			This->mouse_down(mouse_pt);
			break;

		case WM_LBUTTONUP:
			This->mouse_up(mouse_pt);
			break;

		case WM_RBUTTONDOWN:
			This->right_mouse_down(mouse_pt);
			break;

		case WM_MOUSEMOVE:
			This->mouse_move(mouse_pt);
			break;
		}
		return 0;
	}

	case WM_ERASEBKGND:
	{
		RECT rt;
		GetClientRect(hwnd, &rt);
		FillRect(reinterpret_cast<HDC>(wParam), (LPRECT)&rt, This->get_bkgnd_brush());
		return 0;
	}

	//case WM_CTLCOLORSTATIC:
	//{
	//	if (TControl* ctl = reinterpret_cast<TControl*>(GetWindowLongPtr((HWND)lParam, GWLP_USERDATA)))
	//	{
	//		SetBkColor(reinterpret_cast<HDC>(wParam), This->get_bk_color()); // цвет под тестом
	//		SetTextColor(reinterpret_cast<HDC>(wParam), ctl->get_text_color()); // цвет текста
	//	}
	//	return (LRESULT)This->get_bkgnd_brush(); // кисть с цветом общего фона
	//}

	case WM_SETCURSOR:
		if (This) This->cursor(CursorType::ARROW);
		//return 0;
		break;

	case WM_SETFOCUS:
		if (This) This->focus();
		return 0;

	case WM_ACTIVATE:
		if (This && !This->activate(wParam != WA_INACTIVE)) return 0;
		break;

	case WM_SYSCOMMAND:
		if (This->sys_command(LOWORD(wParam))) return 0;
		else break;

	case WM_TIMER:
		This->timer();
		return 0;

	case WM_CLOSE:
		This->on_close();
		if (!This->query_close()) return 0;
		break; // let DefWindowProc handle this...

	case WM_SHOWWINDOW:
		This->on_showhide(!IsWindowVisible(hwnd));
		break; // let DefWindowProc handle this...

	case WM_DESTROY:
		This->destroy();
		//if (This->m_hmenu) DestroyMenu(This->m_hmenu);  // but why here?
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
