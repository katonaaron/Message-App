#pragma once

#include "stdafx.h"
#include "UtilityLib.h"

typedef struct _CM_SERVER_DATA
{
    CM_SERVER* Server;
    INT64 MaxConnections;
    HANDLE EndEvent;
} CM_SERVER_DATA;

DWORD WINAPI ReceiveClients(LPVOID Param);