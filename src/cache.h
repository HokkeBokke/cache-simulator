#include <stdint.h>
#include <inttypes.h>

typedef void (*replacement_fn)(uint32_t);
typedef void (*write_fn)(uint32_t);

typedef struct cache_request cache_request_t;
typedef struct metadata metadata_t;
typedef struct cache_line cache_line_t;
typedef struct cache cache_t;

cache_t* create_cache(int cache_size, int line_size, int assosiativity, replacement_fn replacement_policy, write_fn write_policy);
void cache_seek(cache_t* cache, uint64_t address, data_t* data);
void cache_write(cache_t* cache, uint64_t address, data_t* data);

/* REPLACEMENT POLICIES */
void replacement_fifo(cache_t* cache, uint32_t address, data_t* data);

/* WRITE POLICIES */