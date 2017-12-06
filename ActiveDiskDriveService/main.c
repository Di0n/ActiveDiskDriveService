#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "utils.h"

#define SVC_NAME			TEXT("ActiveDiskDriveService")
#define SVC_DISPLAY_NAME	TEXT("Active Disk Drive Service")
#define SVC_START_TYPE		SERVICE_DEMAND_START
#define SVC_ERROR			((DWORD)0xC0020001L)
#define SVC_DESCRIPTION		TEXT("Prevents the disk drive from  going to sleep.")

#define STATE_OBJECT		TEXT("so.adds")


SERVICE_STATUS			g_SvcStatus;
SERVICE_STATUS_HANDLE	g_SvcStatusHandle;
HANDLE					g_SvcStopEvent = NULL;


VOID ServiceInstall(void);
VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
VOID ServiceInit(DWORD, LPSTR *);

VOID ServiceReportEvent(LPCTSTR, const WORD, const DWORD);
VOID ReportServiceStatus(DWORD, DWORD, DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

void __cdecl _tmain(int argc, TCHAR *argv[])
{
	if (lstrcmpi(argv[1], TEXT("install")) == 0)
	{
		ServiceInstall();
		return;
	}

	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ SVC_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		ServiceReportEvent(TEXT("StartServiceCtrlDispatcher"), EVENTLOG_ERROR_TYPE, GetLastError());
	}
}

VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv)
{
	DWORD status = E_FAIL;

	g_SvcStatusHandle = RegisterServiceCtrlHandler(SVC_NAME, ServiceCtrlHandler);

	if (!g_SvcStatusHandle)
	{
		ServiceReportEvent(TEXT("RegisterServiceCtrlHandler"), EVENTLOG_ERROR_TYPE, GetLastError());
		return;
	}

	g_SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_SvcStatus.dwServiceSpecificExitCode = 0;

	ServiceInit(argc, argv);

	HANDLE thread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	WaitForSingleObject(thread, INFINITE);

	CloseHandle(thread);
	CloseHandle(g_SvcStopEvent);

	ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

DWORD WINAPI ServiceWorkerThread(LPVOID lparam)
{
	int count = 8;
	while (WaitForSingleObject(g_SvcStopEvent, 0) != WAIT_OBJECT_0)
	{
		Sleep(3000);
		if (((count+1) > 8 ? 8 : count++) == 8)
		{
			TCHAR appDir[MAX_PATH] = { 0 };
			GetApplicationDir(appDir, sizeof(appDir));
			TCHAR filePath[MAX_PATH];
			const int chars = snprintf(filePath, sizeof(filePath), "%s%s", appDir, STATE_OBJECT);

			if (chars < 0 || chars >= sizeof(filePath))
			{
				ServiceReportEvent(chars < 0 ? TEXT("Encoding error") : TEXT("String incomplete"), EVENTLOG_ERROR_TYPE, 0);
				continue;
			}

			FILE *file;
			const errno_t errorNumber = fopen_s(&file, filePath, "w+");

			if (errorNumber == 0) count = 0;

			else ServiceReportEvent(TEXT("Could not open state object file."), EVENTLOG_ERROR_TYPE, errorNumber);

			if (file) fclose(file);
		}
	}
	return ERROR_SUCCESS;
}

VOID ServiceInit(DWORD argc, LPSTR *argv)
{
	ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 1000);

	g_SvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (g_SvcStopEvent == NULL)
	{
		ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	ReportServiceStatus(SERVICE_RUNNING, NOERROR, 0);
}

VOID ReportServiceStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	g_SvcStatus.dwCurrentState = dwCurrentState;
	g_SvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	g_SvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		g_SvcStatus.dwControlsAccepted = 0;
	else g_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		g_SvcStatus.dwCheckPoint = 0;
	else g_SvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(g_SvcStatusHandle, &g_SvcStatus);
}

VOID WINAPI ServiceCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 3000);

		// Signal the service to stop.

		SetEvent(g_SvcStopEvent);
		ReportServiceStatus(g_SvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

VOID ServiceReportEvent(LPCTSTR msg, const WORD eventType, const DWORD errorCode)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[256];

	hEventSource = RegisterEventSource(NULL, SVC_NAME);

	if (hEventSource != NULL)
	{
		StringCchPrintf(Buffer, 256, TEXT("%s\nError code: %d"), msg, errorCode);
		
		lpszStrings[0] = SVC_NAME;
		lpszStrings[1] = Buffer;

		ReportEventA(hEventSource,        // event log handle
			eventType,				// event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}

/*
VOID ServiceReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];
	
	hEventSource = RegisterEventSource(NULL, SVC_NAME);
	
	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVC_NAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
} */
VOID ServiceInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH)) // NULL = ""
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Create the service

	schService = CreateService(
		schSCManager,				// SCM database 
		SVC_NAME,                   // name of service 
		SVC_DISPLAY_NAME,			// service name to display 
		SERVICE_ALL_ACCESS,			// desired access 
		SERVICE_WIN32_OWN_PROCESS,	// service type 
		SVC_START_TYPE,				// start type 
		SERVICE_ERROR_NORMAL,		// error control type 
		szPath,						// path to service's binary 
		NULL,						// no load ordering group 
		NULL,						// no tag identifier 
		NULL,						// no dependencies 
		NULL,						// LocalSystem account 
		NULL);						// no password 

	

	

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	else
	{
		SERVICE_DESCRIPTION description = { SVC_DESCRIPTION };
		ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &description);

		printf("Service installed successfully\n");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}