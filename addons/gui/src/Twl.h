// TWL.H
// Steve Donovan, 2003
// This is GPL'd software, and the usual disclaimers apply.
// See LICENCE
//////////////////////////////////

#pragma once
#include <windows.h>
#include "windowsx.h"
#include <memory>
#include <list>
#include "utf.h"
#include "log.h"

// int to void*
#pragma warning (disable: 4312)
// lua_integer to int
#pragma warning (disable: 4244)
// size_t to int
#pragma warning (disable: 4267)
// warnings about 'int' to 'bool' conversions can be suppressed
#pragma warning (disable: 4800)
// returnig value ignored
#pragma warning (disable: 6031)

class TWin;
using ChildList = std::list<TWin*>;
class MessageHandler;
class TEventWindow;

struct Point : POINT
{
	explicit Point(int xp = 0, int yp = 0) :POINT{ xp, yp } {}
	void set(int xp, int yp) { x = xp; y = yp; }
};

struct Rect : RECT
{
	enum class Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT };
	explicit Rect(TEventWindow* pwin);
	explicit Rect(int x0 = 0, int y0 = 0, int x1 = 0, int y1 = 0) :RECT{ x0, y0, x1, y1 } {}
	bool is_inside(Point p) const;
	Point corner(Corner idx) const;
	int width() const;
	int height() const;
	void offset_by(int dx, int dy);
};

enum class Alignment { alNone, alTop, alBottom, alLeft, alRight, alClient };

// a basic wrapper for a HWND
class TWin
{
public:
	TWin(TEventWindow* parent, const wchar_t* winclss, const wchar_t* text, int id, DWORD style = 0);
	explicit TWin(HWND hwnd = NULL);
	explicit TWin(TEventWindow* form);
	void set(HWND hwnd);
	//std::unique_ptr<TWin> create_child(const wchar_t* winclss, const wchar_t* text, int id, DWORD styleEx = 0);
	virtual ~TWin();
	virtual void update();
	HWND handle() const { return m_hwnd; }
	HWND parent_handle() const;
	void  invalidate(Rect* lprt = nullptr) const;
	void  get_client_rect(const Rect& rt) const;
	void  get_rect(Rect& rt, bool use_parent_client = false) const;
	int   width() const;
	int   height() const;
	virtual void set_text(const wchar_t* str);
	void  get_text(std::wstring& str) const;
	void  set_text(int id, const wchar_t* str) const;
	void  get_text(int id, std::wstring& str);
	void  set_int(int id, int val) const;
	int   get_int(int id) const;
	int   get_ctrl_id() const;
	std::unique_ptr<TWin> get_active_window();
	int   get_id() const;
	void  set_focus() const;
	void  mouse_capture(bool do_grab) const;
	void  resize(int x0, int y0, int w, int h) const;
	void  resize(const Rect& rt) const;
	void  resize(int w, int h) const;
	void  move(int x0, int y0) const;
	void  map_points(Point* pt, int n, TWin* target_wnd = nullptr) const;
	void  on_top() const;
	void  to_foreground() const;
	virtual void  show(int how = SW_SHOW);
	virtual void  hide();
	bool  visible() const;
	void  set_parent(TWin* w = nullptr) const;
	DWORD get_style() const;
	void  set_style(DWORD s) const;
	LRESULT send_msg(UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const;
	void  close() const;
	static int message(const wchar_t* msg, int type = 0);
	Alignment align() const { return m_align; }
	void align(Alignment a, int size = 0);
	std::unique_ptr<TWin> get_foreground_window();
	bool set_enable(bool state) const;
	TEventWindow* get_parent_win() const { return m_form; };
	void set_parent_win(TEventWindow* form) { m_form = form; };
	void remove_transparent() const;
	void set_transparent(int);

protected:
	HWND m_hwnd;
	Alignment m_align;

private:
	TEventWindow* m_form;
};

///// Wrapping up the Windows Device Context
class TDC
{
public:
	TDC(TWin* ptr);
	~TDC();
	void set_hdc(HDC hdc) { m_hdc = hdc; }
	HDC get_hdc() const { return m_hdc; }
	void set_twin(TWin* w) { m_twin = w; }
	void get(TWin* pw = nullptr);
	void release(TWin* pw = nullptr);
	void kill() const;

	HGDIOBJ select(HGDIOBJ obj) const;
	void select_stock(int val);
	void reset_pen();

	void xor_pen(bool on_off) const;
	// this changes both the _pen_ and the _text_ colour
	void set_back_color(COLORREF clr) const;
	void set_text_color(int r, int g, int b);
	void set_text_color(COLORREF rgb);
	void set_pen(COLORREF rgb = 0, int width = 0, DWORD style = PS_SOLID);

	void set_solid_brush(COLORREF rgb = 0);
	void set_hatch_brush(int style, COLORREF rgb = 0);

