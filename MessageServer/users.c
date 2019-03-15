#include "users.h"

//Globals
PLIST_ENTRY gConnections;
PLIST_ENTRY gUsers;
PSRWLOCK gUsersGuard, gConnectionsGuard, gCountGuard;
INT gConnectionsCount, gMaxConnections;
BOOL gIsUsersInitialized = FALSE;

CM_ERROR InitUsersModule(INT MaxConnections)
{
    if (MaxConnections <= 0)
        return CM_INVALID_PARAMETER;

    static LIST_ENTRY connections, users;
    InitializeListHead(&connections);
    InitializeListHead(&users);
    gConnections = &connections;
    gUsers = &users;

    static SRWLOCK usersGuard = SRWLOCK_INIT, connectionsGuard = SRWLOCK_INIT, countGuard = SRWLOCK_INIT;
    gUsersGuard = &usersGuard;
    gConnectionsGuard = &connectionsGuard;
    gCountGuard = &countGuard;

    gConnectionsCount = 0;
    gMaxConnections = MaxConnections;

    gIsUsersInitialized = TRUE;
    return CM_SUCCESS;
}

CM_ERROR UserCreate(CM_USER ** User, TCHAR * Username)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == User || NULL == Username)
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

    *User = user;
    return CM_SUCCESS;
}

void UserDestroy(CM_USER * User)
{
    if (User != NULL)
    {
        free(User->Username);
        UserConnectionDestroy(User->Connection);
        free(User);

    }
}

CM_ERROR UserAdd(CM_USER * User)
{
    if (!gIsUsersInitialized)
        return CM_MODULE_NOT_INITIALIZED;
    if (NULL == User)
        return CM_INVALID_PARAMETER;

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
    User->IsLoggedIn = 1;
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
        user = CONTAINING_RECORD(user, CM_USER, Entry);
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