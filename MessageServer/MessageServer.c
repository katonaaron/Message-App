// MessageServer.cpp : Defines the entry point for the console application.
//

#include "common.h"
#include "users.h"
#include "handle_clients.h"

//TODO: Put this somewhere else
#define MAX_THREADS 10
#define MIN_THREADS 1

int ReadMaxConnections(TCHAR* String, INT* MaxConnections)
{
    if (NULL == String || NULL == MaxConnections)
    {
        return -1;
    }

    for (TCHAR* c = String; *c != 0; c++)
    {
        if (!isdigit(*c))
        {
            return -1;
        }
    }

    *MaxConnections = _tstoi(String);

    if (*MaxConnections <= 0)
    {
        return -1;
    }

    return 0;
}

int _tmain(int argc, TCHAR* argv[])
{
    INT retVal = 0, rollback = 0;
    CM_ERROR error = CM_SUCCESS;

    CM_SERVER* server = NULL;
    CM_SERVER_CLIENT* client = NULL;
    CM_DATA_BUFFER* received = NULL;   

    //Threadpool variables
    PTP_POOL pool = NULL;
    PTP_WORK work = NULL;
    TP_CALLBACK_ENVIRON callBackEnviron;
    PTP_CLEANUP_GROUP cleanupgroup = NULL;

    INT maxConnections;
    CM_USER_CONNECTION* userConnection = NULL;

    if (argc < 2 || ReadMaxConnections(argv[1], &maxConnections) != 0)
    {
        _tprintf_s(TEXT("Error: invalid maximum number of connections\n"));
        retVal = -1;
        goto main_cleanup;
    }

    //DEBUG
    EnableCommunicationModuleLogger();

    error = InitUsersModule(maxConnections);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("InitUsersModule"));
        retVal = -1;
        goto main_cleanup;
    }

    error = InitCommunicationModule();
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("InitCommunicationModule"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 1;
    
    error = CreateServer(&server);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CreateServer"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 2;

    error = CreateDataBuffer(&received, MAX_MESSAGE_SIZE);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CreateDataBuffer"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 3;

    InitializeThreadpoolEnvironment(&callBackEnviron);
    pool = CreateThreadpool(NULL);
    if (NULL == pool) {
        PrintError(GetLastError(), TEXT("CreateThreadpool"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 4;

    SetThreadpoolThreadMaximum(pool, maxConnections);
    if (!SetThreadpoolThreadMinimum(pool, MIN_THREADS)) //TODO: This is not max
    {
        PrintError(GetLastError(), TEXT("SetThreadpoolThreadMinimum"));
        retVal = -1;
        goto main_cleanup;
    }

    cleanupgroup = CreateThreadpoolCleanupGroup();
    if (NULL == cleanupgroup) {
        PrintError(GetLastError(), TEXT("CreateThreadpoolCleanupGroup"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback = 5;

    SetThreadpoolCallbackPool(&callBackEnviron, pool);
    SetThreadpoolCallbackCleanupGroup(&callBackEnviron, cleanupgroup, NULL);


    _tprintf_s(TEXT("Success\n"));
    while (TRUE)
    {
        error = AwaitNewClient(server, &client);
        if (CM_IS_ERROR(error))
        {
            PrintError(error, TEXT("AwaitNewClient"));
            retVal = -1;
            goto main_cleanup;
        }
        rollback = 6;  

        error = UserConnectionCreate(&userConnection, client);
        if (CM_IS_ERROR(error))
        {
            PrintError(error, TEXT("UserConnectionCreate"));
            retVal = -1;
            goto main_cleanup;
        }
        rollback = 7;

        work = CreateThreadpoolWork(ProcessClient, userConnection, &callBackEnviron);
        if (NULL == work) {
            PrintError(error, TEXT("CreateThreadpoolWork"));
            retVal = -1;
            goto main_cleanup;
        }
        rollback = 5;

        SubmitThreadpoolWork(work);
    }

main_cleanup:
    switch (rollback)
    {
    case 7:
        UserConnectionDestroy(userConnection);
        client = NULL;
    case 6:
        AbandonClient(client);
    case 5:
        CloseThreadpoolCleanupGroupMembers(cleanupgroup, TRUE, NULL);
        CloseThreadpoolCleanupGroup(cleanupgroup);
    case 4:
        CloseThreadpool(pool);
    case 3:
        DestroyDataBuffer(received);
    case 2:
        DestroyServer(server);
    case 1:
        UninitCommunicationModule();
    default:
        UninitUsersModule();
        break;
    }

    return retVal;
}

