#pragma once

#include "common.h"

typedef enum _CM_VALIDATION
{
    CM_CREDENTIALS_OK = 0,
    CM_INVALID_USERNAME,
    CM_INVALID_PASSWORD,
    CM_WEAK_PASSWORD
}CM_VALIDATION, *PCM_VALIDATION;

void ReadCommands(CM_CLIENT* Client, HANDLE* ReceiverThread);
CM_VALIDATION Validate(TCHAR* Username, TCHAR* Password);