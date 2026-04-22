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

#include <Windows.h>
#include <Uxtheme.h>
#include <commctrl.h>
#include <richedit.h>
#include <list>
#include <format>
#include <cassert> // for assert

#include "twl_utils.hpp"
#include "twl.hpp"
#include "twl_menu.hpp"
#include "twl_cntrls.hpp"
#include "twl_notify.hpp"
#include "log.hpp"

constexpr wchar_t EW_CLASSNAME[] = L"EVNTWNDCLSS";

//static HWND hModeless = NULL;
//static Point g_mouse_pt_right{};
HINSTANCE hInst{};

// Miscelaneous functions!!
namespace
{
	COLORREF RGBF(float r, float g, float b)
	{
		return RGB(byte(255 * r), byte(255 * g), byte(255 * b));
	}
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
	switch (idx)
	{
	case Corner::TOP_LEFT:
		return Point(left, top);
	case Corner::TOP_RIGHT:
		return Point(right, top);
	case Corner::BOTTOM_LEFT:
		return Point(left, bottom);
	case Corner::BOTTOM_RIGHT:
		return Point(right, bottom);
	default:
		break;
	}
	return Point();
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

TDC::TDC(TWin* ptr) :m_hdc(NULL), m_pen(NULL), m_brush(NULL), m_twin(ptr)
{}

TDC::~TDC()
{
	if (m_pen) DeleteObject(m_pen);	m_pen = NULL;
	if (m_brush) DeleteObject(m_brush);	m_brush = NULL;
	kill();
}

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
	if (m_hdc) DeleteDC(m_hdc); m_hdc = NULL;
}

HGDIOBJ TDC::select(HGDIOBJ obj) const
{
	return SelectObject(m_hdc, obj);
}

void TDC::select_stock(int val)
{
	get();
	select(GetStockObject(val));
	release();
}

void TDC::xor_pen(bool on_off) const
{
	SetROP2(m_hdc, on_off ? R2_XORPEN : R2_COPYPEN);
}

// this changes both the _pen_ and the _text_ colour
void TDC::set_text_color(COLORREF rgb)
{
	get();
	SetTextColor(m_hdc, rgb);
	release();
}

void TDC::set_text_color(int r, int g, int b)
{
	set_text_color(RGB(r, g, b));
}

void TDC::set_solid_brush(COLORREF rgb)
{
	get();
	if (m_brush) DeleteObject(m_brush);
	m_brush = CreateSolidBrush(rgb);
	select(m_brush);
	release();
}

void TDC::set_hatch_brush(int style, COLORREF rgb)
{
	get();
	if (m_brush) DeleteObject(m_brush);
	m_brush = CreateHatchBrush(style, rgb);
	select(m_brush);
	release();
}

void TDC::set_pen(COLORREF rgb, int width, DWORD style)
{
	get();
	if (m_pen) DeleteObject(m_pen);
	m_pen = CreatePen(style, width, rgb);
	select(m_pen);
	release();
}

// after selecting stock pen
void TDC::reset_pen()
{
	get();
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
	SIZE sz{};
	//HFONT oldfont = NULL;
	get();
	//if (font) oldfont = select(*font);
	GetTextExtentPoint32(m_hdc, text, lstrlen(text), &sz);
	//if (font) select(oldfont);
	release();
	return sz;
}

// wrappers around common graphics calls
void TDC::draw_text(const wchar_t* msg, int x, int y) const
{
	TextOut(m_hdc, x, y, msg, lstrlen(msg));
}

void TDC::move_to(int x, int y) const
{
	MoveToEx(m_hdc, x, y, NULL);
}

void TDC::line_to(int x, int y) const
{
	LineTo(m_hdc, x, y/*,NULL*/);
}

void TDC::rectangle(const Rect& rt) const
{
	Rectangle(m_hdc, rt.left, rt.top, rt.right, rt.bottom);
}

void TDC::ellipse(const Rect& rt) const
{
	Ellipse(m_hdc, rt.left, rt.top, rt.right, rt.bottom);
}

