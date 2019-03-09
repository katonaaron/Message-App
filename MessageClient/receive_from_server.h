#pragma once

#include "common.h"

typedef struct _CM_CLIENT_CONNECTION
{
    CM_CLIENT* Client;
    HANDLE StartStopEvent;
    BOOL IsConnected;
} CM_CLIENT_CONNECTION, *PCM_CLIENT_CONNECTION;


DWORD WINAPI ReceiveFromServer(LPVOID Param);