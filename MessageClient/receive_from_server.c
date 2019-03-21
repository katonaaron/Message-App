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
        if (error)
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
            switch (*(CM_VALIDATION*)message->Buffer)
            {
            case CM_INVALID_USERNAME:
                _tprintf_s(TEXT("Error: Invalid username\n"));
                break;
            case CM_INVALID_PASSWORD:
                _tprintf_s(TEXT("Error: Invalid password\n"));
                break;
            case CM_WEAK_PASSWORD:
                _tprintf_s(TEXT("Error: Password too weak\n"));
                break;
            case CM_USER_ALREADY_EXISTS:
                _tprintf_s(TEXT("Error: Username already registered\n"));
                break;
            case CM_CLIENT_ALREADY_LOGGED_IN:
                _tprintf_s(TEXT("Error: User already logged in\n"));
                break;
            case CM_VALIDATION_OK:
                _tprintf_s(TEXT("Success\n"));
                break;
            default:
                PrintErrorMessage(TEXT("Invalid response from server on operation: CM_REGISTER"));
                break;
            }
            break;
        case CM_LOGIN:
            switch (*(CM_VALIDATION*)message->Buffer)
            {

            case CM_INVALID_PASSWORD:
            case CM_INVALID_USERNAME:
                _tprintf_s(TEXT("Error: Invalid username/password combination\n"));
                break;
            case CM_CLIENT_ALREADY_LOGGED_IN:
                _tprintf_s(TEXT("Error: Another user already logged in\n"));
                break;
            case CM_USER_ALREADY_LOGGED_IN:
                _tprintf_s(TEXT("Error: User already logged in\n"));
                break;
            case CM_VALIDATION_OK:
                _tprintf_s(TEXT("Success\n"));
                break;
            default:
                PrintErrorMessage(TEXT("Invalid response from server on operation: CM_LOGIN"));
                break;
            }
            break;
        case CM_LOGOUT:
            switch (*(CM_VALIDATION*)message->Buffer)
            {
            case CM_CLIENT_NOT_LOGGED_IN:
                _tprintf_s(TEXT("Error: No user currently logged in\n"));
                break;
            case CM_VALIDATION_OK:
                _tprintf_s(TEXT("Success\n"));
                break;
            default:
                PrintErrorMessage(TEXT("Invalid response from server on operation: CM_LOGOUT"));
                break;
            }
            break;
        case CM_MSG:
            switch (*(CM_VALIDATION*)message->Buffer)
            {
            case CM_INVALID_USERNAME:
                _tprintf_s(TEXT("Error: No such user\n"));
                break;
            case CM_CLIENT_NOT_LOGGED_IN:
                _tprintf_s(TEXT("Error: No user currently logged in\n"));
                break;
            case CM_VALIDATION_OK:
                _tprintf_s(TEXT("Success\n"));
                break;
            default:
                PrintErrorMessage(TEXT("Invalid response from server on operation: CM_MSG"));
                break;
            }
            break;
        case CM_BROADCAST:
            break;
        case CM_SENDFILE:
            break;
        case CM_LIST:
            _tprintf_s(TEXT("%s\n"), (TCHAR*)message->Buffer);
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
        case CM_MSG_TEXT:
        {
            size_t length = (message->Size / sizeof(TCHAR));
            TCHAR* buffer = (TCHAR*)message->Buffer;
            TCHAR last = buffer[length - 1];
            buffer[length - 1] = 0;
            _tprintf_s(TEXT("Message%*s%c"), (int)length, buffer, last);
        }
        break;
        default:
            PrintErrorMessage(TEXT("Invalid operation code from server"));
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
