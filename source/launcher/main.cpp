#include "util/format.h"

#include <Windows.h>
#include <fstream>

#include <detours.h>
#include <structopt/app.hpp>

enum ReturnReason
{
    SUCCESS,
    FAILED_SPAWNING_GAME_PROCESS,
    FAILED_PARSING_COMMAND_LINE,
    FAILED_CREATING_PIPES,
    FAILED_SETTING_READ_PIPE_PROPERTIES,
    FAILED_CREATING_CONSOLE,
    FAILED_DESTROYING_CONSOLE,
};

struct CommandLineOptions
{
    std::optional<std::string> exe_dir;
    std::optional<bool> console = false;
    std::optional<bool> overlunky = false;
};
VISITABLE_STRUCT(CommandLineOptions, exe_dir, console, overlunky);

static HANDLE s_Process = NULL;
static FILE* s_ConsoleStdOut = NULL;
static FILE* s_ConsoleStdErr = NULL;
static FILE* s_ConsoleStdIn = NULL;

bool CreateConsole()
{
    if (AllocConsole() == FALSE)
        return false;

    if (SetConsoleTitle("Playlunky") == FALSE)
        return false;

    if (freopen_s(&s_ConsoleStdOut, "CONOUT$", "w", stdout) != 0)
        return false;

    if (freopen_s(&s_ConsoleStdErr, "CONOUT$", "w", stderr) != 0)
        return false;

    if (freopen_s(&s_ConsoleStdIn, "CONIN$", "w", stdin) != 0)
        return false;

    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();

    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD console_mode;
    if (GetConsoleMode(console, &console_mode) == TRUE)
    {
        console_mode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS);
        SetConsoleMode(console, console_mode);
    }

    return true;
}

bool DestroyConsole()
{
    if (fclose(s_ConsoleStdOut) != 0)
        return false;
    if (fclose(s_ConsoleStdErr) != 0)
        return false;
    if (fclose(s_ConsoleStdIn) != 0)
        return false;
    if (FreeConsole() == FALSE)
        return false;
    return true;
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        TerminateProcess(s_Process, 0);
        CloseHandle(s_Process);
        break;
    }

    return TRUE;
}

int WinMain(
    [[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nShowCmd)
{
    try
    {
        auto options = structopt::app("playlunky_launcher").parse<CommandLineOptions>(__argc, __argv);

        const bool load_overlunky = options.overlunky.value_or(false);

        char dir_path[MAX_PATH] = {};
        GetCurrentDirectoryA(MAX_PATH, dir_path);

        char spel2_dll_path[MAX_PATH] = {};
        sprintf_s(spel2_dll_path, MAX_PATH, "%s/spel2.dll", dir_path);

        char playlunky_dll_path[MAX_PATH] = {};
        sprintf_s(playlunky_dll_path, MAX_PATH, "%s/playlunky64.dll", dir_path);

        if (!options.exe_dir.has_value())
        {
            options.exe_dir = dir_path;
        }
        const char* cwd_path = options.exe_dir.value().c_str();

        char overlunky_dll_path[MAX_PATH] = {};
        sprintf_s(overlunky_dll_path, MAX_PATH, "%s/Overlunky/Overlunky.dll", cwd_path);

        const char* dll_paths[] = {
            spel2_dll_path,
            playlunky_dll_path,
            overlunky_dll_path
        };

        char exe_path[MAX_PATH] = {};
        sprintf_s(exe_path, MAX_PATH, "%s/Spel2.exe", cwd_path);

        SECURITY_ATTRIBUTES sa{};
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;

        HANDLE out_read = 0;
        HANDLE out_write = 0;

        if (!CreatePipe(&out_read, &out_write, &sa, 0))
        {
            return FAILED_CREATING_PIPES;
        }

        if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0))
        {
            return FAILED_SETTING_READ_PIPE_PROPERTIES;
        }

        STARTUPINFOA si{};
        si.cb = sizeof(STARTUPINFO);
        si.hStdOutput = out_write;
        si.hStdError = out_write;
        si.dwFlags |= STARTF_USESTDHANDLES;

        const auto child_env = []()
        {
            std::string child_env = "SteamAppId=418530";

            const auto this_env = GetEnvironmentStrings();
            auto lpszVariable = this_env;
            while (*lpszVariable)
            {
                child_env += '\0';
                child_env += lpszVariable;
                lpszVariable += strlen(lpszVariable) + 1;
            }
            FreeEnvironmentStrings(this_env);

            child_env += '\0';
            return child_env;
        }();

        DWORD num_dlls = sizeof(dll_paths) / sizeof(const char*);
        if (!load_overlunky)
            num_dlls--;

        PROCESS_INFORMATION pi{};
        if (DetourCreateProcessWithDlls(NULL, exe_path, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, (LPVOID)child_env.c_str(), cwd_path, &si, &pi, num_dlls, dll_paths, NULL))
        {
            fmt::print("Spawned process: {}, PID: {}\n", exe_path, pi.dwProcessId);

            s_Process = pi.hProcess;

            CloseHandle(pi.hThread);
            CloseHandle(out_write);

            {
                if (!CreateConsole())
                {
                    return FAILED_CREATING_CONSOLE;
                }

                const bool use_console = options.console.value_or(false);
                if (use_console)
                {
                    SetConsoleCtrlHandler(HandlerRoutine, TRUE);
                }

                char buffer[1024 + 1] = {};
                DWORD read = 0;

                while (ReadFile(out_read, buffer, 1024, &read, NULL))
                {
                    buffer[read] = '\0';
                    fmt::print("{}", buffer);

                    if (!use_console)
                    {
                        if (std::string_view{ buffer }.find("All mods initialized...") != std::string_view::npos)
                        {
                            break;
                        }
                    }
                }

                CloseHandle(out_read);

                if (!DestroyConsole())
                {
                    return FAILED_DESTROYING_CONSOLE;
                }
            }

            return SUCCESS;
        }

        fmt::print("Failed to spawn process: {}, Error code: {}\n", exe_path, GetLastError());
        return FAILED_SPAWNING_GAME_PROCESS;
    }
    catch (structopt::exception& e)
    {
        fmt::print(e.what());
        fmt::print(e.help());
    }

    return FAILED_PARSING_COMMAND_LINE;
}
