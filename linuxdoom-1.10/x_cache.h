#ifndef __X_CACHE_H_
#define __X_CACHE_H_

#include "doomtype.h"

struct session;

// functionality to write game state to a cache
byte* X_GetStateFromCache(boolean global, int stateId, int* outlen);
void X_WriteStateToCache(boolean global, int stateId, byte* state, int len);

const char* X_GetSessions(boolean global);

#endif
