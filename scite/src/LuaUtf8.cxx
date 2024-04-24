#include <errno.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <string>

#include <chrono>
#include "GUI.h"

extern "C" {
#ifdef _WIN32
	bool CFileExists(const char* filename)
	{
		FILE* fp = NULL;
		fp = fopen(filename, "r");
		if (fp != NULL)
		{
			fclose(fp);
			return true;
		}
		return false;
	}

	FILE* scite_lua_fopen(const char* filename, const char* mode) {
		GUI::gui_string sFilename = GUI::StringFromUTF8(filename);
		GUI::gui_string sMode = GUI::StringFromUTF8(mode);
		FILE* f = _wfopen(sFilename.c_str(), sMode.c_str());
		if (f == NULL)
			// Fallback on narrow string in case already in CP_ACP
			f = fopen(filename, mode);
		return f;
	}

	FILE* scite_lua_popen(const char* filename, const char* mode) {
		GUI::gui_string sFilename = GUI::StringFromUTF8(filename);
		GUI::gui_string sMode = GUI::StringFromUTF8(mode);
		FILE* f = _wpopen(sFilename.c_str(), sMode.c_str());
		if (f == NULL)
			// Fallback on narrow string in case already in CP_ACP
			f = _popen(filename, mode);
		return f;
	}

	int scite_lua_remove(const char* filename) {
		if (CFileExists(filename))
			return remove(filename);
		else {
			GUI::gui_string sFilename = GUI::StringFromUTF8(filename);
			return _wremove(sFilename.c_str());
		}
	}

	int scite_lua_rename(const char* oldfilename, const char* newfilename) {
		GUI::gui_string sOldFilename = GUI::StringFromUTF8(oldfilename);
		GUI::gui_string sNewFilename = GUI::StringFromUTF8(newfilename);
		int r = _wrename(sOldFilename.c_str(), sNewFilename.c_str());
		if (r != 0) // TODO: Check errno...
			r = rename(oldfilename, newfilename);
		return r;
	}

	int scite_lua_system(const char* command) {
		GUI::gui_string sCommand = GUI::StringFromUTF8(command);
		return _wsystem(sCommand.c_str());
	}
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
// ============ SYSTEM ==================================
static int impl_scite_lua_remove(lua_State* L) {
	const char* fn = luaL_checkstring(L, 1);
	lua_pushinteger(L, scite_lua_remove(fn));
	return 1;
}

static int impl_scite_lua_rename(lua_State* L) {
	const char* old_fn = luaL_checkstring(L, 1);
	const char* new_fn = luaL_checkstring(L, 2);
	lua_pushinteger(L, scite_lua_rename(old_fn, new_fn));
	return 1;
}

// ============ STRING ==================================
static int lua_string_from_utf8(lua_State* L) {
	if (lua_gettop(L) != 2) luaL_error(L, "Wrong arguments count for string.from_utf8");
	const char* s = luaL_checkstring(L, 1);
	int cp = 0;
	if (!lua_isnumber(L, 2))
		cp = GUI::CodePageFromName(lua_tostring(L, 2));
	else
		cp =  static_cast<int>(lua_tointeger(L, 2));
	std::string ss = GUI::ConvertFromUTF8(s, cp);
	lua_pushstring(L, ss.c_str());
	return 1;
}

static int lua_string_to_utf8(lua_State* L) {
	if (lua_gettop(L) != 2) luaL_error(L, "Wrong arguments count for string.to_utf8");
	const char* s = luaL_checkstring(L, 1);
	int cp = 0;
	if (!lua_isnumber(L, 2))
		cp = GUI::CodePageFromName(lua_tostring(L, 2));
	else
		cp = static_cast<int>(lua_tointeger(L, 2));
	std::string ss = GUI::ConvertToUTF8(s, cp);
	lua_pushstring(L, ss.c_str());
	return 1;
}

static int lua_string_utf8_to_upper(lua_State* L) {
	const char* s = luaL_checkstring(L, 1);
	//std::string ss = GUI::UTF8ToUpper(s);
	std::string ss = GUI::UpperCaseUTF8(s);
	lua_pushstring(L, ss.c_str());
	return 1;
}

static int lua_string_utf8_to_lower(lua_State* L) {
	const char* s = luaL_checkstring(L, 1);
	//std::string ss = GUI::UTF8ToLower(s);
	std::string ss = GUI::LowerCaseUTF8(s);
	lua_pushstring(L, ss.c_str());
	return 1;
}

static int lua_string_utf8len(lua_State* L) {
	const char* str = luaL_checkstring(L, 1);
	GUI::gui_string wstr = GUI::StringFromUTF8(str);
	lua_pushinteger(L, wstr.length());
	return 1;
}

// --------------------------------------------------------------
#ifdef _WIN32
//#define lprint(X) lua_getglobal(L,"print"); lua_pushstring(L, X); lua_call(L,1,0);

#if LUA_VERSION_NUM < 502
#define LUA_POF         "luaopen_"
#define LUA_OFSEP       "_"
#define POF             LUA_POF
#define LIB_FAIL        "open"
#define ERRLIB          1
#define ERRFUNC         2
#define LIBPREFIX       "LOADLIB: "
#include "windows.h"

static void pusherror(lua_State* L) {
	int error = GetLastError();
	char buffer[128];
	if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, error, 0, buffer, sizeof(buffer), NULL))
		lua_pushstring(L, buffer);
	else
		lua_pushfstring(L, "system error %d\n", error);
}

