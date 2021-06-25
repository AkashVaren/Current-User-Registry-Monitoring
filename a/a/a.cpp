#include <Windows.h>
#include <tchar.h>
#include<fstream>
#include<iostream>
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <ctime>
#define INFO_BUFFER_SIZE	260
using namespace std;
ofstream MyFile("C:\\Users\\akash\\Desktop\\file as DB\\filename.txt");
SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  _T("My Sample Service")

int _tmain(int argc, TCHAR* argv[])
{
    OutputDebugString(_T("My Sample Service: Main: Entry"));

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {(LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: Main: StartServiceCtrlDispatcher returned error"));
        return GetLastError();
    }

    OutputDebugString(_T("My Sample Service: Main: Exit"));
    return 0;
}


VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;

    OutputDebugString(_T("My Sample Service: ServiceMain: Entry"));

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    HANDLE hThread;
    if (g_StatusHandle == NULL)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: RegisterServiceCtrlHandler returned error"));
        goto EXIT;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    /*
     * Perform tasks neccesary to start the service here
     */
    OutputDebugString(_T("My Sample Service: ServiceMain: Performing Service Start Operations"));

    // Create stop event to wait on later.
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
        }
        goto EXIT;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    // Start the thread that will perform the main task of the service
    hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    OutputDebugString(_T("My Sample Service: ServiceMain: Waiting for Worker Thread to complete"));

    // Wait until our worker thread exits effectively signaling that the service needs to stop
    WaitForSingleObject(hThread, INFINITE);

    OutputDebugString(_T("My Sample Service: ServiceMain: Worker Thread Stop Event signaled"));


    /*
     * Perform any cleanup tasks
     */
    OutputDebugString(_T("My Sample Service: ServiceMain: Performing Cleanup Operations"));

    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

