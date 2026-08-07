#pragma once

class twBitMap;

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
	void kill();

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
	BOOL set_bitmap(twBitMap* pBitMap, int x, int y, int x1, int y1) const;
	BOOL stretch_bitmap(twBitMap* pBitMap, int x, int y, int x1, int y1) const;
	void polyline(const Point* pts, int npoints) const;
	void polygone(const Point* pts, int npoints) const;
	void polybezier(const Point* pts, DWORD npoints) const;
	void draw_focus_rect(const Rect& rt) const;
	void draw_line(const Point& p1, const Point& p2) const;
	void set_pixel(int x, int y, COLORREF clr) const;

private:
	//Handle m_hdc, m_pen, m_font, m_brush;
	HDC m_hdc{};
	HPEN m_pen{};
	HBRUSH m_brush{};
	TWin* m_twin{};
};
