#pragma once

#include "common.h"

#define CM_NOT_FOUND  MAKE_ERROR_CODE(CM_ERROR_SEVERITY, 0xA1)
#define CM_MODULE_NOT_INITIALIZED MAKE_ERROR_CODE(CM_ERROR_SEVERITY, 0xA2)

void PrintError(CM_ERROR Code, const TCHAR* Function);
void PrintErrorMessage(const TCHAR* Message);