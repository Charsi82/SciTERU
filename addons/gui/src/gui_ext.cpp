// gui_ext.cpp
// This is a simple set of predefined GUI windows for SciTE,
// built using the YAWL library.
// Steve Donovan, 2007.
//////////////////////////////////////////////////////////

#include "twl.h"
#include <vector>
#include <io.h>		// _A_SUBDIR
#include <direct.h>	// _chdir
#include <format>
#include "lua.hpp"
#include "twl_menu.h"
#include "twl_notify.h"
#include "twl_cntrls.h"
#include "twl_listview.h"
#include "twl_tab.h"
#include "twl_splitter.h"
#include "twl_treeview.h"
#include "luabinder.h"

extern HINSTANCE hInst;

int new_inifile(lua_State* L);
void lua_openclass_iniFile(lua_State* L);
int add_tooltip(lua_State* L);
void lua_openclass_TTipCtrl(lua_State* L);

#define output(x) lua_pushstring(L, (x)); OutputMessage(L);
#define lua_pushwstring(L, str) lua_pushstring((L), UTF8FromString(str).c_str())

// vector of wide strings
typedef std::vector<std::wstring> vecws;

const char* LIB_VERSION = "0.2.8";
const char* DEFAULT_ICONLIB = "toolbar\\cool.dll";
const char* WINDOW_CLASS = "WINDOW*";
const char* MT_TDC = "TDC*";

HWND hSciTE = NULL, hContent = NULL, hCode = NULL;
WNDPROC old_scite_proc, old_scintilla_proc, old_content_proc;
TWin* extra_window = nullptr;
TWin* extra_window_splitter = nullptr;
bool forced_resize = false;
Rect cwb, extra;

class PaletteWindow;

// dumping Lua stack for debug
// #define RB_DS

#ifdef RB_DS
void OutputMessage(lua_State* L);
void dump_stack(lua_State* L)
{
	if (!L) return;
	output("\r\n==== dump start ====");
	int sz = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= sz; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_pcall(L, 1, 1, 0);
		const char* str = lua_tolstring(L, -1, 0);
		if (str)
		{
			output(str);
		}
		else
		{
			output(lua_typename(L, lua_type(L, i)));
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	output("\r\n===== dump end ====");
}

#define DS dump_stack(L);

void dump_stack_to_log(lua_State* L)
{
	if (!L) return;
	log_add("\r\n==== dump start ====");
	int sz = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= sz; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_pcall(L, 1, 1, 0);
		const char* str = lua_tolstring(L, -1, 0);
		if (str)
		{
			log_add(str);
		}
		else
		{
			log_add(lua_typename(L, lua_type(L, i)));
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	log_add("\r\n===== dump end ====");
}
#endif

//#define DSL dump_stack_to_log(L);
#define DSL

COLORREF lua_optColor(lua_State* L, int idx = 1, COLORREF def_clr = 0)
{
	if (const char* s_clr = luaL_optstring(L, idx, nullptr))
	{
		unsigned int r = 0, g = 0, b = 0;
		sscanf_s(s_clr, "#%02x%02x%02x", &r, &g, &b);
		return RGB(r, g, b);
	}
	return def_clr;
}

bool optboolean(lua_State* L, int idx, bool res = false)
{
	return lua_isnoneornil(L, idx) ? res : lua_toboolean(L, idx);
}

namespace
{
	TWin* get_parent()
	{
		static TWin s_parent(hSciTE);
		return &s_parent;
	}

	TWin* get_code_window()
	{
		static TWin code_window(hCode);
		return &code_window;
	}

	TWin* get_content_window()
	{
		static TWin content_window(hContent);
		return &content_window;
	}

	TWin* get_desktop_window()
	{
		static TWin desktop(GetDesktopWindow());
		return &desktop;
	}

	// print version
	int do_version(lua_State* L)
	{
		lua_getglobal(L, "print");
		lua_pushstring(L, LIB_VERSION);
		lua_pcall(L, 1, 0, 0);
		return 0;
	}

	// show a message on the SciTE output window
	void OutputMessage(lua_State* L)
	{
		if (lua_isstring(L, -1))
		{
			lua_getglobal(L, "print");
			lua_insert(L, -2);
			lua_pcall(L, 1, 0, 0);
		}
	}

	const void* gui_st_name = "_GUI_ST_NAME";

	void reinit_storage(lua_State* L)
	{
		lua_newtable(L);
		lua_rawsetp(L, LUA_REGISTRYINDEX, gui_st_name);
	}
	//#pragma optimize( "", off )
	// снимает с вершины стека значение и сохраняет в хранилище
	// @result индекс в хранилище
	int lua_reg_store(lua_State* L)
	{
		lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);/* ... data tbl */
		lua_insert(L, -2);/* ... tbl data */
		int ret = luaL_ref(L, -2);/* ... tbl */
		lua_pop(L, 1);/* ... */
		return ret;
	}

	// кладет на стек данные по указанному индексу
	// @param индекс данных в хранилище
	void lua_reg_restore(lua_State* L, int idx)
	{
		lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);
		lua_rawgeti(L, -1, idx);
		lua_remove(L, -2);
	}

	// удаляет данные из хранилища,
	// проверяет аргумент индекса на валидность
	// @param idx ненулевой индекс данных в хранилище
	void lua_reg_release(lua_State* L, int idx)
	{
		if (idx)
		{
			lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);
			luaL_unref(L, -1, idx);
			lua_pop(L, 1);
		}
	}
	//#pragma optimize( "", on )

	// https://ilovelua.wordpress.com

	int errorHandler(lua_State* L)
	{
		//stack: err
		lua_getglobal(L, "debug"); // stack: err debug
		lua_getfield(L, -1, "traceback"); // stack: err debug debug.traceback

		// debug.traceback() возвращает 1 значение
		if (lua_pcall(L, 0, 1, 0))
		{
			lua_pushstring(L, "Error in debug.traceback() call: ");
			lua_insert(L, -2);
			lua_pushstring(L, "\n");
			lua_concat(L, 3); //"Error in debug.traceback() call: "+err+"\n"
			//OutputMessage(L);
		}
		else
		{
			// stack: err debug stackTrace
			lua_insert(L, -2); // stack: err stackTrace debug
			lua_pop(L, 1); // stack: err stackTrace
			lua_pushstring(L, "Error:"); // stack: err stackTrace "Error:"
			lua_insert(L, -3); // stack: "Error:" err stackTrace
			lua_pushstring(L, "\n"); // stack: "Error:" err stackTrace "\n"
			lua_insert(L, -2); // stack: "Error:" err "\n" stackTrace
			lua_concat(L, 4); // stack: "Error:"+err+"\n"+stackTrace
		}
		return 1;
	}
	//#pragma optimize( "", off )
	// маленький помощник, чтобы самим не считать количество lua_push...() и lua_pop()
	class LuaStackGuard
	{
		lua_State* luaState_;
		int top_;
	public:
		explicit LuaStackGuard(lua_State* L) : luaState_(L)
		{
			//log_add("lsg init");
			top_ = lua_gettop(L);
		}

		~LuaStackGuard()
		{
			//log_add("lsg destroy");
			lua_settop(luaState_, top_);
		}
	};
	//#pragma optimize( "", on )
}

#define LSG auto lsg = std::make_unique<LuaStackGuard>(L)

static void force_contents_resize()
{
	// get the code pane's extents, and don't try to resize it again!
	Rect m{};
	get_code_window()->get_rect(m, true);
	if (cwb.right == m.right && cwb.left == m.left) return; // top and left is 0 every times
	int w = extra_window->width();
	int h = m.height();
	int sw = extra_window_splitter->width();
	extra = m;
	cwb = m;
	if (extra_window->align() == Alignment::alLeft)
	{
		// on the left goes the extra pane, followed by the splitter
		extra.right = extra.left + w;
		extra_window->resize(m.left, m.top, w, h);
		extra_window_splitter->resize(m.left + w, m.top, sw, h);
		cwb.left += w + sw;
	}
	else
	{
		int margin = m.right - w;
		extra.left = margin;
		extra_window->resize(margin, m.top, w, h);
		extra_window_splitter->resize(margin - sw, m.top, sw, h);
		cwb.right -= w + sw;
	}
	// and then the code pane; note the hack necessary to prevent a nasty recursion here.
	forced_resize = true;
	get_code_window()->resize(cwb);
	forced_resize = false;
}

static LRESULT SciTEWndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_ACTIVATEAPP:
		//floating toolbars may grab the focus, so restore it.
		if (wParam) get_code_window()->set_focus();
		break;
	}
	return CallWindowProc(old_scite_proc, hwnd, iMessage, wParam, lParam);
}

static LRESULT ScintillaWndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if (iMessage == WM_SIZE)
	{
		if (extra_window)
		{
			if (!forced_resize)
			{
				force_contents_resize();
			}
		}
		if (extra_window_splitter)// fix hiding splitter when change horz size
			extra_window_splitter->invalidate();
	}
	return CallWindowProc(old_scintilla_proc, hwnd, iMessage, wParam, lParam);
}

static LRESULT ContentWndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if (iMessage == WM_SETCURSOR)
	{
		Point ptCursor;
		GetCursorPos(&ptCursor);
		ScreenToClient(hContent, &ptCursor);
		if (extra.is_inside(ptCursor))
		{
			return DefWindowProc(hSciTE, iMessage, wParam, lParam);
		}
	}
	return CallWindowProc(old_content_proc, hwnd, iMessage, wParam, lParam);
}

//////////// dialogs functions ////////////

// gui.colour_dlg(default_colour)
// @param default_colour  colour either in form '#RRGGBB" or as a 32-bit integer
// @return chosen colour, in same form as default_colour
int do_colour_dlg(lua_State* L)
{
	bool in_rgb = lua_isstring(L, 1);
	COLORREF cval = in_rgb ? lua_optColor(L, 1) : luaL_optinteger(L, 1, 0);
	auto p = get_parent();
	if (p && run_colordlg((HWND)p->handle(), cval))
	{
		if (in_rgb)
		{
			char buff[12];
			sprintf_s(buff, "#%02X%02X%02X", GetRValue(cval), GetGValue(cval), GetBValue(cval));
			lua_pushstring(L, buff);
		}
		else
			lua_pushinteger(L, cval);
		return 1;
	}
	return 0;
}

// gui.message(message_string, window_type)
// @param message_string
// @param window_type (0 for plain message, 1 for warning box)
// MSG_ERROR=2,MSG_WARNING=1, MSG_QUERY=3;
// @return bOK
int do_message(lua_State* L)
{
	auto msg = StringFromUTF8(luaL_checkstring(L, 1));
	auto kind = StringFromUTF8(luaL_optstring(L, 2, "message"));
	int type = 0;
	//if (kind == L"message") type = 0; else
	if (kind == L"warning") type = 1;
	else if (kind == L"error") type = 2;
	else if (kind == L"query") type = 3;
	lua_pushboolean(L, TWin::message(msg.c_str(), type));
	return 1;
}

// gui.open_dlg([sCaption = "Open File"][, sFilter = "All (*.*)|*.*"])
// @param sCaption [= "Open File"]
// @param sFilter  [= "All (*.*)|*.*"]
// @return sFileName or nil
int do_open_dlg(lua_State* L)
{
	const std::wstring caption = StringFromUTF8(luaL_optstring(L, 1, "Open File"));
	const std::wstring filter = StringFromUTF8(luaL_optstring(L, 2, "All (*.*)|*.*"));
	bool multi = optboolean(L, 3);
	constexpr int PATHSIZE = 1024;
	wchar_t tmp[PATHSIZE]{};
	if (!run_ofd((HWND)get_parent()->handle(), tmp, caption, filter, multi)) return 0;
	std::wstring path = tmp;
	// tmp : path\0file1\0file2\0..\0filen
	if (path.find(L'.') == std::wstring::npos)
	{
		int count = 0;
		wchar_t* filename = tmp + lstrlen(tmp) + 1;
		while (*filename != L'\0' && (filename - tmp < PATHSIZE))
		{
			std::wstring file = tmp;
			file += L"\\";
			file += filename;
			lua_pushwstring(L, file);
			filename += lstrlen(filename) + 1;
			count++;
		}
		return count;
	}
	// tmp : pathfile
	else
	{
		lua_pushwstring(L, tmp);
	}
	return 1;
}

// gui.save_dlg([sCaption = "Save File"][, sFilter = "All (*.*)|*.*"])
// @param sCaption [= "Save File"]
// @param sFilter  [= "All (*.*)|*.*"]
// @return sFileName or nil
int do_save_dlg(lua_State* L)
{
	auto caption = StringFromUTF8(luaL_optstring(L, 1, "Save File"));
	auto filter = StringFromUTF8(luaL_optstring(L, 2, "All (*.*)|*.*"));
	wchar_t tmp[1024]{};
	if (get_parent() && !run_ofd((HWND)get_parent()->handle(), tmp, caption, filter)) return 0;
	lua_pushwstring(L, tmp);
	return 1;
}

