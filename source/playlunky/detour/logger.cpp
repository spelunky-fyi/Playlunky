#include "logger.h"
#include "log.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "imgui.h"
#include "sigfun.h"
#include "util/format.h"

#include <cstdio>
#include <fstream>
#include <memory>

struct DetourDoLog
{
    inline static SigScan::Function<void(__stdcall*)(std::ofstream*, const char*, void*, LogLevel)> Trampoline{
        .Signature = "\x48\x83\x80\x90\x01\x00\x00\x01\x48\x83\xbf\x88\x00\x00\x00\x00"_sig
    };
    static void Detour(std::ofstream* stream, const char* message, void* param_3, LogLevel log_level)
    {
        fmt::print("{}\n", message);
        std::fflush(stdout);

        if (stream != nullptr)
        {
            Trampoline(stream, message, param_3, log_level);
        }
    }

    static void Log(const char* message, LogLevel log_level)
    {
        const std::string adjusted_message = fmt::format("Playlunky :: {}", message);
        DetourDoLog::Detour(s_Stream, adjusted_message.c_str(), nullptr, log_level);
    }

    inline static std::ofstream* s_Stream{ nullptr };
};

struct DetourOpenLog
{
    inline static SigScan::Function<void*(__stdcall*)(void*, const char*, int)> Trampoline{
        .Signature = "\x48\xc7\x45\x00\xfe\xff\xff\xff\x48\x83\xb9\x80\x00\x00\x00\x00"_sig
    };
    static void* Detour(void* stream, const char* log_file, int mode)
    {
        stream = Trampoline(stream, log_file, mode);
        if (DetourDoLog::s_Stream == nullptr)
        {
            DetourDoLog::s_Stream = reinterpret_cast<std::ofstream*>(reinterpret_cast<size_t>(stream) - 0x8);
        }
        return stream;
    }
};

std::vector<DetourEntry> GetLogDetours()
{
    return {
        DetourHelper<DetourDoLog>::GetDetourEntry("DoLog"),
        DetourHelper<DetourOpenLog>::GetDetourEntry("ConstructLog")
    };
}

void Log(std::string message, LogLevel log_level)
{
    LogLevel spelunky_log_level = static_cast<int>(log_level) > static_cast<int>(LogLevel::Fatal) ? LogLevel::Info : log_level;
    DetourDoLog::Log(message.c_str(), spelunky_log_level);
    if (log_level == LogLevel::Error)
    {
        PrintError(std::move(message));
    }
    else if (log_level == LogLevel::InfoScreen)
    {
        PrintInfo(std::move(message));
    }
}