void TDC::round_rect(const Rect& rt, int rw, int rh) const
{
	RoundRect(m_hdc, rt.left, rt.top, rt.right, rt.bottom, rw, rh);
}

void TDC::chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const
{
	Chord(m_hdc, x1, y1, x2, y2, x3, y3, x4, y4);
}

void TDC::polyline(const Point* pts, int npoints) const
{
	Polyline(m_hdc, pts, npoints);
}

void TDC::polygone(const Point* pts, int npoints) const
{
	Polygon(m_hdc, pts, npoints);
}

void TDC::draw_focus_rect(const Rect& rt) const
{
	DrawFocusRect(m_hdc, static_cast<const RECT*>(&rt));
}

void TDC::draw_line(const Point& p1, const Point& p2) const
{
	POINT pts[] = { {p1.x,p1.y},{p2.x,p2.y} };
	Polyline(m_hdc, pts, 2);
}

void TDC::set_pixel(int x, int y, COLORREF clr) const
{
	SetPixel(m_hdc, x, y, clr);
}

void TDC::polybezier(const Point* pts, DWORD npoints) const
{
	PolyBezier(m_hdc, pts, npoints);
}

void TDC::set_back_color(COLORREF clr) const
{
	SetBkColor(m_hdc, clr);
}

////// TWin ///////////
TWin::TWin(TEventWindow* parent, const wchar_t* winclss, const wchar_t* text, DWORD style)

{
	DWORD err;
	HWND hwndChild = CreateWindowEx(WS_EX_LEFT, winclss, text, WS_CHILD | WS_VISIBLE | style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent->m_hwnd, reinterpret_cast<HMENU>(parent->next_id()), hInst, NULL);
	if (hwndChild == NULL)
	{
		err = GetLastError();
		char tmp[MAX_PATH];
		strerror_s(tmp, MAX_PATH, err);
		std::string errstr = std::format("TWin::TWin can't create window error: {}", tmp);
		log_add(errstr.c_str());
	}
	set(hwndChild);
	set_parent_win(parent);
}

TWin::TWin(TEventWindow* form) : m_form(form)
{
	set(NULL);
}

TWin::TWin(HWND hwnd /*= NULL*/)
{
	m_form = nullptr;
	set(hwnd);
}

void TWin::set(HWND hwnd)
{
	m_hwnd = hwnd;
	m_align = Alignment::alNone;
}

TWin::~TWin()
{
	//log_add("~TWin[0x%d]", this);
}

void TWin::update()
{
	UpdateWindow(m_hwnd);
}

void TWin::invalidate(Rect* lprt) const
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

HWND TWin::parent_handle() const
{
	return GetParent(m_hwnd);
}

void TWin::get_client_rect(const Rect& rt) const
{
	GetClientRect(m_hwnd, (LPRECT)&rt);
}

void TWin::get_rect(Rect& rt, bool use_parent_client) const
{
	GetWindowRect(m_hwnd, (LPRECT)&rt);
	if (use_parent_client)
	{
		HWND hp = GetParent(m_hwnd);
		MapWindowPoints(NULL, hp, (LPPOINT)&rt, 2);
	}
}

void TWin::map_points(Point* pt, int n, TWin* target_wnd) const
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

std::wstring TWin::get_text() const
{
	std::wstring str;
	if (int len = GetWindowTextLength(m_hwnd))
	{
		++len;
		str.resize(len);
		GetWindowText(m_hwnd, str.data(), len);
	}
	return str;
}

// These guys work with the specified _child_ of the window

void TWin::set_text(int id, const wchar_t* str) const
{
	SetDlgItemText(m_hwnd, id, str);
}

void TWin::set_int(int id, int val) const
{
	SetDlgItemInt(m_hwnd, id, val, TRUE);
}

std::wstring TWin::get_text(int id) const
{
	std::wstring str;
	if (int len = GetWindowTextLength(GetDlgItem(m_hwnd, id)))
	{
		++len;
		str.resize(len);
		GetDlgItemText(m_hwnd, id, str.data(), len);
	}
	return str;
}

