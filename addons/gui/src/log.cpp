#include <windows.h>
#include <string>
#include <fstream>
#include <format>

//#define GUILIB_LOG_ON
#if defined(GUILIB_LOG_ON) || defined(_DEBUG)
namespace
{
	class Log
	{
		std::ofstream outf;
		void output(const char* txt);
		Log();
		~Log();

	public:
		void add(const char* txt);
	};

	Log::Log()
	{
		std::string path(MAX_PATH, '\0');
		GetModuleFileNameA(NULL, path.data(), static_cast<DWORD>(path.size()));
		path.erase(path.find_last_of("\\"));
		outf.open(path.append("\\gui_log.txt"));
		output("log started:");
	};

	Log::~Log()
	{
		output("log closed:");
		outf.close();
	}

	void Log::output(const char* txt)
	{
		SYSTEMTIME currentTime{};
		GetLocalTime(&currentTime);
		const std::string tmp = std::format("[{:02d}:{:02d}:{:02d} {:02d}.{:02d}.{}] {}",
			currentTime.wHour, currentTime.wMinute, currentTime.wSecond,
			currentTime.wDay, currentTime.wMonth, currentTime.wYear, txt);
		outf << tmp << std::endl;
	}

	void Log::add(const char* txt)
	{
		output(txt);
	}
}
#endif

void log_add([[maybe_unused]] const char* s)
{
#if defined(GUILIB_LOG_ON) || defined(_DEBUG)
	if (s && *s)
	{
		static Log _log;
		_log.add(s);
	}
#endif
}