#include "users.h"

//Globals
PLIST_ENTRY gConnections;
PLIST_ENTRY gUsers;
PSRWLOCK gUsersGuard, gConnectionsGuard, gCountGuard, gFileGuard;
INT gConnectionsCount, gMaxConnections;
BOOL gIsUsersInitialized = FALSE;
HANDLE gRegistrationFile;

static CM_ERROR ReadUsersFromFile()
{
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(gRegistrationFile, &fileSize) == 0)
        return GetLastError();

    if (fileSize.QuadPart == 0)
        return CM_SUCCESS;

    HANDLE fileMapping = CreateFileMapping(
        gRegistrationFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if (NULL == fileMapping)
        return GetLastError();

    TCHAR* buffer = MapViewOfFile(
        fileMapping,
        FILE_MAP_READ,
        0,
        0,
        0
    );
    if (NULL == buffer)
        return GetLastError();

    TCHAR line[MAX_INPUT_LENGTH + 1];
    TCHAR *next = buffer, *prev = buffer, *sep;
    CM_USER* user;
    CM_ERROR error = CM_SUCCESS;

    while ((next = _tcschr(next, TEXT('\n'))) != NULL)
    {
        _tcsncpy_s(line, MAX_INPUT_LENGTH + 1, prev, next - prev);
        line[next - prev] = TEXT('\0');
        sep = _tcschr(line, TEXT(','));
        if (NULL == sep)
            return CM_SUCCESS;
        *sep = TEXT('\0');

        error = UserCreate(&user, line, sep + 1);
        if (error)
            return error;

        error = UserAdd(user, FALSE);
        if (error)
        {
            UserDestroy(user);
            return error;
        }
        next++;
        prev = next;
    }

    if (!UnmapViewOfFile(buffer))
        return GetLastError();
    if (!CloseHandle(fileMapping))
        return GetLastError();

    DWORD res = SetFilePointer(gRegistrationFile, 0, NULL, FILE_END);
    if (INVALID_SET_FILE_POINTER == res)
        return INVALID_SET_FILE_POINTER;

    return CM_SUCCESS;
}

CM_ERROR InitUsersModule(INT MaxConnections)
{
    if (gIsUsersInitialized)
        return CM_SUCCESS;
    if (MaxConnections <= 0)
        return CM_INVALID_PARAMETER;

    static LIST_ENTRY connections, users;
    InitializeListHead(&connections);
    InitializeListHead(&users);
    gConnections = &connections;
    gUsers = &users;

    static SRWLOCK usersGuard = SRWLOCK_INIT, connectionsGuard = SRWLOCK_INIT, countGuard = SRWLOCK_INIT, fileGuard = SRWLOCK_INIT;
    gUsersGuard = &usersGuard;
    gConnectionsGuard = &connectionsGuard;
    gCountGuard = &countGuard;
    gFileGuard = &fileGuard;

    gConnectionsCount = 0;
    gMaxConnections = MaxConnections;

    gIsUsersInitialized = TRUE;

    gRegistrationFile = CreateFile(
        TEXT("C:\\registration.txt"),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );    
    if (INVALID_HANDLE_VALUE == gRegistrationFile)
        return GetLastError();
    return ReadUsersFromFile();
}

void UninitUsersModule()
{
    if (gIsUsersInitialized)
    {
        gIsUsersInitialized = FALSE; //TODO: Sync threads
        CloseHandle(gRegistrationFile);
        gRegistrationFile = NULL;        

        CM_USER* user;
        while (gUsers->Flink != gUsers)
        {
            user = CONTAINING_RECORD(gUsers->Flink, CM_USER, Entry);
            RemoveEntryList(gUsers->Flink);
            UserDestroy(user);
        }

        CM_USER_CONNECTION* useConnection;
        while (gConnections->Flink != gConnections)
        {
            useConnection = CONTAINING_RECORD(gConnections->Flink, CM_USER_CONNECTION, Entry);
            RemoveEntryList(gConnections->Flink);
            UserConnectionDestroy(useConnection);
        }
    }
}

CM_ERROR UserCreate(CM_USER ** User, TCHAR * Username, TCHAR * Password)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == User || NULL == Username || NULL == Password)
        return CM_INVALID_PARAMETER;

    PCM_USER user = (PCM_USER)malloc(sizeof(CM_USER));
    if (NULL == user)
    {
        *User = NULL;
        return CM_NO_MEMORY;
    }

    user->IsLoggedIn = FALSE;
    user->Connection = NULL;

    size_t length = _tcsclen(Username) + 1;
    user->Username = (TCHAR*)malloc(GET_STRING_SIZE(length));
    if (NULL == user->Username)
    {
        free(user);
        *User = NULL;
        return CM_NO_MEMORY;
    }
    _tcscpy_s(user->Username, length, Username);

    length = _tcsclen(Password) + 1;
    user->Password = (TCHAR*)malloc(GET_STRING_SIZE(length));
    if (NULL == user->Password)
    {
        free(user->Username);
        free(user);
        *User = NULL;
        return CM_NO_MEMORY;
    }
    _tcscpy_s(user->Password, length, Password);

    *User = user;
    return CM_SUCCESS;
}

void UserDestroy(CM_USER * User)
{
    if (User != NULL)
    {
        free(User->Username);
        free(User->Password);
        UserConnectionDestroy(User->Connection);
        free(User);

    }
}

