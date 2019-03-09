// MessageClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "communication_api.h"
#include "UtilityLib.h"

#include "receive_from_server.h"
#include "client_operations.h"

int _tmain(int argc, TCHAR* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //Debug
    EnableCommunicationModuleLogger();

    INT retVal = 0, rollback = 0;
    CM_ERROR error = CM_SUCCESS;

    CM_CLIENT_CONNECTION connection = { NULL, NULL, FALSE };
    HANDLE receiverThread = NULL;

    error = InitCommunicationModule();
    if (CM_IS_ERROR(error))
    {

        PrintError(error, TEXT("InitCommunicationModule"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 1;

    connection.StartStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == connection.StartStopEvent)
    {
        PrintError(GetLastError(), TEXT("CreateEvent"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 2;


    error = CreateClientConnectionToServer(&connection.Client);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("Error: no running server found\n"));
        retVal = 0;
        goto main_cleanup;
    }
    rollback = 3;

    receiverThread = CreateThread(NULL, 0, ReceiveFromServer, &connection, 0, NULL);
    if (NULL == receiverThread)
    {
        PrintError(GetLastError(), TEXT("CreateThread"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 4;

    //TODO: Should we have a timeout?
    HANDLE handles[2];
    handles[0] = receiverThread;
    handles[1] = connection.StartStopEvent;
    DWORD result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    if (result != WAIT_OBJECT_0 + 1)
    {
        if (WAIT_OBJECT_0 == result)
        {
            _tprintf_s(TEXT("Unexpected error: Receiver thread closed\n"));
        }
        else
        {
            PrintError(GetLastError(), TEXT("WaitForSingleObject"));
        }
        retVal = -1;
        goto main_cleanup;
    }

    if (connection.IsConnected)
    {
        _tprintf_s(TEXT("Successful connection\n"));
    }
    else
    {
        _tprintf_s(TEXT("Error: maximum concurrent connection count reached\n"));
        retVal = 0;
        goto main_cleanup;
    }
    rollback = 5;

    ReadCommands(connection.Client, receiverThread);

main_cleanup:
    switch (rollback)
    {
    case 5:
        if (!SetEvent(connection.StartStopEvent))
        {
            PrintError(GetLastError(), TEXT("SetEvent"));
            rollback = 4;
            retVal = -1;
            goto main_cleanup;
        }
        UninitCommunicationModule();
        result = WaitForSingleObject(receiverThread, INFINITE);
        if (WAIT_OBJECT_0 != result)
        {
             PrintError(GetLastError(), TEXT("WaitForSingleObject"));
             retVal = -1;
        }
        CloseHandle(receiverThread);
        DestroyClient(connection.Client);
        CloseHandle(connection.StartStopEvent);
        break;
    case 4:
        CloseHandle(receiverThread);
    case 3:
        DestroyClient(connection.Client);
    case 2:
        CloseHandle(connection.StartStopEvent);
    case 1:
        UninitCommunicationModule();
    default:
        break;
    }
    return 0;
}