int TWin::get_ctrl_id() const
{
	return GetDlgCtrlID(m_hwnd);
}

int TWin::get_int(int id) const
{
	BOOL success;
	return static_cast<int>(GetDlgItemInt(m_hwnd, id, &success, TRUE));
}

//std::unique_ptr<TWin> TWin::get_active_window()
//{
//	return std::make_unique<TWin>(GetActiveWindow());
//}
//
//std::unique_ptr<TWin> TWin::get_foreground_window()
//{
//	return std::make_unique<TWin>(GetForegroundWindow());
//}

bool TWin::set_enable(bool state) const
{
	return EnableWindow(m_hwnd, state);
}

void TWin::remove_transparent() const
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

//void TWin::to_foreground() const
//{
//	SetForegroundWindow(m_hwnd);
//}

void TWin::set_focus() const
{
	SetFocus(m_hwnd);
}

void TWin::mouse_capture(bool do_grab) const
{
	if (do_grab) SetCapture(m_hwnd);
	else ReleaseCapture();
}

void TWin::close() const
{
	send_msg(WM_CLOSE);
}

int TWin::get_id() const
{
	return GetWindowLongPtr(m_hwnd, GWL_ID);
}

void TWin::resize(int x0, int y0, int w, int h) const
{
	MoveWindow(m_hwnd, x0, y0, w, h, TRUE);
}

void TWin::resize(const Rect& rt) const
{
	MoveWindow(m_hwnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, TRUE);
}

void TWin::resize(int w, int h) const
{
	SetWindowPos(m_hwnd, NULL, 0, 0, w, h, SWP_NOMOVE);
}

void TWin::move(int x0, int y0) const
{
	SetWindowPos(m_hwnd, NULL, x0, y0, 0, 0, SWP_NOSIZE);
}

void TWin::show(int how)
{
	ShowWindow(m_hwnd, how);
}

void TWin::hide()
{
	ShowWindow(m_hwnd, SW_HIDE);
}

bool TWin::visible() const
{
	return IsWindowVisible(m_hwnd);
}

void TWin::set_parent(TWin* w) const
{
	SetParent(m_hwnd, w ? w->handle() : NULL);
}

void TEventWindow::set_statusbar(int parts, int* widths)
{
	if (!statusbar_id)
	{
		statusbar_id = next_id();
		//HWND hWndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | SBARS_TOOLTIPS/*| SBARS_SIZEGRIP*/, nullptr, m_hwnd, statusbar_id);
		HWND hWndStatus = CreateWindowEx(
			0,                       // No extended styles
			STATUSCLASSNAME,         // Predefined class name for status bars
			NULL,           // No window text
			WS_CHILD | WS_VISIBLE |	  // Must be a visible child window
			SBARS_SIZEGRIP, // Includes a sizing grip
			0, 0, 0, 0,              // Position and size are handled by parent
			handle(),              // Handle to the parent window
			(HMENU)statusbar_id,   // Unique child-window identifier
			GetModuleHandle(NULL),   // Application instance handle
			NULL                     // No additional data
		);
		SendDlgItemMessage(handle(), statusbar_id, SB_SETPARTS, parts, (LPARAM)widths);
		TWin* pSBar = new TWin(hWndStatus);
		pSBar->align(Alignment::alBottom);
		add(pSBar);
	}
}

void TEventWindow::set_statusbar_text(int part_id, const wchar_t* str)
{
	if (statusbar_id)
		SendDlgItemMessage(handle(), statusbar_id, SB_SETTEXT, part_id, (LPARAM)str);
}

void TEventWindow::set_statusbar_bkcolor(COLORREF clr)
{
	if (statusbar_id)
	{
		SetWindowTheme(GetDlgItem(handle(), statusbar_id), L"", L"");
		SendDlgItemMessage(handle(), statusbar_id, SB_SETBKCOLOR, 0, (LPARAM)clr);
	}
}

