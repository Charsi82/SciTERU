// TWL_TDC.CPP
// custom paint	wrapper
/////////////////////

#include <windows.h>
#include <list>
#include <string>
#include <vector>
#include <memory>

#include "Twl.hpp"
#include "lua.hpp"
#include "luabinder.hpp"
#include "utf.h"

////////////////////////////////////////////////////////////////////
constexpr const char* MT_TDC = "TDC*";
COLORREF lua_optColor(lua_State* L, int idx = 1, COLORREF def_clr = 0);
////////////////////////////////////////////////////////////////////

namespace
{
	struct twTDC
	{
		TDC* _h;
		twTDC(TDC* h) :_h(h) {}
		static const char* classname() { return "TDC*"; }
	};

	TDC* lua_checkTDC(lua_State* L)
	{
		twTDC* ptwTDC = check_arg<twTDC>(L);
		return ptwTDC ? ptwTDC->_h : nullptr;
	}

	int tdc_tostring(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			lua_pushstring(L, twTDC::classname());
			return 1;
		}
		return 0;
	}

	int tdc_rectangle(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			Rect rect(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5));
			pTDC->rectangle(rect);
		}
		return 0;
	}

	int tdc_ellipse(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			Rect rect(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5));
			pTDC->ellipse(rect);
		}
		return 0;
	}

	int tdc_round_rect(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			Rect rect(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5));
			int rw = luaL_optinteger(L, 6, 5);
			int rh = luaL_optinteger(L, 7, 5);
			pTDC->round_rect(rect, rw, rh);
		}
		return 0;
	}

	int tdc_chord(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			int x1 = luaL_checkinteger(L, 2);
			int y1 = luaL_checkinteger(L, 3);
			int x2 = luaL_checkinteger(L, 4);
			int y2 = luaL_checkinteger(L, 5);
			int x3 = luaL_checkinteger(L, 6);
			int y3 = luaL_checkinteger(L, 7);
			int x4 = luaL_checkinteger(L, 8);
			int y4 = luaL_checkinteger(L, 9);
			pTDC->chord(x1, y1, x2, y2, x3, y3, x4, y4);
		}
		return 0;
	}

	int tdc_draw_text(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			if (const char* txt = luaL_checkstring(L, 2))
			{
				pTDC->draw_text(StringFromUTF8(txt).c_str(), luaL_optinteger(L, 3, 0), luaL_optinteger(L, 4, 0));
			}
		}
		return 0;
	}

	int tdc_back_text(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			if (lua_gettop(L) == 2)
			{
				pTDC->set_back_color(lua_optColor(L, 2));
			}
			else
			{
				pTDC->set_back_color(GetSysColor(COLOR_BTNFACE));
			}
		}
		return 0;
	}

	int tdc_color_text(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			switch (lua_gettop(L))
			{
			case 2:
			{
				pTDC->set_text_color(lua_optColor(L, 2));
				break;
			}
			case 4:
			{
				const int r = luaL_checkinteger(L, 2);
				const int g = luaL_checkinteger(L, 3);
				const int b = luaL_checkinteger(L, 4);
				pTDC->set_text_color(r, g, b);
				break;
			}
			default:
				pTDC->set_text_color(0, 0, 0);
				break;
			}
		}
		return 0;
	}

	int tdc_move_to(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			const int posx = luaL_checkinteger(L, 2);
			const int posy = luaL_checkinteger(L, 3);
			pTDC->move_to(posx, posy);
		}
		return 0;
	}

	int tdc_line_to(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			const int posx = luaL_checkinteger(L, 2);
			const int posy = luaL_checkinteger(L, 3);
			pTDC->line_to(posx, posy);
		}
		return 0;
	}

	int tdc_draw_line(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			Point p1(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3));
			Point p2(luaL_checkinteger(L, 4), luaL_checkinteger(L, 5));
			pTDC->draw_line(p1, p2);
		}
		return 0;
	}

	std::vector<Point> lua_to_points(lua_State* L, int idx)
	{
		std::vector<Point> ret;
		while (lua_isinteger(L, idx) && lua_isinteger(L, idx + 1))
		{
			int x = lua_tointeger(L, idx++);
			int y = lua_tointeger(L, idx++);
			ret.emplace_back(x, y);
		}
		return ret;
	}

	int tdc_polyline(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			std::vector<Point> vp = lua_to_points(L, 2);
			if (vp.size()) pTDC->polyline(vp.data(), vp.size());
		}
		return 0;
	}

	int tdc_polygone(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			std::vector<Point> vp = lua_to_points(L, 2);
			if (vp.size()) pTDC->polygone(vp.data(), vp.size());
		}
		return 0;
	}

	int tdc_polybezier(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			std::vector<Point> vp = lua_to_points(L, 2);
			if (vp.size()) pTDC->polybezier(vp.data(), vp.size());
		}
		return 0;
	}

	int tdc_set_pixel(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			int x = luaL_checkinteger(L, 2);
			int y = luaL_checkinteger(L, 3);
			COLORREF clr = lua_optColor(L, 4);
			pTDC->set_pixel(x, y, clr);

		}
		return 0;
	}

	int tdc_set_pen(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			COLORREF clr = lua_optColor(L, 2);
			int width = luaL_optinteger(L, 3, 0);
			DWORD style = luaL_optinteger(L, 4, 0);
			pTDC->set_pen(clr, width, style);
		}
		return 0;
	}

	int tdc_set_xorpen(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			bool flag = lua_toboolean(L, 2);
			pTDC->xor_pen(flag);
		}
		return 0;
	}

	int tdc_reset_pen(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			pTDC->reset_pen();
		}
		return 0;
	}

	int tdc_set_solid_brush(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			COLORREF clr = lua_optColor(L, 2);
			pTDC->set_solid_brush(clr);
		}
		return 0;
	}

	int tdc_set_hatch_brush(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			COLORREF clr = lua_optColor(L, 2);
			int style = luaL_optinteger(L, 3, 0);
			pTDC->set_hatch_brush(style, clr);
		}
		return 0;
	}

	int tdc_set_text_align(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			int flags = luaL_checkinteger(L, 2);
			pTDC->set_text_align(flags);
		}
		return 0;
	}

	int tdc_select_stock(lua_State* L)
	{
		if (TDC* pTDC = lua_checkTDC(L))
		{
			int id = luaL_checkinteger(L, 2);
			pTDC->select_stock(id);
		}
		return 0;
	}
}

