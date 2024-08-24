#include <string>
#include <windows.h>
#include "log.h"

Log::Log()
{
	char pFilename[MAX_PATH]{};
	GetModuleFileNameA(NULL, pFilename, sizeof(pFilename));
	std::string path(pFilename);
	path.erase(path.find_last_of("\\"));
	outf.open(path.append("\\log.txt"));
}

Log::~Log()
{
	SYSTEMTIME currentTime{};
	GetLocalTime(&currentTime);
	char tmp[32]{};
	sprintf_s(tmp, "[ %02d:%02d:%02d %02d.%02d.%d ]",
		currentTime.wHour, currentTime.wMinute, currentTime.wSecond,
		currentTime.wDay, currentTime.wMonth, currentTime.wYear);
	add(tmp);
	outf.close();
}

void Log::add(const char* txt)
{
	outf << txt << std::endl;
	outf.flush();
}

Log& log_instance()
{
	static Log _log;
	return _log;
}

void log_add(const char* s)
{
#ifdef LOG_ON
	if (s && *s)
		log_instance().add(s);
#endif
}