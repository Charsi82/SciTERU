// TToolTip
//////////////////////////////
#include "twl.h"
#include <CommCtrl.h>
#include "luabinder.h"
#include <Uxtheme.h>

class TToolTip
{
public:
	TToolTip(HWND hParent, int nID, bool isBaloon);
	~TToolTip();
	void remove_tip();
	static const char* classname() { return "TToolTip"; }
	void set_text(const wchar_t* str) const;
	void set_caption(int icon, const wchar_t* str) const;
	void set_color(DWORD clr, DWORD clrb) const;

private:
	HWND m_hParent;
	int m_nID;
	HWND m_hTT;
};

TToolTip::TToolTip(HWND hParent, int nID, bool isBaloon) :m_hParent(hParent), m_nID(nID)
{
	m_hTT = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | (isBaloon ? TTS_BALLOON : 0),
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, NULL, NULL);
	TOOLINFO ti{};
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = m_hParent;
	ti.uId = (UINT_PTR)GetDlgItem(m_hParent, m_nID);

	const DWORD ttl = 10000; // lifetime in 10 seconds
	SendMessage(m_hTT, TTM_SETDELAYTIME, TTDT_AUTOPOP | TTDT_AUTOMATIC, MAKELPARAM((ttl), (0)));
	SendMessage(m_hTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

TToolTip::~TToolTip()
{
	if (m_hTT)
		DestroyWindow(m_hTT);
}

void TToolTip::remove_tip()
{
	if (!m_hTT) return;
	TOOLINFO ti{};
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND;
	ti.hwnd = m_hParent;
	ti.uId = (UINT_PTR)GetDlgItem(m_hParent, m_nID);
	SendMessage(m_hTT, TTM_DELTOOL, 0, (LPARAM)&ti);
	DestroyWindow(m_hTT);
	m_hTT = NULL;
}

void TToolTip::set_text(const wchar_t* str) const
{
	TOOLINFO ti{};
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = m_hParent;
	ti.uId = (UINT_PTR)GetDlgItem(m_hParent, m_nID);
	ti.lpszText = (LPWSTR)str;
	SendMessage(m_hTT, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
}

void TToolTip::set_caption(int icon, const wchar_t* str) const
{
	SendMessage(m_hTT, TTM_SETTITLE, icon, (LPARAM)str);
}

void TToolTip::set_color(DWORD clr, DWORD clrb) const
{
	SetWindowTheme(m_hTT, L" ", L" "); // disable theme
	SendMessage(m_hTT, TTM_SETTIPTEXTCOLOR, clr, 0);
	SendMessage(m_hTT, TTM_SETTIPBKCOLOR, clrb, 0);
}

///////////////////////////////////
TWin* window_arg(lua_State* L, int idx = 1);
#define check_ttip check_arg<TToolTip>
bool optboolean(lua_State* L, int idx, bool res = false);
COLORREF lua_optColor(lua_State* L, int idx = 1, COLORREF def_clr = 0);

int tooltip_set_text(lua_State* L)
{
	if (TToolTip* ttip = check_ttip(L))
	{
		ttip->set_text(StringFromUTF8(luaL_checkstring(L, 2)).c_str());
		lua_settop(L, 1);
		return 1;
	}
	return 0;
}

int tooltip_set_color(lua_State* L)
{
	if (TToolTip* ttip = check_ttip(L))
	{
		DWORD clr_text = lua_optColor(L, 2);
		DWORD clr_back = lua_optColor(L, 3);
		ttip->set_color(clr_text, clr_back);
		lua_settop(L, 1);
		return 1;
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
		lua_settop(L, 1);
		return 1;
	}
	return 0;
}

int tooltip_remove(lua_State* L)
{
	if (TToolTip* ttip = check_ttip(L))
	{
		ttip->remove_tip();
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
	{ "remove",	     tooltip_remove			},
	{ NULL, NULL}
};

void lua_openclass_TTipCtrl(lua_State* L)
{
	LuaBinder<TToolTip>().createClass(L);
}

int add_tooltip(lua_State* L)
{
	if (const TEventWindow* pWnd = dynamic_cast<TEventWindow*>(window_arg(L)))
	{
		const int id = luaL_checkinteger(L, 2);
		lua_push_newobject<TToolTip>(L, pWnd->handle(), id, optboolean(L, 3));
		return 1;
	}
	return 0;
}
