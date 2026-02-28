#include <windows.h>
#include <string>
#include <fstream>
#include <format>

//#define GUILIB_LOG_ON
namespace
{
	class Log
	{
		std::ofstream outf;
		void add_time(const char* txt);

	public:
		Log();
		~Log();
		static Log& Instance();
		void add(const char* txt);
	};

	Log::Log()
	{
		std::string path(MAX_PATH, '\0');
		GetModuleFileNameA(NULL, path.data(), static_cast<DWORD>(path.size()));
		path.erase(path.find_last_of("\\"));
		outf.open(path.append("\\gui_log.txt"));
		add_time("log started:");
	};

	Log::~Log()
	{
		add_time("log closed:");
		outf.close();
	}

	void Log::add_time(const char* txt)
	{
		SYSTEMTIME currentTime{};
		GetLocalTime(&currentTime);
		const std::string tmp = std::format("{} {:02d}:{:02d}:{:02d} {:02d}.{:02d}.{} \n", txt,
			currentTime.wHour, currentTime.wMinute, currentTime.wSecond,
			currentTime.wDay, currentTime.wMonth, currentTime.wYear);
		outf << tmp << std::flush;
	}

	void Log::add(const char* txt)
	{
		outf << txt << std::endl;
		outf.flush();
	}

	Log& Log::Instance()
	{
		static Log _log;
		return _log;
	}
}

void log_add(const char* s)
{
#if defined(GUILIB_LOG_ON) || defined(_DEBUG)
	if (s && *s)
		Log::Instance().add(s);
#endif
}