#include <math.h>
#include "cache.h"
#include "memory.h"

static unsigned long cache_hits;
static unsigned long cache_misses;

typedef struct cache_request {
    uint64_t tag;
    uint64_t index;
    uint64_t offset;
} cache_request_t;

typedef struct metadata {
    uint64_t tag;
    uint8_t valid_bit;
    uint8_t dirty_bit;
} metadata_t;

typedef struct cache_line {
    metadata_t* metadata;
    data_t* data;
    cache_line_t* next;
} cache_line_t;

typedef struct cache {
    size_t cache_size;
    size_t line_size;
    uint8_t assosiativity;
    size_t num_sets;
    replacement_fn replacement_policy;
    write_fn write_policy;
    cache_line_t** pool;
} cache_t;

cache_t* create_cache(int cache_size, int line_size, int assosiativity, replacement_fn replacement_policy, write_fn write_policy) {
    cache_t* cache = malloc(sizeof(cache_t));
    cache->cache_size = cache_size;
    cache->line_size = line_size;
    cache->assosiativity = assosiativity;
    cache->replacement_policy = replacement_policy;
    cache->write_policy = write_policy;
    cache->num_sets = cache_size / line_size / assosiativity;
    cache->pool = malloc(cache->num_sets);
}

cache_request_t create_cache_request(cache_t* cache, uint64_t address) {
    int num_offset_bits = log(cache->line_size);
    int num_index_bits = log(cache->line_size);
    int num_tag_bits = 32 - num_offset_bits - num_index_bits;

    cache_request_t req;
    req.tag = address & (((int)(pow(2, num_tag_bits+1))-1) << 32);
    req.index = address & ((int)(pow(2, num_index_bits+1)-1) << (32 - num_tag_bits)) % cache->num_sets;
    req.offset = address & ((int)(pow(2, num_offset_bits+1)) - 1);

    return req;
}

cache_line_t* create_cache_line(cache_t* cache, uint64_t address) {
    cache_line_t* cache_line = malloc(sizeof(cache_line_t));
    
    metadata_t* metadata = malloc(sizeof(metadata_t));
    metadata->tag = create_cache_request(cache, address).tag;
    metadata->valid_bit = 256;
    metadata->dirty_bit = 0;

    cache_line->metadata = metadata;
    cache_line->data = malloc(cache->line_size);
    cache_line->next = NULL;

    // Read line_size bytes from RAM
    memcpy(cache_line->data, (uint8_t*)address, cache->line_size);

    return cache_line;
}

void cache_seek(cache_t* cache, uint64_t address, data_t* data) {
    cache_request_t req = create_cache_request(cache, address);
    cache_line_t* cache_line = cache->pool[req.index];
    int num_cache_lines = 0;
    while (cache_line != NULL) {
        num_cache_lines++;

        if (req.tag == cache_line->metadata->tag) {
            *data = cache_line->data[req.offset];
            cache_hits++;
            return;
        }

        cache_line = cache_line->next;
    }

    // CACHE MISS!!!!
    cache_misses++;
    // replacement policy?
    // this uses fifo
    cache_line_t* new_cache_line = create_cache_line(cache, address);
    new_cache_line->next = cache->pool[req.index];
    cache->pool[req.index] = new_cache_line;

    int target_lines_in_set = cache->cache_size / cache->line_size;
    if (num_cache_lines >= target_lines_in_set) {
        cache_line = new_cache_line;
        for (int i = 1; i < target_lines_in_set; i++) {
            cache_line = cache_line->next;
        }

        assert(cache_line->next != NULL);
        free(cache_line->next->metadata);
        free(cache_line->next->data);
        free(cache_line->next);
        cache_line->next = NULL;
    }

    *data = new_cache_line->data[req.offset];
}

void cache_write(cache_t* cache, uint64_t address, data_t* data) {  }

/* REPLACEMENT POLICIES */

void replacement_fifo(cache_t* cache, uint32_t address, data_t* data) {
    cache_request_t req = create_cache_request(cache, address);
    
    cache_line_t* cache_line = cache->pool[req.index];
    size_t line_counter = 0;
    if (cache_line != NULL) {
        line_counter = 1;
        for (; cache_line->next != NULL; line_counter++) {
            cache_line = cache_line->next;
        }
    }

    cache_line_t* new_cache_line = create_cache_line(cache, address);
    new_cache_line->next = cache->pool[req.index];
    cache->pool[req.index] = new_cache_line;
    *data = new_cache_line->data[req.offset];

    if (line_counter < cache->assosiativity) 
        return;
    if (cache_line != NULL) {
        free(cache_line->next->metadata);
        free(cache_line->next->data);
        free(cache_line->next);
        cache_line->next = NULL;
    }
}

/* WRITE POLICIES */
void write_back() {  }

void write_through() {  }