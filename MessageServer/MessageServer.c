// MessageServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "UtilityLib.h"
#include "handle_clients.h"

#define MAX_THREADS 10
#define MIN_THREADS 1

INT64 gCurrentConnections = 0;

int ReadMaxConnections(TCHAR* String, INT64* MaxConnections)
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

    *MaxConnections = _tstoi64(String);

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
    CM_SERVER_CLIENT_DATA data;
    CM_DATA_BUFFER* received = NULL;
    INT64 maxConnections;

    //Threadpool variables
    PTP_POOL pool = NULL;
    PTP_WORK work = NULL;
    TP_CALLBACK_ENVIRON callBackEnviron;
    PTP_CLEANUP_GROUP cleanupgroup = NULL;

    if (argc < 2 || ReadMaxConnections(argv[1], &maxConnections) != 0)
    {
        _tprintf_s(TEXT("Error: invalid maximum number of connections\n"));
        retVal = -1;
        goto main_cleanup;
    }
    _tprintf_s(TEXT("Success\n"));

    //DEBUG
    EnableCommunicationModuleLogger();

    error = InitCommunicationModule();
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("InitCommunicationModule"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;
    
    error = CreateServer(&server);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CreateServer"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;

    error = CreateDataBuffer(&received, sizeof(CM_MESSAGE) + MAX_BUFFER_SIZE);
    if (CM_IS_ERROR(error))
    {
        PrintError(error, TEXT("CreateDataBuffer"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;

    InitializeThreadpoolEnvironment(&callBackEnviron);
    pool = CreateThreadpool(NULL);
    if (NULL == pool) {
        PrintError(GetLastError(), TEXT("CreateThreadpool"));
        retVal = -1;
        goto main_cleanup;
    }
    rollback++;

    SetThreadpoolThreadMaximum(pool, MIN_THREADS);
    if (!SetThreadpoolThreadMinimum(pool, MAX_THREADS))
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
    rollback++;

    SetThreadpoolCallbackPool(&callBackEnviron, pool);
    SetThreadpoolCallbackCleanupGroup(&callBackEnviron, cleanupgroup, NULL);

    data.Server = server;
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
        data.Client = client;      

        //TODO: set or list
        data.Accept = gCurrentConnections < maxConnections;
        if (data.Accept)
        {
            InterlockedIncrement64(&gCurrentConnections);
        }

        work = CreateThreadpoolWork(ProcessClient, &data, &callBackEnviron);
        if (NULL == work) {
            PrintError(error, TEXT("CreateThreadpoolWork"));
            retVal = -1;
            goto main_cleanup;
        }
        rollback = 7;

        SubmitThreadpoolWork(work);
    }

main_cleanup:
    switch (rollback)
    {
    case 7: //TODO 6,7 not clear
        CloseThreadpoolCleanupGroupMembers(cleanupgroup, FALSE, NULL);
    case 6:
        AbandonClient(client);
    case 5:
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
        break;
    }

    return retVal;
}

