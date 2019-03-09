#include "stdafx.h"
#include "handle_clients.h"

VOID CALLBACK ProcessClient(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    CM_SERVER_CLIENT_DATA* data = (CM_SERVER_CLIENT_DATA*)Parameter;
    INT rollback = 0;
    CM_ERROR error = CM_SUCCESS;
    CM_MESSAGE* message = NULL;

    error = SendMessageToClient(data->Client, &data->Accept, sizeof(BOOL), CM_CONNECT);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("SendMessageToClient"));
        goto process_client_cleanup;
    }

    BOOL exit = FALSE;
    while (!exit)
    {
        error = ReceiveMessageFromClient(data->Client, &message);
        if (CM_IS_ERROR(error))
        {
            PrintError(error, TEXT("ReceiveMessageFromClient"));
            goto process_client_cleanup;
        }

        switch (message->Operation)
        {
        case CM_ECHO:
            _tprintf_s(TEXT("%s\n"), (TCHAR*)message->Buffer);
            error = SendMessageToClient(data->Client, message->Buffer, message->Size, CM_ECHO);
            if (CM_IS_ERROR(error))
            {
                PrintError(error, TEXT("SendMessageToClient"));
                goto process_client_cleanup;
            }
            break;
        case CM_REGISTER:
        {
            TCHAR *username, *password, *next_token = NULL;
            username = _tcstok_s((TCHAR*)message->Buffer, TEXT(" \n"), &next_token);
            password = _tcstok_s(NULL, TEXT("\n"), &next_token);
            if (NULL == username || NULL == password)
            {
                PrintErrorMessage(TEXT("Invalid register credentials"));
            }
            else
            {
                _tprintf_s(TEXT("username: %s, password: %s\n"), username, password);
            }
            break;
        }
        case CM_LOGIN:
        {
            TCHAR *username, *password, *next_token = NULL;
            username = _tcstok_s((TCHAR*)message->Buffer, TEXT(" \n"), &next_token);
            password = _tcstok_s(NULL, TEXT("\n"), &next_token);
            if (NULL == username || NULL == password)
            {
                PrintErrorMessage(TEXT("Invalid login credentials"));
            }
            else
            {
                _tprintf_s(TEXT("username: %s, password: %s\n"), username, password);
            }
            break;
        }
        case CM_LOGOUT:
            _tprintf_s(TEXT("logout\n"));
            break;
        case CM_MSG:
        {
            TCHAR *username, *text, *next_token = NULL;
            username = _tcstok_s((TCHAR*)message->Buffer, TEXT(" \n"), &next_token);
            text = _tcstok_s(NULL, TEXT("\n"), &next_token);
            if (NULL == username)
            {
                PrintErrorMessage(TEXT("Error: No such user"));
            }
            else
            {
                _tprintf_s(TEXT("username: %s, text: %s\n"), username, text);
            }
            break;
        }
        case CM_BROADCAST:
            _tprintf_s(TEXT("broadcast: %s\n"), (TCHAR*)message->Buffer);
            break;
        case CM_SENDFILE:
            _tprintf_s(TEXT("file will be sent to: %s\n"), (TCHAR*)message->Buffer);
            break;
        case CM_LIST:
            _tprintf_s(TEXT("list\n"));
            break;
        case CM_HISTORY:
        {
            TCHAR *username, *count, *next_token = NULL;
            username = _tcstok_s((TCHAR*)message->Buffer, TEXT(" \n"), &next_token);
            count = _tcstok_s(NULL, TEXT("\n"), &next_token);
            if (NULL == username)
            {
                PrintErrorMessage(TEXT("Error: No such user"));
            }
            else if (NULL == count || _tstoi(count) <= 0)
            {
                _tprintf_s(TEXT("count <= 0\n"));
            }
            else
            {
                _tprintf_s(TEXT("history: username: %s, count: %d\n"), username, _tstoi(count));
            }
            break;
        }
        case CM_EXIT:
            exit = TRUE;
            break;
        default:
            break;
        }
        free(message);
        message = NULL;
    }

process_client_cleanup:
    switch (rollback)
    {
    default:
        AbandonClient(data->Client);
        free(message);
        break;
    }
    return;
}