// gui.select_dir_dlg([sDescription = ""][, sInitialdir = ""])
// @param sDescription [= ""]
// @param sInitialdir  [= ""]
// @return sDirName or nil
int do_select_dir_dlg(lua_State* L)
{
	auto descr = StringFromUTF8(luaL_optstring(L, 1, "Browse for folder..."));
	auto initdir = StringFromUTF8(luaL_optstring(L, 2, "C:\\"));
	wchar_t tmp[MAX_PATH]{};
	if (get_parent() && !run_seldirdlg((HWND)get_parent()->handle(), tmp, descr.c_str(), initdir.c_str())) return 0;
	lua_pushwstring(L, tmp);
	return 1;
}

// gui.prompt_value( sPrompt_string [, default_value = ""])
// @param sPrompt_string
// @param sDefaultValue [=""]
int do_prompt_value(lua_State* L)
{
	//auto varname = StringFromUTF8(luaL_checkstring(L, 1));
	auto def_value = StringFromUTF8(luaL_optstring(L, 2, ""));
	//PromptDlg dlg(get_parent(), varname.c_str(), def_value.c_str());
	//dlg.set_icon_from_window(get_parent());
	//lua_pushwstring(L, dlg.show_modal() ? dlg.m_val : def_value);
	output("function 'gui.prompt_value()' removed!!! please use shell.inputbox().");
	lua_pushwstring(L, def_value);
	return 1;
}
/// end dialos functions

/// others functions

static int do_files(lua_State* L)
{
	_wfinddata_t c_file;
	std::wstring mask = StringFromUTF8(luaL_checkstring(L, 1));
	bool look_for_dir = optboolean(L, 2);
	intptr_t hFile = _wfindfirst(mask.c_str(), &c_file);
	lua_newtable(L);
	if (hFile == /*INVALID_HANDLE_VALUE*/ -1) { return 1; }
	int idx = 1;
	do
		if (((c_file.attrib & _A_SUBDIR) != 0) == look_for_dir)
		{
			if (look_for_dir && (!wcscmp(c_file.name, L".") || !wcscmp(c_file.name, L".."))) continue;
			lua_pushinteger(L, idx++);
			lua_pushwstring(L, c_file.name);
			lua_settable(L, -3);
		}
	while (!_wfindnext(hFile, &c_file));
	return 1;
}

static int do_chdir(lua_State* L)
{
	const char* dirname = luaL_checkstring(L, 1);
	int res = _chdir(dirname);
	lua_pushboolean(L, res == 0);
	return 1;
}

std::wstring GetLastErrorAsString(DWORD errorCode)
{
	std::wstring errorMsg;
	// Get the error message, if any.
	// If both error codes (passed error n GetLastError) are 0, then return empty
	if (errorCode == 0)
		errorCode = GetLastError();
	if (errorCode == 0)
		return errorMsg; //No error message has been recorded

	LPWSTR messageBuffer = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, nullptr);

	errorMsg += messageBuffer;

	//Free the buffer.
	LocalFree(messageBuffer);

	return errorMsg;
}

int do_run(lua_State* L)
{
	std::wstring wsFile = StringFromUTF8(luaL_checkstring(L, 1));
	std::wstring wsParameters = StringFromUTF8(lua_tostring(L, 2));
	std::wstring wsDirectory = StringFromUTF8(lua_tostring(L, 3));
	DWORD err = shell_execute(wsFile.c_str(), wsParameters.c_str(), wsDirectory.c_str());
	lua_pushinteger(L, err);
	if (err)
	{
		lua_pushwstring(L, GetLastErrorAsString(err));
		return 2;
	}
	else
	{
		return 1;
	}
}

// gui.get_ascii(uchar)
// @param uChar
// @return symbol, html number, html code
int do_get_ascii(lua_State* L)
{
	unsigned char x = static_cast<unsigned char>(luaL_checkinteger(L, 1));
	std::string s = getAscii(x);
	lua_pushstring(L, s.c_str());

	char htmlNumber[8]{};
	if ((x >= 32 && x <= 126) || (x >= 160 /*&& x <= 255*/))
	{
		sprintf_s(htmlNumber, "&#%d", x);
	}
	else
	{
		int n = getHtmlNumber(x);
		if (n > -1)
		{
			sprintf_s(htmlNumber, "&#%d", n);
		}
		else
		{
			sprintf_s(htmlNumber, "");
		}
	}
	lua_pushstring(L, htmlNumber);

	std::string htmlName = getHtmlName(x);
	lua_pushstring(L, htmlName.c_str());
	return 3;
}
/// end others functions

/// windows functions

inline void lua_pushargs(lua_State* L, int value)
{
	lua_pushinteger(L, value);
}

inline void lua_pushargs(lua_State* L, const char* value)
{
	lua_pushstring(L, value);
}

inline void lua_pushargs(lua_State* L, bool value)
{
	lua_pushboolean(L, value);
}

inline void lua_pushargs(lua_State* L, void* value)
{
	lua_pushlightuserdata(L, value);
}

inline void lua_pushargs(lua_State* L, TDC* value)
{
	lua_pushlightuserdata(L, value);
	luaL_getmetatable(L, MT_TDC);
	lua_setmetatable(L, -2);
}

template<typename T, typename... Args>
void lua_pushargs(lua_State* L, T value, Args... args)
{
	lua_pushargs(L, value);
	lua_pushargs(L, args...);
}

int dispatch_ref(lua_State* L, int data)
{
	if (data)
	{
		LSG;
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		lua_pushcfunction(L, errorHandler);// stack: func arg1 arg2 ... argn errorHandler
		const int errorHandlerIndex = -2;
		lua_insert(L, errorHandlerIndex); //stack: errorHandler func arg1 arg2 ... argn
		if (lua_pcall(L, 0, 1, errorHandlerIndex))
			OutputMessage(L);
		else
			return lua_toboolean(L, -1);
	}
	return 0;
}

template<typename... Args>
int dispatch_ref(lua_State* L, int data, Args... args)
{
	if (data)
	{
		LSG;
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		lua_pushargs(L, args...);// stack: func arg1 arg2 ... argn
		lua_pushcfunction(L, errorHandler);// stack: func arg1 arg2 ... argn errorHandler
		const int args_count = sizeof...(args);
		const int errorHandlerIndex = -(args_count + 2);
		lua_insert(L, errorHandlerIndex); //stack: errorHandler func arg1 arg2 ... argn
		if (lua_pcall(L, args_count, 1, errorHandlerIndex))
			OutputMessage(L);
		else
			return lua_toboolean(L, -1);
	}
	return 0;
}

static inline void throw_error(lua_State* L, const char* msg)
{
	luaL_traceback(L, L, msg, 1);
	lua_error(L);
}

void function_ref(lua_State* L, int idx, int* pr)
{
	//if (*pr) luaL_unref(L, LUA_REGISTRYINDEX, *pr);
	lua_reg_release(L, *pr);
	if (!lua_isfunction(L, idx)) throw_error(L, "function required");
	lua_pushvalue(L, idx);
	//*pr = luaL_ref(L, LUA_REGISTRYINDEX);
	*pr = lua_reg_store(L);
}

class TLuaState
{
protected:
	static lua_State* L;
public:
	static void set_LuaState(lua_State* l) { L = l; }
};

lua_State* TLuaState::L = nullptr;

class LuaWindow : public TEventWindow, protected TLuaState
{
protected:
	int on_close_idx;
	int on_show_idx;
	int on_command_idx;
	int on_scroll_idx;
	int on_timer_idx;
	int on_size_idx;
	int on_key_idx;
	int on_move_idx;
	int on_activate_idx;
	int on_paint;

public:
	LuaWindow(const wchar_t* caption, TWin* parent, DWORD stylex = 0, bool is_child = false, DWORD style = -1)
		:TEventWindow(caption, parent, stylex, is_child, style), on_close_idx(0), on_show_idx(0),
		on_command_idx(0), on_scroll_idx(0), on_timer_idx(0), on_size_idx(0), on_key_idx(0), on_move_idx(0),
		on_activate_idx(0), on_paint(0)
	{
	}

	virtual ~LuaWindow()
	{
		lua_reg_release(L, on_close_idx);
		lua_reg_release(L, on_show_idx);
		lua_reg_release(L, on_command_idx);
		lua_reg_release(L, on_scroll_idx);
		lua_reg_release(L, on_timer_idx);
		lua_reg_release(L, on_size_idx);
		lua_reg_release(L, on_key_idx);
		lua_reg_release(L, on_move_idx);
		lua_reg_release(L, on_activate_idx);
		lua_reg_release(L, on_paint);
	}

	void size() override;
	void move() override;
	bool command(int arg1, int arg2) override;
	void scroll(int code, int posn) override;
	void timer() override;
	void destroy() override;
	void on_close() override;
	void on_showhide(bool show) override;
	void keydown(int key) override;
	bool activate(bool arg) override;
	void paint(TDC*) override;
	static void handler(int data);

	void set_on_key(int iarg)
	{
		function_ref(L, iarg, &on_key_idx);
	}

	void set_on_size(int iarg)
	{
		function_ref(L, iarg, &on_size_idx);
	}

	void set_on_close(int iarg)
	{
		function_ref(L, iarg, &on_close_idx);
	}

	void set_on_command(int iarg)
	{
		function_ref(L, iarg, &on_command_idx);
	}

	void set_on_scroll(int iarg)
	{
		function_ref(L, iarg, &on_scroll_idx);
	}

	void set_on_move(int iarg)
	{
		function_ref(L, iarg, &on_move_idx);
	}

	void set_on_timer(int iarg, int delay)
	{
		function_ref(L, iarg, &on_timer_idx);
		create_timer(delay);
	}

	void set_on_show(int iarg)
	{
		function_ref(L, iarg, &on_show_idx);
	}

	void set_on_activate(int iarg)
	{
		function_ref(L, iarg, &on_activate_idx);
	}

	void set_on_paint(int iarg)
	{
		function_ref(L, iarg, &on_paint);
	}

private:
	static void output_unknown(const char* str);

	static void call_SciteCommand();
};

void MessageHandler::trigger(UINT data) const
{
	if (data) LuaWindow::handler(data);
}

class PaletteWindow : public LuaWindow
{
public:
	PaletteWindow(const wchar_t* caption, TWin* parent = nullptr, DWORD stylex = 0, DWORD style = -1) : LuaWindow(caption, parent, stylex, false, style) {}
	void show(int how = SW_SHOW) override;
	bool query_close() override;
};

void PaletteWindow::show(int how)
{
	TEventWindow::show(how);
}

bool PaletteWindow::query_close()
{
	hide();
	return false;
}

struct WinWrap
{
	TWin* window;
	TWin* data;
};

static int wrap_window(lua_State* L, TWin* win)
{
	WinWrap* wrp = static_cast<WinWrap*>(lua_newuserdata(L, sizeof(WinWrap)));
	wrp->window = win;
	wrp->data = nullptr;
	luaL_getmetatable(L, WINDOW_CLASS);
	lua_setmetatable(L, -2);
	return 1;
}

inline TWin* window_arg(lua_State* L, int idx = 1)
{
	if (WinWrap* wrp = static_cast<WinWrap*>(luaL_checkudata(L, idx, WINDOW_CLASS)))
		return wrp->window;
	throw_error(L, "not a window");
	return nullptr;
}

//template<typename ... Args>
//std::string string_format(const std::string_view format, Args ... args)
//{
//	int size_s = std::snprintf(nullptr, 0, format.data(), args ...) + 1; // Extra space for '\0'
//	if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
//	auto size = static_cast<size_t>(size_s);
//	std::unique_ptr<char[]> buf(new char[size]);
//	std::snprintf(buf.get(), size, format.data(), args ...);
//	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
//}

template<class T>
T* lua_cast(lua_State* L, int idx = 1)
{
	TWin* pWin = window_arg(L, idx);
	T* ptr = dynamic_cast<T*>(pWin);
	if (!ptr)
	{
		const std::string errstring = std::format("object of '{}' expected, got '{}'", typeid(T).name(), typeid(pWin).name());
		throw_error(L, errstring.c_str());
	}
	return ptr;
}

void LuaWindow::handler(int data)
{
	LSG;
	//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
	lua_reg_restore(L, data);
	if (lua_isinteger(L, -1))
	{
		call_SciteCommand();
	}
	else if (lua_isstring(L, -1))
	{
		// IDM_*
		// func name
		// cmd id

		std::string str = lua_tostring(L, -1);
		if (str._Starts_with("IDM_"))
		{
			lua_remove(L, -1);
			lua_getglobal(L, str.c_str());
			if (lua_isinteger(L, -1))
			{
				call_SciteCommand();
			}
			else
				output_unknown(str.c_str());
		}
		else
		{
			if (lua_isnumber(L, -1)) // can convert to cmd id?
			{
				int id = lua_tonumber(L, -1);
				lua_pushinteger(L, id);
				lua_remove(L, -2);
				call_SciteCommand();
			}
			else
			{
				lua_getglobal(L, str.c_str());
				lua_remove(L, -2);
				if (lua_isfunction(L, -1)) // func name
				{
					if (lua_pcall(L, 0, 0, 0)) OutputMessage(L);
				}
				else
					output_unknown(str.c_str());
			}
		}
	}
	else if (lua_isfunction(L, -1))
	{
		if (lua_pcall(L, 0, 0, 0)) OutputMessage(L);
	}
	else
		output_unknown(lua_tostring(L, -1));
}

