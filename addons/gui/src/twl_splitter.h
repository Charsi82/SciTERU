#pragma once
#include "twl.h"

class TSplitterB : public TEventWindow
{
protected:
	int m_split, m_new_size;
	CursorType m_cursor;
	bool m_line_visible, m_down, m_vertical;
	HDC m_line_dc;
	TWin* m_control;
	TWin* m_form;
	static Point m_start;

public:
	TSplitterB(TWin* parent, TWin* control, int thick = 3);

	// overrides
	void mouse_down(Point& pt) override;
	void mouse_move(Point& pt) override;
	void mouse_up(Point& pt) override;

protected:
	virtual void on_resize(const Rect& rt);

private:
	void draw_line();
	void update_size(short xx, short yy);
};

class TSplitter : public TSplitterB
{
public:
	TSplitter(TEventWindow* parent, TWin* control);
	void on_resize(const Rect& rt) override;
};