static lua_CFunction ll_sym(lua_State* L, void* lib, const char* sym) {
	lua_CFunction f = (lua_CFunction)GetProcAddress((HINSTANCE)lib, sym);
	if (f == NULL) pusherror(L);
	return f;
}

static void* ll_load(lua_State* L, const char* path) {
	HINSTANCE lib = LoadLibraryW(GUI::StringFromUTF8(path).c_str());
	if (lib == NULL) pusherror(L);
	return lib;
}

static void** ll_register(lua_State* L, const char* path) {
	void** plib;
	lua_pushfstring(L, "%s%s", LIBPREFIX, path);
	lua_gettable(L, LUA_REGISTRYINDEX);  /* check library in registry? */
	if (!lua_isnil(L, -1))  /* is there an entry? */
		plib = (void**)lua_touserdata(L, -1);
	else {  /* no entry yet; create one */
		lua_pop(L, 1);
		plib = (void**)lua_newuserdata(L, sizeof(const void*));
		*plib = NULL;
		luaL_getmetatable(L, "_LOADLIB");
		lua_setmetatable(L, -2);
		lua_pushfstring(L, "%s%s", LIBPREFIX, path);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	}
	return plib;
}

static int ll_loadfunc(lua_State* L, const char* path, const char* sym) {
	void** reg = ll_register(L, path);
	if (*reg == NULL) *reg = ll_load(L, path);
	if (*reg == NULL)
		return ERRLIB;  /* unable to load library */
	else {
		lua_CFunction f = ll_sym(L, *reg, sym);
		if (f == NULL)
			return ERRFUNC;  /* unable to find function */
		lua_pushcfunction(L, f);
		return 0;  /* return function */
	}
}

static int readable(const char* filename) {
	FILE* f = _wfopen(GUI::StringFromUTF8(filename).c_str(), GUI_TEXT("r"));  /* try to open file */
	if (f == NULL) return 0;  /* open failed */
	fclose(f);
	return 1;
}

static const char* pushnexttemplate(lua_State* L, const char* path) {
	const char* l;
	while (*path == *LUA_PATHSEP) path++;  /* skip separators */
	if (*path == '\0') return NULL;  /* no more templates */
	l = strchr(path, *LUA_PATHSEP);  /* find next separator */
	if (l == NULL) l = path + strlen(path);
	lua_pushlstring(L, path, l - path);  /* template */
	return l;
}

