#include "common.h"

void PrintError(CM_ERROR Code, const TCHAR * Function)
{
    _tprintf_s(TEXT("Unexpected error: %s failed with error code: 0x%X: \n"), Function, Code);
}

void PrintErrorMessage(const TCHAR * Message)
{
    _tprintf_s(TEXT("Unexpected error: %s\n"), Message);
}
