#include "utils.h"

TCHAR GetApplicationPath()
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

void GetApplicationDir(char* buffer, size_t size)
{
	TCHAR path[MAX_PATH];
	if (!GetModuleFileName(NULL, path, MAX_PATH)) 
		buffer = NULL;
	else
		for (size_t i = strlen(path); i > 0; i--)
			if (path[i] == '\\')
			{
				path[i + 1] = 0;
				memcpy(buffer, path, strlen(path));
				break;
			}
}