void LuaWindow::output_unknown(const char* str)
{
	lua_pushfstring(L, "unknown command '%s'", str);
	OutputMessage(L);
}

void LuaWindow::call_SciteCommand()
{
	// val
	lua_getglobal(L, "scite");			// val, scite
	lua_getfield(L, -1, "MenuCommand"); // val, scite, scite.MenuCommand
	lua_insert(L, 1);					// scite.MenuCommand, val, scite
	lua_pop(L, 1);						// scite.MenuCommand, val
	if (lua_pcall(L, 1, 0, 0)) OutputMessage(L); // call scite.MenuCommand( cmdID )
}

inline bool IsKeyDown(int key) noexcept
{
	return (::GetKeyState(key) & 0x80000000) != 0;
}

void LuaWindow::keydown(int key)
{
	dispatch_ref(L, on_key_idx, key, IsKeyDown(VK_CONTROL), IsKeyDown(VK_MENU), IsKeyDown(VK_SHIFT));
}

void LuaWindow::size()
{
	TEventWindow::size();
	Rect rt;
	get_client_rect(rt);
	dispatch_ref(L, on_size_idx, rt.width(), rt.height());
}

void LuaWindow::move()
{
	dispatch_ref(L, on_move_idx);
}

bool LuaWindow::command(int arg1, int arg2)
{
	return dispatch_ref(L, on_command_idx, arg1, arg2);
}

void LuaWindow::scroll(int code, int posn)
{
	dispatch_ref(L, on_scroll_idx, code, posn);
}

void LuaWindow::timer()
{
	dispatch_ref(L, on_timer_idx);
}

void LuaWindow::destroy()
{
	kill_timer();
}

void LuaWindow::on_close()
{
	dispatch_ref(L, on_close_idx);
}

void LuaWindow::on_showhide(bool show)
{
	dispatch_ref(L, on_show_idx, show);
}

bool LuaWindow::activate(bool arg)
{
	dispatch_ref(L, on_activate_idx, arg);
	return true;
}

void LuaWindow::paint(TDC* pTDC)
{
	dispatch_ref(L, on_paint, pTDC);
}

class ContainerWindow : public PaletteWindow
{
public:
	ContainerWindow(const wchar_t* caption, DWORD style = WS_CAPTION | WS_SYSMENU)
		: PaletteWindow(caption, get_parent(), WS_EX_TOPMOST, style)
	{
		set_icon_from_window(get_parent());
	}
	bool query_close() override;
};

bool ContainerWindow::query_close()
{
	hide();
	return false;
}

inline TEventWindow* ew_arg(lua_State* L, int idx = 1)
{
	return dynamic_cast<TEventWindow*>(window_arg(L, idx));
}

// show window w
// w:show()
int window_show(lua_State* L)
{
	if (auto w = window_arg(L)) w->show();
	return 0;
}

// hide window w
// w:hide()
int window_hide(lua_State* L)
{
	if (auto w = window_arg(L)) w->hide();
	return 0;
}

// close window w
// w:close()
int window_close(lua_State* L)
{
	if (TEventWindow* w = ew_arg(L)) w->close();
	return 0;
}

// enable window w
// w:enable(true)
int window_enable(lua_State* L)
{
	if (TWin* win = window_arg(L))
		win->set_enable(optboolean(L, 2));
	return 0;
}

// enable window w
// w:set_focus()
int window_set_focus(lua_State* L)
{
	if (TWin* win = window_arg(L))
		win->set_focus();
	return 0;
}

// set or get window size
// w:size(width, height)
// @param window
// @param width [=250]
// @param height [=300]
int window_size(lua_State* L)
{
	TWin* win = window_arg(L);
	if (lua_gettop(L) > 1)
	{
		int w = luaL_optnumber(L, 2, 250);
		int h = luaL_optnumber(L, 3, 200);
		window_arg(L)->resize(w, h);
		return 0;
	}
	else
	{
		Rect rt;
		win->get_rect(rt, true);
		lua_pushinteger(L, rt.width());
		lua_pushinteger(L, rt.height());
		return 2;
	}
}

// centered window2 inside window1
// win1:center_h(win2)
int window_center_h(lua_State* L)
{
	TWin* win = window_arg(L);
	TWin* win2 = window_arg(L, 2);
	Rect rt{};
	if (win->parent_handle() == win2->parent_handle())
		win->get_rect(rt, true);
	else
		win->get_client_rect(rt);
	Rect rt2{};
	win2->get_rect(rt2, true);
	int posx = (rt.width() - rt2.width()) / 2;
	if (win->parent_handle() == win2->parent_handle())
		posx += rt.left;
	//rt.top += (rt.height() - rt2.height()) / 2;
	win2->move(posx, rt2.top);
	return 0;
}

int window_center_v(lua_State* L)
{
	TWin* win = window_arg(L);
	TWin* win2 = window_arg(L, 2);
	Rect rt{};
	if (win->parent_handle() == win2->parent_handle())
		win->get_rect(rt, true);
	else
		win->get_client_rect(rt);
	Rect rt2{};
	win2->get_rect(rt2, true);
	//rt.left += (rt.width() - rt2.width()) / 2;
	int posy = (rt.height() - rt2.height()) / 2;
	if (win->parent_handle() == win2->parent_handle())
		posy += rt.top; // += (rt.height() - rt2.height()) / 2;
	win2->move(rt2.left, posy);
	return 0;
}

int window_center(lua_State* L)
{
	window_center_v(L);
	window_center_h(L);
	return 0;
}

int window_resize(lua_State* L)
{
	if (TEventWindow* win = ew_arg(L))
	{
		bool resize = lua_toboolean(L, 2);
		int w = luaL_checkinteger(L, 3);
		int h = luaL_checkinteger(L, 4);
		win->enable_resize(resize, w, h);
		win->size();
	}
	return 0;
}

// parent_wnd:emplace_v(child1, child2,...)
int window_emplace_v(lua_State* L)
{
	if (TEventWindow* win_parent = ew_arg(L))
	{
		std::list<TWin*> wnd_list;
		for (int i = 2; i <= lua_gettop(L); ++i) wnd_list.push_back(window_arg(L, i));
		Rect rt;
		win_parent->get_rect(rt);
		int pw = rt.height();
		int cnt = 0;
		for (const auto win : wnd_list)
			if (win->align() == Alignment::alNone)
			{
				pw -= win->height();
				++cnt;
			}
		int margin_v = pw / (cnt + 1);
		if (margin_v < 1) margin_v = 1;
		int pos_y = margin_v;
		for (const auto win : wnd_list)
			if (win->align() == Alignment::alNone)
			{
				Rect rt2;
				win->get_rect(rt2, true);
				win->move(rt2.left, pos_y);
				pos_y += margin_v + rt2.height();
			}
	}
	return 0;
}

// parent_wnd:emplace_h(child1, child2,...)
int window_emplace_h(lua_State* L)
{
	if (TEventWindow* win_parent = ew_arg(L))
	{
		std::list<TWin*> wnd_list;
		for (int i = 2; i <= lua_gettop(L); ++i) wnd_list.push_back(window_arg(L, i));
		Rect rt;
		win_parent->get_rect(rt);
		int pw = rt.width();
		int cnt = 0;
		for (const auto win : wnd_list)
			if (win->align() == Alignment::alNone)
			{
				pw -= win->width();
				++cnt;
			}
		int margin_h = pw / (cnt + 1);
		if (margin_h < 1) margin_h = 1;
		int pos_x = margin_h;
		for (const auto win : wnd_list)
			if (win->align() == Alignment::alNone)
			{
				Rect rt2;
				win->get_rect(rt2, true);
				win->move(pos_x, rt2.top);
				pos_x += margin_h + rt2.width();
			}
	}
	return 0;
}

// set or get window position
// w:position()
// @param self
// @param pos_x
// @param pos_y
int window_position(lua_State* L)
{
	if (lua_gettop(L) > 1)
	{
		int x = (int)luaL_optnumber(L, 2, 10);
		int y = (int)luaL_optnumber(L, 3, 10);
		window_arg(L)->move(x, y);
		return 0;
	}
	else
	{
		TWin* win = window_arg(L);
		Rect rt;
		win->get_rect(rt, true);

		lua_pushinteger(L, rt.left);
		lua_pushinteger(L, rt.top);
		return 2;
	}
}

// wnd:bounds()
// @return bVisible, left, top, width, height
int window_get_bounds(lua_State* L)
{
	TWin* win = window_arg(L);
	Rect rt;
	win->get_rect(rt);
	lua_pushboolean(L, win->visible());
	lua_pushinteger(L, rt.left);
	lua_pushinteger(L, rt.top);
	lua_pushinteger(L, rt.width());
	lua_pushinteger(L, rt.height());
	return 5;
}

static std::vector<std::unique_ptr<ContainerWindow>> collect_windows;

// create new window
// w = gui.window( sCaption )
// @param strCaption
// @return window
int new_window(lua_State* L)
{
	std::wstring caption = StringFromUTF8(luaL_optstring(L, 1, nullptr));
	DWORD style = luaL_optinteger(L, 2, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME);
	collect_windows.push_back(std::make_unique<ContainerWindow>(caption.c_str(), style));
	ContainerWindow* cw = collect_windows.back().get();
	return wrap_window(L, cw);
}

class PanelWindow : public LuaWindow
{
public:
	PanelWindow() : LuaWindow(nullptr, get_parent(), 0, true) {}
};

// create new panel
// gui.panel( nWidth )
// @param nWidth[=100]
// @return window
int new_panel(lua_State* L)
{
	PanelWindow* pw = new PanelWindow();
	pw->align(Alignment::alLeft, static_cast<int>(luaL_optinteger(L, 1, 100)));
	return wrap_window(L, pw);
}

class LuaControl : protected TLuaState
{
protected:
	int on_select_idx{};
	int on_double_idx{};
	int on_key_idx{};
	int on_close_idx{};
	int on_focus_idx{};
	int on_tip_idx{};
	int on_updn_idx{};
	int on_paint_idx{};

public:
	LuaControl() = default;

	void set_paint(int iarg)
	{
		function_ref(L, iarg, &on_paint_idx);
	}

	void set_select(int iarg)
	{
		function_ref(L, iarg, &on_select_idx);
	}

	void set_double_click(int iarg)
	{
		function_ref(L, iarg, &on_double_idx);
	}

	void set_onkey(int iarg)
	{
		function_ref(L, iarg, &on_key_idx);
	}

	void set_on_close(int iarg)
	{
		function_ref(L, iarg, &on_close_idx);
	}

	void set_on_focus(int iarg)
	{
		function_ref(L, iarg, &on_focus_idx);
	}

	void set_on_tip(int iarg)
	{
		function_ref(L, iarg, &on_tip_idx);
	}

	void set_on_updown(int iarg)
	{
		function_ref(L, iarg, &on_updn_idx);
	}
};

class TTabControlLua : public TTabControl, public LuaControl
{
public:
	TTabControlLua(TEventWindow* parent) : TTabControl(parent) {}

private:
	void handle_select(int id) override;
};

void TTabControlLua::handle_select(int id)
{
	TWin* page = reinterpret_cast<TWin*>(get_data(id));
	get_parent_win()->set_client(page);
	get_parent_win()->size();
	dispatch_ref(L, on_select_idx, id);
}

// tabbar:add_tab(sCaption, wndPanel)
// @param sCaption
// @param panel
int tabbar_add(lua_State* L)
{
	TTabControl* tab = lua_cast<TTabControl>(L);
	std::wstring caption = StringFromUTF8(luaL_checkstring(L, 2));
	if (TWin* wnd = window_arg(L, 3))
	{
		int icon_idx = luaL_optinteger(L, 4, -1);
		tab->add(caption.data(), wnd, icon_idx);
	}
	return 0;
}

// tabbar:select_tab(idx) or idx = tabbar:select_tab()
// @param sCaption
// @param panel
int tabbar_sel(lua_State* L)
{
	if (TTabControl* tab = lua_cast<TTabControl>(L))
	{
		if (lua_gettop(L) == 1)
		{
			lua_pushinteger(L, tab->selected());
			return 1;
		}
		tab->selected(luaL_checkinteger(L, 2));
	}
	return 0;
}

int tabbar_fixedwidth(lua_State* L)
{
	if (TTabControl* tab = lua_cast<TTabControl>(L))
	{
		if (lua_gettop(L) == 1)
		{
			lua_pushboolean(L, tab->fixedwidth());
			return 1;
		}
		tab->fixedwidth(lua_toboolean(L, 2));
	}
	return 0;
}

int window_remove(lua_State* L)
{
	TEventWindow* form = ew_arg(L);
	if (WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, 2)))
	{
		// remove window
		form->remove(wrp->window);
		// remove splitter
		if (TWin* split = wrp->data)
			form->remove(split);
	}
	return 0;
}

