#pragma once

#include "common.h"

#define GET_MESSAGE_SIZE(MessageBufferSize) ((MessageBufferSize) + sizeof(CM_MESSAGE))
#define GET_STRING_SIZE(StringLength) ((StringLength) * sizeof(TCHAR))

#define MAX_INPUT_LENGTH 256
#define MAX_INPUT_BUFFER_SIZE GET_STRING_SIZE(MAX_INPUT_LENGTH)
#define MAX_MESSAGE_BUFFER_LENGTH (2 * MAX_INPUT_LENGTH)
#define MAX_MESSAGE_BUFFER_SIZE GET_STRING_SIZE(MAX_MESSAGE_BUFFER_LENGTH)
#define MAX_MESSAGE_SIZE GET_MESSAGE_SIZE(MAX_MESSAGE_BUFFER_SIZE)