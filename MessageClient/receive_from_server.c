#include "stdafx.h"
#include "receive_from_server.h"

DWORD WINAPI ReceiveFromServer(LPVOID Param)
{

    DWORD rollback = 0, result;
    CM_ERROR error;
    CM_CLIENT_CONNECTION* connection = (CM_CLIENT_CONNECTION*)Param;
    CM_MESSAGE* message = NULL;

    while (TRUE)
    {
        error = ReceiveMessageFromServer(connection->Client, &message);
        result = WaitForSingleObject(connection->StartStopEvent, 0);
        if (WAIT_OBJECT_0 == result)
        {
            error = CM_SUCCESS;
            goto receive_from_server_cleanup;
        }
        else if (WAIT_TIMEOUT != result)
        {
            PrintError(GetLastError(), TEXT("WaitForSingleObject"));
            goto receive_from_server_cleanup;
        }
        if (CM_IS_ERROR(error))
        {
            PrintError(error, TEXT("ReceiveMessageFromServer"));
            goto receive_from_server_cleanup;
        }

        switch (message->Operation)
        {
        case CM_ECHO:
            _tprintf_s(TEXT("%s\n"), (TCHAR*)message->Buffer);
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
            connection->IsConnected = (BOOL)message->Buffer[0];
            if (!SetEvent(connection->StartStopEvent))
            {
                PrintError(GetLastError(), TEXT("SetEvent"));
                goto receive_from_server_cleanup;
            }
            if (!connection->IsConnected)
            {
                goto receive_from_server_cleanup;
            }
            break;
        default:
            break;
        }
        free(message);
        message = NULL;
    }

receive_from_server_cleanup:
    switch (rollback)
    {
    default:
        free(message);
        break;
    }
    return 0;
}