void TEventWindow::set_tooltip(int id, const wchar_t* tiptext, const wchar_t* caption, bool balloon, bool close_btn, int icon)
{
	TOOLINFO ti{};
	DWORD style = WS_POPUP | TTS_ALWAYSTIP;
	if (balloon) style |= TTS_BALLOON;
	if (caption && close_btn) style |= TTS_CLOSE;
	HWND hwTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, handle(), NULL, hInst, NULL);
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	//ti.hwnd = m_hwnd;
	ti.uId = (UINT_PTR)GetDlgItem(handle(), id);
	//ti.hinst = hInst;
	ti.lpszText = (LPWSTR)tiptext;

	SendMessage(hwTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);

	SendMessage(hwTooltip, TTM_ACTIVATE, TRUE, 0);
	const DWORD ttl = 10000; // lifetime in 10 seconds
	SendMessage(hwTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP | TTDT_AUTOMATIC, MAKELPARAM((ttl), (0)));

	if (icon > TTI_ERROR_LARGE) icon = TTI_NONE;
	if (caption) SendMessage(hwTooltip, TTM_SETTITLE, icon, (LPARAM)caption);
}

DWORD TWin::get_style() const
{
	return GetWindowLongPtr(m_hwnd, GWL_STYLE);
}

void TWin::set_style(DWORD s) const
{
	SetWindowLongPtr(m_hwnd, GWL_STYLE, s);
}

LRESULT TWin::send_msg(UINT msg, WPARAM wparam, LPARAM lparam) const

{
	return SendMessage(m_hwnd, msg, wparam, lparam);
}

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
	return MessageBox({}, msg, title, flags) == retval;
}

