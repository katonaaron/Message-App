#pragma once

#include "stdafx.h"
#include "common.h"

typedef struct _CM_SERVER_CLIENT_DATA
{
    CM_SERVER* Server;
    CM_SERVER_CLIENT* Client;
    BOOL Accept;
} CM_SERVER_CLIENT_DATA;

//DWORD WINAPI ReceiveClients(LPVOID Param);
VOID CALLBACK ProcessClient(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK  Work);