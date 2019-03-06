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
    CM_CLIENT* client = NULL;
    BOOL connected = FALSE;
    CM_RECEIVER_DATA receiverData = { NULL, NULL, NULL };
    HANDLE receiverThread = NULL;

    error = InitCommunicationModule();
    if (CM_IS_ERROR(error))
    {

        PrintError(error, TEXT("InitCommunicationModule"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 1;

    receiverData.StartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == receiverData.StartEvent)
    {
        PrintError(GetLastError(), TEXT("CreateEvent"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 2;


    error = CreateClientConnectionToServer(&client);
    if (CM_IS_ERROR(error))
    {
        _tprintf_s(TEXT("Error: no running server found\n"));
        retVal = 0;
        goto main_cleanup;
    }
    rollback = 3;

    receiverData.Client = client;
    receiverData.Connected = &connected;
    receiverThread = CreateThread(NULL, 0, ReceiveFromServer, &receiverData, 0, NULL);
    if (NULL == receiverThread)
    {
        PrintError(GetLastError(), TEXT("CreateThread"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 4;


    DWORD result = WaitForSingleObject(receiverData.StartEvent, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
        PrintError(GetLastError(), TEXT("WaitForSingleObject"));
        retVal = -1;
        goto main_cleanup;
    }

    if (connected)
    {
        _tprintf_s(TEXT("Successful connection\n"));
    }
    else
    {
        _tprintf_s(TEXT("Error: maximum concurrent connection count reached\n"));
        retVal = 0;
        goto main_cleanup;
    }

    ReadCommand(client);

main_cleanup:
    switch (rollback)
    {
    case 4:
        CloseHandle(receiverThread);
    case 3:
        DestroyClient(client);
    case 2:
        CloseHandle(receiverData.StartEvent);
    case 1:
        UninitCommunicationModule();
    default:
        break;
    }
    return 0;
}

