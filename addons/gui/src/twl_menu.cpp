// TWL_MENU.CPP
// Steve Donovan, 2003
// This is GPL'd software, and the usual disclaimers apply.
// See LICENCE
/////////////////////

#include <windows.h>
#include <list>
#include <string>
#include <vector>
#include "twl_menu.hpp"
#include "lua.hpp"
#include "luabinder.hpp"
#include "utf.h"

UINT MessageHandler::Item::last_item_id = 1;

MessageHandler::Item::Item(UINT _data)
	: data(_data), id(++last_item_id)
{}

MessageHandler::MessageHandler() : m_list{}
{}

MessageHandler::~MessageHandler()
{
	m_list.clear();
}

UINT MessageHandler::add_item(UINT data)
{
	return m_list.emplace_back(data).id;
}

void MessageHandler::remove(UINT id)
{
	m_list.remove_if([id](const Item& item) { return item.id == id; });
}

bool MessageHandler::dispatch(UINT id)
{
	auto it = std::find_if(m_list.begin(), m_list.end(), [id](const Item& item) { return item.id == id; });
	if (it != m_list.end())
	{
		if (it->data) trigger(it->data);
		return true;// found, but no action
	}
	return false;
}

void MessageHandler::add_handler(MessageHandler* hndlr)
{
	if (hndlr) m_list.splice(m_list.end(), hndlr->m_list);
}

////////////////////////////////
int store_data(lua_State* L, const std::wstring_view item, MessageHandler* pmh);
int new_menu(lua_State* L, HMENU hm, MessageHandler* = nullptr);

namespace
{
	class CMenu
	{
		HMENU _hMenu;
		MessageHandler* _pmh;

	public:
		CMenu(HMENU hm, MessageHandler* pmh) : _hMenu(hm), _pmh(pmh) {};
		static const char* classname() { return "TWL_POPUPMenu*"; }
		MessageHandler* get_msg_handler() const { return _pmh; };
		int getMenuItemCount() const;
		void setCheckMenuItem(int id, bool state) const;
		bool getCheckMenuItem(int id) const;
		void setEnableMenuItem(int id, bool state) const;
		bool getEnableMenuItem(int id) const;
		HMENU getSubMenu(int id) const;
		int getMenuItemID(int id) const;
		void modifyMenuItem(int id, std::wstring_view caption) const;
		void setIconMenuItem(int id, int idx, std::wstring_view file) const;
		void insertMenuItem(int id, int cmdID, std::wstring_view caption) const;
		void insertSeparator(int id) const;
		void deleteMenuItem(int id, bool by_position) const;
		HMENU insertSubMenu(int id, std::wstring_view caption) const;
	};

	int CMenu::getMenuItemCount() const
	{
		return GetMenuItemCount(_hMenu);
	}

	void CMenu::setCheckMenuItem(int id, bool state) const
	{
		CheckMenuItem(_hMenu, id, MF_BYPOSITION | (state ? MF_CHECKED : MF_UNCHECKED));
	}

	bool CMenu::getCheckMenuItem(int id) const
	{
		auto Flag = GetMenuState(_hMenu, id, MF_BYPOSITION);
		return (Flag & MF_CHECKED);
	}

	void CMenu::setEnableMenuItem(int id, bool state) const
	{
		EnableMenuItem(_hMenu, id, MF_BYPOSITION | (state ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	}

	bool CMenu::getEnableMenuItem(int id) const
	{
		auto flags = GetMenuState(_hMenu, id, MF_BYPOSITION);
		return !(flags & (MF_DISABLED | MF_GRAYED));
	}

	HMENU CMenu::getSubMenu(int id) const
	{
		return GetSubMenu(_hMenu, id);
	}

	int CMenu::getMenuItemID(int id) const
	{
		return GetMenuItemID(_hMenu, id);
	}

	void CMenu::modifyMenuItem(int id, std::wstring_view caption) const
	{
		auto flags = GetMenuState(_hMenu, id, MF_BYPOSITION);
		::ModifyMenuW(_hMenu, id, MF_STRING | MF_BYPOSITION | flags, getMenuItemID(id), caption.data());
	}

	HBITMAP GetHBitmap(HICON hIcon)
	{
		HDC hDC = GetDC(NULL);
		HDC hMemDC = CreateCompatibleDC(hDC);
		constexpr int IconSize = 16;
		HBITMAP hMemBmp = CreateCompatibleBitmap(hDC, IconSize, IconSize);
		HBITMAP hResultBmp = NULL;
		HGDIOBJ hOrgBMP = SelectObject(hMemDC, hMemBmp);
		///
		RECT rc = { 0, 0, IconSize, IconSize };
		HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_MENUBAR));
		FillRect(hMemDC, &rc, hBrush);
		DeleteObject(hBrush);
		///
		DrawIconEx(hMemDC, 0, 0, hIcon, IconSize, IconSize, 0, NULL, DI_NORMAL);

		hResultBmp = hMemBmp;
		hMemBmp = NULL;

		SelectObject(hMemDC, hOrgBMP);
		DeleteDC(hMemDC);
		ReleaseDC(NULL, hDC);
		DestroyIcon(hIcon);
		return hResultBmp;
	}

	void CMenu::setIconMenuItem(int id, int idx, std::wstring_view file) const
	{
		HICON hIcon = NULL;
		if (ExtractIconEx(file.data(), idx, NULL, &hIcon, 1))
		{
			SetMenuItemBitmaps(_hMenu, id, MF_BYPOSITION, GetHBitmap(hIcon), NULL);
		}
	}

	void CMenu::deleteMenuItem(int id, bool by_position) const
	{
		if (by_position && id == -1) id = getMenuItemCount() - 1;
		DeleteMenu(_hMenu, id, by_position ? MF_BYPOSITION : MF_BYCOMMAND);
	}

