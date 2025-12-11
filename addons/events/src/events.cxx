#include <vector>
#include <map>
#include <string>
#include <windows.h>
#include <memory>
#include "lua.hpp"

constexpr auto EVENTS_CLASS = "events_class";

template<typename... Args>
void lua_print(lua_State* L, const char* fmt, Args ...args)
{
	lua_getglobal(L, "print");
	lua_pushfstring(L, fmt, args...);
	lua_call(L, 1, 0);
}

void throw_error(lua_State* L, const char* msg)
{
	lua_pushstring(L, msg);
	lua_error(L);
}

#ifdef DEBUG
#define output(x) lua_print(L, x)
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
		const char* str = lua_tostring(L, -1);
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
#endif

namespace
{
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
}

class Event
{
private:
	bool m_break;
	bool m_removeThisCallback;
	int in_progress;
	std::string finger_print;
	std::string name;
	std::vector<int> callbacks;
	std::vector<int> to_remove;
	static lua_State* m_L;

public:
	explicit Event(const char* _name) :m_break(false), m_removeThisCallback(false),
		in_progress(0), finger_print(""), name(_name), callbacks{}, to_remove{}
	{
	};

	~Event()
	{
		for (int callbackRef : callbacks)
			luaL_unref(m_L, LUA_REGISTRYINDEX, callbackRef);
	}

	static void set_state(lua_State* L) { m_L = L; };

	// methods
	const char* get_name() const { return name.c_str(); }

	// register callback
	void reg(int func_ref) { callbacks.push_back(func_ref); }

	// register callback
	void insert_begin(int func_ref) { callbacks.emplace(callbacks.begin(), func_ref); }

	// unregister callback by index
	void unreg(int func_ref)
	{
		luaL_unref(m_L, LUA_REGISTRYINDEX, func_ref);
		callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), func_ref));
	}

	// unreg function from top stack
	void unreg()
	{
		for (int callbackRef : callbacks)
		{
			lua_rawgeti(m_L, LUA_REGISTRYINDEX, callbackRef);
			if (lua_compare(m_L, -1, -2, 0) == 1)
				return unreg(callbackRef);
			lua_pop(m_L, 1);
		}
	}

	// trigger
	bool trigger()
	{
		if (++in_progress > 1)
		{
			//dump_stack(m_L);
			int stack_size = lua_gettop(m_L);
			lua_print(m_L, "recursive call trigger for event '%s'", name.c_str());
			//for (int i = 1; i <= stack_size; ++i)
				//lua_print(m_L, "arg [%d] [%s]", i, luaL_tolstring(m_L, i, 0));
			luaL_traceback(m_L, m_L, "callback removed", 1);
			lua_print(m_L, lua_tostring(m_L, -1));
			lua_settop(m_L, stack_size);
			//dump_stack(m_L);
			m_removeThisCallback = true;
			--in_progress;
			return true;
		}
		bool result = false;
		int stack_size = lua_gettop(m_L);
		for (const int callbackRef : callbacks)
		{
			lua_rawgeti(m_L, LUA_REGISTRYINDEX, callbackRef);
			if (lua_isfunction(m_L, -1))
			{
				for (int i = stack_size; i; --i) lua_pushvalue(m_L, stack_size - i + 1);
				lua_pushcfunction(m_L, errorHandler);
				const int errorHandlerIndex = -(stack_size + 2);
				lua_insert(m_L, errorHandlerIndex);
				if (lua_pcall(m_L, stack_size, 1, errorHandlerIndex))
				{
					lua_print(m_L, lua_tostring(m_L, -1));
					lua_pop(m_L, 2); // pop errorHandler and error info
					m_removeThisCallback = true;
				}
				else
				{
					result = lua_toboolean(m_L, -1);
					if (!result) lua_pop(m_L, 2); // pop errorHandler and result
				}
				if (m_removeThisCallback)
				{
					to_remove.push_back(callbackRef);
					m_removeThisCallback = false;
				}
				if (m_break or result) { m_break = false; result = true; break; }
			}
			else
			{
				lua_pop(m_L, 1);
			}
		}
		for (int ref : to_remove) unreg(ref);
		to_remove.clear();
		finger_print = "";
		--in_progress;
		return result;
	};

	// stop
	void stop() { m_break = true; };

	// print info
	void print()
	{
		lua_print(m_L, "event '%s' with %d callbacks", name.c_str(), callbacks.size());
		if (finger_print.size())
			lua_print(m_L, "fp = %s", finger_print.c_str());
		lua_print(m_L, m_break ? "m_break:true" : "m_break:false");
		lua_print(m_L, m_removeThisCallback ? "m_removeThisCallback:true" : "m_removeThisCallback:false");
		lua_print(m_L, "in_progress = %d", in_progress);
	};

	// remove
	void removeThisCallback() { m_removeThisCallback = true; };

	// set fingerprint
	void set_fp(const char* fp_name) { finger_print = fp_name; };

	// clear
	void clear()
	{
		if (in_progress) m_break = true;
		to_remove.clear();
		for (int ref : callbacks) luaL_unref(m_L, LUA_REGISTRYINDEX, ref);
		callbacks.clear();
	};
};

