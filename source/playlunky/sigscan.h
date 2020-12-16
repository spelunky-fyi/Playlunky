#pragma once

namespace SigScan
{
	void* FindPattern(const char* module_name, const char* signature);
	void* FindPattern(const char* signature);
};