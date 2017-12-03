#include "inifile.h"

LPSTR ReadString(LPCSTR iniFile, LPCSTR section, LPCSTR key, LPCSTR defaultValue)
{
	char result[255];
	memset(result, 0x00, 255);
	GetPrivateProfileString(section, key, defaultValue, result, 255, iniFile);
	return result;
}

UINT ReadUint(LPCSTR iniFile, LPCSTR section, LPCSTR key, const INT defaultValue)
{
	return GetPrivateProfileInt(section, key, defaultValue, iniFile);
}