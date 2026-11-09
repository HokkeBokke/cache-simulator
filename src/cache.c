#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "cache.h"

typedef struct metrics {
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t writes;
} metrics_t;

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
    replacement_policy_t replacement_policy;
    write_fn write_policy;
    cache_line_t** pool;
    cache_t* parent_cache;
    metrics_t metrics;
} cache_t;

cache_t* create_cache(int cache_size, int line_size, int assosiativity, replacement_policy_t replacement_policy, write_fn write_policy, cache_t* parent_cache) {
    cache_t* cache = malloc(sizeof(cache_t));
    cache->cache_size = cache_size;
    cache->line_size = line_size;
    cache->assosiativity = assosiativity;
    cache->replacement_policy = replacement_policy;
    cache->write_policy = write_policy;
    cache->num_sets = cache_size / line_size / assosiativity;
    cache->pool = calloc(cache->num_sets, sizeof(cache_line_t*));
    cache->parent_cache = parent_cache;
    cache->metrics.cache_hits = 0;
    cache->metrics.cache_misses = 0;
    cache->metrics.writes = 0;

    return cache;
}

void destroy_cache(cache_t* cache) {
    for (int i = 0; i < cache->num_sets; i++) {
        cache_line_t* line = cache->pool[i];
        while (line != NULL) {
            cache_line_t* next = line->next;
            free(line->metadata);
            free(line->data);
            free(line);
            line = next;
        }
    }
    free(cache->pool);
    free(cache);
    cache = NULL;
}

cache_request_t create_cache_request(cache_t* cache, uint64_t address) {
    if (cache == NULL || address == 0) {
        printf("create_cache_request: Missing argument: %s\n", cache == NULL ? "cache" : "address");
        exit(-1);
    }

    int num_offset_bits = log2(cache->line_size);
    int num_index_bits = (uint64_t)log2(cache->num_sets);

    cache_request_t req = {
        .tag = address >> (num_index_bits + num_offset_bits),
        .index = (address >> num_offset_bits) & (cache->num_sets - 1),
        .offset = address & ((int)(pow(2, num_offset_bits)) - 1),
    };

    return req;
}

cache_line_t* create_cache_line(cache_t* cache, uint64_t address, cache_request_t* req) {
    cache_line_t* cache_line = malloc(sizeof(cache_line_t));
    if (cache_line == NULL) {
        printf("Could not allocate memory for cache line\n");
        exit(-1);
    }
    
    metadata_t* metadata = malloc(sizeof(metadata_t));
    if (metadata == NULL) {
        printf("Could not allocate memory for cache line metadata\n");
        exit(-1);
    }
    metadata->tag = req->tag;
    metadata->valid_bit = 1;
    metadata->dirty_bit = 0;

    cache_line->metadata = metadata;
    cache_line->data = malloc(cache->line_size);
    cache_line->next = NULL;

    // Read line_size bytes from RAM
    // memcpy(cache_line->data, (void*)address, cache->line_size);

    return cache_line;
}

uint8_t cache_seek(cache_t* cache, uint64_t address, data_t* data) {
    cache_request_t req = create_cache_request(cache, address);
    cache_line_t* cache_line = cache->pool[req.index];
    int num_cache_lines = 0;
    while (cache_line != NULL) {
        num_cache_lines++;

        if (req.tag == cache_line->metadata->tag) {
            if (data)
                *data = cache_line->data[req.offset];

            cache->replacement_policy.access(cache, cache_line, req);

            cache->metrics.cache_hits++;
            return 1;
        }

        cache_line = cache_line->next;
    }

    // CACHE MISS!!!!
    cache->metrics.cache_misses++;
    
    cache->replacement_policy.replacement(cache, address, data, &req);

    //*data = new_cache_line->data[req.offset];

    return 0;
}

uint8_t cache_write(cache_t* cache, uint64_t address, data_t* data) {
    cache_request_t req = create_cache_request(cache, address);
    cache_line_t* cache_line = cache->pool[req.index];
    int num_cache_lines = 0;
    while (cache_line != NULL) {
        num_cache_lines++;

        if (req.tag == cache_line->metadata->tag) {
            if (data)
                cache_line->data[req.offset] = *data;
            cache->metrics.cache_hits++;
            cache->metrics.writes++;
            cache->write_policy(cache, cache_line, address, data);
            return 1;
        }

        cache_line = cache_line->next;
    }

    // CACHE MISS!!!!
    cache->metrics.cache_misses++;

    cache->replacement_policy.replacement(cache, address, data, &req);

    // new_cache_line->data[req.offset] = *data;

    return 0;
}

