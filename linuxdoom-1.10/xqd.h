#ifndef xqd_H
#define xqd_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define XQD_ABI_VERSION 0x01ULL

// TODO ACF 2020-01-17: these aren't very C-friendly names
typedef struct {
    uint32_t handle;
} RequestHandle;

typedef struct {
    uint32_t handle;
} ResponseHandle;

typedef struct {
    uint32_t handle;
} BodyHandle;

typedef struct {
    uint32_t handle;
} LogEndpointHandle;

typedef struct {
    uint32_t handle;
} DictionaryHandle;

typedef enum BodyWriteEnd {
    BodyWriteEndBack  = 0,
    BodyWriteEndFront = 1,
} BodyWriteEnd;

typedef struct {
    uint32_t mask;
} CacheLookupOptionsMask;

typedef struct {
    RequestHandle request_headers;
} CacheLookupOptions;

typedef struct {
    uint32_t handle;
} CacheHandle;

typedef struct {
    uint32_t state;
} CacheLookupState;


typedef struct {
    uint64_t max_age_ns;
    RequestHandle request_headers;
    byte* vary_rule_ptr;
    size_t vary_rule_len;
    uint64_t initial_age_ns;
    uint64_t stale_while_revalidate_ns;
    byte* surrogate_keys_ptr;
    size_t surrogate_keys_len;
    uint64_t length;
    byte* user_metadata_ptr;
    size_t user_metadata_len;
} CacheWriteOptions;

typedef struct {
    uint32_t mask;
} CacheWriteOptionsMask;

typedef struct {
    uint64_t from;
    uint64_t to;
} CacheGetBodyOptions;

typedef struct {
    uint32_t mask;
} CacheGetBodyOptionsMask;

#define CACHE_OVERRIDE_NONE (0u)
#define CACHE_OVERRIDE_PASS (1u<<0)
#define CACHE_OVERRIDE_TTL (1u<<1)
#define CACHE_OVERRIDE_STALE_WHILE_REVALIDATE (1u<<2)
#define CACHE_OVERRIDE_PCI (1u<<3)

// TODO ACF 2019-12-05: nicer type for the return value (XqdStatus)

int xqd_body_append(BodyHandle dst_handle, BodyHandle src_handle);

int xqd_body_new(BodyHandle *handle_out);

int xqd_body_read(BodyHandle body_handle, char *buf, size_t buf_len, size_t *nread);

int xqd_body_write(BodyHandle body_handle, const char *buf, size_t buf_len, BodyWriteEnd end,
                   size_t *nwritten);

int xqd_init(uint64_t abi_version);

int xqd_req_body_downstream_get(RequestHandle *req_handle_out, BodyHandle *body_handle_out);

/**
 * Set the cache override behavior for this request.
 *
 * The default behavior, equivalent to `CACHE_OVERRIDE_NONE`, respects the cache control headers
 * from the origin's response.
 *
 * Calling this function with `CACHE_OVERRIDE_PASS` will ignore the subsequent arguments and Pass
 * unconditionally.
 *
 * To override, TTL, stale-while-revalidate, or stale-with-error, set the appropriate bits in the
 * tag using the corresponding constants, and pass the override values in the appropriate arguments.
 *
 * xqd_req_cache_override_v2_set also includes an optional Surrogate-Key which will be set or added
 * to any received from the origin.
 */
int xqd_req_cache_override_set(RequestHandle req_handle, int tag, uint32_t ttl,
                               uint32_t stale_while_revalidate);
int xqd_req_cache_override_v2_set(RequestHandle req_handle, int tag, uint32_t ttl,
                                  uint32_t stale_while_revalidate,
                                  const char *surrogate_key, size_t surrogate_key_len);

int xqd_req_header_append(RequestHandle req_handle, const char *name, size_t name_len,
                          const char *value, size_t value_len);

int xqd_req_header_insert(RequestHandle req_handle, const char *name, size_t name_len,
                          const char *value, size_t value_len);

int xqd_req_header_remove(RequestHandle req_handle, const char *name, size_t name_len);

int xqd_req_header_names_get(RequestHandle req_handle, char *buf, size_t buf_len, uint32_t cursor,
                             int64_t *ending_cursor, size_t *nwritten);

int xqd_req_header_value_get(RequestHandle req_handle, const char *name, size_t name_len,
                              char *value, size_t value_max_len, size_t *nwritten);

int xqd_req_header_values_get(RequestHandle req_handle, const char *name, size_t name_len,
                              char *buf, size_t buf_len, uint32_t cursor, int64_t *ending_cursor,
                              size_t *nwritten);