static const char* findfile(lua_State* L, const char* name,
	const char* pname) {
	const char* path;
	name = luaL_gsub(L, name, ".", LUA_DIRSEP);
	lua_getglobal(L, "package"); //lua_getfield(L, LUA_GLOBALSINDEX, "package");
	lua_getfield(L, -1, pname);
	lua_remove(L, -2);
	//lua_getfield(L, LUA_ENVIRONINDEX, pname);
	path = lua_tostring(L, -1);
	if (path == NULL)
		luaL_error(L, "'package.%s' must be a string", pname);
	lua_pushliteral(L, "");  /* error accumulator */
	while ((path = pushnexttemplate(L, path)) != NULL) {
		const char* filename;
		filename = luaL_gsub(L, lua_tostring(L, -1), LUA_PATH_MARK, name);
		lua_remove(L, -2);  /* remove path template */
		if (readable(filename))  /* does file exist and is readable? */
			return filename;  /* return that file name */
		lua_pushfstring(L, "\n\tno file '%s'", filename);
		lua_remove(L, -2);  /* remove file name */
		lua_concat(L, 2);  /* add entry to possible error message */
	}
	return NULL;  /* not found */
}

static void loaderror(lua_State* L, const char* filename) {
	luaL_error(L, "error loading module " "'%s'" " from file " "'%s'" ":\n\t%s",
		lua_tostring(L, 1), filename, lua_tostring(L, -1));
}

static const char* mkfuncname(lua_State* L, const char* modname) {
	const char* funcname;
	const char* mark = strchr(modname, '-');
	if (mark) modname = mark + 1;
	funcname = luaL_gsub(L, modname, ".", LUA_OFSEP);
	funcname = lua_pushfstring(L, POF"%s", funcname);
	lua_remove(L, -2);  /* remove 'gsub' result */
	return funcname;
}

static int utf8_loader_Lua(lua_State* L) {
	const char* filename;
	const char* name = luaL_checkstring(L, 1);
	filename = findfile(L, name, "path");
	if (filename == NULL) return 1;  /* library not found in this path */
	if (luaL_loadfile(L, filename) != 0)
		loaderror(L, filename);
	return 1;  /* library loaded successfully */
}

static int utf8_loader_C(lua_State* L) {
	const char* funcname;
	const char* name = luaL_checkstring(L, 1);
	const char* filename = findfile(L, name, "cpath");
	if (filename == NULL) return 1;  /* library not found in this path */
	funcname = mkfuncname(L, name);
	if (ll_loadfunc(L, filename, funcname) != 0)
		loaderror(L, filename);
	return 1;  /* library loaded successfully */
}

static int utf8_loader_Croot(lua_State* L) {
	const char* funcname;
	const char* filename;
	const char* name = luaL_checkstring(L, 1);
	const char* p = strchr(name, '.');
	int stat;
	if (p == NULL) return 0;  /* is root */
	lua_pushlstring(L, name, p - name);
	filename = findfile(L, lua_tostring(L, -1), "cpath");
	if (filename == NULL) return 1;  /* root not found */
	funcname = mkfuncname(L, name);
	if ((stat = ll_loadfunc(L, filename, funcname)) != 0) {
		if (stat != ERRFUNC) loaderror(L, filename);  /* real error */
		lua_pushfstring(L, "\n\tno module " "'%s'" " in file " "'%s'",
			name, filename);
		return 1;  /* function not found */
	}
	return 1;
}
#else // LUA502

#define LUA_CSUBSEP		LUA_DIRSEP
#define LUA_LSUBSEP		LUA_DIRSEP
#define l_likely(x)		luai_likely(x)
#define l_unlikely(x)	luai_unlikely(x)
/* error codes for 'lookforfunc' */
#define ERRLIB		1
#define ERRFUNC		2
#define LUA_OFSEP	"_"
#define LUA_POF		"luaopen_"
#define LUA_IGMARK	"-"
#define LUA_LLE_FLAGS	0
typedef void (*voidf)(void);
#include <windows.h>

static const char* const CLIBS = "_CLIBS";

