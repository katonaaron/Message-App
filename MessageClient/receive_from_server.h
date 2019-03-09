#pragma once

#include "stdafx.h"
#include "communication_api.h"
#include "UtilityLib.h"

typedef struct _CM_CLIENT_CONNECTION
{
    CM_CLIENT* Client;
    HANDLE StartStopEvent;
    BOOL IsConnected;
} CM_CLIENT_CONNECTION;


DWORD WINAPI ReceiveFromServer(LPVOID Param);