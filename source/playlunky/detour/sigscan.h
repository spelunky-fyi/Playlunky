#pragma once

namespace SigScan
{
	void* FindPattern(const char* module_name, const char* signature, bool code_only);
	void* FindPattern(const char* signature, bool code_only);
};