	// wrappers around common graphics calls
	void set_text_align(int flags);
	SIZE get_text_extent(const wchar_t* text);
	void draw_text(const wchar_t* msg, int x = 0, int y = 0) const;
	void move_to(int x, int y) const;
	void line_to(int x, int y) const;
	void rectangle(const Rect& rt) const;
	void ellipse(const Rect& rt) const;
	void round_rect(const Rect& rt, int rw, int rh) const;
	void chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const;
	void polyline(const Point* pts, int npoints) const;
	void polygone(const Point* pts, int npoints) const;
	void polybezier(const Point* pts, DWORD npoints) const;
	void draw_focus_rect(const Rect& rt) const;
	void draw_line(const Point& p1, const Point& p2) const;
	void set_pixel(int x, int y, COLORREF clr) const;
												
private:
	//Handle m_hdc, m_pen, m_font, m_brush;
	HDC m_hdc;
	HPEN m_pen;
	HBRUSH m_brush;
	TWin* m_twin;
};

enum class CursorType { RESTORE, ARROW, HOURGLASS, SIZE_VERT, SIZE_HORZ, CROSS, HAND, UPARROW };

class TEventWindow : public TWin
{
	bool m_do_resize;
	POINT m_fixed_size;
	std::unique_ptr<MessageHandler> m_dispatcher;
	//Handle m_accel;
	HMENU m_hpopup_menu;
	//std::unique_ptr<TDC> m_dc;
	bool m_child_messages;
	DWORD m_style_extra;
	HCURSOR m_old_cursor;
	ChildList m_children;
	UINT statusbar_id;

public:
	enum
	{
		TM_CHAR_HEIGHT = 1,
		TM_CHAR_WIDTH, TM_CAPTION_HEIGHT, TM_MENU_HEIGHT, TM_CLIENT_EXTRA,
		TM_SCREEN_WIDTH, TM_SCREEN_HEIGHT, TM_END
	};
	explicit TEventWindow(const wchar_t* caption, TWin* parent = nullptr, DWORD style_extra = 0, bool is_child = false, DWORD style_override = -1);
	TEventWindow(TEventWindow& rhs) = delete;
	TEventWindow& operator=(TEventWindow& rhs) = delete;
	virtual ~TEventWindow();

	POINT fixed_size() const;
	void enable_resize(bool do_resize, int w = 0, int h = 0);
	bool cant_resize() const;
	//TDC* get_dc();
	std::unique_ptr<TDC> get_dc();
	bool child_messages() const { return m_child_messages; }
	void view_child_messages(bool yesno) { m_child_messages = yesno; }
	virtual void client_resize(int cwidth, int cheight);
	void set_statusbar(int parts, int* widths);
	void set_statusbar_text(int part_id, const wchar_t* str);
	void set_tooltip(int id, const wchar_t* tiptext, const wchar_t* caption, bool balloon, bool close_btn, int icon);
	void add(TWin* win);
	void remove(TWin* win);
	void set_client(TWin* cli);

	void set_defaults();
	void set_window();
	//void add_accelerator(Handle accel);
	void set_icon(const wchar_t* file);
	void set_icon_from_window(TWin* win);

	bool check_notify(LPARAM lParam, int& ret);
	void create_window(const wchar_t* caption, TWin* parent, bool is_child);
	void create_timer(int msec);
	void kill_timer();
	void set_menu(const wchar_t* res);
	void set_menu(HMENU menu);
	void set_popup_menu(HMENU menu);
	HMENU get_popup_menu() const;
	void last_mouse_pos(int& x, int& y);
	HMENU get_menu() { return m_hmenu; }
	void check_menu(int id, bool check) const;
	void add_handler(MessageHandler* m_hand);
	MessageHandler* get_handler();
	void quit(int retcode = 0);
	int metrics(int ntype);
	void cursor(CursorType curs);
	LRESULT run();
	HACCEL GetAcceleratorTable() { return m_hACCEL; }

	//-----------Event Handling-------------------
	  //virtual void show() {}
	virtual void size();
	virtual void paint(TDC*);
	virtual void ncpaint(TDC*);
	virtual bool query_close() { return true; }
	virtual void on_close() {}
	virtual void on_showhide(bool show) {}
	virtual bool activate(bool yes) { return true; }
	virtual void on_select(Rect& rt) {}

	// input
	virtual void keydown(int vkey);

	// mouse messages
	virtual void mouse_down(Point& pt);
	virtual void mouse_move(Point& pt);
	virtual void mouse_up(Point& pt);
	virtual void right_mouse_down(Point& pt);

	// scrolling
	// virtual void hscroll(int code, int posn);
	// virtual void vscroll(int code, int posn);
	virtual void scroll(int code, int posn);

	// command
	virtual bool command(int id, int code);
	virtual bool sys_command(int id);
	virtual int  notify(int id, void* ph);

	// other
	virtual void move();
	virtual void timer();
	virtual void focus();
	virtual void destroy();
	void show(int how = 0) override;

	void set_background(float r, float g, float b); // how to spec. colour??
	HBRUSH get_bkgnd_brush() const { return m_bkgnd_brush; }
	COLORREF get_bk_color() const { return m_bk_color; }

protected:
	HBRUSH m_bkgnd_brush;
	HMENU m_hmenu;
	DWORD m_style;
	UINT_PTR m_timer;
	COLORREF m_bk_color;
	TWin* m_client;
	HACCEL m_hACCEL;
};