CM_ERROR UserAdd(CM_USER * User, BOOL WriteToFile)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == User)
        return CM_INVALID_PARAMETER;

    if (WriteToFile)
    {
        DWORD bytesWritten, bytesToWrite;
        BOOL result;
        bytesToWrite = (DWORD)GET_STRING_SIZE(_tcslen(User->Username));

        size_t usernameLength = _tcslen(User->Username), passwordLength = _tcslen(User->Password);
        size_t length = usernameLength + passwordLength + 2; // , \n
        TCHAR* line = (TCHAR*)malloc(GET_STRING_SIZE(length));
        if (NULL == line)
            return CM_NO_MEMORY;

        _tcscpy_s(line, length, User->Username);
        line[usernameLength] = TEXT(',');
        _tcscpy_s(line + usernameLength + 1, passwordLength + 2, User->Password);
        line[usernameLength + passwordLength + 1] = TEXT('\n');
        bytesToWrite = (DWORD)GET_STRING_SIZE(length);

        AcquireSRWLockExclusive(gFileGuard);
        result = WriteFile(gRegistrationFile, line, bytesToWrite, &bytesWritten, NULL);
        if (!result || bytesToWrite != bytesWritten)
            return GetLastError();
        ReleaseSRWLockExclusive(gFileGuard);
    }
    AcquireSRWLockExclusive(gUsersGuard);
    InsertHeadList(gUsers, &User->Entry);
    ReleaseSRWLockExclusive(gUsersGuard);
    return CM_SUCCESS;
}

CM_ERROR UserLogIn(CM_USER * User, CM_USER_CONNECTION * UserConnection)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == User || NULL == UserConnection)
        return CM_INVALID_PARAMETER;

    AcquireSRWLockExclusive(gUsersGuard);
    User->Connection = UserConnection;
    User->IsLoggedIn = TRUE;
    ReleaseSRWLockExclusive(gUsersGuard);
    return CM_SUCCESS;
}

CM_ERROR UserLogOut(CM_USER * User)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == User)
        return CM_INVALID_PARAMETER;

    AcquireSRWLockExclusive(gUsersGuard);
    User->Connection = NULL;
    User->IsLoggedIn = FALSE;
    ReleaseSRWLockExclusive(gUsersGuard);

    return CM_SUCCESS;
}

CM_ERROR UserFind(TCHAR* Username, CM_USER** FoundUser)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == Username || NULL == FoundUser)
        return CM_INVALID_PARAMETER;


    PLIST_ENTRY node;
    PCM_USER user = NULL;
    BOOL found = FALSE;

    AcquireSRWLockShared(gUsersGuard);
    node = gUsers->Flink;
    while (node != gUsers && !found)
    {
        user = CONTAINING_RECORD(node, CM_USER, Entry);
        found = !_tcscmp(user->Username, Username);
        node = node->Flink;
    }
    ReleaseSRWLockShared(gUsersGuard);

    if (!found)
    {
        *FoundUser = NULL;
        return CM_NOT_FOUND;
    }

    *FoundUser = user;
    return CM_SUCCESS;
}

CM_ERROR UserConnectionCreate(CM_USER_CONNECTION** UserConnection, CM_SERVER_CLIENT* Client)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == UserConnection || NULL == Client)
        return CM_INVALID_PARAMETER;

    PCM_USER_CONNECTION userConnection = (PCM_USER_CONNECTION)malloc(sizeof(CM_USER_CONNECTION));
    if (NULL == userConnection)
    {
        *UserConnection = NULL;
        return CM_NO_MEMORY;
    }

    userConnection->Client = Client;

    AcquireSRWLockExclusive(gCountGuard);
    if (gConnectionsCount < gMaxConnections)
    {
        userConnection->ServerAccepted = TRUE;
        gConnectionsCount++;
    }
    else
    {
        userConnection->ServerAccepted = FALSE;
    }
    ReleaseSRWLockExclusive(gCountGuard);

    *UserConnection = userConnection;
    return CM_SUCCESS;
}

void UserConnectionDestroy(CM_USER_CONNECTION * UserConnection)
{
    if (UserConnection)
    {
        AbandonClient(UserConnection->Client);
        if (UserConnection->ServerAccepted)
        {
            AcquireSRWLockExclusive(gCountGuard);
            gConnectionsCount--;
            ReleaseSRWLockExclusive(gCountGuard);
        }
        free(UserConnection);
    }
}

CM_ERROR UserConnectionAdd(CM_USER_CONNECTION * UserConnection)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == UserConnection)
        return CM_INVALID_PARAMETER;

    AcquireSRWLockExclusive(gConnectionsGuard);
    InsertHeadList(gConnections, &UserConnection->Entry);
    ReleaseSRWLockExclusive(gConnectionsGuard);
    return CM_SUCCESS;
}

CM_ERROR UserConnectionRemove(CM_USER_CONNECTION * UserConnection)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == UserConnection)
        return CM_INVALID_PARAMETER;

    AcquireSRWLockExclusive(gConnectionsGuard);
    RemoveEntryList(&UserConnection->Entry);
    ReleaseSRWLockExclusive(gConnectionsGuard);
    return CM_SUCCESS;
}