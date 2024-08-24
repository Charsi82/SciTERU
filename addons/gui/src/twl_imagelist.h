#pragma once
#include <CommCtrl.h>

class TImageList
{
	HIMAGELIST m_handle;
	bool m_small_icons;
public:
	HIMAGELIST handle() { return m_handle; }
	explicit TImageList(bool s = true);
	void create(int cx, int cy);
	int add_icon(const wchar_t* iconfile);
	int add(const wchar_t* bitmapfile, COLORREF mask_clr = 1);
	int load_icons_from_module(const wchar_t* mod);
	void set_back_colour(COLORREF clrRef);
	void load_shell_icons();
};