// wnd_parent:add( child_window[, sAligment = "client"][, nSize = 100][, bSplitted = true])
// @param wnd
// @param sAligment [= "client"]
// @param nSize [= 100]
// @param bSplitted [= true]
int window_add(lua_State* L)
{
	TEventWindow* cw = ew_arg(L);
	TWin* child = window_arg(L, 2);
	const std::string align = luaL_optstring(L, 3, "client");
	const int sz = static_cast<int>(luaL_optnumber(L, 4, 100));
	const bool splitter = optboolean(L, 5);
	child->set_parent(cw);
	if (align == "top")
	{
		child->align(Alignment::alTop, sz);
	}
	else if (align == "bottom")
	{
		child->align(Alignment::alBottom, sz);
	}
	else if (align == "left")
	{
		child->align(Alignment::alLeft, sz);
	}
	else if (align == "none")
	{
		child->align(Alignment::alNone);
	}
	else if (align == "right")
	{
		child->align(Alignment::alRight, sz);
	}
	else
	{
		child->align(Alignment::alClient, sz);
	}
	cw->add(child);
	if (splitter && child->align() != Alignment::alClient && child->align() != Alignment::alNone)
	{
		TSplitter* split = new TSplitter(cw, child);
		cw->add(split);
		if (WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, 2)))
			wrp->data = split;
	}
	return 0;
}

// button, checkbox
int do_check(lua_State* L)
{
	TButtonBase* btn = reinterpret_cast<TButtonBase*>(window_arg(L));
	if (!btn) return 0;
	int args = lua_gettop(L);
	if (args > 1)
	{
		btn->check(luaL_checkinteger(L, 2));
		return 0;
	}
	lua_pushinteger(L, btn->check());
	return 1;
}

int do_ctrl_set_icon(lua_State* L)
{
	if (THasIconCtrl* lbl = dynamic_cast<THasIconCtrl*>(window_arg(L)))
	{
		const char* path = luaL_optstring(L, 2, DEFAULT_ICONLIB);
		const int icon_idx = luaL_optinteger(L, 3, 0);
		lbl->set_icon(StringFromUTF8(path).c_str(), icon_idx);
	}
	return 0;
}

int do_ctrl_set_bitmap(lua_State* L)
{
	if (THasIconCtrl* lbl = dynamic_cast<THasIconCtrl*>(window_arg(L)))
	{
		const char* path = luaL_checkstring(L, 2);
		lbl->set_bitmap(StringFromUTF8(path).c_str());
	}
	return 0;
}

int trbar_get_pos(lua_State* L)
{
	if (TTrackBar* trb = lua_cast<TTrackBar>(L))
	{
		lua_pushinteger(L, trb->pos());
		return 1;
	}
	return 0;
}

int trbar_set_pos(lua_State* L)
{
	if (TTrackBar* trb = lua_cast<TTrackBar>(L))
		trb->pos(luaL_optinteger(L, 2, 0));
	return 0;
}

int trbar_sel_clear(lua_State* L)
{
	if (TTrackBar* trb = lua_cast<TTrackBar>(L))
		trb->sel_clear();
	return 0;
}

int trbar_set_range(lua_State* L)
{
	if (TTrackBar* trb = lua_cast<TTrackBar>(L))
	{
		int imin = luaL_optinteger(L, 2, 0);
		int imax = luaL_optinteger(L, 3, 100);
		trb->range(imin, imax);
		int pos = trb->pos();
		if (pos < imin) trb->pos(imin);
		if (pos > imax) trb->pos(imax);
	}
	return 0;
}

//////////////////// Memo ////////////////////

class TMemoLua : public TMemo, public LuaControl
{
public:
	TMemoLua(TEventWindow* parent, int id = -1, bool do_scroll = false, bool plain = false) : TMemo(parent, id, do_scroll, plain) {}

private:
	void handle_onkey(int id) override;
};

void TMemoLua::handle_onkey(int id)
{
	dispatch_ref(L, on_key_idx, id, IsKeyDown(VK_CONTROL), IsKeyDown(VK_MENU), IsKeyDown(VK_SHIFT));
}

int window_set_text(lua_State* L)
{
	if (TWin* win = window_arg(L))
		win->set_text(StringFromUTF8(luaL_checkstring(L, 2)).c_str());
	return 0;
}

int window_get_text(lua_State* L)
{
	if (TWin* win = window_arg(L))
	{
		std::wstring str;
		win->get_text(str);
		lua_pushwstring(L, str);
		return 1;
	}
	return 0;
}

int memo_set_colour(lua_State* L)
{
	if (TMemoLua* wnd = lua_cast<TMemoLua>(L))
	{
		wnd->set_text_colour(lua_optColor(L, 2)); // must be only ASCII
		wnd->set_background_colour(lua_optColor(L, 3));
	}
	else
	{
		throw_error(L, "memo_set_colour failed");
	}
	return 0;
}

static void shake_scite_descendants()
{
	Rect frt;
	get_parent()->get_rect(frt, false);
	get_parent()->send_msg(WM_SIZE, SIZE_RESTORED, (LPARAM)MAKELONG(frt.width(), frt.height()));
}

class SideSplitter : public TSplitterB
{
public:
	SideSplitter(TWin* form, TWin* control) : TSplitterB(form, control, 5) {}

	void paint(TDC* dc) override;
	void on_resize(const Rect& rt) override;
};

void SideSplitter::paint(TDC* dc)
{
	Rect rt(this);
	dc->rectangle(rt);
}

void SideSplitter::on_resize(const Rect& rt)
{
	TSplitterB::on_resize(rt);
	shake_scite_descendants();
}

// gui.set_panel() - hide sidebar panel
// gui.set_panel(parent_window, sAlignment) - show sidebar and attach to parent_window with sAlignment
static int do_set_panel(lua_State* L)
{
	if (!get_content_window())
	{
		lua_pushstring(L, "Window subclassing was not successful");
		lua_error(L);
	}
	if (!lua_isuserdata(L, 1) && extra_window)
	{
		extra_window->hide();
		extra_window = nullptr;
		extra_window_splitter->close();
		delete extra_window_splitter;
		extra_window_splitter = nullptr;
		shake_scite_descendants();
	}
	else
	{
		extra_window = window_arg(L);
		const char* def_align = "left";
		auto align = luaL_optstring(L, 2, def_align);
		if (!strcmp(align, def_align))
			extra_window->align(Alignment::alLeft);
		else
			extra_window->align(Alignment::alRight);

		extra_window->set_parent(get_content_window());
		extra_window->show();
		extra_window_splitter = new SideSplitter(get_content_window(), extra_window);
		extra_window_splitter->show();
		force_contents_resize();

		// fix resizing
		Rect rt{};
		extra_window->get_rect(rt, true);
		rt.top += 1;
		extra_window->resize(rt);
		rt.top -= 1;
		extra_window->resize(rt);
	}
	return 0;
}

/////////////////////
// do_day_of_year
#include <algorithm>
#include <ctime>

int do_day_of_year(lua_State* L)
{
	if (lua_gettop(L) == 3)
	{
		auto days = [](int day, int month, int year)
			{
				const int a = (14 - month) / 12;
				const int y = year - a;
				const int mnth = month + 12 * a - 3;
				return  day
					+ (153 * mnth + 2) / 5
					+ 365 * y
					+ y / 4
					- y / 100
					+ y / 400;
			};
		const int dd = std::clamp((int)luaL_checkinteger(L, 1), 1, 31);
		const int mm = std::clamp((int)luaL_checkinteger(L, 2), 1, 12);
		const int yy = luaL_checkinteger(L, 3);
		lua_pushinteger(L, (lua_Integer)days(dd, mm, yy) - days(0, 1, yy));
	}
	else
	{
		time_t seconds = time(NULL);
		tm timeinfo{};
		localtime_s(&timeinfo, &seconds);
		lua_pushinteger(L, (lua_Integer)timeinfo.tm_yday + 1);
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////

int do_test_function(lua_State* L)
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//  this will allow us to hand keyboard focus over to editor
//  gui.pass_focus() переключаем фокус на редактор
static int do_pass_focus(lua_State* L)
{
	lua_getglobal(L, "editor");
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, "Focus");
	TWin* code_wnd = get_code_window();
	if (code_wnd) code_wnd->set_focus();
	return 0;
}

// parent_wnd:client(child_wnd)
int window_client(lua_State* L)
{
	TEventWindow* cw = lua_cast<TEventWindow>(L);
	TWin* child = window_arg(L, 2);
	if (cw && child)
	{
		child->set_parent(cw);
		cw->set_client(child);
	}
	else
		throw_error(L, "wnd:client(arg) - arg must provide a child window");
	return 0;
}

class TListViewLua : public TListView, public LuaControl
{
public:
	TListViewLua(TEventWindow* parent, bool multiple_columns = false, bool single_select = true, bool large_icons = false)
		: TListView(parent, multiple_columns, single_select, large_icons)
	{
		if (!multiple_columns) add_column(nullptr, 200);
	}

private:
	void clear_impl() override;
	void handle_select(int id) override;
	void handle_double_click(int row, int col, const char* s) override;
	void handle_onkey(int id) override;
	void handle_onfocus(bool yes) override;
	void remove_item_impl(int idx) override;
};

void TListViewLua::remove_item_impl(int idx)
{
	lua_reg_release(L, get_item_data(idx));
}

void TListViewLua::clear_impl()
{
	int items_count = count();
	for (int i = 0; i < items_count; i++)
		//luaL_unref(L, LUA_REGISTRYINDEX, data);
		lua_reg_release(L, get_item_data(i));
	//log_add("TListViewLua clear [%d]", items_count);
}

void TListViewLua::handle_select(int id)
{
	dispatch_ref(L, on_select_idx, id);
}

void TListViewLua::handle_double_click(int row, int col, const char* s)
{
	dispatch_ref(L, on_double_idx, row, col, s);
}

void TListViewLua::handle_onkey(int id)
{
	dispatch_ref(L, on_key_idx, id, IsKeyDown(VK_CONTROL), IsKeyDown(VK_MENU), IsKeyDown(VK_SHIFT));
}

void TListViewLua::handle_onfocus(bool yes)
{
	dispatch_ref(L, on_focus_idx, yes);
}

class TTreeViewLua : public TTreeView, public LuaControl
{
public:
	TTreeViewLua(TEventWindow* parent, DWORD style) : TTreeView(parent, style) {}

private:
	void handle_select(void* itm) override;
	void handle_dbclick(void* itm) override;
	void handle_onkey(int id) override;
	size_t handle_ontip(void* item, wchar_t* str) override;
	void clean_data(int data) override;
};

void TTreeViewLua::handle_select(void* itm)
{
	dispatch_ref(L, on_select_idx, itm);
}

void TTreeViewLua::handle_dbclick(void* itm)
{
	dispatch_ref(L, on_double_idx, itm);
}

void TTreeViewLua::handle_onkey(int id)
{
	dispatch_ref(L, on_key_idx, id, IsKeyDown(VK_CONTROL), IsKeyDown(VK_MENU), IsKeyDown(VK_SHIFT));
}

size_t TTreeViewLua::handle_ontip(void* item, wchar_t* str)
{
	//dispatch_ref(L, ontip_idx, item);
	if (on_tip_idx != 0)
	{
		LSG;
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, on_tip_idx);
		lua_pushargs(L, item);// stack: func arg1 arg2 ... argn
		lua_pushcfunction(L, errorHandler);// stack: func arg1 arg2 ... argn errorHandler
		//const int args_count = 1;
		const int errorHandlerIndex = -(1 + 2);
		lua_insert(L, errorHandlerIndex); //stack: errorHandler func arg1 arg2 ... argn
		if (lua_pcall(L, 1, 1, errorHandlerIndex))
			OutputMessage(L);
		else
		{
			std::wstring res = StringFromUTF8(lua_tostring(L, -1));
			wcscpy_s(str, MAX_PATH, res.c_str());
			return res.size();
		}
	}
	return 0;
}

void TTreeViewLua::clean_data(int data)
{
	//luaL_unref(L, LUA_REGISTRYINDEX, data);
	lua_reg_release(L, data);
}

// tree:set_iconlib( [path = "toolbar\\cool.dll"] [, small = true] )
// tabbar:set_iconlib( [path = "toolbar\\cool.dll"] [, small = true] )
// list_wnd:set_iconlib( [path = "toolbar\\cool.dll"] [, small = true] )
int do_set_iconlib(lua_State* L)
{
	if (THasIconWin* win = dynamic_cast<THasIconWin*>(window_arg(L)))
	{
		std::wstring txt = StringFromUTF8(luaL_optstring(L, 2, DEFAULT_ICONLIB));
		bool small_size = optboolean(L, 3, true);
		int icons_loaded = win->load_icons(txt.c_str(), small_size);
		lua_pushinteger(L, icons_loaded);
		return 1;
	}
	return 0;
}

#define check_treewnd lua_cast<TTreeViewLua>
// tree_wnd:tree_set_colour() -- set colors
int do_tree_set_colour(lua_State* L)
{
	auto tr = check_treewnd(L);
	tr->set_foreground(lua_optColor(L, 2));
	tr->set_background(lua_optColor(L, 3));
	return 0;
}

