#include "common.h"

void PrintError(CM_ERROR Code, const TCHAR * Function)
{
    _tprintf_s(TEXT("Unexpected error: %s failed with error code: 0x%X: \n"), Function, Code);
}
