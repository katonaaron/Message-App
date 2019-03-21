#pragma once

#include "common.h"

typedef enum _CM_OP_CODE {
    CM_ECHO,
    CM_REGISTER,
    CM_LOGIN,
    CM_LOGOUT,
    CM_MSG,
    CM_BROADCAST,
    CM_SENDFILE,
    CM_LIST,
    CM_HISTORY,
    CM_CONNECT,
    CM_EXIT,
    CM_MSG_TEXT
} CM_OP_CODE;

typedef struct _CM_MESSAGE
{
    CM_OP_CODE Operation;
    CM_SIZE Size;
    DWORD Parts;
    CM_SIZE TotalSize;
    BYTE Buffer[1];
} CM_MESSAGE;

CM_ERROR SendMessageToServer(CM_CLIENT* Client, void* Message, CM_SIZE MessageSize, CM_OP_CODE Operation);
CM_ERROR SendMessageToClient(CM_SERVER_CLIENT* Client, void* Message, CM_SIZE MessageSize, CM_OP_CODE Operation);
CM_ERROR ReceiveMessageFromServer(CM_CLIENT* Client, CM_MESSAGE** Message);
CM_ERROR ReceiveMessageFromClient(CM_SERVER_CLIENT* Client, CM_MESSAGE** Message);
