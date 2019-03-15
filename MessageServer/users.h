#pragma once

#include "common.h"
#include "list.h"

typedef struct _CM_USER_CONNECTION
{
	LIST_ENTRY Entry;
	CM_SERVER_CLIENT* Client;
	BOOL ServerAccepted;
}CM_USER_CONNECTION, *PCM_USER_CONNECTION;

typedef struct _CM_USER
{
	LIST_ENTRY Entry;
	TCHAR* Username;
	BOOL IsLoggedIn;
	CM_USER_CONNECTION* Connection;
}CM_USER, *PCM_USER;

CM_ERROR InitUsersModule(INT MaxConnections);

//CM_USER functions
CM_ERROR UserCreate(CM_USER** User, TCHAR* Username);
void UserDestroy(CM_USER* User);

CM_ERROR UserAdd(CM_USER* User);
CM_ERROR UserLogIn(CM_USER* User, CM_USER_CONNECTION* UserConnection);
CM_ERROR UserLogOut(CM_USER* User);
CM_ERROR UserFind(TCHAR* Username, CM_USER** FoundUser);

//CM_USER_CONNECTION functions
CM_ERROR UserConnectionCreate(CM_USER_CONNECTION** UserConnection, CM_SERVER_CLIENT* Client);
void UserConnectionDestroy(CM_USER_CONNECTION* UserConnection);

CM_ERROR UserConnectionAdd(CM_USER_CONNECTION* UserConnection);
CM_ERROR UserConnectionRemove(CM_USER_CONNECTION* UserConnection);

//Globals
extern PLIST_ENTRY gConnections;
extern PLIST_ENTRY gUsers;
extern PSRWLOCK gUsersGuard, gConnectionsGuard, gCountGuard;
extern INT gConnectionsCount, gMaxConnections;
extern BOOL gIsUsersInitialized;