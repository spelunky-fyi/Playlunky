#pragma once

#include "util/format.h"

enum class LogLevel
{
    Info = 0,
    InfoScreen = 3,
    Fatal = 1,
    Error = 2
};
void Log(std::string message, LogLevel log_level);

template<typename... Args>
const void LogInfo(fmt::v10::format_string<Args...> format, Args&&... args)
{
    std::string message = fmt::format(format, std::forward<Args>(args)...);
    Log(std::move(message), LogLevel::Info);
}
template<typename... Args>
const void LogInfoScreen(fmt::v10::format_string<Args...> format, Args&&... args)
{
    std::string message = fmt::format(format, std::forward<Args>(args)...);
    Log(std::move(message), LogLevel::InfoScreen);
}
template<typename... Args>
const void LogError(fmt::v10::format_string<Args...> format, Args&&... args)
{
    std::string message = fmt::format(format, std::forward<Args>(args)...);
    Log(std::move(message), LogLevel::Error);
}
template<typename... Args>
const void LogFatal(fmt::v10::format_string<Args...> format, Args&&... args)
{
    std::string message = fmt::format(format, std::forward<Args>(args)...);
    Log(std::move(message), LogLevel::Fatal);
}