static void pusherrornotfound(lua_State* L, const char* path) {
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	luaL_addstring(&b, "no file '");
	luaL_addgsub(&b, path, LUA_PATH_SEP, "'\n\tno file '");
	luaL_addstring(&b, "'");
	luaL_pushresult(&b);
}

static const char* getnextfilename(char** path, char* end) {
	char* sep;
	char* name = *path;
	if (name == end)
		return NULL;  /* no more names */
	else if (*name == '\0') {  /* from previous iteration? */
		*name = *LUA_PATH_SEP;  /* restore separator */
		name++;  /* skip it */
	}
	sep = strchr(name, *LUA_PATH_SEP);  /* find next separator */
	if (sep == NULL)  /* separator not found? */
		sep = end;  /* name goes until the end */
	*sep = '\0';  /* finish file name */
	*path = sep;  /* will start next search from here */
	return name;
}

static int readable(const char* filename) {
	FILE* f = _wfopen(GUI::StringFromUTF8(filename).c_str(), GUI_TEXT("r"));  /* try to open file */
	if (f == NULL) return 0;  /* open failed */
	fclose(f);
	return 1;
}

static const char* searchpath(lua_State* L, const char* name,
	const char* path,
	const char* sep,
	const char* dirsep) {
	luaL_Buffer buff;
	char* pathname;  /* path with name inserted */
	char* endpathname;  /* its end */
	const char* filename;
	/* separator is non-empty and appears in 'name'? */
	if (*sep != '\0' && strchr(name, *sep) != NULL)
		name = luaL_gsub(L, name, sep, dirsep);  /* replace it by 'dirsep' */
	luaL_buffinit(L, &buff);
	/* add path to the buffer, replacing marks ('?') with the file name */
	luaL_addgsub(&buff, path, LUA_PATH_MARK, name);
	luaL_addchar(&buff, '\0');
	pathname = luaL_buffaddr(&buff);  /* writable list of file names */
	endpathname = pathname + luaL_bufflen(&buff) - 1;
	while ((filename = getnextfilename(&pathname, endpathname)) != NULL) {
		if (readable(filename))  /* does file exist and is readable? */
			return lua_pushstring(L, filename);  /* save and return name */
	}
	luaL_pushresult(&buff);  /* push path to create error message */
	pusherrornotfound(L, lua_tostring(L, -1));  /* create error message */
	return NULL;  /* not found */
}

static int checkload(lua_State* L, int stat, const char* filename) {
	if (l_likely(stat)) {  /* module loaded successfully? */
		lua_pushstring(L, filename);  /* will be 2nd argument to module */
		return 2;  /* return open function and file name */
	}
	else
		return luaL_error(L, "error loading module '%s' from file '%s':\n\t%s",
			lua_tostring(L, 1), filename, lua_tostring(L, -1));
}

static const char* findfile(lua_State* L, const char* name,
	const char* pname,
	const char* dirsep) {
	const char* path;
	lua_getfield(L, lua_upvalueindex(1), pname);
	path = lua_tostring(L, -1);
	if (l_unlikely(path == NULL))
		luaL_error(L, "'package.%s' must be a string", pname);
	return searchpath(L, name, path, ".", dirsep);
}

static int utf8_searcher_Lua(lua_State* L)
{
	const char* filename;
	const char* name = luaL_checkstring(L, 1);
	filename = findfile(L, name, "path", LUA_LSUBSEP);
	if (filename == NULL) return 1;  /* module not found in this path */
	std::string fn = GUI::ConvertFromUTF8(filename, 0);
	return checkload(L, (luaL_loadfile(L, fn.c_str()) == LUA_OK), filename);
	//return checkload(L, (luaL_loadfile(L, filename) == LUA_OK), filename);
}

static void* checkclib(lua_State* L, const char* path) {
	void* plib;
	lua_getfield(L, LUA_REGISTRYINDEX, CLIBS);
	lua_getfield(L, -1, path);
	plib = lua_touserdata(L, -1);  /* plib = CLIBS[path] */
	lua_pop(L, 2);  /* pop CLIBS table and 'plib' */
	return plib;
}

