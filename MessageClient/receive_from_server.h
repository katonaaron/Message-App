#pragma once

#include "stdafx.h"
#include "communication_api.h"
#include "UtilityLib.h"

typedef struct _CM_RECEIVER_DATA
{
    CM_CLIENT* Client;
    HANDLE StartEvent;
    BOOL* Connected;
} CM_RECEIVER_DATA;


DWORD WINAPI ReceiveFromServer(LPVOID Param);