#include "common_validation.h"

CM_VALIDATION ValidateUsernamePassword(TCHAR * Username, TCHAR * Password)
{
    if (NULL == Username || *Username == 0)
    {
        return CM_INVALID_USERNAME;
    }
    if (NULL == Password)
    {
        return CM_INVALID_PASSWORD;
    }

    for (TCHAR* c = Username; *c; c++)
    {
        if (!_istalnum(*c))
        {
            return CM_INVALID_USERNAME;
        }
    }

    int length = 0;

    BOOL hasCapitalLetter = FALSE, hasSpecialChar = FALSE;

    for (TCHAR* c = Password; *c; c++)
    {
        if (*c == TEXT(',') || *c == TEXT(' '))
        {
            return CM_INVALID_PASSWORD;
        }

        if (!_istalnum(*c))
        {
            hasSpecialChar = TRUE;
        }
        else if (!_istupper(*c))
        {
            hasCapitalLetter = TRUE;
        }
        if (length < 5)
        {
            length++;
        }
    }

    if (!hasCapitalLetter || !hasSpecialChar || length < 5)
    {
        return CM_WEAK_PASSWORD;
    }

    return CM_VALIDATION_OK;
}