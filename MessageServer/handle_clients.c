#include "stdafx.h"
#include "handle_clients.h"

/*DWORD WINAPI ReceiveClients(LPVOID Param)
{
    DWORD result = 1;
    CM_ERROR error = CM_SUCCESS;

    CM_SERVER_DATA* server_data = (CM_SERVER_DATA*)Param;
    CM_SERVER_CLIENT* newClient = NULL;



    do
    {
        error = AwaitNewClient(server_data->Server, &newClient);
        if (CM_IS_ERROR(error))
        {
            PrintError(error, TEXT("AwaitNewClient"));
            return 1;
        }

        result = WaitForSingleObject(server_data->EndEvent, 0);
        if (WAIT_FAILED == result)
        {
            PrintError(GetLastError(), TEXT("WaitForSingleObject"));
            AbandonClient(newClient);
            return 1;
        }

        //_tprintf_s(TEXT("%lld, 0x%X\n"), ++i, result);
        //Sleep(1000);
    } while (WAIT_OBJECT_0 != result);
    return 0;
}*/

VOID CALLBACK ProcessClient(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    CM_SERVER_CLIENT_DATA* data = (CM_SERVER_CLIENT_DATA*)Parameter;
    INT rollback = 0;

    CM_MESSAGE* message = malloc(sizeof(CM_MESSAGE) + sizeof(BOOL));
    message->Operation = CM_CONNECT;
    message->Size = sizeof(BOOL);
    memcpy(message->Buffer, &data->Accept, message->Size);
    rollback = 1;

    CM_DATA_BUFFER* dataToSend = NULL;
    CM_ERROR error = CreateDataBuffer(&dataToSend, sizeof(CM_MESSAGE) + message->Size);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CreateDataBuffer"));
        goto process_client_cleanup;
    }
    rollback = 2;

    error = CopyDataIntoBuffer(dataToSend, (const CM_BYTE*)message, sizeof(CM_MESSAGE) + message->Size);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CopyDataIntoBuffer"));
        goto process_client_cleanup;
    }

    CM_SIZE sendByteCount = 0;
    error = SendDataToClient(data->Client, dataToSend, &sendByteCount);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("SendDataToClient"));
        goto process_client_cleanup;
    }

    _tprintf_s(TEXT("Successfully sent data from server client:\n \tSent data size: %d\n")
        , sendByteCount
    );

process_client_cleanup:
    switch (rollback)
    {
    case 2:
        DestroyDataBuffer(dataToSend);
    case 1:
        free(message);
    default:
        //AbandonClient(data->Client);
        break;
    }
    return;
}