static void pusherror(lua_State* L) {
	int error = GetLastError();
	char buffer[128];
	if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, error, 0, buffer, sizeof(buffer) / sizeof(char), NULL))
		lua_pushstring(L, buffer);
	else
		lua_pushfstring(L, "system error %d\n", error);
}

static void* lsys_load(lua_State* L, const char* path, int seeglb) {
	HMODULE lib = LoadLibraryExW(GUI::StringFromUTF8(path).c_str(), NULL, LUA_LLE_FLAGS);
	(void)(seeglb);  /* not used: symbols are 'global' by default */
	if (lib == NULL) pusherror(L);
	return lib;
}

static void addtoclib(lua_State* L, const char* path, void* plib) {
	lua_getfield(L, LUA_REGISTRYINDEX, CLIBS);
	lua_pushlightuserdata(L, plib);
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, path);  /* CLIBS[path] = plib */
	lua_rawseti(L, -2, luaL_len(L, -2) + 1);  /* CLIBS[#CLIBS + 1] = plib */
	lua_pop(L, 1);  /* pop CLIBS table */
}

static lua_CFunction lsys_sym(lua_State* L, void* lib, const char* sym) {
	lua_CFunction f = (lua_CFunction)(voidf)GetProcAddress((HMODULE)lib, sym);
	if (f == NULL) pusherror(L);
	return f;
}

static int lookforfunc(lua_State* L, const char* path, const char* sym) {
	void* reg = checkclib(L, path);  /* check loaded C libraries */
	if (reg == NULL) {  /* must load library? */
		reg = lsys_load(L, path, *sym == '*');  /* global symbols if 'sym'=='*' */
		if (reg == NULL) return ERRLIB;  /* unable to load library */
		addtoclib(L, path, reg);
	}
	if (*sym == '*') {  /* loading only library (no function)? */
		lua_pushboolean(L, 1);  /* return 'true' */
		return 0;  /* no errors */
	}
	else {
		lua_CFunction f = lsys_sym(L, reg, sym);
		if (f == NULL)
			return ERRFUNC;  /* unable to find function */
		lua_pushcfunction(L, f);  /* else create new function */
		return 0;  /* no errors */
	}
}

static int loadfunc(lua_State* L, const char* filename, const char* modname) {
	const char* openfunc;
	const char* mark;
	modname = luaL_gsub(L, modname, ".", LUA_OFSEP);
	mark = strchr(modname, *LUA_IGMARK);
	if (mark) {
		int stat;
		openfunc = lua_pushlstring(L, modname, mark - modname);
		openfunc = lua_pushfstring(L, LUA_POF"%s", openfunc);
		stat = lookforfunc(L, filename, openfunc);
		if (stat != ERRFUNC) return stat;
		modname = mark + 1;  /* else go ahead and try old-style name */
	}
	openfunc = lua_pushfstring(L, LUA_POF"%s", modname);
	return lookforfunc(L, filename, openfunc);
}

static int utf8_searcher_C(lua_State* L) 
{
	const char* name = luaL_checkstring(L, 1);
	const char* filename = findfile(L, name, "cpath", LUA_CSUBSEP);
	if (filename == NULL) return 1;  /* module not found in this path */
	return checkload(L, (loadfunc(L, filename, name) == 0), filename);
}

static int utf8_searcher_Croot(lua_State* L) 
{
	const char* filename;
	const char* name = luaL_checkstring(L, 1);
	const char* p = strchr(name, '.');
	int stat;
	if (p == NULL) return 0;  /* is root */
	lua_pushlstring(L, name, p - name);
	filename = findfile(L, lua_tostring(L, -1), "cpath", LUA_CSUBSEP);
	if (filename == NULL) return 1;  /* root not found */
	if ((stat = loadfunc(L, filename, name)) != 0) {
		if (stat != ERRFUNC)
			return checkload(L, 0, filename);  /* real error */
		else {  /* open function not found */
			lua_pushfstring(L, "no module '%s' in file '%s'", name, filename);
			return 1;
		}
	}
	lua_pushstring(L, filename);  /* will be 2nd argument to module */
	return 2;
}
#endif // LUA502
// ==============================================================
#endif // WIN32

