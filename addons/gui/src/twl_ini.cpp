// TWL_INI.CPP
/////////////////////

#include "Twl.h"
#include "luabinder.h"
#include "utf.h"

class IniFile
{
private:
	static constexpr int BUFSZ = MAX_PATH;
	TCHAR m_file[BUFSZ];
	TCHAR m_section[BUFSZ];

public:
	explicit IniFile(const wchar_t* file, bool in_cwd = false);
	static const char* classname() { return "IniFile"; }
	void set_section(const wchar_t* section);
	void write_string(const wchar_t* key, const wchar_t* value);
	void remove_section(const wchar_t* sect_to_remove);
	void remove_key(const wchar_t* key);
	const std::wstring get_keys(const wchar_t* key);
	const std::wstring get_sections();
	const std::wstring read_string(const wchar_t* key, const wchar_t* def = nullptr);
	double read_number(const wchar_t* key, double def = 0.0);
	const TCHAR* get_path() const;
	const TCHAR* get_section() const;
};

IniFile::IniFile(const wchar_t* file, bool in_cwd)
{
	ZeroMemory(m_file, BUFSZ);
	if (!in_cwd)
		wcscpy_s(m_file, file);
	else
	{
		size_t nLen = GetModuleFileName(NULL, m_file, BUFSZ);
		while (nLen > 0 && m_file[nLen] != L'\\') m_file[nLen--] = 0;
		wcscat_s(m_file, file);
	}
	wcscpy_s(m_section, get_sections().c_str());
}

double IniFile::read_number(const wchar_t* key, double def)
{
	std::wstring s = read_string(key);
	return (s.size()) ? _wtof(s.c_str()) : def;
}

void IniFile::set_section(const wchar_t* section)
{
	wcscpy_s(m_section, section);
}

void IniFile::write_string(const wchar_t* key, const wchar_t* value)
{
	WritePrivateProfileString(m_section, key, value, m_file);
}

void IniFile::remove_section(const wchar_t* sect_to_remove)
{
	if (sect_to_remove && *sect_to_remove)
		WritePrivateProfileString(sect_to_remove, nullptr, nullptr, m_file);
	else
		WritePrivateProfileString(m_section, nullptr, nullptr, m_file);
}

void IniFile::remove_key(const wchar_t* key)
{
	WritePrivateProfileString(m_section, key, nullptr, m_file);
}

const std::wstring IniFile::get_keys(const wchar_t* key)
{
	const size_t buffsize = 1024;
	std::wstring tmp(buffsize, 0);
	GetPrivateProfileString(key, 0, 0, tmp.data(), buffsize, m_file);
	return tmp;
}

const std::wstring IniFile::get_sections()
{
	const size_t buffsize = 1024;
	std::wstring tmp(buffsize, 0);
	GetPrivateProfileSectionNames(tmp.data(), buffsize, m_file);
	return tmp;
}

const std::wstring IniFile::read_string(const wchar_t* key, const wchar_t* def)
{
	const size_t buffsize = 1024;
	std::wstring tmp(buffsize, 0);
	GetPrivateProfileString(m_section, key, def, tmp.data(), BUFSZ, m_file);
	return tmp;
}

const TCHAR* IniFile::get_path() const
{
	return m_file;
}

const TCHAR* IniFile::get_section() const
{
	return m_section;
}

//////////////////////////////////

#define check_inifile check_arg<IniFile>

// ini:set_section(sect)
int do_set_section(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const char* sect = luaL_checkstring(L, 2);
	pini->set_section(StringFromUTF8(sect).c_str());
	return 0;
}

// ini:write_string(key, val)
int do_write_string(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const char* key = luaL_checkstring(L, 2);
	const char* val = lua_tostring(L, 3);
	pini->write_string(StringFromUTF8(key).c_str(), StringFromUTF8(val).c_str());
	return 0;
}

// ini:read_string(key[, def_value=""])
int do_read_string(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const char* key = luaL_checkstring(L, 2);
	const char* def = luaL_optstring(L, 3, "");
	std::wstring str = pini->read_string(StringFromUTF8(key).c_str());
	lua_pushstring(L, str.size() ? UTF8FromString(str).c_str() : def);
	return 1;
}

// ini:read_number(key[, def_value=0])
int do_read_number(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const char* key = luaL_checkstring(L, 2);
	lua_Number def = luaL_optnumber(L, 3, 0.f);
	lua_Number res = pini->read_number(StringFromUTF8(key).c_str(), def);
	lua_pushnumber(L, res);
	if (res == lua_tointeger(L, -1)) lua_pushinteger(L, res);
	return 1;
}

// ini:remove_section([sect_to_remove=curr_section])
int do_remove_section(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const char* sect_to_remove = luaL_optstring(L, 2, nullptr);
	pini->remove_section(StringFromUTF8(sect_to_remove).c_str());
	return 0;
}

// ini:remove_key()
int do_remove_key(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const char* key = luaL_checkstring(L, 2);
	pini->remove_key(StringFromUTF8(key).c_str());
	return 0;
}

// ini:get_path()
int do_get_path(lua_State* L)
{
	const IniFile* pini = check_inifile(L);
	std::wstring str = pini->get_path();
	lua_pushstring(L, str.size() ? UTF8FromString(str).c_str() : "");
	return 1;
}

static void push_items(lua_State* L, const std::wstring& str)
{
	const wchar_t* pNextKey = str.data();
	int idx = 0;
	while (pNextKey && *pNextKey)
	{
		const std::wstring tmp(pNextKey);
		lua_pushinteger(L, ++idx);
		lua_pushstring(L, UTF8FromString(tmp).c_str());
		lua_settable(L, -3);
		pNextKey += tmp.size() + 1;
	}
}

// ini:get_keys()
int do_get_keys(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const char* key = luaL_optstring(L, 2, nullptr);
	const std::wstring str = pini->get_keys(key ? StringFromUTF8(key).c_str() : pini->get_section());
	lua_newtable(L);
	push_items(L, str);
	return 1;
}

// ini:get_sections()
int do_get_sections(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	const std::wstring str = pini->get_sections();
	lua_newtable(L);
	push_items(L, str);
	return 1;
}

// ini:get_section()
int do_get_section(lua_State* L)
{
	IniFile* pini = check_inifile(L);
	lua_pushstring(L, UTF8FromString(pini->get_section()).c_str());
	return 1;
}

const luaL_Reg LuaBinder<IniFile>::metamethods[] =
{
	{ "__gc",			do_destroy<IniFile> },
	{ NULL, NULL}
};

const luaL_Reg LuaBinder<IniFile>::methods[] =
{
	{ "set_section",	do_set_section		},
	{ "write_string",	do_write_string		},
	{ "write_number",	do_write_string		},
	{ "read_string",	do_read_string		},
	{ "read_number",	do_read_number		},
	{ "remove_section",	do_remove_section	},
	{ "remove_key",		do_remove_key		},
	{ "get_path",		do_get_path			},
	{ "get_keys",		do_get_keys			},
	{ "get_sections",	do_get_sections		},
	{ "get_section",	do_get_section		},
	{ NULL, NULL }
};

void lua_openclass_iniFile(lua_State* L)
{
	LuaBinder<IniFile>().createClass(L);
}

int new_inifile(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	bool in_cwd = lua_toboolean(L, 2);
	//lua_push_newobject(L, new IniFile(StringFromUTF8(path).c_str(), in_cwd));
	lua_push_newobject<IniFile>(L, StringFromUTF8(path).c_str(), in_cwd);
	return 1;
}