lua_State* Event::m_L = nullptr;

class CEvtManager
{
public:
	static CEvtManager& Instance()
	{
		static CEvtManager Self;
		return Self;
	}

	~CEvtManager() { clear_all(); };

	Event* get(const char* name)
	{
		auto it = map_events.find(name);
		if (it == map_events.end())
		{
			//Event* pEvt = new Event(name);
			//map_events.emplace(name, pEvt);
			//return pEvt;
			//return map_events.emplace(name, new Event(name)).first->second;
			return map_events.emplace(name, std::make_unique<Event>(name)).first->second.get();
		}
		return it->second.get();
	}

	void clear_all() { map_events.clear(); }

private:
	CEvtManager() = default;
	std::map<std::string, std::unique_ptr<Event>> map_events;
};

namespace
{
	Event* event_arg(lua_State* L, int idx = 1)
	{
		Event* ev_object = static_cast<Event*>(luaL_checkudata(L, idx, EVENTS_CLASS));
		if (!ev_object) throw_error(L, "not a 'event' object");
		return ev_object;
	}

	int do_event(lua_State* L)
	{
		const char* name = luaL_checkstring(L, 1);
		if (!strlen(name)) throw_error(L, "empty name for event");
		lua_pushlightuserdata(L, CEvtManager::Instance().get(name));
		luaL_getmetatable(L, EVENTS_CLASS);
		lua_setmetatable(L, -2);
		return 1;
	}

	int do_register(lua_State* L)
	{
		luaL_checktype(L, 2, LUA_TFUNCTION);
		lua_pushvalue(L, 2);
		int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
		if (lua_toboolean(L, 3))
			event_arg(L)->insert_begin(ref_idx);
		else
			event_arg(L)->reg(ref_idx);
		lua_settop(L, 1);
		return 1;
	}

	int do_unregister(lua_State* L)
	{
		luaL_checktype(L, 2, LUA_TFUNCTION);
		event_arg(L)->unreg();
		lua_settop(L, 1);
		return 1;
	}

	int do_trigger(lua_State* L)
	{
		return event_arg(L)->trigger() ? 1 : 0;
	}

	int do_print(lua_State* L)
	{
		event_arg(L)->print();
		lua_settop(L, 1);
		return 1;
	}

	int do_stop(lua_State* L)
	{
		event_arg(L)->stop();
		lua_settop(L, 1);
		return 1;
	}

	int do_setRemove(lua_State* L)
	{
		event_arg(L)->removeThisCallback();
		lua_settop(L, 1);
		return 1;
	}

	int do_setfp(lua_State* L)
	{
		event_arg(L)->set_fp(luaL_checkstring(L, 2));
		lua_settop(L, 1);
		return 1;
	}

	int do_clear(lua_State* L)
	{
		event_arg(L)->clear();
		lua_settop(L, 1);
		return 1;
	}

	int do_name(lua_State* L)
	{
		lua_pushstring(L, event_arg(L)->get_name());
		return 1;
	}

	int do_tostring(lua_State* L)
	{
		lua_pushfstring(L, "event '%s'", event_arg(L)->get_name());
		return 1;
	}

	const struct luaL_Reg event_methods[] =
	{
		{"name",				do_name},
		{"register",			do_register},
		{"unregister",			do_unregister},
		{"trigger",				do_trigger},
		{"__call",				do_trigger},
		{"__tostring",			do_tostring},
		{"stop",				do_stop},
		{"setFingerprint",		do_setfp},
		{"removeThisCallback",	do_setRemove},
		{"clear",				do_clear},
		{"print",				do_print},
		{NULL, NULL}
	};
}

extern "C" __declspec(dllexport)
int luaopen_events(lua_State* L)
{
	Event::set_state(L);
	CEvtManager::Instance().clear_all();
	luaL_newmetatable(L, EVENTS_CLASS);  // create metatable for event objects
	lua_pushvalue(L, -1);  // copy metatable to top stack
	lua_setfield(L, -2, "__index");  // metatable.__index = metatable
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, event_methods);
#else
	luaL_setfuncs(L, event_methods, 0); // add methods
#endif
	lua_pushcfunction(L, do_event);
	lua_setglobal(L, "event");
	return 1;
}
