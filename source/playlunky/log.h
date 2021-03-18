#pragma once

#include "util/format.h"

enum class LogLevel {
	Info = 0,
	InfoScreen = 3,
	Fatal = 1,
	Error = 2
};
void Log(std::string message, LogLevel log_level);

template<class... Args>
void LogInfo(const char* format, Args&&... args) {
	std::string message = fmt::format(format, std::forward<Args>(args)...);
	Log(std::move(message), LogLevel::Info);
}
template<class... Args>
void LogInfoScreen(const char* format, Args&&... args) {
	std::string message = fmt::format(format, std::forward<Args>(args)...);
	Log(std::move(message), LogLevel::InfoScreen);
}
template<class... Args>
void LogError(const char* format, Args&&... args) {
	std::string message = fmt::format(format, std::forward<Args>(args)...);
	Log(std::move(message), LogLevel::Error);
}
template<class... Args>
void LogFatal(const char* format, Args&&... args) {
	std::string message = fmt::format(format, std::forward<Args>(args)...);
	Log(std::move(message), LogLevel::Fatal);
}
