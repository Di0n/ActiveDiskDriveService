#include "inifile.h"

LPSTR ReadString(LPCSTR iniFile, LPCSTR section, LPCSTR key, LPCSTR defaultValue)
{
	const size_t bufferSize = 256;
	char* result = malloc(bufferSize);
	memset(result, 0, bufferSize);
	GetPrivateProfileString(section, key, defaultValue, result, (DWORD)bufferSize, iniFile);
	return result;
}

UINT ReadUint(LPCSTR iniFile, LPCSTR section, LPCSTR key, const INT defaultValue)
{
	return GetPrivateProfileInt(section, key, defaultValue, iniFile);
}