/** @file memory.h
 *  @brief Public API of memory hierarchy.
 *  @see memory.c
 */

#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>
#include "common.h"
#include "cache.h"

/** Hits, misses and writes of a single cache.
 */
typedef struct cache_stats {
    uint64_t hits;
    uint64_t misses;
    uint64_t writes;
} cache_stats_t;

/** Statistics of the whole memory hierarchy.
 */
typedef struct memory_stats {
    cache_stats_t l1_inst;
    cache_stats_t l1_data;
    cache_stats_t l2;
} memory_stats_t;

/** Initialize memory hierarchy.
 */
void memory_init(cache_params_t l1_inst_params, cache_params_t l1_data_params, cache_params_t l2_params);

/** Fetch instruction at given memory address.
 *
 *  @param[in] address Memory address of instruction.
 *  @param[out] data Instruction data returned by reference.
 */
void memory_fetch(uint32_t* address, data_t *data);

/** Read data from memory at given memory address.
 *
 *  @param[in] address Memory address to read from.
 *  @param[out] data Memory data returned by reference.
 */
void memory_read(double* address, data_t *data);

/** Write to memory at given memory address.
 *
 *  @param[in] address Memory address to write to.
 *  @param[in] data Data to write to memory.
 */
void memory_write(double* address, data_t *data);

/** Get hit, miss and write counts of all caches. Call before memory_finish.
 *
 *  @param[out] stats Statistics returned by reference.
 */
void memory_get_stats(memory_stats_t* stats);

/** Deinitialize memory hierarchy without printing statistics.
 */
void memory_destroy(void);

/** Print statistics, then clean up and deinitialize memory hierarchy.
 */
void memory_finish(void);

#endif
