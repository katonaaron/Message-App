#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "stdafx.h"

typedef enum {
    CM_ECHO,
    CM_REGISTER,
    CM_LOGIN,
    CM_LOGOUT,
    CM_MSG,
    CM_BROADCAST,
    CM_SENDFILE,
    CM_LIST,
    CM_HISTORY,
    CM_FAILED
} CM_OP_CODE;

typedef struct _CM_MESSAGE
{
    CM_OP_CODE Operation;
    CM_SIZE Size;
    BYTE Buffer[1];
} CM_MESSAGE;

#endif // _MESSAGE_H_