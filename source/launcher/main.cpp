#include <Windows.h>
#include <iostream>

#include <detours/detours.h>

#include <fmt/format.h>
#include <structopt/app.hpp>

struct CommandLineOptions {
	std::optional<std::string> exe_dir;
};
VISITABLE_STRUCT(CommandLineOptions, exe_dir);

static HANDLE s_Process = NULL;

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

int main(int argc, char* argv[])
{
	try {
		auto options = structopt::app("playlunky_launcher").parse<CommandLineOptions>(argc, argv);

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
			return -1;
		}

		if (!SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0)) {
			return -1;
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
				std::cout << buffer;
			}

			CloseHandle(out_read);
			return 0;
		}

		fmt::print("Failed to spawn process: {}, Error code: {}\n", exe_path, GetLastError());
	}
	catch (structopt::exception& e) {
		fmt::print(e.what());
		fmt::print(e.help());
	}

	return -1;
}