// tree_wnd:tree_expand( item )
int tree_expand(lua_State* L)
{
	auto tr = check_treewnd(L);
	HANDLE itm = lua_touserdata(L, 2);
	if (itm)
		tr->expand(itm);
	else
		luaL_error(L, "tree_expand: tree_item expected");
	return 0;
}

// tree_wnd:tree_collapse( item )
int tree_collapse(lua_State* L)
{
	auto tr = check_treewnd(L);
	HANDLE itm = lua_touserdata(L, 2);
	if (itm)
		tr->collapse(itm);
	else
		luaL_error(L, "tree_collapse: tree_item expected");
	return 0;
}

// tree_wnd:tree_remove_item( item )
int tree_remove_item(lua_State* L)
{
	auto tr = check_treewnd(L);
	HANDLE itm = lua_touserdata(L, 2);
	if (itm)
		tr->remove_item(itm);
	else
		luaL_error(L, "tree_remove_item: tree_item expected");
	return 0;
}

// tree_wnd:tree_remove_childs( item )
int tree_remove_childs(lua_State* L)
{
	auto tr = check_treewnd(L);
	HANDLE itm = lua_touserdata(L, 2);
	if (itm)
		tr->remove_childs(itm);
	else
		luaL_error(L, "tree_remove_childs: tree_item expected");
	return 0;
}

// tree_wnd:set_insert_mode( item )
// tree_wnd:set_insert_mode( 'type one of "last, first, sort, root"' )
int tree_set_insert_mode(lua_State* L)
{
	auto tr = check_treewnd(L);
	int type_id = lua_type(L, 2);
	switch (type_id)
	{
	case LUA_TSTRING:
		tr->insert_mode(lua_tostring(L, 2));
		break;

	case LUA_TLIGHTUSERDATA:
		if (HANDLE item = lua_touserdata(L, 2))
			tr->insert_mode(item);
		break;

	default:
		tr->insert_mode("last");
	}
	return 0;
}

// tree_wnd:tree_get_item_parent() -- return handle parent of item
int tree_get_item_parent(lua_State* L)
{
	auto tr = check_treewnd(L);
	HANDLE hItem = lua_touserdata(L, 2);
	if (HANDLE hParent = tr->get_item_parent(hItem))
	{
		lua_pushlightuserdata(L, hParent);
		return 1;
	}
	return 0;
}

// tree_wnd:tree_get_item() -- return handle of item
int tree_get_item(lua_State* L)
{
	auto tr = check_treewnd(L);
	const char* caption = luaL_checkstring(L, 2);
	HANDLE parent = lua_touserdata(L, 3);
	if (HANDLE sel_itm = tr->get_item_by_name(StringFromUTF8(caption).c_str(), parent))
	{
		lua_pushlightuserdata(L, sel_itm);
		return 1;
	}
	return 0;
}

// tree_wnd:tree_get_item_selected() -- return handle of selected item
int tree_get_item_selected(lua_State* L)
{
	auto tr = check_treewnd(L);
	HANDLE sel_itm = tr->get_selected();
	if (sel_itm)
		lua_pushlightuserdata(L, sel_itm);
	else
		lua_pushnil(L);
	return 1;
}

// tree_wnd:tree_set_item_text( item_ud, caption )
int tree_set_item_text(lua_State* L)
{
	auto tr = check_treewnd(L);
	if (HANDLE ud = lua_touserdata(L, 2))
	{
		std::wstring txt = StringFromUTF8(luaL_checkstring(L, 3));
		tr->set_item_text(ud, txt.data());
	}
	else
		luaL_error(L, "tree_set_item_text: there is no tree at 2nd arg");
	return 0;
}

// tree_wnd:tree_get_item_text( item_ud )
int tree_get_item_text(lua_State* L)
{
	auto tr = check_treewnd(L);
	if (HANDLE ud = lua_touserdata(L, 2))
	{
		const std::wstring str = tr->get_item_text(ud);
		lua_pushwstring(L, str);
		return 1;
	}
	else
		luaL_error(L, "tree_get_item_text: there is no treeitem at 2nd arg");
	return 0;
}

// tree_wnd:tree_get_item_data( item_ud )
int tree_get_item_data(lua_State* L)
{
	auto tr = check_treewnd(L);
	if (HANDLE ud = lua_touserdata(L, 2))
	{
		if (int data = tr->get_item_data(ud))
			//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
			lua_reg_restore(L, data);
		else
			lua_pushnil(L);
		return 1;
	}
	else
		luaL_error(L, "tree_get_item_data: there is no treeitem at 2nd arg");
	return 0;
}

int do_tree_set_LabelEditable(lua_State* L)
{
	auto tr = check_treewnd(L);
	tr->makeLabelEditable(lua_toboolean(L, 2));
	return 0;
}

int do_wnd_set_theme(lua_State* L)
{
	if (auto lst = dynamic_cast<TListViewLua*>(window_arg(L)))
		lst->set_theme(lua_toboolean(L, 2));
	else if (auto tree = dynamic_cast<TTreeViewLua*>(window_arg(L)))
		tree->set_theme(lua_toboolean(L, 2));
	else
		luaL_error(L, "do_wnd_set_theme: there is no tree or list at 1st arg");
	return 0;
}

// list_wnd:select_item( id )
// tree_wnd:select_item( item_ud )
int window_select_item(lua_State* L)
{
	if (auto lv = dynamic_cast<TListViewLua*>(window_arg(L)))
	{
		lv->select_item((int)luaL_checkinteger(L, 2));
	}
	else if (auto tr = check_treewnd(L))
	{
		tr->select(lua_touserdata(L, 2));
	}
	return 0;
}

// list_wnd:delete_item(index)
int window_delete_item(lua_State* L)
{
	TListViewLua* lv = lua_cast<TListViewLua>(L);
	lv->remove_item(static_cast<int>(luaL_checkinteger(L, 2)));
	return 0;
}

void construct_menu(lua_State* L, vecws& items, HMENU hm, MessageHandler* dispatcher)
{
	while (items.size())
	{
		std::wstring item = items[0];
		items.erase(items.begin(), items.begin() + 1);
		size_t pos = item.find(L"POPUPBEGIN");
		if (pos != std::string::npos)
		{
			std::wstring itm_caption = item.substr(pos + 11);
			HMENU submenu = CreatePopupMenu();
			AppendMenu(hm, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(submenu), itm_caption.data());
			construct_menu(L, items, submenu, dispatcher);
			continue;
		}
		pos = item.find(L"POPUPEND");
		if (pos != std::string::npos) return;
		pos = item.find(L"|");
		if (pos == std::string::npos)
		{
			AppendMenu(hm, MF_SEPARATOR, 0, NULL);
		}
		else
		{
			auto text = item.substr(0, pos);
			auto fun = item.substr(pos + 1);
			lua_pushwstring(L, fun);
			const UINT ref_idx = lua_reg_store(L);
			//Item itm(ref_idx);
			AppendMenu(hm, MF_STRING, dispatcher->add_item(ref_idx), text.data());
		}
	}
}

vecws table_to_str_array(lua_State* L, int idx)
{
	vecws res;
	if (!lua_istable(L, idx))
	{
		throw_error(L, "table argument expected");
	}
	lua_pushnil(L); // first key
	while (lua_next(L, idx) != 0)
	{
		res.emplace_back(StringFromUTF8(lua_tostring(L, -1)));
		lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
	}
	return res;
}

int window_context_menu(lua_State* L)
{
	TWin* w = window_arg(L);
	if (LuaWindow* cw = dynamic_cast<LuaWindow*>(w))
	{
		HMENU hm = CreatePopupMenu();
		auto items = table_to_str_array(L, 2);
		construct_menu(L, items, hm, cw->get_handler());
		cw->set_popup_menu(hm);
	}
	else if (TNotifyWin* tc = dynamic_cast<TNotifyWin*>(w))
	{
		HMENU hm = CreatePopupMenu();
		auto items = table_to_str_array(L, 2);
		construct_menu(L, items, hm, tc->get_parent_win()->get_handler());
		tc->set_popup_menu(hm);
	}
	return 0;
}

int window_aux_item(lua_State* L, bool at_index)
{
	TWin* w = window_arg(L);
	if (TListViewLua* lv = dynamic_cast<TListViewLua*>(w))
	{
		int ref_idx = 0;
		int next_arg = at_index ? 3 : 2;
		int ipos = at_index ? luaL_checkinteger(L, 2) : lv->count();
		if (!lua_isnoneornil(L, next_arg + 1))
		{
			lua_pushvalue(L, next_arg + 1);
			//ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			ref_idx = lua_reg_store(L);
		}
		if (lua_isstring(L, next_arg))
		{
			lv->add_item_at(ipos, StringFromUTF8(luaL_checkstring(L, next_arg)).c_str(), luaL_optinteger(L, next_arg + 2, 0), ref_idx); // single column init with string
		}
		else
		{
			vecws items = table_to_str_array(L, next_arg);
			const int _min = min(lv->columns(), items.size());
			int idx = lv->add_item_at(ipos, items.at(0).data(), 0, ref_idx); // init first column
			for (int i = 1; (i < _min) && items.at(i).size(); ++i) // init others
				lv->add_subitem(idx, items.at(i).data(), i);
		}
	}
	else if (TTreeViewLua* tv = dynamic_cast<TTreeViewLua*>(w))
	{
		HANDLE parent = lua_touserdata(L, 3);
		int icon_idx = luaL_optinteger(L, 4, -1);
		int selicon_idx = luaL_optinteger(L, 5, icon_idx);
		int ref_idx = 0;
		if (!lua_isnoneornil(L, 6))
		{
			lua_pushvalue(L, 6);
			//ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			ref_idx = lua_reg_store(L);
		}
		if (HANDLE h = tv->add_item(StringFromUTF8(luaL_checkstring(L, 2)).c_str(), parent, icon_idx, selicon_idx, ref_idx))
		{
			lua_pushlightuserdata(L, h);
			return 1;
		}
	}
	return 0;
}

// list_wnd:add_item(text, data)
// list_wnd:add_item({text1,text2}, data)
// tree:add_item( text [, parent_item = root][, id_icon = -1][, id_selicon = -1][, data = null] )
int window_add_item(lua_State* L)
{
	return window_aux_item(L, false);
}

// list_wnd:insert_item(index, string)
int window_insert_item(lua_State* L)
{
	window_aux_item(L, true);
	return 0;
}

// list_wnd:add_column( sTitle, iSize)
// @param sTitle
// @param iWidth
int window_add_column(lua_State* L)
{
	if(TListViewLua* lv = lua_cast<TListViewLua>(L))
		lv->add_column(StringFromUTF8(luaL_checkstring(L, 2)).c_str(), (int)luaL_checkinteger(L, 3));
	return 0;
}

// list_wnd:get_item_text( index )
int window_get_item_text(lua_State* L)
{
	std::wstring buff(MAX_PATH, 0);
	if (TListViewLua* lv = lua_cast<TListViewLua>(L))
		lv->get_item_text((int)luaL_checkinteger(L, 2), buff.data(), MAX_PATH);
	lua_pushwstring(L, buff);
	return 1;
}

// list_wnd:get_item_data( index )
int window_get_item_data(lua_State* L)
{
	TListViewLua* lv = lua_cast<TListViewLua>(L);
	if (!lv) return 0;
	lua_reg_restore(L, lv->get_item_data((int)luaL_checkinteger(L, 2)));
	return 1;
}

// list_wnd:set_list_colour( sForeColor, sBackColor )
int window_set_colour(lua_State* L)
{
	if (TListViewLua* lv = lua_cast<TListViewLua>(L))
	{
		lv->set_foreground(lua_optColor(L, 2));
		lv->set_background(lua_optColor(L, 3));
	}
	return 0;
}

// list_wnd:get_selected_item(index)
// @return index
int window_selected_item(lua_State* L)
{
	TListViewLua* lv = lua_cast<TListViewLua>(L);
	lua_pushinteger(L, lv ? lv->selected_id() : -1);
	return 1;
}

// list_wnd:get_selected_items(index)
// @return {idx1, idx2, idx3, ...}
int window_selected_items(lua_State* L)
{
	TListViewLua* lv = lua_cast<TListViewLua>(L);
	lua_newtable(L);
	if (!lv) return 1;
	int i = -1;
	int idx = 0;
	while (lv)
	{
		i = lv->next_selected_id(i);
		if (i < 0) break;
		lua_pushinteger(L, i);
		lua_rawseti(L, -2, ++idx);
	}
	return 1;
}

// list_wnd:selected_count()
// @return count of selected items
int window_selected_count(lua_State* L)
{
	if (TListViewLua* lv = lua_cast<TListViewLua>(L))
		lua_pushinteger(L, lv->selected_count());
	return 1;
}

// list_wnd:autosize( index [, by_contents = false])
// @param nIndex
// @param bFlag [=false]
int window_autosize(lua_State* L)
{
	if (TListViewLua* lv = lua_cast<TListViewLua>(L))
		lv->autosize_column((int)luaL_checkinteger(L, 2), optboolean(L, 3));
	return 0;
}

/////////////  TListBoxLua  /////////////