int xqd_req_header_values_set(RequestHandle req_handle, const char *name, size_t name_len,
                              const char *values, size_t values_len);

int xqd_req_method_get(RequestHandle req_handle, char *method, size_t method_max_len,
                       size_t *nwritten);

int xqd_req_method_set(RequestHandle req_handle, const char *method, size_t method_len);

int xqd_req_new(RequestHandle *req_handle);

int xqd_req_send(RequestHandle req_handle, BodyHandle body_handle, const char *backend,
                 size_t backend_len, ResponseHandle *resp_handle_out,
                 BodyHandle *resp_body_handle_out);

int xqd_req_uri_get(RequestHandle req_handle, char *uri, size_t uri_max_len, size_t *nwritten);

int xqd_req_uri_set(RequestHandle req_handle, const char *uri, size_t uri_len);

int xqd_req_version_get(RequestHandle req_handle, uint32_t *version);

int xqd_req_version_set(RequestHandle req_handle, uint32_t version);

int xqd_resp_header_append(ResponseHandle resp_handle, const char *name, size_t name_len,
                           const char *value, size_t value_len);

int xqd_resp_header_insert(ResponseHandle resp_handle, const char *name, size_t name_len,
                           const char *value, size_t value_len);

int xqd_resp_header_remove(ResponseHandle resp_handle, const char *name, size_t name_len);

int xqd_resp_header_names_get(ResponseHandle resp_handle, char *buf, size_t buf_len,
                              uint32_t cursor, int64_t *ending_cursor, size_t *nwritten);

int xqd_resp_header_value_get(ResponseHandle resp_handle, const char *name, size_t name_len,
                              char *value, size_t value_max_len, size_t *nwritten);

int xqd_resp_header_values_get(ResponseHandle resp_handle, const char *name, size_t name_len,
                               char *buf, size_t buf_len, uint32_t cursor, int64_t *ending_cursor,
                               size_t *nwritten);

int xqd_resp_header_values_set(ResponseHandle resp_handle, const char *name, size_t name_len,
                               const char *buf, size_t buf_len);

int xqd_resp_new(ResponseHandle *resp_handle_out);

int xqd_resp_send_downstream(ResponseHandle resp_handle, BodyHandle body_handle, uint32_t streaming);

int xqd_resp_status_get(ResponseHandle resp_handle, uint16_t *status);

int xqd_resp_status_set(ResponseHandle resp_handle, uint16_t status);

int xqd_resp_version_get(ResponseHandle resp_handle, uint32_t *version);

int xqd_resp_version_set(ResponseHandle resp_handle, uint32_t version);

int xqd_uap_parse(const char *user_agent, size_t user_agent_len, char *family,
                  size_t family_max_len, size_t *family_nwritten, char *major, size_t major_max_len,
                  size_t *major_nwritten, char *minor, size_t minor_max_len, size_t *minor_nwritten,
                  char *patch, size_t patch_max_len, size_t *patch_nwritten);

int xqd_log_endpoint_get(const char *name, size_t name_len, LogEndpointHandle *endpoint_handle);

int xqd_log_write(LogEndpointHandle endpoint_handle, const char* msg, size_t msg_len,
                  size_t *nwritten);

int xqd_req_original_header_count(uint32_t *count);

__attribute__((import_module("fastly_cache")))
int lookup(const char* cache_key, size_t cache_key_len, CacheLookupOptionsMask options_mask, CacheLookupOptions *options, CacheHandle *handle_out);

__attribute__((import_module("fastly_cache")))
int insert(const char* cache_key, size_t cache_key_len, CacheWriteOptionsMask options_mask, CacheWriteOptions *options, BodyHandle *body_handle_out);

__attribute__((import_module("fastly_cache")))
int get_state(CacheHandle handle, CacheLookupState *lookup_state_out);

__attribute__((import_module("fastly_cache")))
int get_body(CacheHandle handle, CacheGetBodyOptionsMask mask, CacheGetBodyOptions* options, BodyHandle *handle_out);

__attribute__((import_module("fastly_http_body")))
__attribute__((import_name("write")))
int write_body_handle(BodyHandle body_handle, byte* buf, size_t buf_len, BodyWriteEnd end, size_t *written);

__attribute__((import_module("fastly_http_body")))
__attribute__((import_name("close")))
int close_body_handle(BodyHandle body_handle);


#ifdef __cplusplus
}
#endif
#endif