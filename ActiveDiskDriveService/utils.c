#include "utils.h"

char* GetApplicationPath()
{
	HMODULE hModule = GetModuleHandle(NULL);
	char* path = malloc(MAX_PATH);
	memset(path, 0, MAX_PATH);
	DWORD result = GetModuleFileName(hModule, path, MAX_PATH);
	size_t size = strlen(path);
	if (result == 0)
	{
		free(path);
		path = NULL;
	}
	else
	{
		for (size_t i = strlen(path); i > 0; i--)
		{
			if (path[i] == '\\')
			{
				path[i] = 0;
				break;
			}
		}
	}
	return path;
}

