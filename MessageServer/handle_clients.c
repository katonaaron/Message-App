#include "stdafx.h"
#include "handle_clients.h"

DWORD WINAPI ReceiveClients(LPVOID Param)
{
    DWORD result = 1;

    CM_SERVER_DATA* server_data = (CM_SERVER_DATA*)Param;
    UINT64 i = 0;

    while (TRUE)
    {
        result = WaitForSingleObject(server_data->EndEvent, 0);
        if (WAIT_OBJECT_0 == result)
        {
            return 0;
        }

        _tprintf_s(TEXT("%lld\n"), ++i);
        Sleep(1000);
    }
    return 0;
}
