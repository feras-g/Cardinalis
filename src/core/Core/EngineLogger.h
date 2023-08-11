#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger
{
public:
	static void Init(const char* loggerName, spdlog::level::level_enum e_LogLevel = spdlog::level::trace);
	static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }
	static void ExitOnError(const char* msg);

	Logger() = delete;
protected:
	static std::shared_ptr<spdlog::logger> s_Logger;
private:
};
#ifdef ENGINE_DEBUG
	#define LOG_ERROR(...) Logger::GetLogger()->error(__VA_ARGS__)
	#define LOG_WARN(...)  Logger::GetLogger()->warn(__VA_ARGS__)
	#define LOG_INFO(...)  Logger::GetLogger()->info(__VA_ARGS__)
	#define LOG_DEBUG(...) Logger::GetLogger()->debug(__VA_ARGS__)
	#define EXIT_ON_ERROR(...) Logger::ExitOnError(__VA_ARGS__)
#else
	#define LOG_ERROR(...) 
	#define LOG_WARN(...)  
	#define LOG_INFO(...)
	#define LOG_DEBUG(...)
	#define EXIT_ON_ERROR(...)
#endif // ENGINE_DEBUG