class TListBoxLua :public TListBox, public LuaControl
{
public:
	TListBoxLua(TEventWindow* parent, int id, bool is_sorted) : TListBox(parent, id, is_sorted) {}

private:
	void clear_impl() override
	{
		const int items_count = count();
		for (int i = 0; i < items_count; i++)
			//luaL_unref(L, LUA_REGISTRYINDEX, data);
			lua_reg_release(L, get_data(i));
	}
};

// list_box:insert(idx, str, data)
int do_listbox_insert(lua_State* L)
{
	TListBoxLua* lb = lua_cast<TListBoxLua>(L);
	if (!lb) return 0;
	const int id = luaL_checkinteger(L, 2) - 1;
	const char* val = luaL_checkstring(L, 3);
	lb->insert(id, StringFromUTF8(val).c_str());
	if (!lua_isnil(L, 4))
	{
		lua_pushvalue(L, 4);
		//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
		int ref_idx = lua_reg_store(L);
		lb->set_data(id, ref_idx);
	}
	return 0;
}

// list_box:append(str, data)
int do_listbox_append(lua_State* L)
{
	TListBoxLua* lb = lua_cast<TListBoxLua>(L);
	if (!lb) return 0;
	const int id = lb->count();
	const char* val = luaL_checkstring(L, 2);
	lb->insert(id, StringFromUTF8(val).c_str());
	if (!lua_isnil(L, 3))
	{
		lua_pushvalue(L, 3);
		//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
		int ref_idx = lua_reg_store(L);
		lb->set_data(id, ref_idx);
	}
	return 0;
}

// list_box:remove(idx)
int do_listbox_remove(lua_State* L)
{
	if (TListBoxLua* lb = lua_cast<TListBoxLua>(L))
	{
		const int id = luaL_checkinteger(L, 2) - 1;
		//luaL_unref(L, LUA_REGISTRYINDEX, data);
		lua_reg_release(L, lb->get_data(id));
		lb->remove(id);
	}
	return 0;
}

// list_wnd:count() or listbox:count()
// @return count of elements
int do_list_elements_count(lua_State* L)
{
	if (auto lb = dynamic_cast<TListBoxLua*>(window_arg(L)))
	{
		lua_pushinteger(L, lb->count());
		return 1;
	}
	else if (auto lv = dynamic_cast<TListViewLua*>(window_arg(L)))
	{
		lua_pushinteger(L, lv->count());
		return 1;
	}
	else
		throw_error(L, "1st argument must be ListBox or ListView");
	return 0;
}

int do_listbox_get_text(lua_State* L)
{
	TListBoxLua* lb = lua_cast<TListBoxLua>(L);
	if (!lb) return 0;
	const int id = luaL_checkinteger(L, 2) - 1;
	lua_pushwstring(L, lb->get_text(id));
	return 1;
}

int do_listbox_get_data(lua_State* L)
{
	TListBoxLua* lb = lua_cast<TListBoxLua>(L);
	if (!lb) return 0;
	const int id = luaL_checkinteger(L, 2) - 1;
	if (int data = lb->get_data(id))
	{
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		return 1;
	}
	return 0;
}

////////////////////////// generic methods for window //////////////////////////

// list_wnd:clear()
// tree_wnd:clear()
// listbox_wnd:clear()
int window_clear(lua_State* L)
{
	TWin* w = window_arg(L);
	if (TListViewLua* lv = dynamic_cast<TListViewLua*>(w))
	{
		lv->clear();
	}
	else if (TTreeViewLua* tr = dynamic_cast<TTreeViewLua*>(w))
	{
		tr->clear();
	}
	else if (TListBoxLua* lb = dynamic_cast<TListBoxLua*>(w))
	{
		lb->clear();
	}
	return 0;
}

int do_wnd_selection(lua_State* L)
{
	if (TTrackBar* trb = dynamic_cast<TTrackBar*>(window_arg(L)))
	{
		if (lua_gettop(L) == 1)
		{
			lua_pushinteger(L, trb->sel_start());
			lua_pushinteger(L, trb->sel_end());
			return 2;
		}
		else
		{
			int imin = luaL_optinteger(L, 2, 0);
			int imax = luaL_optinteger(L, 3, 100);
			trb->selection(imin, imax);
		}
	}
	else if (auto lb = dynamic_cast<TListBoxLua*>(window_arg(L)))
	{
		if (lua_gettop(L) == 1)
		{
			lua_pushinteger(L, (lua_Integer)lb->selected() + 1);
			return 1;
		}
		else
		{
			// set selection
			lb->selected(luaL_checkinteger(L, 2) - 1);
		}
	}
	return 0;
}

// wnd:on_tip(function or <name global function>)
int window_on_tip(lua_State* L)
{
	if (LuaControl* lc = lua_cast<LuaControl>(L)) lc->set_on_tip(2);
	return 0;
}

// wnd:on_select(function or <name global function>)
int window_on_select(lua_State* L)
{
	if (LuaControl* lc = lua_cast<LuaControl>(L)) lc->set_select(2);
	return 0;
}

// wnd:on_double_click(function or <name global function>)
int window_on_double_click(lua_State* L)
{
	if (LuaControl* lc = lua_cast<LuaControl>(L)) lc->set_double_click(2);
	return 0;
}

// wnd:on_close(function or <name global function>)
int window_on_close(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->set_on_close(2);
	return 0;
}

// wnd:on_command(function or <name global function>)
int window_on_command(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->set_on_command(2);
	return 0;
}

// wnd:on_size(function)
int window_on_size(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->set_on_size(2);
	return 0;
}

// wnd:on_scroll(function or <name global function>)
int window_on_scroll(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->set_on_scroll(2);
	return 0;
}

// wnd:on_timer(function, delay)
int window_on_timer(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->set_on_timer(2, luaL_optnumber(L, 3, 1) * 1000);
	return 0;
}

// wnd:stop_timer()
int window_stop_timer(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->kill_timer();
	return 0;
}

// wnd:on_show(function or <name global function>)
int window_on_show(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->set_on_show(2);
	return 0;
}

// wnd:on_move(function or <name global function>)
int window_on_move(lua_State* L)
{
	if (LuaWindow* cw = lua_cast<LuaWindow>(L)) cw->set_on_move(2);
	return 0;
}

// wnd:on_focus(function or <name global function>)
int window_on_focus(lua_State* L)
{
	TWin* w = window_arg(L);
	if (LuaControl* lc = dynamic_cast<LuaControl*>(w))
	{
		lc->set_on_focus(2);
	}
	else if (LuaWindow* lw = dynamic_cast<LuaWindow*>(w))
	{
		lw->set_on_activate(2);
	}
	return 0;
}

int window_on_paint(lua_State* L)
{
	TWin* w = window_arg(L);
	if (LuaWindow* lw = dynamic_cast<LuaWindow*>(w))
	{
		lw->set_on_paint(2);
	}
	return 0;
}

// wnd:on_key(function or <name global function>)
int window_on_key(lua_State* L)
{
	TWin* w = window_arg(L);
	if (LuaControl* lc = dynamic_cast<LuaControl*>(w))
	{
		lc->set_onkey(2);
	}
	else if (LuaWindow* lw = dynamic_cast<LuaWindow*>(w))
	{
		lw->set_on_key(2);
	}
	return 0;
}

////////////// ComboBox //////////////

#define check_combo lua_cast<TComboBox>

int do_cbox_clear(lua_State* L)
{
	auto cbox = check_combo(L);
	if (!cbox) return 0;
	const int cnt = cbox->count();
	for (int idx = 0; idx < cnt; ++idx)
		//luaL_unref(L, LUA_REGISTRYINDEX, data);
		lua_reg_release(L, cbox->get_data(idx));
	cbox->reset();
	return 0;
}

int do_cbox_append_string(lua_State* L)
{
	auto cbox = check_combo(L);
	if (!cbox) return 0;
	auto text = StringFromUTF8(luaL_checkstring(L, 2));
	int idx = cbox->add_string(text.c_str());
	if (!lua_isnil(L, 3))
	{
		lua_pushvalue(L, 3);
		//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
		int ref_idx = lua_reg_store(L);
		cbox->set_data(idx, ref_idx);
	}
	return 0;
}

int do_cbox_insert_string(lua_State* L)
{
	auto cbox = check_combo(L);
	if (!cbox) return 0;
	int id = luaL_checkinteger(L, 2);
	const char* text = luaL_checkstring(L, 3);
	cbox->insert_string(id, StringFromUTF8(text).c_str());
	if (!lua_isnil(L, 4))
	{
		lua_pushvalue(L, 4);
		//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
		int ref_idx = lua_reg_store(L);
		cbox->set_data(id, ref_idx);
	}
	return 0;
}

int do_cbox_find_string(lua_State* L)
{
	auto cbox = check_combo(L);
	if (!cbox) return 0;
	auto text = StringFromUTF8(luaL_checkstring(L, 2));
	int start_pos = luaL_optinteger(L, 3, -1);
	int idx = cbox->find_string(start_pos, text.c_str());
	lua_pushinteger(L, idx);
	return 1;
}

int do_cbox_remove(lua_State* L)
{
	auto cbox = check_combo(L);
	if (!cbox) return 0;
	int idx = luaL_checkinteger(L, 2);
	lua_reg_release(L, cbox->get_data(idx));
	lua_pushinteger(L, cbox->remove_item(idx));
	return 1;
}

int do_cbox_get_data(lua_State* L)
{
	auto cbox = check_combo(L);
	if (!cbox) return 0;
	int id = cbox->get_cursel();
	if (int data = cbox->get_data(id))
	{
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		return 1;
	}
	return 0;
}

int do_cbox_set_items_h(lua_State* L)
{
	if (auto cbox = check_combo(L))	cbox->set_height(luaL_checkinteger(L, 2));
	return 0;
}

int do_cbox_setcursel(lua_State* L)
{
	if (auto cbox = check_combo(L)) cbox->set_cursel(luaL_checkinteger(L, 2));
	return 0;
}

int do_cbox_getcursel(lua_State* L)
{
	auto cbox = check_combo(L);
	lua_pushinteger(L, cbox ? cbox->get_cursel() : 0);
	return 1;
}

int do_cbox_count(lua_State* L)
{
	auto cbox = check_combo(L);
	lua_pushinteger(L, cbox ? cbox->count() : 0);
	return 1;
}

int do_cbox_get_item_text(lua_State* L)
{
	auto cbox = check_combo(L);
	if (!cbox) return 0;
	const int idx = luaL_checkinteger(L, 2);
	const std::wstring str = cbox->get_item_text(idx);
	lua_pushwstring(L, str);
	return 1;
}

int do_get_ctrl_id(lua_State* L)
{
	if (TWin* w = window_arg(L))
	{
		if (dynamic_cast<LuaControl*>(w))
			lua_pushinteger(L, w->get_ctrl_id());
		else
			lua_pushinteger(L, w->get_id());
	}
	else
	{
		lua_pushinteger(L, 0);
	}
	return 1;
}

// simple tooltip
// wnd:set_tiptext(ctrID, "tips","optCaprion", bBaloonStype, bCloseBtn, IconType)
int do_set_tooltip(lua_State* L)
{
	if (TEventWindow* form = lua_cast<TEventWindow>(L))
	{
		int id = luaL_checkinteger(L, 2);
		auto text = StringFromUTF8(luaL_checkstring(L, 3));
		auto caption = StringFromUTF8(luaL_optstring(L, 4, nullptr));
		bool baloon = optboolean(L, 5);
		bool close_btn = optboolean(L, 6);
		unsigned int icon = luaL_optinteger(L, 7, 0);
		form->set_tooltip(id, text.c_str(), caption.c_str(), baloon, close_btn, icon);
	}
	return 0;
}

int do_remove_transparent(lua_State* L)
{
	if (TEventWindow* form = lua_cast<TEventWindow>(L))
		form->remove_transparent();
	return 0;
}

int do_set_transparent(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	const int percent = luaL_optinteger(L, 2, 255);
	form->set_transparent(std::clamp(percent, 0, 255));
	return 0;
}

/////////////////// status bar //////////////////////////////

//win_parent:statusbar(100, ..., 200,-1)
int do_statusbar(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	const int nbParts = lua_gettop(L) - 1;
	auto parts = std::make_unique<int[]>(nbParts);
	for (int idx = 0; idx < nbParts; idx++)
		parts[idx] = static_cast<int>(luaL_checkinteger(L, idx + 2));
	form->set_statusbar(nbParts, parts.get());
	return 0;
}

// win_parent:status_setpart(part_id,"text")
int do_status_setpart(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	int part_id = luaL_checkinteger(L, 2);
	const char* text = luaL_checkstring(L, 3);
	form->set_statusbar_text(part_id, StringFromUTF8(text).c_str());
	return 0;
}

//////////////////  ProgressControl  ///////////////////////////////

int do_set_progress_pos(lua_State* L)
{
	if (auto pc = lua_cast<TProgressControl>(L)) pc->set_pos(luaL_checkinteger(L, 2));
	return 0;
}

int do_set_progress_range(lua_State* L)
{
	if (auto pc = lua_cast<TProgressControl>(L)) pc->set_range(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3));
	return 0;
}

