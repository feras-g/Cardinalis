#include "logger.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

#include "imgui.h"

std::shared_ptr<spdlog::logger> logger::s_logger;

void logger::init(const char* name, spdlog::level::level_enum log_level)
{
	// https://github.com/gabime/spdlog#create-stdoutstderr-logger-object
	// https://github.com/gabime/spdlog/wiki/3.-Custom-formatting

	spdlog::set_pattern("%^[%T] %n: %v%$");

	// Create 
	s_logger = spdlog::stdout_color_mt(name);
	s_logger->set_level(log_level);
	s_logger->flush();
}

void logger::exit_on_error(const char* msg)
{
#if defined(_WIN32)
		MessageBoxA(nullptr, msg, "Error", MB_ICONERROR);
#else
		s_logger->error(msg);
#endif
		exit(EXIT_FAILURE);
}
