#include "client_operations.h"

void ReadCommands(CM_CLIENT* Client, HANDLE* ReceiverThread)
{
    TCHAR buffer[MAX_INPUT_LENGTH + 1], *word = NULL, *next_token = NULL;
    BOOL exit = FALSE;
    DWORD result;

    CM_ERROR error = CM_SUCCESS;

    while (!exit)
    {
        _fgetts(buffer, MAX_INPUT_LENGTH, stdin);

        result = WaitForSingleObject(ReceiverThread, 0);
        if (WAIT_OBJECT_0 == result)
        {
            PrintErrorMessage(TEXT("Receiver thread closed"));
            return;
        }
        else if (WAIT_TIMEOUT != result)
        {
            PrintError(GetLastError(), TEXT("WaitForSingleObject"));
            return;
        }

        word = _tcstok_s(buffer, TEXT(" \n"), &next_token);

        if (_tcscmp(word, TEXT("echo")) == 0)
        {

            word = _tcstok_s(NULL, TEXT("\n"), &next_token);

            if (NULL == word)
            {
                error = SendMessageToServer(Client, TEXT(""), sizeof(TCHAR), CM_ECHO);
            }
            else
            {
                error = SendMessageToServer(Client, word, (CM_SIZE)(_tcslen(word) + 1) * sizeof(TCHAR), CM_ECHO);
            }

            if (error)
            {
                PrintError(error, TEXT("SendMessageToServer"));
                return;
            }
        }
        else if (_tcscmp(word, TEXT("register")) == 0)
        {
            TCHAR* username, *password;
            username = _tcstok_s(NULL, TEXT(" \n"), &next_token);
            password = _tcstok_s(NULL, TEXT("\n"), &next_token);

            switch (ValidateUsernamePassword(username, password))
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
            case CM_VALIDATION_OK:
                *(password - 1) = '\n';
                error = SendMessageToServer(Client, username, (CM_SIZE)(_tcslen(username) + _tcslen(password) + 2) * sizeof(TCHAR), CM_REGISTER);
                if (error)
                {
                    PrintError(error, TEXT("SendMessageToServer"));
                    return;
                }
            default:
                break;
            }
        }
        else if (_tcscmp(word, TEXT("login")) == 0)
        {
            TCHAR* username, *password;
            username = _tcstok_s(NULL, TEXT(" \n"), &next_token);
            password = _tcstok_s(NULL, TEXT("\n"), &next_token);

            if (NULL == username || NULL == password)
            {
                _tprintf_s(TEXT("Error: Invalid username/password combination\n"));
            }
            else
            {
                *(password - 1) = '\n';
                error = SendMessageToServer(Client, username, (CM_SIZE)(_tcslen(username) + _tcslen(password) + 2) * sizeof(TCHAR), CM_LOGIN);
                if (error)
                {
                    PrintError(error, TEXT("SendMessageToServer"));
                    return;
                }
            }
        }
        else if (_tcscmp(word, TEXT("logout")) == 0)
        {
            error = SendMessageToServer(Client, NULL, 0, CM_LOGOUT);
            if (error)
            {
                PrintError(error, TEXT("SendMessageToServer"));
                return;
            }
        }
        else if (_tcscmp(word, TEXT("msg")) == 0)
        {
            TCHAR* username, *text;
            username = _tcstok_s(NULL, TEXT(" \n"), &next_token);
            text = _tcstok_s(NULL, TEXT("\n"), &next_token);

            if (NULL == username)
            {
                _tprintf_s(TEXT("Error: No such user\n"));
            }
            else
            {
                *(text - 1) = '\n';
                error = SendMessageToServer(Client, username, (CM_SIZE)(_tcslen(username) + _tcslen(text) + 2) * sizeof(TCHAR), CM_MSG);
                if (error)
                {
                    PrintError(error, TEXT("SendMessageToServer"));
                    return;
                }
            }
        }
        else if (_tcscmp(word, TEXT("broadcast")) == 0)
        {
            error = SendMessageToServer(Client, next_token, (CM_SIZE)(_tcslen(next_token) + 1) * sizeof(TCHAR), CM_BROADCAST);
            if (error)
            {
                PrintError(error, TEXT("SendMessageToServer"));
                return;
            }
        }
        else if (_tcscmp(word, TEXT("sendfile")) == 0)
        {
            TCHAR* username, *path;
            username = _tcstok_s(NULL, TEXT(" \n"), &next_token);
            path = _tcstok_s(NULL, TEXT("\n"), &next_token);

            if (NULL == username)
            {
                _tprintf_s(TEXT("Error: No such user\n"));
            }
            else if (NULL == path)
            {
                _tprintf_s(TEXT("Error: File not found\n"));
            }
            else
            {
                error = SendMessageToServer(Client, username, (CM_SIZE)(_tcslen(username) + 1) * sizeof(TCHAR), CM_SENDFILE);
                if (error)
                {
                    PrintError(error, TEXT("SendMessageToServer"));
                    return;
                }
            }
        }
        else if (_tcscmp(word, TEXT("list")) == 0)
        {
            error = SendMessageToServer(Client, NULL, 0, CM_LIST);
            if (error)
            {
                PrintError(error, TEXT("SendMessageToServer"));
                return;
            }
        }
        else if (_tcscmp(word, TEXT("history")) == 0)
        {
            TCHAR* username, *count;
            username = _tcstok_s(NULL, TEXT(" \n"), &next_token);
            count = _tcstok_s(NULL, TEXT("\n"), &next_token);

            if (NULL == username)
            {
                _tprintf_s(TEXT("Error: No such user\n"));
            }
            else if (NULL == count || _tstoi(count) <= 0)
            {
                _tprintf_s(TEXT("\n"));
            }
            else
            {
                *(count - 1) = '\n';
                error = SendMessageToServer(Client, username, (CM_SIZE)(_tcslen(username) + _tcslen(count) + 2) * sizeof(TCHAR), CM_HISTORY);
                if (error)
                {
                    PrintError(error, TEXT("SendMessageToServer"));
                    return;
                }
            }
        }
        else if (_tcscmp(word, TEXT("exit")) == 0)
        {
            exit = TRUE;
        }
    }
}