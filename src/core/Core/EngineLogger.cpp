#include "EngineLogger.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

std::shared_ptr<spdlog::logger> Logger::s_Logger;

void Logger::Init(const char* loggerName, spdlog::level::level_enum e_LogLevel)
{
	// https://github.com/gabime/spdlog#create-stdoutstderr-logger-object
	// https://github.com/gabime/spdlog/wiki/3.-Custom-formatting

	spdlog::set_pattern("%^[%T] %n: %v%$");

	// Create 
	s_Logger = spdlog::stdout_color_mt(loggerName);
	s_Logger->set_level(e_LogLevel);
	s_Logger->flush();
}

void Logger::ExitOnError(const char* msg)
{
#if defined(_WIN32)
		MessageBoxA(nullptr, msg, "Error", MB_ICONERROR);
#else
		s_Logger->error(msg);
#endif
		exit(EXIT_FAILURE);
}
