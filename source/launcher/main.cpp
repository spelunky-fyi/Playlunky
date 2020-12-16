#include <Windows.h>
#include <iostream>

#include <detours/detours.h>

#include <fmt/format.h>

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

int main(int argc, const char* argv[])
{
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);

	char dir_path[MAX_PATH] = {};
	GetCurrentDirectoryA(MAX_PATH, dir_path);

	char dll_path[MAX_PATH] = {};
	sprintf_s(dll_path, MAX_PATH, "%s/playlunky64.dll", dir_path);

	char cwd_path[MAX_PATH] = {};
	if (argc > 1)
	{
		strcpy_s(cwd_path, argv[1]);
	}
	else
	{
		strcpy_s(cwd_path, dir_path);
	}

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
	return -1;
}
