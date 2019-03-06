#include "stdafx.h"
#include "receive_from_server.h"

//TODO what if fails??
DWORD WINAPI ReceiveFromServer(LPVOID Param)
{
    CM_ERROR error;
    CM_RECEIVER_DATA* receiverData = (CM_RECEIVER_DATA*)Param;
    CM_DATA_BUFFER* dataToReceive = NULL;
    CM_SIZE dataToReceiveSize = MAX_BUFFER_SIZE * sizeof(TCHAR);
    CM_SIZE receivedByteCount = 0;
    CM_MESSAGE* message = NULL;
    
    error = CreateDataBuffer(&dataToReceive, dataToReceiveSize);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CreateDataBuffer"));
        return 0;
    }

    while (TRUE)
    {
        receivedByteCount = 0;
        error = ReceiveDataFormServer(receiverData->Client, dataToReceive, &receivedByteCount);
        if (CM_IS_ERROR(error))
        {
            PrintError(error, TEXT("ReceiveDataFormServer"));
            goto receive_from_server_cleanup;
        }

        message = (CM_MESSAGE*)dataToReceive->DataBuffer;

        switch (message->Operation)
        {
        case CM_ECHO:
            break;
        case CM_REGISTER:
            break;
        case CM_LOGIN:
            break;
        case CM_LOGOUT:
            break;
        case CM_MSG:
            break;
        case CM_BROADCAST:
            break;
        case CM_SENDFILE:
            break;
        case CM_LIST:
            break;
        case CM_HISTORY:
            break;
        case CM_CONNECT:
            *(receiverData->Connected) = (BOOL)message->Buffer[0];
            if (!SetEvent(receiverData->StartEvent))
            {
                PrintError(GetLastError(), TEXT("SetEvent"));
                //TODO FAIL???
                goto receive_from_server_cleanup;
            }
            if (!(*(receiverData->Connected)))
            {
                goto receive_from_server_cleanup;
            }
            break;
        default:
            break;
        }

    }

receive_from_server_cleanup:
    DestroyDataBuffer(dataToReceive);
    return 0;
}
