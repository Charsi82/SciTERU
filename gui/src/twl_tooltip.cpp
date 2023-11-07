// TToolTip
//////////////////////////////
#include "twl.h"
#include <CommCtrl.h>
#include "luabinder.h"
#include <Uxtheme.h>

class TToolTip
{
public:
	TToolTip(DWORD style, int id, HWND hparent);
	~TToolTip();
	static const char* classname() { return "TToolTip"; }
	void set_text(const wchar_t* str);
	void set_caption(int icon, const wchar_t* str);
	void set_color(DWORD clr, DWORD clrb);

private:
	void init_tooltip(const wchar_t* str);
	HWND parent_hwnd;
	HWND m_hwnd;
	int m_ID;
};

TToolTip::TToolTip(DWORD style, int id, HWND hparent) :parent_hwnd(hparent),
m_hwnd(CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL)), m_ID(id)
{ }

TToolTip::~TToolTip()
{
	if (m_hwnd)
		DestroyWindow(m_hwnd);
}

void TToolTip::init_tooltip(const wchar_t* str)
{
	TOOLINFO ti{};
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd = parent_hwnd;
	ti.uId = (UINT_PTR)GetDlgItem(parent_hwnd, m_ID);
	ti.lpszText = (LPWSTR)str;
	SendMessage(m_hwnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
	SendMessage(m_hwnd, TTM_ACTIVATE, TRUE, 0);
	const DWORD ttl = 10000; // lifetime in 10 seconds
	SendMessage(m_hwnd, TTM_SETDELAYTIME, TTDT_AUTOPOP | TTDT_AUTOMATIC, MAKELPARAM((ttl), (0)));
}

void TToolTip::set_text(const wchar_t* str)
{
	TOOLINFO ti{};
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = parent_hwnd;
	ti.uId = (UINT_PTR)GetDlgItem(parent_hwnd, m_ID);
	int res = SendMessage(m_hwnd, TTM_GETTOOLINFO, 0, (LPARAM)&ti);
	if (!res) return init_tooltip(str);
	ti.lpszText = (LPWSTR)str;
	SendMessage(m_hwnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
}

void TToolTip::set_caption(int icon, const wchar_t* str)
{
	SendMessage(m_hwnd, TTM_SETTITLE, icon, (LPARAM)str);
}

void DisableWinTheme(HWND hWnd)
{
	SetWindowTheme(hWnd, L" ", L" ");
}

void TToolTip::set_color(DWORD clr, DWORD clrb)
{
	DisableWinTheme(m_hwnd);
	SendMessage(m_hwnd, TTM_SETTIPTEXTCOLOR, clr, 0);
	SendMessage(m_hwnd, TTM_SETTIPBKCOLOR, clrb, 0);
}

///////////////////////////////////
TWin* window_arg(lua_State* L, int idx = 1);
#define check_ttip check_arg<TToolTip>
bool optboolean(lua_State* L, int idx, bool res = false);
COLORREF convert_colour_spec(const char* clrs);

int tooltip_set_text(lua_State* L)
{
	if (TToolTip* ttip = check_ttip(L))
		ttip->set_text(StringFromUTF8(luaL_checkstring(L, 2)).c_str());
	return 0;
}

int tooltip_set_color(lua_State* L)
{
	if (TToolTip* ttip = check_ttip(L))
	{
		DWORD clr_text = convert_colour_spec(luaL_checkstring(L, 2));
		DWORD clr_back = convert_colour_spec(luaL_checkstring(L, 3));
		ttip->set_color(clr_text, clr_back);
	}
	return 0;
}

int tooltip_set_caption(lua_State* L)
{
	if (TToolTip* ttip = check_ttip(L))
	{
		std::wstring caption = StringFromUTF8(luaL_checkstring(L, 2));
		unsigned int icon_idx = luaL_optinteger(L, 3, 0);
		ttip->set_caption(icon_idx, caption.c_str());
	}
	return 0;
}

const luaL_Reg LuaBinder<TToolTip>::metamethods[] =
{
	{ "__gc",		do_destroy<TToolTip> },
	{ NULL, NULL}
};

const luaL_Reg LuaBinder<TToolTip>::methods[] =
{
	{ "set_caption", tooltip_set_caption	},
	{ "set_text",	 tooltip_set_text		},
	{ "set_color",	 tooltip_set_color		},
	{ NULL, NULL}
};

void lua_openclass_TTipCtrl(lua_State* L)
{
	LuaBinder<TToolTip>().createClass(L);
}

int new_tooltip(lua_State* L)
{
	if (const TEventWindow* pWnd = dynamic_cast<TEventWindow*>(window_arg(L)))
	{
		const int id = luaL_checkinteger(L, 2);
		DWORD style = WS_POPUP | TTS_ALWAYSTIP;
		if (optboolean(L, 3)) style |= TTS_BALLOON;
		if (optboolean(L, 4)) style |= TTS_BALLOON | TTS_CLOSE;
		lua_push_newobject<TToolTip>(L, style, id, (HWND)pWnd->handle());
		return 1;
	}
	return 0;
}