uint64_t cache_hits(cache_t* cache) {
    return cache->metrics.cache_hits;
}

uint64_t cache_misses(cache_t* cache) {
    return cache->metrics.cache_misses;
}

uint64_t cache_writes(cache_t* cache) {
    return cache->metrics.writes;
}

/* REPLACEMENT POLICIES */

void replacement_fifo(cache_t* cache, uint32_t address, data_t* data, cache_request_t* req) {
    cache_line_t* last_cache_line = cache->pool[req->index];
    cache_line_t* sec_last_cache_line = NULL;
    size_t line_counter = 0;
    if (last_cache_line != NULL) {
        line_counter = 1;
        for (; last_cache_line->next != NULL; line_counter++) {
            sec_last_cache_line = last_cache_line;
            last_cache_line = last_cache_line->next;
        }
    }

    cache_line_t* new_cache_line = create_cache_line(cache, address, req);
    new_cache_line->next = cache->pool[req->index];
    cache->pool[req->index] = new_cache_line;
    //*data = new_cache_line->data[req->offset];

    if (line_counter < cache->assosiativity) 
        return;

    if (last_cache_line != NULL) {
        if (cache->parent_cache && last_cache_line->metadata->dirty_bit) {
            cache->parent_cache->metrics.writes++;
        }

        free(last_cache_line->metadata);
        free(last_cache_line->data);
        free(last_cache_line);
        if (sec_last_cache_line == NULL)
            sec_last_cache_line = new_cache_line;
        if (sec_last_cache_line != NULL)
            sec_last_cache_line->next = NULL;
    }
}

void access_fifo(cache_t* cache, cache_line_t* cache_line, cache_request_t req) {
    // do nothing
}

void replacement_lru(cache_t* cache, uint32_t address, data_t* data, cache_request_t* req) {
    cache_line_t* last_cache_line = cache->pool[req->index];
    cache_line_t* sec_last_cache_line = NULL;
    size_t line_counter = 0;
    if (last_cache_line != NULL) {
        line_counter = 1;
        for (; last_cache_line->next != NULL; line_counter++) {
            sec_last_cache_line = last_cache_line;
            last_cache_line = last_cache_line->next;
        }
    }

    cache_line_t* new_cache_line = create_cache_line(cache, address, req);
    new_cache_line->next = cache->pool[req->index];
    cache->pool[req->index] = new_cache_line;
    //*data = new_cache_line->data[req->offset];

    if (line_counter < cache->assosiativity) 
        return;

    if (last_cache_line != NULL) {
        if (cache->parent_cache && last_cache_line->metadata->dirty_bit) {
            cache->parent_cache->metrics.writes++;
        }

        free(last_cache_line->metadata);
        free(last_cache_line->data);
        free(last_cache_line);
        if (sec_last_cache_line == NULL)
            sec_last_cache_line = new_cache_line;
        if (sec_last_cache_line != NULL)
            sec_last_cache_line->next = NULL;
    }
}

// move cache line to front of list, keeping list sorted
// from most recently used to least recently used.
void access_lru(cache_t* cache, cache_line_t* cache_line, cache_request_t req) {
    cache_line_t* prev = cache->pool[req.index];
    if (prev == cache_line) return;
    while (prev->next != cache_line) prev = prev->next;

    prev->next = cache_line->next;
    cache_line->next = cache->pool[req.index];
    cache->pool[req.index] = cache_line;
}

/* WRITE POLICIES */
void write_back(cache_t* cache, cache_line_t* cache_line, uint32_t address, data_t* data) {
    cache_line->metadata->dirty_bit = 1;
}

void write_through(cache_t* cache, cache_line_t* cache_line, uint32_t address, data_t* data) {
    if (cache->parent_cache) {
        cache_write(cache->parent_cache, address, data);
    }
}
