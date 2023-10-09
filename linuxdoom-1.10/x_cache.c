#include "x_cache.h"
#include "i_system.h"

#ifdef XQD
#include "xqd.h"
#include "z_zone.h"
#include "base64.h"

const int CacheLookupState_Found = 1 << 0; // 1
const int CacheLookupState_Usable = 1 << 1; // 2
const int CacheLookupState_Stale = 1 << 2; // 4
const int CacheLookupState_MustInsertOrUpdate = 1 << 3; // 8
const int CacheWriteOptionsMask_RequestHeaders = 1 << 1;
const int CacheWriteOptionsMask_VaryRule = 1 << 2;
const int CacheWriteOptionsMask_InitialAgeNs = 1 << 3;
const int CacheWriteOptionsMask_StaleWhileRevalidateNs = 1 << 4;
const int CacheWriteOptionsMask_SurrogateKeys = 1 << 5;
const int CacheWriteOptionsMask_Length = 1 << 6;
const int CacheWriteOptionsMask_UserMetadata = 1 << 7;
const int CacheWriteOptionsMask_SensitiveData = 1 << 8;
const int CacheGetBodyOptionsMask_From = 1 << 1;
const int CacheGetBodyOptionsMask_To = 1 << 2;

byte*
X_GetStateFromCache(boolean global, int stateId, int* outlen)
{
	char stateString[256];
	snprintf(stateString, 256, "%d", stateId);
	CacheHandle cache_handle;

	// do a lookup
	CacheLookupOptionsMask lookup_mask = {0};
	int ret = lookup(stateString, strlen(stateString), lookup_mask, NULL, &cache_handle);

	CacheLookupState lookup_state;
	ret = get_state(cache_handle, &lookup_state);

	if (lookup_state.state & CacheLookupState_Found) {
		CacheGetBodyOptionsMask mask = {0};
		BodyHandle body_handle = {0};
		ret = get_body(cache_handle, mask, NULL, &body_handle);

		byte* buf = Z_Malloc(200000,PU_STATIC,0);
		int bodyindex=0;
		int nread;
		do {
			int ret = xqd_body_read(body_handle, buf+bodyindex, 200000-bodyindex, &nread);
			bodyindex += nread;
		} while (nread > 0 && bodyindex < 200000);

		// TODO to fully bullet proof this we could keep reading and allocating until
		// we have the entire body
		if (bodyindex == 200000 ) {
			I_Error("We may be truncating the deserialized data");
		}
		*outlen = bodyindex;
		return buf;
	} else {
		*outlen = 0;
		return NULL;
	}
}

int64_t ms_to_ns(int64_t ms) {
	return ms * 1000000;
}

int64_t s_to_ns(int64_t s) {
	return ms_to_ns(s*1000);
}

void
X_WriteStateToCache(boolean global, int stateId, byte* state, int len)
{
	char stateString[256];
	snprintf(stateString, 256, "%d", stateId);
	CacheHandle cache_handle;

	// do a lookup
	CacheLookupOptionsMask lookup_mask = {0};
	int ret = lookup(stateString, strlen(stateString), lookup_mask, NULL, &cache_handle);

	CacheLookupState lookup_state;
	ret = get_state(cache_handle, &lookup_state);

	if (lookup_state.state == 0 || lookup_state.state & CacheLookupState_Stale) {
		CacheWriteOptionsMask write_mask = {0};
		write_mask.mask = CacheWriteOptionsMask_Length | CacheWriteOptionsMask_StaleWhileRevalidateNs;
		CacheWriteOptions write_options = {0};
		write_options.max_age_ns =  0;
		write_options.stale_while_revalidate_ns = s_to_ns(30);
		write_options.length = len;

		BodyHandle body_handle = {0};
		ret = insert(stateString, strlen(stateString), write_mask, &write_options, &body_handle);
		fflush(stdout);

		size_t total_written = 0;
		while (total_written < len) {
			size_t written = 0;
			ret = write_body_handle(body_handle, &state[total_written], len-total_written, 0, &written); 
			total_written += written;
			if (ret != 0) break;
		}

		ret = close_body_handle(body_handle);
	}
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