int do_get_progress_range(lua_State* L)
{
	auto pc = lua_cast<TProgressControl>(L);
	if (!pc) return 0;
	int low = 0;
	int high = 0;
	pc->get_range(low, high);
	lua_pushinteger(L, low);
	lua_pushinteger(L, high);
	return 2;
}

int do_progress_go(lua_State* L)
{
	if (auto pc = lua_cast<TProgressControl>(L))	pc->go();
	return 0;
}

int do_progress_step(lua_State* L)
{
	if (auto pc = lua_cast<TProgressControl>(L))	pc->set_step(luaL_checkinteger(L, 2));
	return 0;
}

////////////////////////////////////////////////////////////////////

int do_log(lua_State* L)
{
	const char* text = luaL_checkstring(L, 1);
	log_add(text);
	return 0;
}

int do_create_link(lua_State* L)
{
	std::wstring path = StringFromUTF8(luaL_checkstring(L, 1));
	std::wstring link = StringFromUTF8(luaL_checkstring(L, 2));
	std::wstring wd = StringFromUTF8(lua_tostring(L, 3));
	if (wd.empty())
	{
		size_t pos = path.find_last_of(L'\\');
		wd = path.substr(0, pos);
	}
	std::wstring descr = StringFromUTF8(lua_tostring(L, 4));
	int res = CreateShellLink(path.c_str(), link.c_str(), wd.c_str(), descr.c_str());
	lua_pushinteger(L, res);
	return 1;
}

int do_get_shell_folder(lua_State* L)
{
	int folder_id = luaL_checkinteger(L, 1);
	std::wstring res = GetKnownFolder(folder_id);
	lua_pushwstring(L, res);
	return 1;
}

int do_get_scite_window(lua_State* L)
{
	return wrap_window(L, get_parent());
}

int do_get_desktop_window(lua_State* L)
{
	return wrap_window(L, get_desktop_window());
}

////////////////////////////////////////////////////////////////////

//parent:add_tabbar("align")
int add_tabbar(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	std::string align = luaL_optstring(L, 2, "top");
	TTabControlLua* pTab = new TTabControlLua(form);
	Alignment al = Alignment::alTop;
	if (align == "top")
		al = Alignment::alTop;
	else
		al = Alignment::alBottom;
	pTab->align(al);
	form->add(pTab);
	return wrap_window(L, pTab);
}

//child:set_align([sAlign = "client"][, nSize = 100])
int do_set_align(lua_State* L)
{
	WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, 1));
	if (!wrp)
	{
		throw_error(L, "not a window");
		return 0;
	}
	if (TWin* child = dynamic_cast<TWin*>(wrp->window))
	{
		std::string align = luaL_optstring(L, 2, "client");
		const int sz = luaL_optnumber(L, 3, 100);
		//child->set_parent(p);
		Alignment al = Alignment::alClient;
		if (align == "top")
			al = Alignment::alTop;
		else if (align == "bottom")
			al = Alignment::alBottom;
		else if (align == "none")
			al = Alignment::alNone;
		else if (align == "left")
			al = Alignment::alLeft;
		else if (align == "right")
			al = Alignment::alRight;
		child->align(al, sz);
		TEventWindow* form = child->get_parent_win();
		form->remove(child);
		if (al == Alignment::alClient)
			form->set_client(child);
		else
			form->add(child);
		// has splitter
		if (wrp->data)
		{
			form->remove(wrp->data);
			//wrp->data->hide();
			wrp->data->set_parent();
			wrp->data->close();
			delete wrp->data;
			wrp->data = nullptr;
		}
		if (al != Alignment::alClient && al != Alignment::alNone && optboolean(L, 4))
		{
			TSplitter* split = new TSplitter(form, child);
			form->add(split);
			wrp->data = split;
		}
	}
	else
		throw_error(L, "There is no parent window to split");
	return 0;
}

// parent:add_tree(style)
int add_tree(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	DWORD style = luaL_optinteger(L, 2, 0x2); // stack: parent, style
	TTreeViewLua* pTree = new TTreeViewLua(form, style);
	form->add(pTree);
	return wrap_window(L, pTree);
}

//parent:add_label(style, "caption")
int add_label(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	DWORD label_style = luaL_optinteger(L, 2, 0x1);
	const char* caption = luaL_optstring(L, 3, nullptr);
	TLabel* pLabel = new TLabel(form, label_style);
	if (caption) pLabel->set_text(StringFromUTF8(caption).c_str());
	return wrap_window(L, pLabel);
}

//parent:add_memo()
int add_memo(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	return wrap_window(L, new TMemoLua(form));
}

//parent:add_list(bSingleSelect, bMultiColumn)
int add_list(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	bool multiple_columns = optboolean(L, 2);
	bool single_select = optboolean(L, 3, true);
	bool large_icons = optboolean(L, 4);
	TListViewLua* lv = new TListViewLua(form, multiple_columns, single_select, large_icons);
	form->add(lv);
	return wrap_window(L, lv);
}

//parent:add_groupbox(sCaption, nTextAlign)
int add_groupbox(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	const char* caption = luaL_optstring(L, 2, nullptr);
	TGroupBox* pGrBox = new TGroupBox(form, StringFromUTF8(caption).c_str(), luaL_optinteger(L, 3, 0));
	return wrap_window(L, pGrBox);
}

//parent:add_progress(vertical, hasborder, smooth, smoothrevers)
int add_progress(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	bool vertical = optboolean(L, 2);
	bool hasborder = optboolean(L, 3);
	bool smooth = optboolean(L, 4);
	bool smoothrevers = optboolean(L, 5);
	TProgressControl* pc = new TProgressControl(form, -1, vertical, hasborder, smooth, smoothrevers);
	form->add(pc);
	return wrap_window(L, pc);
}

//parent:add_trackbar(style, id)
int add_trackbar(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	DWORD style = luaL_optinteger(L, 2, 0);
	int cntrl_id = luaL_optinteger(L, 3, -1);
	return wrap_window(L, new TTrackBar(form, style, cntrl_id));
}

//parent:add_checkbox(sCaption, id, bTreestate)
int add_checkbox(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	const char* caption = luaL_optstring(L, 2, nullptr);
	int cntrl_id = luaL_optinteger(L, 3, -1);
	bool is3state = optboolean(L, 4);
	return wrap_window(L, new TCheckBox(form, StringFromUTF8(caption).c_str(), cntrl_id, is3state));
}

//parent:add_radiobutton(sCaption, id, bTreestate)
int add_radiobutton(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	auto caption = luaL_checkstring(L, 2);
	int cntrl_id = luaL_optinteger(L, 3, -1);
	bool is_auto = optboolean(L, 4);
	return wrap_window(L, new TRadioButton(form, StringFromUTF8(caption).c_str(), cntrl_id, is_auto));
}

//parent:add_editbox(sCaption, style, id)
int add_editbox(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	const char* caption = luaL_optstring(L, 2, nullptr);
	DWORD style = luaL_optinteger(L, 3, 0);
	int cntrl_id = luaL_optinteger(L, 4, -1);
	return wrap_window(L, new TEdit(form, StringFromUTF8(caption).c_str(), cntrl_id, style));
}

//parent:add_button(sCaption, id)
int add_button(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	const int cntrl_id = luaL_checkinteger(L, 2);
	const char* caption = luaL_optstring(L, 3, nullptr);
	const bool defpushbtn = optboolean(L, 4, true);
	return wrap_window(L, new TButton(form, cntrl_id, StringFromUTF8(caption).c_str(), defpushbtn ? TButtonBase::ButtonStyle::DEFPUSHBUTTON : TButtonBase::ButtonStyle::PUSHBUTTON));
}

//parent:add_listbox(id, bSorted)
int add_listbox(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	int cntrl_id = luaL_checkinteger(L, 2);
	bool is_sorted = optboolean(L, 3);
	return wrap_window(L, new TListBoxLua(form, cntrl_id, is_sorted));
}

//parent:add_combobox(id, style)
int add_combobox(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	int cntrl_id = luaL_optinteger(L, 2, -1);
	DWORD style = luaL_optinteger(L, 3, CBS_DROPDOWN | CBS_AUTOHSCROLL);
	return wrap_window(L, new TComboBox(form, cntrl_id, style));
}

//parent:add_link(sCaption)
int add_link(lua_State* L)
{
	TEventWindow* form = lua_cast<TEventWindow>(L);
	if (!form) return 0;
	const char* caption = luaL_checkstring(L, 2);
	TSysLink* pLink = new TSysLink(form, StringFromUTF8(caption).c_str());
	form->add(pLink);
	return wrap_window(L, pLink);
}

class TUpDownLua : public TUpDownControl, public LuaControl
{
public:
	TUpDownLua(TEventWindow* form, TWin* buddy, DWORD style) : TUpDownControl(form, buddy, style)
	{
	}

private:
	void ud_clicked(int delta) override;
};

void TUpDownLua::ud_clicked(int delta)
{
	dispatch_ref(L, on_updn_idx, delta);
}

//parent:add_updown(buddy, style)
int add_updown(lua_State* L)
{
	TEventWindow* form = dynamic_cast<TEventWindow*>(window_arg(L));
	if (!form)
	{
		luaL_error(L, "1st arg: There is no parent panel to create 'gui.do_updown'");
		return 0;
	}
	TWin* buddy_wnd = window_arg(L, 2);
	if (!buddy_wnd)
	{
		luaL_error(L, "2nd arg: There is no buddy window for updown control 'gui.do_updown'");
		return 0;
	}
	DWORD style = luaL_optinteger(L, 3, 0);
	TUpDownLua* pUDN = new TUpDownLua(form, buddy_wnd, style);
	form->add(pUDN);
	return wrap_window(L, pUDN);
}

#define check_udc lua_cast<TUpDownLua>

// wnd:on_updown(function or <name global function>)
int updown_on_updown(lua_State* L)
{
	if (TUpDownLua* lc = check_udc(L)) lc->set_on_updown(2);
	return 0;
}

int updown_set_current(lua_State* L)
{
	if (TUpDownLua* lc = check_udc(L)) lc->set_current(luaL_checkinteger(L, 2));
	return 0;
}

int updown_get_current(lua_State* L)
{
	if (TUpDownLua* lc = check_udc(L)) lua_pushinteger(L, lc->get_current());
	return 1;
}

int updown_set_range(lua_State* L)
{
	if (TUpDownLua* lc = check_udc(L)) lc->set_range(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3));
	return 0;
}

////////////////////////////////////////////////////////////////////
// custom paint

TDC* lua_cast_TDC(lua_State* L, int idx = 1)
{
	return static_cast<TDC*>(luaL_checkudata(L, idx, MT_TDC));
}

int tdc_tostring(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		lua_pushstring(L, "tdc_object");
		return 1;
	}
	return 0;
}

int tdc_rectangle(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		Rect rect(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5));
		pTDC->rectangle(rect);
	}
	return 0;
}

int tdc_ellipse(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		Rect rect(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5));
		pTDC->ellipse(rect);
	}
	return 0;
}

int tdc_round_rect(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		const int posx = luaL_checkinteger(L, 2);
		const int posy = luaL_checkinteger(L, 3);
		pTDC->move_to(posx, posy);
	}
	return 0;
}

int tdc_line_to(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		const int posx = luaL_checkinteger(L, 2);
		const int posy = luaL_checkinteger(L, 3);
		pTDC->line_to(posx, posy);
	}
	return 0;
}

int tdc_draw_line(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		std::vector<Point> vp = lua_to_points(L, 2);
		if (vp.size()) pTDC->polyline(vp.data(), vp.size());
	}
	return 0;
}

int tdc_polygone(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		std::vector<Point> vp = lua_to_points(L, 2);
		if (vp.size()) pTDC->polygone(vp.data(), vp.size());
	}
	return 0;
}

int tdc_polybezier(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		std::vector<Point> vp = lua_to_points(L, 2);
		if (vp.size()) pTDC->polybezier(vp.data(), vp.size());
	}
	return 0;
}

int tdc_set_pixel(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
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
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		bool flag = lua_toboolean(L, 2);
		pTDC->xor_pen(flag);
	}
	return 0;
}

int tdc_reset_pen(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		pTDC->reset_pen();
	}
	return 0;
}

int tdc_set_solid_brush(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		COLORREF clr = lua_optColor(L, 2);
		pTDC->set_solid_brush(clr);
	}
	return 0;
}

int tdc_set_hatch_brush(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		COLORREF clr = lua_optColor(L, 2);
		int style = luaL_optinteger(L, 3, 0);
		pTDC->set_hatch_brush(style, clr);
	}
	return 0;
}

int tdc_set_text_align(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		int flags = luaL_checkinteger(L, 2);
		pTDC->set_text_align(flags);
	}
	return 0;
}

int tdc_select_stock(lua_State* L)
{
	if (TDC* pTDC = lua_cast_TDC(L))
	{
		int id = luaL_checkinteger(L, 2);
		pTDC->select_stock(id);
	}
	return 0;
}

const luaL_Reg TDC_metamethods[] =
{
	{ "__tostring", tdc_tostring },
	{ NULL, NULL}
};

