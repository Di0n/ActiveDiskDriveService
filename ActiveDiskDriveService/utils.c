#include "utils.h"

char* GetApplicationPath()
{
	HMODULE hModule = GetModuleHandle(NULL);
	char* path = malloc(MAX_PATH);
	memset(path, 0, MAX_PATH);
	DWORD result = GetModuleFileName(hModule, path, MAX_PATH - 1);
	DWORD lastError = GetLastError();

	if (result == 0)
	{
		free(path);
		path = 0;
	}
	else
	{
		char* p = *path;
		free(path);
		path = 0;
		return p;
	}
	return NULL;
	 // TODO strip exe from path
}

