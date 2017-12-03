#ifndef __INIFILE__
#define __INIFILE__

#include <Windows.h>

LPSTR ReadString(LPCSTR iniFile, LPCSTR section, LPCSTR key, LPCSTR defaultValue);
UINT ReadUint(LPCSTR iniFile, LPCSTR section, LPCSTR key, const INT defaultValue);

#endif // !__INIFILE__