const luaL_Reg LuaBinder<twTDC>::metamethods[] =
{
	{ "__gc",		do_destroy<twTDC> },
	{ "__tostring", tdc_tostring },
	{ NULL, NULL }
};

const luaL_Reg LuaBinder<twTDC>::methods[] =
{
	{ "color_back", tdc_back_text		},
	{ "color_text", tdc_color_text		},
	{ "text_align", tdc_set_text_align	},
	{ "draw_text",  tdc_draw_text		},

	{ "reset_pen",	tdc_reset_pen	},
	{ "set_pen",	tdc_set_pen		},
	{ "xor_pen",	tdc_set_xorpen	},

	{ "solid_brush",tdc_set_solid_brush },
	{ "hatch_brush",tdc_set_hatch_brush },

	{ "move_to",	tdc_move_to		},
	{ "set_pixel",	tdc_set_pixel	},
	{ "line_to",	tdc_line_to		},
	{ "draw_line",	tdc_draw_line	},
	{ "polyline",	tdc_polyline	},
	{ "polybezier",	tdc_polybezier	},
	{ "rectangle",  tdc_rectangle	},
	{ "polygone",   tdc_polygone	},
	{ "ellipse",	tdc_ellipse		},
	{ "round_rect",	tdc_round_rect	},
	{ "chord",		tdc_chord		},

	{ "select_stock", tdc_select_stock },
	{ NULL, NULL }
};

void lua_openclass_TDC(lua_State* L)
{
	LuaBinder<twTDC>().createClass(L);
}

void lua_pushTDC(lua_State* L, TDC* hTDC)
{
	lua_push_newobject<twTDC>(L, hTDC);
	/*	lua_pushlightuserdata(L, hTDC);
		luaL_getmetatable(L, MT_TDC);
		lua_setmetatable(L, -2);*/
}
