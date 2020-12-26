#include <Windows.h>

#include <detours.h>
#include <fmt/format.h>
#include <structopt/app.hpp>

enum ReturnReason {
	SUCCESS,
	FAILED_SPAWNING_GAME_PROCESS,
	FAILED_PARSING_COMMAND_LINE,
	FAILED_CREATING_PIPES,
	FAILED_SETTING_READ_PIPE_PROPERTIES,
	FAILED_CREATING_CONSOLE,
	FAILED_DESTROYING_CONSOLE,
};

struct CommandLineOptions {
	std::optional<std::string> exe_dir;
	std::optional<bool> console = false;
};
VISITABLE_STRUCT(CommandLineOptions, exe_dir, console);

static HANDLE s_Process = NULL;
static FILE* s_ConsoleStream = NULL;

bool CreateConsole() {
	if (AllocConsole() == FALSE)
		return false;

	if (SetConsoleTitle("Playlunky") == FALSE)
		return false;

	if (freopen_s(&s_ConsoleStream, "CONOUT$", "w+", stdout) != 0)
		return false;

	return true;
}

bool DestroyConsole() {
	if (fclose(s_ConsoleStream) != 0)
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
	[[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
	[[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nShowCmd)
{
	try {
		auto options = structopt::app("playlunky_launcher").parse<CommandLineOptions>(__argc, __argv);

		if (options.console.value_or(false)) {
			if (!CreateConsole()) {
				return FAILED_CREATING_CONSOLE;
			}
		}

		SetConsoleCtrlHandler(HandlerRoutine, TRUE);

		char dir_path[MAX_PATH] = {};
		GetCurrentDirectoryA(MAX_PATH, dir_path);

		char dll_path[MAX_PATH] = {};
		sprintf_s(dll_path, MAX_PATH, "%s/playlunky64.dll", dir_path);

		if (!options.exe_dir.has_value())
		{
			options.exe_dir = dir_path;
		}
		const char* cwd_path = options.exe_dir.value().c_str();

		char exe_path[MAX_PATH] = {};
		sprintf_s(exe_path, MAX_PATH, "%s/Spel2.exe", cwd_path);

		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;

		HANDLE out_read = 0;
		HANDLE out_write = 0;

		if (!CreatePipe(&out_read, &out_write, &sa, 0)) {
			return FAILED_CREATING_PIPES;
		}

		if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0)) {
			return FAILED_SETTING_READ_PIPE_PROPERTIES;
		}

		STARTUPINFOA si{};
		si.cb = sizeof(STARTUPINFO);
		si.hStdOutput = out_write;
		si.hStdError = out_write;
		si.dwFlags |= STARTF_USESTDHANDLES;

		PROCESS_INFORMATION pi{};
		if (DetourCreateProcessWithDllExA(NULL, exe_path, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, cwd_path, &si, &pi, dll_path, NULL))
		{
			fmt::print("Spawned process: {}, PID: {}\n", exe_path, pi.dwProcessId);

			s_Process = pi.hProcess;

			CloseHandle(pi.hThread);
			CloseHandle(out_write);

			char buffer[1024 + 1] = {};
			DWORD read = 0;

			while (ReadFile(out_read, buffer, 1024, &read, NULL))
			{
				buffer[read] = '\0';
				fmt::print(buffer);
			}

			CloseHandle(out_read);

			if (options.console.value_or(false)) {
				if (!DestroyConsole()) {
					return FAILED_DESTROYING_CONSOLE;
				}
			}

			return SUCCESS;
		}

		fmt::print("Failed to spawn process: {}, Error code: {}\n", exe_path, GetLastError());
		return FAILED_SPAWNING_GAME_PROCESS;
	}
	catch (structopt::exception& e) {
		fmt::print(e.what());
		fmt::print(e.help());
	}

	return FAILED_PARSING_COMMAND_LINE;
}
