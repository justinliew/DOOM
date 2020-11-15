#include "x_cache.h"

#ifdef XQD
#include "xqd.h"
#include "z_zone.h"

int X_ReqUriSet(boolean global, RequestHandle reqHandle, const char* str)
{
	const char* globaluri = "http://kv-global.vranish.dev/%s";
	const char* popuri = "http://kv.vranish.dev/%s";
	const char* uriformat = global ? globaluri : popuri;
	char uri[256];
	snprintf(uri, 256, uriformat, str);
	int res = xqd_req_uri_set(reqHandle, uri, strlen(uri));
	printf("X_GetStateFromCache::xqd_req_uri_set returned %d for %s\n", res, uri);
	return res;
}

const char* X_GetUriName(boolean global)
{
	const char* globalname = "kvglobal";
	const char* popname = "kvlocal";
	const char* name = global ? globalname : popname;
	return name;
}

byte*
X_GetStateFromCache(boolean global, int stateId, int* outlen)
{
	RequestHandle reqHandle;
	BodyHandle bodyHandle, respBodyHandle;
	int res = xqd_req_new(&reqHandle);
	res = xqd_body_new(&bodyHandle);
	char stateString[256];
	snprintf(stateString, 256, "%d", stateId);
	printf("X_GetStateFromCache with %d becomes %s\n", stateId, stateString);
	X_ReqUriSet(global, reqHandle, stateString);
	const char* name = X_GetUriName(global);

	ResponseHandle respHandle;
	res = xqd_req_send(reqHandle, bodyHandle, name, strlen(name), &respHandle, &respBodyHandle );

	byte* buf = Z_Malloc(50000,PU_STATIC,0);
	int bodyindex=0;
	int nread;
	do {
		int ret = xqd_body_read(respBodyHandle, buf+bodyindex, 50000-bodyindex, &nread);
		bodyindex += nread;
	} while (nread > 0 && bodyindex < 50000);

	printf("X_GetStateFromCache read returned %d, nread: %d\n", res, bodyindex);

	*outlen = bodyindex;
	return buf;
}

void
X_WriteStateToCache(boolean global, int stateId, byte* state, int len)
{
	RequestHandle reqHandle;
	BodyHandle bodyHandle;
	int res = xqd_req_new(&reqHandle);
	res = xqd_body_new(&bodyHandle);

	char stateString[256];
	snprintf(stateString, 256, "%d", stateId);
	X_ReqUriSet(global, reqHandle, stateString);
	const char* name = X_GetUriName(global);

	int b64len;
	byte* b64state = base64_encode(state, len, &b64len);
	res = xqd_req_header_append(reqHandle, "do-post-base64", strlen("do-post-base64"), b64state, b64len);

	res = xqd_req_method_set(reqHandle, "POST", strlen("POST"));

	ResponseHandle respHandle;

	res = xqd_req_send(reqHandle, bodyHandle, name, strlen(name), &respHandle, NULL );
	printf("X_WriteStateToCache::xqd_req_send returned %d; wrote %d bytes (encoded from %d bytes)\n", res, b64len, len);
}

/*
Sessions live in cache
When do they need updating?
Whenever someone joins a session. This one is easy to update.
Whenever someone leaves a session. The leave one is a bit tricky especially if it's the last player who leaves since we will just stop. I suppose when someone lists sessions, we can prune then.
So this means we will likely need to do processing on C@E for this, rather than just sending the raw JSON back.

{
	"sessions" :
		[
			{"id" : "abcdef", "num_players" : 2, "players" : [{"id", "name"}]},
			{"id" : "cdefgh", "num_players" : 1}
		]
}*/

#endif // XQD