// MessageServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "UtilityLib.h"
#include "handle_clients.h"


int ReadMaxConnections(TCHAR* String, INT64* MaxConnections)
{
    if (NULL == String || NULL == MaxConnections)
    {
        return -1;
    }

    for (TCHAR* c = String; *c != 0; c++)
    {
        if (!isdigit(*c))
        {
            return -1;
        }
    }

    *MaxConnections = _tstoi64(String);

    if (*MaxConnections <= 0)
    {
        return -1;
    }

    return 0;
}

int _tmain(int argc, TCHAR* argv[])
{
    INT retVal = 0;
    UINT rollback = 0;
    CM_ERROR error = CM_SUCCESS;
    DWORD result = 0;
    CM_SERVER_DATA server_data = {NULL, 0, NULL};
    HANDLE commandThread = NULL;

    if (argc < 2 || ReadMaxConnections(argv[1], &server_data.MaxConnections) != 0)
    {
        _tprintf_s(TEXT("Error: invalid maximum number of connections\n"));
        retVal = -1;
        goto main_cleanup;
    }
    _tprintf_s(TEXT("Success\n"));

    //DEBUG
    EnableCommunicationModuleLogger();

    error = InitCommunicationModule();
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("InitCommunicationModule"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;
    
    error = CreateServer(&server_data.Server);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CreateServer"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;

    server_data.EndEvent = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        NULL                // create without a name
    );
    if (NULL == server_data.EndEvent)
    {
        PrintError(GetLastError(), TEXT("CreateEvent"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;

    commandThread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        ReceiveClients,         // thread function name
        &server_data,           // argument to thread function 
        0,                      // use default creation flags 
        NULL                    // thread identifier not needed
    );
    if (NULL == commandThread)
    {
        PrintError(GetLastError(), TEXT("CreateThread"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;

    TCHAR in = ' ';
    while (in != L'Q' && in != L'q')
    {
        _tscanf_s(TEXT("%c"), &in, 1);
    }

    if (!SetEvent(server_data.EndEvent))
    {
        PrintError(GetLastError(), TEXT("SetEvent"));
        retVal = -1;
        goto main_cleanup;
    }

    result = WaitForSingleObject(commandThread, INFINITE);
    if (WAIT_FAILED == result)
    {
        PrintError(GetLastError(), TEXT("WaitForSingleObject"));
        retVal = -1;
        goto main_cleanup;
    }


main_cleanup:
    switch (rollback)
    {
    case 4:
        CloseHandle(commandThread);
    case 3:
        CloseHandle(server_data.EndEvent);
    case 2:
        DestroyServer(server_data.Server);
    case 1:
        UninitCommunicationModule();
    default:
        break;
    }

    return retVal;
}