EXIT:
    OutputDebugString(_T("My Sample Service: ServiceMain: Exit"));

    return;
}


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Entry"));

    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks neccesary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);
        break;
    default:
        break;
    }
    OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Exit"));
}
// Enable/disable privilege routine
BOOL SetPrivilege( HANDLE hToken,  // access token handle
    LPCTSTR lpszPrivilege,    // name of privilege to enable/disable
    BOOL bEnablePrivilege    // to enable (or disable privilege)
)
{
    // Token privilege structure
    TOKEN_PRIVILEGES tp;
    // Used by local system to identify the privilege
    LUID luid;
    if (!LookupPrivilegeValue(
        NULL,                   // lookup privilege on local system
        lpszPrivilege,          // privilege to lookup
        &luid))                       // receives LUID of privilege
    {
        //MyFile<<"LookupPrivilegeValue() failed, error: "<< GetLastError()<<endl;
        return FALSE;
    }
    //else
       // MyFile<<"LookupPrivilegeValue()  found!"<< lpszPrivilege<<endl;
       // 
    // Number of privilege
    tp.PrivilegeCount = 1;
    // Assign luid to the 1st count
    tp.Privileges[0].Luid = luid;
    // Enable/disable
    if (bEnablePrivilege)
    {
        // Enable
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
       // MyFile<< lpszPrivilege<< " was enabled!" << endl;
    }
    else
    {
        // Disable
        tp.Privileges[0].Attributes = 0;
       //MyFile<< lpszPrivilege << " was disabled!" <<endl;
    }
    // Adjusting the new privilege
    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,      // If TRUE, function disables all privileges,
                    // if FALSE the function modifies privilege based on the tp
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
      //  MyFile<<"AdjustTokenPrivileges() failed to adjust the new privilege, error: "<< GetLastError()<<endl;
        return FALSE;
    }
    else
    {
      //  MyFile << "AdjustTokenPrivileges() is OK - new privilege was adjusted!" << endl;
        return TRUE;
    }
    return FALSE;
}
/*
BOOL CheckWindowsPrivilege(const TCHAR* Privilege)
{// The privilege to be adjusted
    LPCTSTR lpszPrivilege = L"SeSecurityPrivilege";
    // Change this BOOL value to set/unset the SE_PRIVILEGE_ENABLED attribute
    // Initially to enable
    BOOL bEnablePrivilege = TRUE;
    BOOL bRetVal;
    // Open a handle to the access token for the calling process
    //if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
    //    return FALSE;
    // MyFile << "OpenProcessToken() failed, error "<<GetLastError()<<endl;
   // }
   // else
    // MyFile << "OpenProcessToken() is OK, got the handle!\n";
    // Call the user defined SetPrivilege() function to
    // enable and set the needed privilege
    bRetVal = SetPrivilege(hToken, lpszPrivilege, bEnablePrivilege);
    if (!bRetVal)
    {
     //   MyFile << "Failed to enable privilege, error " << GetLastError() << endl;
        return FALSE;
    }
    else {
       // MyFile << "The privilege was enabled!" << endl;
        return TRUE;
    }//**************************************************
    // TODO: Complete your task which need the privilege
   // MyFile << "I am completing my task that need a privilege..." << endl;
    //**************************************************


    // After we have completed our task, don't forget to disable the privilege
    bEnablePrivilege = FALSE;
    bRetVal = SetPrivilege(hToken, lpszPrivilege, bEnablePrivilege);
    if (!bRetVal)
    {
        MyFile << (L"Failed to disable the privilege, error \n", GetLastError());
        return FALSE;
    }
    else
        MyFile<<(L"The privilege was disabled!\n");
    return FALSE;
}*/
void ReportError(LPCWSTR pszFunction, DWORD dwError = GetLastError())
{
    wprintf(L"%s failed w/err 0x%08lx\n", pszFunction, dwError);
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    wchar_t szCurrentUserName[INFO_BUFFER_SIZE] = {};
    wchar_t szUserName[INFO_BUFFER_SIZE] = L"hello";
    wchar_t szDomain[INFO_BUFFER_SIZE] = L".";
    wchar_t szPassword[INFO_BUFFER_SIZE] = L"hello";
    wchar_t* pc = NULL;
    HANDLE hToken = NULL;
    BOOL fSucceeded = FALSE;

    // Print the name of the user associated with the current thread.
    //MyFile << "Before the impersonation ...\n";
    DWORD nSize = ARRAYSIZE(szCurrentUserName);
    if (!GetUserName(szCurrentUserName, &nSize))
    {
        ReportError(L"GetUserName");
        //goto Cleanup;
    }
    char ch[260];
    char DefChar = ' ';
    WideCharToMultiByte(CP_ACP, 0, szCurrentUserName, -1, ch, 260, &DefChar, NULL);
    //A std:string  using the char* constructor.
    std::string ss(ch);
    MyFile << "The current user is " << ch << endl;

    // Gather the credential information of the impersonated user.
    // The impersonation is successful.
   // fSucceeded = TRUE;
    // Print the name of the user associated with the current thread.
    //ZeroMemory(szCurrentUserName, sizeof(szCurrentUserName));
    //nSize = ARRAYSIZE(szCurrentUserName);
    //if (!GetUserName(szCurrentUserName, &nSize))
    //{
    //    ReportError(L"GetUserName");
       // goto Cleanup;
   // }
    //MyFile << "The current user is\n" << szCurrentUserName << endl;;

    // Work as the impersonated user.
    // ...

/*Cleanup:

    // Clean up the buffer containing sensitive password.
    SecureZeroMemory(szPassword, sizeof(szPassword));
   //////-/-/-/-/-/-/-/-/-/-///-/-/-/-//-/-//*/
    OutputDebugString(_T("My Sample Service: ServiceWorkerThread: Entry"));

    //  Periodically check if the service has been requested to stop
    HKEY   currKey;
    DWORD dwDisposition;
    LONG   lErrorCode;
    HANDLE ProcessToken;
    LPCWSTR subkey = L"SOFTWARE\\Python\\PythonCore\\3.9\\Help\\Main Python Documentation";

    /*if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &ProcessToken))
    {
    }*/
    // Attempt to log on the user.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        MyFile << "OpenProcessToken() failed, error " << GetLastError() << endl;
    }
    else {
        MyFile << "OpenProcessToken() is OK, got the handle!\n";
        SetPrivilege(hToken, SE_BACKUP_NAME, TRUE);
        SetPrivilege(hToken, SE_RESTORE_NAME, TRUE);
        SetPrivilege(hToken, SE_RESTORE_NAME, TRUE);
        BOOL bRetVal, bEnablePrivilege = TRUE;
        LPCTSTR lpszPrivilege = L"SeSecurityPrivilege";
        bRetVal = SetPrivilege(hToken, lpszPrivilege, bEnablePrivilege);
        if (!bRetVal)
        {
            MyFile << "Failed to enable privilege, error " << GetLastError() << endl;
            //return FALSE;
        }
        else {
            MyFile << "The privilege was enabled!" << endl;
            //return TRUE;
        }
    }
    if (!LogonUserW(szUserName, szDomain, szPassword,
        LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken))
    {
        MyFile << "error in LogonUser() and returns " << LogonUser((LPCWSTR)"akash", (LPCWSTR)".", (LPCWSTR)"hello",
            LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken) << "-" << GetLastError() << endl;
        //goto Cleanup;
    }
    else
        MyFile << "LogonUser() success " << endl;

    // Impersonate the logged on user.
    if (!ImpersonateLoggedOnUser(hToken))
    {
        MyFile << "\nThe impersonation is failed due to" << ImpersonateLoggedOnUser(hToken) << "-" << GetLastError() << endl;
        //goto Cleanup;
    }
    else
        MyFile << "The impersonation is successful" << endl;

    //A std:string  using the char* constructor.

    //ImpersonateLoggedOnUser(ProcessToken);
    LRESULT lResult;// RegOpenCurrentUser(KEY_ALL_ACCESS, &currKey);
    // Open a handle to the access token for the
    // calling process that is the currently login access token
    //MyFile<< RegOpenKeyEx(HKEY_CURRENT_USER, subkey, NULL, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &currKey)<<"-"<<GetLastError()<<endl;
    lResult = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Test\\Product\\NewOne", 0, KEY_ALL_ACCESS | KEY_NOTIFY | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY, &currKey);
    //MyFile << "Error in RegOpenKeyExA->" << GetLastError() << endl;
    if (lResult != ERROR_SUCCESS)
    {
        //MyFile<<RegOpenKeyEx(HKEY_CURRENT_USER,_T("SOFTWARE\\"), 0, KEY_READ | KEY_WOW64_64KEY, &currKey)<<endl;
        //Get the error message ID, if any.
        DWORD errorMessageID = ::RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Test\\Product\\NewOne", 0, KEY_ALL_ACCESS | KEY_NOTIFY | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_WOW64_64KEY, &currKey);
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        //Copy the error message into a std::string.
        std::string message(messageBuffer, size);
        MyFile << message;
        return ERROR_SUCCESS;
    }
    else
    {
        MyFile << "Key is successfully Opened\n";
    }
    //MyFile<<"I do not have SeAssignPrimaryTokenPrivilege!"<<endl;
 //else
     //MyFile<<"I do have SeAssignPrimaryTokenPrivilege!"<<endl;
    unsigned long type = REG_SZ, size = 1024;
    char res[1024] = "";
    HKEY key;
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        DWORD  dwFilter = REG_NOTIFY_CHANGE_NAME |
            REG_NOTIFY_CHANGE_ATTRIBUTES |
            REG_NOTIFY_CHANGE_LAST_SET |
            REG_NOTIFY_CHANGE_SECURITY;

        while (1)
        {
            HANDLE hevent = CreateEvent(NULL, FALSE, TRUE, NULL);
            if (RegNotifyChangeKeyValue(currKey, TRUE, dwFilter, hevent, TRUE) != ERROR_SUCCESS)
            {
                std::cout << "NOTIFICATION FAILED " << std::endl;
            }

            DWORD dwret = WaitForSingleObject(hevent, 2000);
            if (dwret == WAIT_TIMEOUT)
            {
                MyFile << " TIMEOUT " << endl;
            }
            else if (dwret == WAIT_FAILED)
            {
                //
            }
            else
            {
                MyFile << "Change Occured at ";
                std::time_t now = std::time(0);
                const char* dt = std::ctime(&now);
                MyFile << dt << '\n';
                //Sleep(2000);
                
            }
        }
    }
    /*lErrorCode = RegSaveKey(hKey, L"c:\\load.reg", 0);
    if (lErrorCode != ERROR_SUCCESS)
    {
        MyFile << "Error in RegSaveKey (%d).\n" << lErrorCode;
        return ERROR_SUCCESS;
    }
    else
    {
        MyFile << L"Key is successfully Saved \n";
    }
    lErrorCode = RegLoadKey(HKEY_LOCAL_MACHINE, subkey, L"c:\\load.reg");
    if (lErrorCode != ERROR_SUCCESS)
    {
        MyFile<<"Error in RegLoadKey (%d).\n";
        return;
    }
    else
    {
        MyFile<<"Key is successfully loaded \n";
    }
    lErrorCode = RegCloseKey(currKey);
    if (lErrorCode != ERROR_SUCCESS)
    {
        MyFile << "Error in closing the key (%d)" << lErrorCode;
        return ERROR_SUCCESS;
    }
    else
    {
        MyFile << "Key is successfully closed \n";
    }
    // If the impersonation was successful, undo the impersonation.
    if (fSucceeded)
    {
        MyFile << "Undo the impersonation ...\n";
        if (!RevertToSelf())
        {
            ReportError(L"RevertToSelf");
        }

        // Print the name of the user associated with the current thread.
        ZeroMemory(szCurrentUserName, sizeof(szCurrentUserName));
        nSize = ARRAYSIZE(szCurrentUserName);
        if (!GetUserName(szCurrentUserName, &nSize))
        {
            ReportError(L"GetUserName");
        }
        //MyFile << "The current user is " << (char*)(szCurrentUserName) << endl;
    }
}
return ERROR_SUCCESS;
}
*/
}