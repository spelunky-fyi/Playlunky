#pragma once

#include <fmt/format.h>

enum class LogLevel {
	Info = 0,
	Fatal = 1
};
void Log(const char* message, LogLevel log_level);

template<class... Args>
void LogInfo(const char* format, Args&&... args) {
	const std::string message = fmt::format(format, std::forward<Args>(args)...);
	Log(message.c_str(), LogLevel::Info);
}
template<class... Args>
void LogFatal(const char* format, Args&&... args) {
	const std::string message = fmt::format(format, std::forward<Args>(args)...);
	Log(message.c_str(), LogLevel::Fatal);
}
