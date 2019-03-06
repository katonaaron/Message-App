#include "stdafx.h"
#include "client_operations.h"

void ReadCommand(CM_CLIENT * Client)
{
    UNREFERENCED_PARAMETER(Client);
    TCHAR buffer[MAX_INPUT_LENGTH], *word = NULL, *next_token = NULL;
    BOOL exit = FALSE;

    while (!exit)
    {
        _tscanf_s(TEXT("%s"), buffer, MAX_INPUT_LENGTH);

        word = _tcstok_s(buffer, TEXT(" "), &next_token);

        while (word != NULL && !exit)
        {
            if (_tcscmp(word, TEXT("echo")) == 0)
            {
                
            }
            else if (_tcscmp(word, TEXT("register")) == 0)
            {

            }
            else if (_tcscmp(word, TEXT("login")) == 0)
            {

            }
            else if (_tcscmp(word, TEXT("msg")) == 0)
            {

            }
            else if (_tcscmp(word, TEXT("broadcast")) == 0)
            {

            }
            else if (_tcscmp(word, TEXT("sendfile")) == 0)
            {

            }
            else if (_tcscmp(word, TEXT("list")) == 0)
            {

            }
            else if (_tcscmp(word, TEXT("history")) == 0)
            {

            }
            else if (_tcscmp(word, TEXT("exit")) == 0)
            {
                exit = TRUE;
            }

            if (!exit)
            {
                word = _tcstok_s(NULL, TEXT(" "), &next_token);
            }
        }
    }
}