// конвертируем 1-й аргумент dofile и loadstring
int cf_global_dofile(lua_State* L)
{
	const char* name = luaL_checkstring(L, 1);
	std::string ss = GUI::ConvertFromUTF8(name, 1251);
	lua_remove(L, 1);
	lua_pushstring(L, ss.c_str());
	lua_insert(L, 1);
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_insert(L, 1);
	lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
	return lua_gettop(L);
}

void lua_utf8_register_libs(lua_State* L) {

	// register sting functions
	const luaL_Reg utf8_string_funcs[] = {
		{"to_utf8", lua_string_to_utf8},
		{"from_utf8", lua_string_from_utf8},
		{"utf8upper", lua_string_utf8_to_upper},
		{"utf8lower", lua_string_utf8_to_lower},
		{"utf8len", lua_string_utf8len},
		{NULL, NULL}
	};	
#if LUA_VERSION_NUM < 502
	luaL_register(L, LUA_STRLIBNAME, utf8_string_funcs);
#else
	lua_getglobal(L, LUA_STRLIBNAME);
	luaL_setfuncs(L, utf8_string_funcs, 0);
#endif
	lua_pop(L, 1);

	lua_getglobal(L, "dofile");
	lua_pushcclosure(L, cf_global_dofile, 1);
	lua_setglobal(L, "dofile");

	lua_getglobal(L, "loadfile");
	lua_pushcclosure(L, cf_global_dofile, 1);
	lua_setglobal(L, "loadfile");

	lua_pushcfunction(L, impl_scite_lua_remove);
	lua_setglobal(L, "fs_remove");

	lua_pushcfunction(L, impl_scite_lua_rename);
	lua_setglobal(L, "fs_rename");

#ifdef _WIN32

	// register loaders
#if LUA_VERSION_NUM < 502
	// ************ REGISTER OUR UTF-8 LOADERS ****************************
	const lua_CFunction utf8_loaders[] =
	{ utf8_loader_Lua, utf8_loader_C, utf8_loader_Croot, NULL };
	lua_getglobal(L, "package"); //lua_getfield(L, LUA_GLOBALSINDEX, "package");
	lua_getfield(L, -1, "loaders");
	lua_remove(L, -2);
	int i = lua_objlen(L, -1);
	for (int k = 0; utf8_loaders[k] != NULL; k++) {
		lua_rawgeti(L, -1, k + 2);
		lua_rawseti(L, -2, i + k + 1);
		lua_pushcfunction(L, utf8_loaders[k]);
		lua_rawseti(L, -2, k + 2);
	}
	lua_pop(L, 1);
	// ====================================================================
#else
	const lua_CFunction utf8_searchers[] =
	{ utf8_searcher_Lua, utf8_searcher_C, utf8_searcher_Croot, NULL };
	lua_getglobal(L, "package"); //lua_getfield(L, LUA_GLOBALSINDEX, "package");
	lua_getfield(L, -1, "searchers");
	lua_Unsigned i = lua_rawlen(L, -1); // length of package.searchers (is 4)
	//stack: 'package','package.searchers'
	for (lua_Integer k = 0; utf8_searchers[k] != NULL; k++) {
		lua_rawgeti(L, -1, k + 1); // get package.searchers[k+1] (k+1 from 1 to 3)
		lua_rawseti(L, -2, i + k + 1); // set package.searchers[i+k+1] (k+1 from 4 to 7)

		lua_pushvalue(L, -2);  /* set 'package' as upvalue for all searchers */
		lua_pushcclosure(L, utf8_searchers[k], 1);
		lua_rawseti(L, -2, k + 1);
	}
	lua_pop(L, 1); // pop 'package.searchers'
	lua_pop(L, 1); // pop 'package'
#endif //version num
#endif // win32
}