#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class logger
{
public:
	static void init(const char* loggerName, spdlog::level::level_enum e_LogLevel = spdlog::level::trace);
	static std::shared_ptr<spdlog::logger>& get() { return s_logger; }
	static void exit_on_error(const char* msg);

	logger() = delete;
protected:
	static std::shared_ptr<spdlog::logger> s_logger;
private:
};
#ifdef ENGINE_DEBUG
	#define LOG_ERROR(...) logger::get()->error(__VA_ARGS__)
	#define LOG_WARN(...)  logger::get()->warn(__VA_ARGS__)
	#define LOG_INFO(...)  logger::get()->info(__VA_ARGS__)
	#define LOG_DEBUG(...) logger::get()->debug(__VA_ARGS__)
	#define EXIT_ON_ERROR(...) logger::exit_on_error(__VA_ARGS__)
#else
	#define LOG_ERROR(...) 
	#define LOG_WARN(...)  
	#define LOG_INFO(...)
	#define LOG_DEBUG(...)
	#define EXIT_ON_ERROR(...)
#endif // ENGINE_DEBUG



