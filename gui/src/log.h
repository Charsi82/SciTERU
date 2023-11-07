#pragma once
#include <fstream>

//#define LOG_ON
class Log
{
	std::ofstream outf;
public:
	Log();
	~Log();
	void add(const char* txt);
};

Log& log_instance();

template<typename... Args>
void log_add(const char* s, Args ...args)
{
#ifdef LOG_ON
	if (s && *s)
	{
		constexpr size_t bufsize = 512;
		char buf[bufsize]{};
		sprintf_s(buf, bufsize, s, args...);
		log_instance().add(buf);
	}
#endif
}

void log_add(const char* s);