#pragma once

#include "common.h"
#include "list.h"

typedef struct _CM_USER_CONNECTION
{
	LIST_ENTRY Entry;
	CM_SERVER_CLIENT* Client;
	BOOL ServerAccepted;
}CM_USER_CONNECTION, *PCM_USER_CONNECTION;

typedef struct _CM_USER_FILE
{
    SRWLOCK FileGuard;
    HANDLE File;
    LARGE_INTEGER FilePointer;
}CM_USER_FILE, *PCM_USER_FILE;

typedef struct _CM_USER
{
	LIST_ENTRY Entry;
	TCHAR* Username;
    TCHAR* Password;
	BOOL IsLoggedIn;
	CM_USER_CONNECTION* Connection;
    CM_USER_FILE File;
}CM_USER, *PCM_USER;

CM_ERROR InitUsersModule(INT MaxConnections);
void UninitUsersModule();

//CM_USER functions
CM_ERROR UserCreate(CM_USER** User, TCHAR* Username, TCHAR* Password);
void UserDestroy(CM_USER* User);

CM_ERROR UserAdd(CM_USER* User, BOOL WriteToFile);
CM_ERROR UserLogIn(CM_USER* User, CM_USER_CONNECTION* UserConnection);
CM_ERROR UserLogOut(CM_USER* User);
CM_ERROR UserIsLoggedIn(CM_USER* User, BOOL* IsLoggedIn);
CM_ERROR UserFind(TCHAR* Username, CM_USER** FoundUser);

CM_ERROR UserSendMessage(CM_USER* Sender, CM_USER* Receiver, TCHAR* Message);
CM_ERROR UserReceiveOfflineMessages(CM_USER* User);

//CM_USER_CONNECTION functions
CM_ERROR UserConnectionCreate(CM_USER_CONNECTION** UserConnection, CM_SERVER_CLIENT* Client);
void UserConnectionDestroy(CM_USER_CONNECTION* UserConnection);

CM_ERROR UserConnectionAdd(CM_USER_CONNECTION* UserConnection);
CM_ERROR UserConnectionRemove(CM_USER_CONNECTION* UserConnection);