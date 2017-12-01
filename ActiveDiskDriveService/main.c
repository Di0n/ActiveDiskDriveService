#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#define SVCNAME TEXT("ActiveDiskDriveService")
#define SVC_ERROR		((DWORD)0xC0020001L)

SERVICE_STATUS			gSvcStatus;
SERVICE_STATUS_HANDLE	gSvcStatusHandle;
HANDLE					gSvcStopEvent = NULL;

VOID ServiceInstall(void);
VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
VOID ServiceInit(DWORD, LPSTR *);

VOID ServiceReportEvent(LPSTR);
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
		{ SVCNAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		ServiceReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}
}

VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv)
{
	DWORD status = E_FAIL;

	gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, ServiceCtrlHandler);

	if (!gSvcStatusHandle)
	{
		ServiceReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	ServiceInit(argc, argv);

	HANDLE thread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

	WaitForSingleObject(thread, INFINITE);

	CloseHandle(gSvcStopEvent);

	ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

DWORD WINAPI ServiceWorkerThread(LPVOID lparam)
{
	int count = 0;
	while (WaitForSingleObject(gSvcStopEvent, 0) != WAIT_OBJECT_0)
	{
		if (count == 8)
		{
			FILE *f;
			errno_t error = fopen_s(&f, "D:\\Program Files (x86)\\ADDS\\temp.adds", "w+");
			if (error == 0)
			{
				count = 0;
			}
			else
				printf("Error opening file!\n");

			if (f)
				fclose(f);
		}
		Sleep(3000);
		count++;
	}
	return ERROR_SUCCESS;
}

VOID ServiceInit(DWORD argc, LPSTR *argv)
{
	gSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (gSvcStopEvent == NULL)
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

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

VOID WINAPI ServiceCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(gSvcStopEvent);
		ReportServiceStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

VOID ServiceReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
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
}

VOID ServiceInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName("", szPath, MAX_PATH))
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
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
		SVCNAME,                   // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service installed successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}