const luaL_Reg TDC_methods[] =
{
	{ "color_back", tdc_back_text },
	{ "color_text", tdc_color_text },
	{ "text_align", tdc_set_text_align },
	{ "draw_text",  tdc_draw_text },

	{ "reset_pen",	tdc_reset_pen },
	{ "set_pen",	tdc_set_pen },
	{ "xor_pen",	tdc_set_xorpen },

	{ "solid_brush",tdc_set_solid_brush },
	{ "hatch_brush",tdc_set_hatch_brush },

	{ "move_to",	tdc_move_to },
	{ "set_pixel",	tdc_set_pixel },
	{ "line_to",	tdc_line_to },
	{ "draw_line",	tdc_draw_line },
	{ "polyline",	tdc_polyline },
	{ "polybezier",	tdc_polybezier },
	{ "rectangle",  tdc_rectangle },
	{ "polygone",   tdc_polygone },
	{ "ellipse",	tdc_ellipse },
	{ "round_rect",	tdc_round_rect },
	{ "chord",		tdc_chord },

	{ "select_stock",tdc_select_stock },
	{ NULL, NULL}
};

void init_tdc_metatable(lua_State* L)
{
	LSG;
	luaL_newmetatable(L, MT_TDC);  // create metatable for window objects

	// Add optional metamethods, except __index
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, TDC_metamethods);
#else
	luaL_setfuncs(L, TDC_metamethods, 0);
#endif

	// Add methods
	lua_createtable(L, 0, 0);

#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, TDC_methods);
#else
	luaL_setfuncs(L, TDC_methods, 0);
#endif

	lua_setfield(L, -2, "__index"); // mt.__index = methods
}

////////////////////////////////////////////////////////////////////

static const luaL_Reg gui[] =
{
#ifdef _DEBUG
	{ "test",			do_test_function		},
	{ "log",			do_log 					},
#endif

	// dialogs
	{ "colour_dlg",		do_colour_dlg 			},
	{ "message",		do_message 				},
	{ "open_dlg",		do_open_dlg 			},
	{ "save_dlg",		do_save_dlg 			},
	{ "select_dir_dlg",	do_select_dir_dlg 		},
	{ "prompt_value",	do_prompt_value 		},

	// others
	{ "create_link",	do_create_link 			},
	{ "get_folder",		do_get_shell_folder		},
	{ "day_of_year",	do_day_of_year 			},
	{ "ini_file",		new_inifile 			},
	{ "version",		do_version 				},
	{ "files",			do_files 				},
	{ "chdir",			do_chdir 				},
	{ "run",			do_run					},
	{ "get_ascii",		do_get_ascii			},
	{ "pass_focus",		do_pass_focus			},
	{ "set_panel",		do_set_panel			},

	// windows
	{ "scite_window",	do_get_scite_window		},
	{ "desktop",		do_get_desktop_window	},
	{ "window",			new_window				},
	{ "panel",			new_panel				},

	{ NULL, NULL },
};

static const luaL_Reg window_methods[] =
{
	// window
	{ "show",				window_show			},
	{ "hide",				window_hide			},
	{ "close",				window_close		},
	{ "size",				window_size			},
	{ "enable",				window_enable		},
	{ "set_focus",			window_set_focus	},
	{ "position",			window_position		},
	{ "center_h",			window_center_h		},
	{ "center_v",			window_center_v		},
	{ "center",				window_center		},
	{ "resize",				window_resize		},
	{ "bounds",				window_get_bounds	},
	{ "client",				window_client		},
	{ "add",				window_add			},
	{ "remove_child",		window_remove		},
	{ "context_menu",		window_context_menu	},
	{ "statusbar",			do_statusbar		},
	{ "status_setpart",		do_status_setpart	},
	{ "get_ctrl_id",		do_get_ctrl_id		},
	{ "set_transparent",	do_set_transparent	},
	{ "remove_transparent",	do_remove_transparent},
	{ "set_tiptext",		do_set_tooltip		},

	// progress bar
	{ "set_progress_pos",	do_set_progress_pos		},
	{ "set_progress_range",	do_set_progress_range	},
	{ "get_progress_range",	do_get_progress_range	},
	{ "progress_go",		do_progress_go			},
	{ "set_step",			do_progress_step		},

	// panel, window
	{ "on_close",	window_on_close		},
	{ "on_show",	window_on_show		},
	{ "on_move",	window_on_move		},
	{ "on_focus",	window_on_focus		},
	{ "on_key",		window_on_key		},
	{ "on_command",	window_on_command	},
	{ "on_scroll",	window_on_scroll	},
	{ "on_size",	window_on_size		},
	{ "on_paint",	window_on_paint		},
	{ "on_timer",	window_on_timer		},
	{ "stop_timer",	window_stop_timer	},

	// tabbar
	{ "add_tab",			tabbar_add				},
	{ "select_tab",			tabbar_sel				},
	{ "tab_fixedwidth",		tabbar_fixedwidth		},
	//{ "tab_set_item_size",	tabbar_set_item_size	},
	//{ "tab_rowcount",		tabbar_rowcount	},

	// list, tree
	{ "on_select",			window_on_select		},
	{ "on_double_click",	window_on_double_click	},
	{ "on_tip",				window_on_tip			},
	{ "clear",				window_clear			},
	{ "set_selected_item",	window_select_item		},
	{ "count",				do_list_elements_count	},
	{ "delete_item",		window_delete_item		},
	{ "insert_item",		window_insert_item		},
	{ "add_item",			window_add_item			},
	{ "add_column",			window_add_column		},
	{ "get_item_text",		window_get_item_text	},
	{ "get_item_data",		window_get_item_data	},
	{ "set_list_colour",	window_set_colour		},
	{ "selected_count",		window_selected_count	},
	{ "get_selected_item",	window_selected_item	},
	{ "get_selected_items",	window_selected_items	},
	{ "autosize",			window_autosize			},

	// updown
	{ "on_updown",			updown_on_updown	},
	{ "set_current",		updown_set_current	},
	{ "get_current",		updown_get_current	},
	{ "set_range",			updown_set_range	},

	// tree
	{ "set_tree_colour",	do_tree_set_colour			},
	{ "set_tree_editable",	do_tree_set_LabelEditable	},
	{ "set_theme",			do_wnd_set_theme			},

	// tree, tabbar
	{ "set_iconlib",		do_set_iconlib	},

	// treeItem
	{ "tree_get_item_selected",	tree_get_item_selected	},
	{ "tree_set_insert_mode",	tree_set_insert_mode	},
	{ "tree_remove_item",		tree_remove_item		},
	{ "tree_get_item",			tree_get_item			},
	{ "tree_get_item_parent",	tree_get_item_parent	},
	{ "tree_remove_childs",		tree_remove_childs		},
	{ "tree_get_item_text",		tree_get_item_text		},
	{ "tree_set_item_text",		tree_set_item_text		},
	{ "tree_get_item_data",		tree_get_item_data		},
	{ "tree_collapse",			tree_collapse			},
	{ "tree_expand",			tree_expand				},

	// trackbar
	{ "get_pos",		trbar_get_pos		},
	{ "set_pos",		trbar_set_pos		},
	{ "select",			do_wnd_selection	},
	{ "sel_clear",		trbar_sel_clear		},
	{ "range",			trbar_set_range		},

	// memo
	{ "set_memo_colour", memo_set_colour	},

	// memo, label
	{ "set_text",		window_set_text		},
	{ "get_text",		window_get_text		},

	// label, button
	{ "set_icon",		do_ctrl_set_icon	},
	{ "set_bitmap",		do_ctrl_set_bitmap	},

	// button, checkbox, radio
	{ "check",			do_check			},

	// do_listbox_insert
	{ "insert",			do_listbox_insert	},
	{ "append",			do_listbox_append	},
	{ "remove",			do_listbox_remove	},
	{ "get_line_text",	do_listbox_get_text	},
	{ "get_line_data",	do_listbox_get_data	},

	// combobox
	{ "cb_append",		do_cbox_append_string	},
	{ "cb_insert",		do_cbox_insert_string	},
	{ "cb_clear",		do_cbox_clear			},
	{ "cb_items_h",		do_cbox_set_items_h		},
	{ "cb_setcursel",	do_cbox_setcursel		},
	{ "cb_getcursel",	do_cbox_getcursel		},
	{ "cb_getdata",		do_cbox_get_data		},
	{ "cb_find",		do_cbox_find_string		},
	{ "cb_item_text",	do_cbox_get_item_text	},
	{ "cb_remove",		do_cbox_remove			},
	{ "cb_count",		do_cbox_count			},

	{ "emplace_h",		window_emplace_h		},
	{ "emplace_v",		window_emplace_v		},

	// add controls
	{ "add_tabbar",		add_tabbar 		},
	{ "add_tree",		add_tree 		},
	{ "add_label",		add_label 		},
	{ "add_memo",		add_memo 		},
	{ "add_list",		add_list 		},
	{ "add_groupbox",	add_groupbox 	},
	{ "add_progress",	add_progress 	},
	{ "add_trackbar",	add_trackbar 	},
	{ "add_checkbox",	add_checkbox 	},
	{ "add_radiobutton",add_radiobutton },
	{ "add_editbox",	add_editbox 	},
	{ "add_button",		add_button 		},
	{ "add_listbox",	add_listbox 	},
	{ "add_combobox",	add_combobox 	},
	{ "add_link",		add_link 		},
	{ "add_updown",		add_updown 		},
	{ "set_align",		do_set_align 	},
	{ "add_tooltip",	add_tooltip		},

	{ NULL, NULL },
};

class TPlugin
{
	HMODULE m_hRichEditDll;
public:
	TPlugin();
	~TPlugin();
	void reinit();
};

void destroy_windows()
{
	if (extra_window)
	{
		extra_window->hide();
		extra_window->set_parent();
		extra_window->close();
		// log_add("destroy extra_window");
		delete extra_window;
		extra_window = nullptr;
	}
	if (extra_window_splitter)
	{
		extra_window_splitter->hide();
		extra_window_splitter->set_parent();
		extra_window_splitter->close();
		// log_add("destroy extra_window_splitter");
		delete extra_window_splitter;
		extra_window_splitter = nullptr;
	}
	extra.bottom = extra.top = extra.left = extra.right = 0;
	// shake_scite_descendants();
	collect_windows.clear();
}

TPlugin::TPlugin() : m_hRichEditDll(LoadLibrary(L"riched32.dll"))
{
	// at this point, the SciTE window is available. Can't always assume
	// that it is the foreground window, so we hunt through all windows
	// associated with this thread (the main GUI thread) to find a window
	// matching the appropriate class name

	// EnumThreadWindows(GetCurrentThreadId(),CheckSciteWindow,(LPARAM)&hSciTE);
	hSciTE = FindWindow(L"SciTEWindow", NULL);
	// s_parent = new TWin(hSciTE);

	// Its first child shold be the content pane (editor+output),
	// but we check this anyway....

	//EnumChildWindows(hSciTE,CheckContainerWindow,(LPARAM)&hContent);
	hContent = FindWindowEx(hSciTE, NULL, L"SciTEWindowContent", NULL);

	// the first child of the content pane is the editor pane.
	bool subclassed = false;
	if (hContent)
	{
		log_add("gui inited");
		//content_window = new TWin(hContent);
		hCode = GetWindow(hContent, GW_CHILD);
		if (hCode)
		{
			//code_window = new TWin(hCode);
			old_scite_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hSciTE, GWLP_WNDPROC, (LONG_PTR)SciTEWndProc));
			old_content_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hContent, GWLP_WNDPROC, (LONG_PTR)ContentWndProc));
			old_scintilla_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hCode, GWLP_WNDPROC, (LONG_PTR)ScintillaWndProc));
			subclassed = true;
		}
	}
	if (!subclassed)
		log_add("gui:cannot subclass SciTE Window");
	//get_parent()->message(L"Cannot subclass SciTE Window", 2);
}

TPlugin::~TPlugin()
{
	log_add("gui:on_destroy");
	destroy_windows();
	FreeLibrary(m_hRichEditDll);
}

void TPlugin::reinit()
{
	destroy_windows();
}

TPlugin* getPlugin()
{
	static TPlugin p;
	return &p;
}

extern "C" __declspec(dllexport)
int luaopen_gui(lua_State* L)
{
	if (!hInst)
	{
		MessageBox(NULL, L"This dll work with SciTE.exe only.", L"Error", MB_OK);
		return 0;
	}
	getPlugin()->reinit();
	TLuaState::set_LuaState(L);
	reinit_storage(L);
	lua_openclass_iniFile(L); // IniFile
	lua_openclass_TTipCtrl(L); // ToolTip

	init_tdc_metatable(L);

	luaL_newmetatable(L, WINDOW_CLASS);  // create metatable for window objects
	lua_pushvalue(L, -1);  // push metatable
	lua_setfield(L, -2, "__index");  // metatable.__index = metatable

#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, window_methods);
	//luaL_openlib(L, "gui", gui, 0);
	luaL_register(L, "gui", gui);
#else
	luaL_setfuncs(L, window_methods, 0);
	luaL_newlib(L, gui);
#endif

	lua_pushvalue(L, -1);  /* copy of module */
	lua_setglobal(L, "gui");
	return 1;
}
