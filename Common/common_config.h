#pragma once

#include "common.h"

#define GET_MESSAGE_SIZE(MessageBufferSize) (MessageBufferSize + sizeof(CM_MESSAGE))

#define MAX_INPUT_LENGTH 256
#define MAX_INPUT_BUFFER_SIZE (MAX_INPUT_LENGTH * sizeof(TCHAR))
#define MAX_MESSAGE_BUFFER_SIZE MAX_INPUT_BUFFER_SIZE
#define MAX_MESSAGE_SIZE GET_MESSAGE_SIZE(MAX_MESSAGE_BUFFER_SIZE)