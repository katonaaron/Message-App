#include "handle_clients.h"

static int Register(CM_SERVER_CLIENT* Client, TCHAR* Username, TCHAR* Password, CM_USER** User);
static int Login(CM_USER_CONNECTION* UserConnection, TCHAR* Username, TCHAR* Password, CM_USER** User);
static int Logout(CM_SERVER_CLIENT* Client, CM_USER* User);
static int Message(CM_USER * User, CM_SERVER_CLIENT* Client, TCHAR * ReceiverName, TCHAR * Message);

VOID CALLBACK ProcessClient(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    INT rollback = 0;
    CM_ERROR error = CM_SUCCESS;

    CM_USER_CONNECTION* userConnection = (CM_USER_CONNECTION*)Parameter;
    CM_SERVER_CLIENT* client = userConnection->Client;
    CM_MESSAGE* message = NULL;
    CM_USER* user = NULL;

    error = SendMessageToClient(client, &userConnection->ServerAccepted, sizeof(BOOL), CM_CONNECT);
    if (error)
    {
        PrintError(error, TEXT("SendMessageToClient"));
        goto process_client_cleanup;
    }

    if (userConnection->ServerAccepted)
    {
        error = UserConnectionAdd(userConnection);
        if (error)
        {
            PrintError(error, TEXT("UserConnectionAdd"));
            goto process_client_cleanup;
        }
    }
    else
    {
        goto process_client_cleanup;
    }
    rollback = 1;


    BOOL exit = FALSE;
    while (!exit)
    {
        error = ReceiveMessageFromClient(client, &message);
        if (error)
        {
            PrintError(error, TEXT("ReceiveMessageFromClient"));
            goto process_client_cleanup;
        }

        switch (message->Operation)
        {
        case CM_ECHO:
            _tprintf_s(TEXT("%s\n"), (TCHAR*)message->Buffer);
            error = SendMessageToClient(client, message->Buffer, message->Size, CM_ECHO);
            if (error)
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

            if (Register(client, username, password, &user))
                goto process_client_cleanup;
            break;
        }
        case CM_LOGIN:
        {
            TCHAR *username, *password, *next_token = NULL;
            username = _tcstok_s((TCHAR*)message->Buffer, TEXT(" \n"), &next_token);
            password = _tcstok_s(NULL, TEXT("\n"), &next_token);

            if (Login(userConnection, username, password, &user))
                goto process_client_cleanup;
            break;
        }
        case CM_LOGOUT:
            if (Logout(client, user))
                goto process_client_cleanup;
            break;
        case CM_MSG:
        {
            TCHAR *username, *text, *next_token = NULL;
            username = _tcstok_s((TCHAR*)message->Buffer, TEXT(" \n"), &next_token);
            text = _tcstok_s(NULL, TEXT("\n"), &next_token);

            if (Message(user, client, username, text))
                goto process_client_cleanup;
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
    case 1:
        error = UserConnectionRemove(userConnection);
        if (error)
        {
            PrintError(error, TEXT("UserConnectionRemove"));
        }
    default:
        if (user)
        {
            error = UserLogOut(user);
            if (error)
            {
                PrintError(error, TEXT("UserConnectionRemove"));
            }
        }
        UserConnectionDestroy(userConnection);
        free(message);
        break;
    }
    return;
}

static int Register(CM_SERVER_CLIENT* Client, TCHAR* Username, TCHAR* Password, CM_USER** User)
{
    if (NULL == Client || NULL == User)
    {
        PrintError(CM_INVALID_PARAMETER, TEXT("Register"));
        return -1;
    }

    CM_ERROR error = CM_SUCCESS;
    CM_VALIDATION response = CM_VALIDATION_OK;
    CM_USER* user = NULL, *foundUser = NULL;
    BOOL isLoggedIn;

    if (*User != NULL)
    {
        error = UserIsLoggedIn(*User, &isLoggedIn);
        if (error)
        {
            PrintError(error, TEXT("UserIsLoggedIn"));
            return -1;
        }

        if (isLoggedIn)
        {
            response = CM_CLIENT_ALREADY_LOGGED_IN;
        }
    }


    if (!response)
    {
        response = ValidateUsernamePassword(Username, Password);
    }


    if (!response)
    {
        error = UserFind(Username, &foundUser);
        if (error != CM_NOT_FOUND)
        {
            if (error)
            {
                PrintError(error, TEXT("UserFind"));
                return -1;
            }
            response = CM_USER_ALREADY_EXISTS;
        }
    }

    if (!response)
    {
        error = UserCreate(&user, Username, Password);
        if (error)
        {
            PrintError(error, TEXT("UserCreate"));
            return -1;
        }

        error = UserAdd(user, TRUE);
        if (error)
        {
            UserDestroy(user);
            PrintError(error, TEXT("UserAdd"));
            return -1;
        }

        *User = user;
    }

    error = SendMessageToClient(Client, &response, sizeof(CM_VALIDATION), CM_REGISTER);
    if (error)
    {
        PrintError(error, TEXT("SendMessageToClient"));
        return -1;
    }

    return 0;
}

static int Login(CM_USER_CONNECTION* UserConnection, TCHAR* Username, TCHAR* Password, CM_USER** User)
{
    if (NULL == UserConnection || NULL == User)
    {
        PrintError(CM_INVALID_PARAMETER, TEXT("Login"));
        return -1;
    }

    CM_ERROR error = CM_SUCCESS;
    CM_VALIDATION response = CM_VALIDATION_OK;
    CM_USER *foundUser = NULL;
    BOOL isLoggedIn;
    printf("start\n");
    if (*User != NULL)
    {
        error = UserIsLoggedIn(*User, &isLoggedIn);
        if (error)
        {
            PrintError(error, TEXT("UserIsLoggedIn"));
            return -1;
        }

        if (isLoggedIn)
        {
            response = CM_CLIENT_ALREADY_LOGGED_IN;
        }
    }

    if (!response && (NULL == Username || NULL == Password))
    {
        response = CM_INVALID_USERNAME; printf("invalid username or password\n");
    }

    if (!response)
    {
        error = UserFind(Username, &foundUser);
        if (error == CM_NOT_FOUND)
        {
            response = CM_INVALID_USERNAME; printf("not found\n");
        }
        else if (error || NULL == foundUser)
        {
            PrintError(error, TEXT("UserFind"));
            return -1;
        }
    }

    if (!response && foundUser->IsLoggedIn)
    {
        response = CM_USER_ALREADY_LOGGED_IN; printf("ALREADY_LOGGED_IN\n");
    }

    if (!response && _tcscmp(foundUser->Password, Password))
    {
        response = CM_INVALID_PASSWORD; printf("password\n");
    }

    if (!response)
    {
        error = UserLogIn(foundUser, UserConnection);
        if (error)
        {
            PrintError(error, TEXT("UserLogIn"));
            return -1;
        }
        *User = foundUser; printf("logged in\n");
    }

    error = SendMessageToClient(UserConnection->Client, &response, sizeof(CM_VALIDATION), CM_LOGIN);
    if (error)
    {
        PrintError(error, TEXT("SendMessageToClient"));
        return -1;
    }

   if (!response)
    {
        error = UserReceiveOfflineMessages(*User);
        if (error)
        {
            PrintError(error, TEXT("UserReceiveOfflineMessages"));
            return -1;
        }printf("offline messages sent\n");
    }
    
    return 0;
}

static int Logout(CM_SERVER_CLIENT * Client, CM_USER * User)
{
    if (NULL == Client)
    {
        PrintError(CM_INVALID_PARAMETER, TEXT("Login"));
        return -1;
    }

    CM_VALIDATION response = CM_VALIDATION_OK;
    CM_ERROR error;
    BOOL isLoggedIn;

    if (NULL == User)
    {
        response = CM_CLIENT_NOT_LOGGED_IN;
    }
    else
    {
        error = UserIsLoggedIn(User, &isLoggedIn);
        if (error)
        {
            PrintError(error, TEXT("UserIsLoggedIn"));
            return -1;
        }

        if (!isLoggedIn)
        {
            response = CM_CLIENT_NOT_LOGGED_IN;
        }
        else
        {
            error = UserLogOut(User);
            if (error)
            {
                PrintError(error, TEXT("UserLogOut"));
                return -1;
            }
        }
    }


    error = SendMessageToClient(Client, &response, sizeof(CM_VALIDATION), CM_LOGOUT);
    if (error)
    {
        PrintError(error, TEXT("SendMessageToClient"));
        return -1;
    }
    return 0;
}

static int Message(CM_USER * User, CM_SERVER_CLIENT* Client, TCHAR * ReceiverName, TCHAR * Message)
{
    if (NULL == ReceiverName || NULL == Message || NULL == Client)
    {
        PrintError(CM_INVALID_PARAMETER, TEXT("Message"));
        return -1;
    }

    CM_VALIDATION response = CM_VALIDATION_OK;
    CM_ERROR error;
    BOOL isLoggedIn;

    if (NULL == User || NULL == User->Connection)
    {
        response = CM_CLIENT_NOT_LOGGED_IN;
    }
    else
    {
        error = UserIsLoggedIn(User, &isLoggedIn);
        if (error)
        {
            PrintError(error, TEXT("UserIsLoggedIn"));
            return -1;
        }

        if (!isLoggedIn)
        {
            response = CM_CLIENT_NOT_LOGGED_IN;
        }
    }

    CM_USER* receiver = NULL;

    if (!response)
    {
        error = UserFind(ReceiverName, &receiver);
        if (error == CM_NOT_FOUND)
        {
            response = CM_INVALID_USERNAME;
        }
        else if (error || NULL == receiver)
        {
            PrintError(error, TEXT("UserFind"));
            return -1;
        }
    }

    if (!response)
    {
        error = UserSendMessage(User, receiver, Message);
        if (error)
        {
            PrintError(error, TEXT("UserSendMessage"));
            return -1;
        }
    }

    error = SendMessageToClient(Client, &response, sizeof(CM_VALIDATION), CM_MSG);
    if (error)
    {
        PrintError(error, TEXT("SendMessageToClient"));
        return -1;
    }

    return 0;
}