	void CMenu::insertSeparator(int id) const
	{
		if (id == -1)
			AppendMenu(_hMenu, MF_SEPARATOR, 0, 0);
		else
			InsertMenu(_hMenu, id, MF_SEPARATOR | MF_BYPOSITION, 0, 0);
	}

	void CMenu::insertMenuItem(int id, int cmdID, std::wstring_view caption) const
	{
		if (id == -1)
			AppendMenu(_hMenu, MF_STRING, cmdID, caption.data());
		else
			InsertMenu(_hMenu, id, MF_STRING | MF_BYPOSITION, cmdID, caption.data());
	}

	HMENU CMenu::insertSubMenu(int id, std::wstring_view caption) const
	{
		HMENU hSubMenu = CreatePopupMenu();
		if (id == -1)
			AppendMenu(_hMenu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), caption.data());
		else
			InsertMenu(_hMenu, id, MF_STRING | MF_POPUP | MF_BYPOSITION, reinterpret_cast<UINT_PTR>(hSubMenu), caption.data());
		return hSubMenu;
	}

	//////////////////////////////////////
#define check_cmenu check_arg<CMenu>

	int do_item_count(lua_State* L)
	{
		CMenu* pMenu = check_cmenu(L);
		lua_pushinteger(L, pMenu->getMenuItemCount());
		return 1;
	}

	int do_check_item(lua_State* L)
	{
		CMenu* pMenu = check_cmenu(L);
		int id = static_cast<int>(luaL_checkinteger(L, 2));
		if (lua_gettop(L) == 2)
		{
			lua_pushboolean(L, pMenu->getCheckMenuItem(id));
			return 1;
		}
		bool state = lua_toboolean(L, 3);
		pMenu->setCheckMenuItem(id, state);
		return 0;
	}

	int do_enable_item(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		int id = static_cast<int>(luaL_checkinteger(L, 2));
		if (lua_gettop(L) == 2)
		{
			lua_pushboolean(L, pMenu->getEnableMenuItem(id));
			return 1;
		}
		bool state = lua_toboolean(L, 3);
		pMenu->setEnableMenuItem(id, state);
		return 0;
	}

	int do_command_id(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		int id = static_cast<int>(luaL_checkinteger(L, 2));
		int iID = pMenu->getMenuItemID(id);
		lua_pushinteger(L, iID);
		return 1;
	}

	int do_modify_item(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		int id = static_cast<int>(luaL_optinteger(L, 2, 0));
		auto caption = StringFromUTF8(luaL_optstring(L, 3, nullptr));
		pMenu->modifyMenuItem(id, caption);
		return 0;
	}

	int do_set_icon(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		int id = static_cast<int>(luaL_optinteger(L, 2, 0));
		int idx = static_cast<int>(luaL_optinteger(L, 3, 0));
		auto file = StringFromUTF8(luaL_optstring(L, 4, "toolbar\\cool.dll"));
		pMenu->setIconMenuItem(id, idx, file);
		return 0;
	}

	int do_insert_item(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		auto itemdata = StringFromUTF8(luaL_optstring(L, 2, ""));
		int id = static_cast<int>(luaL_optinteger(L, 3, -1));
		if (itemdata.empty())
		{
			pMenu->insertSeparator(id);
		}
		else
		{
			auto pos = itemdata.find(L"|");
			if (pos != std::wstring::npos)
			{
				itemdata[pos] = 0;
				int cmdID = store_data(L, itemdata.data() + pos + 1, pMenu->get_msg_handler());
				pMenu->insertMenuItem(id, cmdID, itemdata);
			}
		}
		return 0;
	}

	int do_remove_item(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		int id = static_cast<int>(luaL_optinteger(L, 2, -1));
		bool by_pos = !lua_toboolean(L, 3);
		pMenu->deleteMenuItem(id, by_pos);
		return 0;
	}

	int do_insert_submenu(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		auto caption = StringFromUTF8(luaL_optstring(L, 2, "submenu"));
		int id = static_cast<int>(luaL_optinteger(L, 3, -1));
		HMENU hSubMenu = pMenu->insertSubMenu(id, caption);
		return new_menu(L, hSubMenu, pMenu->get_msg_handler());
	}

	int do_get_submenu(lua_State* L)
	{
		const CMenu* pMenu = check_cmenu(L);
		int id = static_cast<int>(luaL_checkinteger(L, 2));
		if (HMENU hSubMenu = pMenu->getSubMenu(id))
		{
			return new_menu(L, hSubMenu, pMenu->get_msg_handler());
		}
		return 0;
	}
}

const luaL_Reg LuaBinder<CMenu>::metamethods[] =
{
	{ "__gc",		do_destroy<CMenu> },
	{ NULL, NULL }
};

const luaL_Reg LuaBinder<CMenu>::methods[] =
{
	{ "count",   		do_item_count	},
	{ "command_id",		do_command_id	},
	{ "check_item",		do_check_item	},
	{ "enable_item",	do_enable_item	},
	{ "modify_item",	do_modify_item	},
	{ "set_icon",		do_set_icon 	},

	{ "insert_item",	do_insert_item	},
	{ "remove_item",	do_remove_item  },

	{ "insert_submenu",	do_insert_submenu},
	{ "get_submenu",	do_get_submenu	},
	{ NULL, NULL }
};

void lua_openclass_CMenu(lua_State* L)
{
	LuaBinder<CMenu>().createClass(L);
}

int new_menu(lua_State* L, HMENU hm, MessageHandler* pmh)
{
	lua_push_newobject<CMenu>(L, hm, pmh);
	return 1;
}
