#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <inttypes.h>
#include "common.h"

typedef struct cache_request cache_request_t;
typedef struct metadata metadata_t;
typedef struct cache_line cache_line_t;
typedef struct cache cache_t;

typedef void (*replacement_fn)(cache_t* cache, uint32_t address, data_t* data, cache_request_t* req);
typedef void (*write_fn)(cache_t* cache, cache_line_t* cache_line, uint32_t address, data_t* data);

cache_t* create_cache(int cache_size, int line_size, int assosiativity, replacement_fn replacement_policy, write_fn write_policy, cache_t* parent_cache);
void destroy_cache(cache_t* cache);

/**
 * Check if address data is cached
 * 
 * @param cache: cache to search in
 * @param address: address where data is stored
 * @param data: pointer to destination for data
 * @returns 1 on cache hit, 0 on miss
 */
uint8_t cache_seek(cache_t* cache, uint64_t address, data_t* data);

uint8_t cache_write(cache_t* cache, uint64_t address, data_t* data);

uint64_t cache_hits(cache_t* cache);
uint64_t cache_misses(cache_t* cache);
uint64_t cache_writes(cache_t* cache);

/* REPLACEMENT POLICIES */
void replacement_fifo(cache_t* cache, uint32_t address, data_t* data, cache_request_t* req);

/* WRITE POLICIES */
void write_back(cache_t* cache, cache_line_t* cache_line, uint32_t address, data_t* data);
void write_through(cache_t* cache, cache_line_t* cache_line, uint32_t address, data_t* data);


#endif