void TWin::on_top() const  // *add 0.9.4
{
	SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

//-TEventWindow class member definitions--
TEventWindow::TEventWindow(const wchar_t* caption, TWin* parent, DWORD style_extra, bool is_child, DWORD style_override) :
	m_style_extra(style_extra), m_children{}, m_client(nullptr), statusbar_id(0) //, m_hACCEL(NULL)
{
	set_defaults();
	if (style_override != -1) m_style = style_override;
	create_window(caption, parent, is_child);
	//m_dc->set_text_align(0);
	enable_resize(true);
	log_add(std::format("TEventWindow 0x{}", (size_t)this).c_str());
}

void TEventWindow::create_window(const wchar_t* caption, TWin* parent, bool is_child)
{
	HWND hParent = NULL;
	if (parent)
	{
		hParent = parent->handle();
		if (is_child) m_style = WS_CHILD;
	}
	HWND hWnd = CreateWindowEx(m_style_extra, EW_CLASSNAME, caption, m_style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hParent, NULL, hInst, this);
	set(hWnd);
	align(Alignment::alNone);
	//set_window();
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
	m_old_cursor = LoadCursor(NULL, IDC_ARROW);
	cursor(CursorType::ARROW);
}

//void TEventWindow::add_handler(MessageHandler* m_hand)
//{
	//get_handler()->add_handler(m_hand);
//}

MessageHandler* TEventWindow::get_handler()
{
	if (!m_dispatcher) m_dispatcher = std::make_unique<MessageHandler>();
	return m_dispatcher.get();
}

bool TEventWindow::cant_resize() const
{
	return !m_do_resize;
}

//std::unique_ptr<TDC> TEventWindow::get_dc()
//{
//	return std::make_unique<TDC>(this);
//}

int TEventWindow::metrics(int ntype)
// Encapsulates what we need from GetSystemMetrics() and GetTextMetrics()
{
	if (ntype < TM_CAPTION_HEIGHT)
	{
		// text metrics
		TEXTMETRIC tm;
		TDC tDC(this);
		tDC.get();
		GetTextMetrics(tDC.get_hdc(), &tm);
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

POINT TEventWindow::fixed_size() const
{
	return m_fixed_size;
}

POINT TEventWindow::get_cursor_position() const
{
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(handle(), &p);
	return p;
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

UINT TEventWindow::next_id() const
{
	static UINT ctrl_id = 100;
	return ctrl_id++;
}

bool TEventWindow::check_notify(LPARAM lParam, int& ret)
{
	LPNMHDR ph = reinterpret_cast<LPNMHDR>(lParam);
	for (const auto win : m_children)
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

//void TEventWindow::set_window()
//{
//	align(Alignment::alNone);
//}

COLORREF TEventWindow::get_background() const
{
	return m_bk_color;
}

void TEventWindow::set_background(COLORREF rgb)
{
	if (m_bkgnd_brush) DeleteObject(m_bkgnd_brush);
	m_bkgnd_brush = CreateSolidBrush(rgb);
	//auto dc = get_dc();
	//dc->get(this);
	//SetBkColor(dc->get_hdc(), rgb);
	//dc->select(m_bkgnd_brush);
	//m_bk_color = rgb;
	//dc->release(this); 
	TDC dc(this);
	dc.get();
	SetBkColor(dc.get_hdc(), rgb);
	dc.select(m_bkgnd_brush);
	m_bk_color = rgb;
	dc.release(this);
	invalidate();
}

void TEventWindow::set_menu(HMENU menu)
{
	if (m_hmenu != menu)
	{
		DestroyMenu(m_hmenu);
		m_hmenu = menu;
	}
	SetMenu(handle(), m_hmenu);
}

void TEventWindow::set_popup_menu(HMENU menu)
{
	if (m_hpopup_menu) DestroyMenu(m_hpopup_menu);
	m_hpopup_menu = menu;
}

HMENU TEventWindow::get_main_menu()	   // todo: use GetMenu()
{
	if (m_hmenu) return m_hmenu;
	m_hmenu = CreateMenu();
	SetMenu(handle(), m_hmenu);
	return m_hmenu;
}

HMENU TEventWindow::get_popup_menu() const
{
	return m_hpopup_menu;
}

void TEventWindow::show(int how)
{
	// default:  use the 'nCmdShow' we were _originally_ passed
	if (how == 0) how = SW_SHOW;
	ShowWindow(handle(), how);
}

enum { uIDtimer = 100 };

void TEventWindow::create_timer(int msec)
{
	if (m_timer) kill_timer();
	m_timer = SetTimer(handle(), uIDtimer, msec, NULL);
}

void TEventWindow::kill_timer()
{
	KillTimer(handle(), uIDtimer);
}

constexpr int WM_QUIT_LOOP = 0x999;

// Message Loop!
// *NB* this loop shd cover ALL cases, controlled by global
// variables like mdi_sysaccell,  accell, hModelessDlg, etc.
LRESULT TEventWindow::run()
{
	MSG msg{};
	while (BOOL bRet = GetMessage(&msg, NULL, 0, 0))
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else if (bRet == 0 || msg.message == WM_QUIT_LOOP)
		{
			return msg.wParam;
		}
		else
		{
			//if (!GetAcceleratorTable() || !TranslateAccelerator(m_hwnd, GetAcceleratorTable(), &msg))
			//{
				//if (!hModeless || !IsDialogMessage(hModeless, &msg))
				//{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			//}
		//}
		}
	}
	return msg.wParam;
}

void TEventWindow::quit(int retcode)
{
	PostMessage(handle(), WM_QUIT_LOOP, retcode, 0);
}

TEventWindow::~TEventWindow()
{
	log_add(std::format("~TEventWindow 0x{}", (size_t)this).c_str());
	//DestroyAcceleratorTable(m_hACCEL);
	if (m_timer) kill_timer();
	int i = 0;
	for (const auto win : m_children)
	{
		try
		{
			delete win;
		}
		catch (const std::exception& ex)
		{
			//std::string fmt = std::format("del list item {} 0x{:x} : {}", ++i, (size_t)win, ex.what());
			//log_add(fmt.c_str());
		};
	}
	m_children.clear();
	DestroyMenu(m_hpopup_menu);
	DestroyMenu(m_hmenu);
	DestroyWindow(handle());
}

void TEventWindow::set_icon_impl(HICON hIcon, int small_icon)
{
	SetClassLongPtr(handle(), small_icon ? GCLP_HICONSM : GCLP_HICON, (LPARAM)hIcon);
}

void TEventWindow::set_icon(const wchar_t* file, int small_icon)
{
	if (HICON hIcon = (HICON)LoadImage(hInst, file, IMAGE_ICON, 0, 0, LR_LOADFROMFILE))
		set_icon_impl(hIcon, small_icon);
}

void TEventWindow::set_icon_from_window(TWin* win)
{
	if (HICON hIcon = (HICON)GetClassLongPtr(win->handle(), GCLP_HICON))
		set_icon_impl(hIcon, false);
	if (HICON hIconSM = (HICON)GetClassLongPtr(win->handle(), GCLP_HICONSM))
		set_icon_impl(hIconSM, true);
}

// *change 0.6.0 support for VCL-style alignment of windows
void TEventWindow::size()
{
	if (statusbar_id)
		SendDlgItemMessage(handle(), statusbar_id, WM_SIZE, 0, 0);

	size_t n = m_children.size();
	if (n == 0) return;
	Rect m;
	get_client_rect(m);
	//if (m_tool_bar) m.top += m_tool_bar->height();  //*6
	// we will only be resizing _visible_ windows with explicit alignments.
	for (const auto win : m_children)
		if (win->align() == Alignment::alNone || !win->visible()) n--;
	if (n == 0) return;
	HDWP hdwp = BeginDeferWindowPos(n);
	for (const auto win : m_children)
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

void TEventWindow::paint(TDC* pTDC) {}
bool TEventWindow::command(int, int) { return true; }
bool TEventWindow::sys_command(int) { return false; }
void TEventWindow::ncpaint(TDC*) {}
void TEventWindow::mouse_down(Point&) {}
void TEventWindow::mouse_up(Point&) {}
void TEventWindow::right_mouse_down(Point&) {}
void TEventWindow::mouse_move(Point&) {}
void TEventWindow::keydown(int) {}
void TEventWindow::destroy() {}
void TEventWindow::timer() {}
int  TEventWindow::notify(int id, void* ph)
{
	//if (id == statusbar_id)
	//{
	//	auto lpnm = (LPNMMOUSE)ph;
	//	std::wstring msg = std::format(L"sb clicked {}", lpnm->dwItemSpec);
	//	MessageBox(handle(), msg.c_str(), L"caption", MB_OK);
	//	return 1;
	//}
	return 0;
}
void TEventWindow::scroll(int code, int posn) {};
void TEventWindow::move() {};
LRESULT TEventWindow::run_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		size();
		return 0;
	}

	case WM_MOVE:
	{
		move();
		break;
	}

	case WM_GETMINMAXINFO:
	{
		if (cant_resize())
		{
			LPMINMAXINFO pSizeInfo = reinterpret_cast<LPMINMAXINFO>(lParam);
			//pSizeInfo->ptMaxTrackSize = This->fixed_size();
			//pSizeInfo->ptMinTrackSize = This->fixed_size();
			pSizeInfo->ptMaxTrackSize = pSizeInfo->ptMinTrackSize = fixed_size();
		}
		return 0;
	}

	case WM_CONTEXTMENU:
	{
		if (get_popup_menu() == NULL) break;
		Point cursor_pos{ LOWORD(lParam), HIWORD(lParam) };
		HWND wnd = WindowFromPoint(cursor_pos);
		if (wnd != hwnd) break;
		TrackPopupMenu(get_popup_menu(), TPM_LEFTALIGN | TPM_TOPALIGN,
			cursor_pos.x, cursor_pos.y, 0, hwnd, NULL);
		return 0;
	}

	case WM_NOTIFY:
	{
		int ret = 0;
		if (!check_notify(lParam, ret))
			return notify(static_cast<int>(wParam), reinterpret_cast<void*>(lParam));
		return ret;
	}

	case WM_COMMAND:
	{
		if (get_handler()->dispatch(LOWORD(wParam)))
			return 0;
		if (command(LOWORD(wParam), HIWORD(wParam)))
			return 0;
		break;
	}

	case WM_KEYDOWN:
		keydown(LOWORD(wParam));
		return 0;

	case WM_HSCROLL:
	case WM_VSCROLL:
	{
		const UINT ctrlID = static_cast<UINT>(GetWindowLongPtr((HWND)lParam, GWL_ID));
		//if (MessageHandler* disp = This->get_handler())
		//{
			//disp->dispatch(ctrlID);
		//}
		if (lParam)
		{
			switch (LOWORD(wParam))
			{
				//case SB_THUMBTRACK: // change position
			case SB_THUMBPOSITION: // stop changing
				scroll(ctrlID, HIWORD(wParam));
				break;

			case SB_ENDSCROLL: // end of scroll or click on scrollbar, wParam is 0
				scroll(ctrlID, SendMessage((HWND)lParam, TBM_GETPOS, 0, 0));
			}
		}
		return 0;
	}

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		//auto dc = get_dc();
		//dc->set_hdc(BeginPaint(hwnd, &ps));
		//paint(dc.get());
		//dc->set_hdc(NULL);
		//EndPaint(hwnd, &ps); 
		TDC dc(this);
		dc.set_hdc(BeginPaint(hwnd, &ps));
		paint(&dc);
		dc.set_hdc(NULL);
		EndPaint(hwnd, &ps);
		return 0;
	}

	// Mouse messages....
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_MOUSEMOVE:
	{
		Point mouse_pt{ LOWORD(lParam), HIWORD(lParam) };
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			mouse_down(mouse_pt);
			return 0;;

		case WM_LBUTTONUP:
			mouse_up(mouse_pt);
			return 0;

		case WM_RBUTTONDOWN:
			right_mouse_down(mouse_pt);
			return 0;

		case WM_MOUSEMOVE:
			mouse_move(mouse_pt);
			return 0;
		}
		return 0;
	}

	case WM_ERASEBKGND:
	{
		RECT rt;
		GetClientRect(hwnd, &rt);
		FillRect(reinterpret_cast<HDC>(wParam), (LPRECT)&rt, get_bkgnd_brush());
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
		cursor(CursorType::ARROW);
		break;

	case WM_SETFOCUS:
		focus();
		return 0;

	case WM_ACTIVATE:
		if (!activate(wParam != WA_INACTIVE)) return 0;
		break;

	case WM_SYSCOMMAND:
		if (sys_command(LOWORD(wParam))) return 0;
		else break;

	case WM_TIMER:
		timer();
		return 0;

	case WM_CLOSE:
		on_close();
		if (!query_close()) return 0;
		break; // let DefWindowProc handle this...

	case WM_SHOWWINDOW:
		on_showhide(!IsWindowVisible(hwnd));
		break; // let DefWindowProc handle this...

	case WM_DESTROY:
		destroy();
		//if (This->m_hmenu) DestroyMenu(This->m_hmenu);  // but why here?
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		LPCREATESTRUCT lpCreat = reinterpret_cast<LPCREATESTRUCT>(lParam);
		TEventWindow* This = static_cast<TEventWindow*>(lpCreat->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(This));
		return 0;
	}

	TEventWindow* This = reinterpret_cast<TEventWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (This) return This->run_wndProc(hwnd, msg, wParam, lParam);
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//----------------Window Registration----------------------

static bool been_registered = false;

void RegisterEventWindow(HICON hIcon = NULL, HCURSOR hCurs = NULL)
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
	wndclass.hCursor = hCurs;
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
	{
		//std::wstring tmp(MAX_PATH, 0);
		//auto ret = GetModuleFileName(NULL, tmp.data(), MAX_PATH);
		//if (tmp.erase(ret).ends_with(L"SciTE.exe")) {}
		hInst = hinstDLL;
		RegisterEventWindow();
		break;
	}

	case DLL_PROCESS_DETACH:
		UnregisterEventWindow();  // though it is important only on NT platform...
		break;
	};
	return TRUE;
}
