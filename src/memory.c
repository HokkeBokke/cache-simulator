/** @file memory.c
 *  @brief Implements starting point for a memory hierarchy with caching and
 * RAM.
 *  @see memory.h
 */

#include "memory.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

static unsigned long instr_count;

static cache_t* l1_inst_cache;
static cache_t* l1_data_cache;
static cache_t* l2_cache;

void memory_init(cache_params_t l1_inst_params, cache_params_t l1_data_params, cache_params_t l2_params)
{
  /* Initialize memory subsystem here. */
  l2_cache = create_cache(l2_params, NULL);
  l1_inst_cache = create_cache(l1_inst_params, l2_cache);
  l1_data_cache = create_cache(l1_data_params, l2_cache);
  
  instr_count = 0;
}

void memory_fetch(uint32_t* address, data_t *data)
{
  //printf("memory: fetch %p\n", address);
  if (data)
    *data = (data_t)0;

  if (!cache_seek(l1_inst_cache, (uint64_t)address, data)) {
    cache_seek(l2_cache, (uint64_t)address, data);
  }
  
  instr_count++;
}

void memory_read(double* address, data_t *data)
{
  //printf("memory: read %p\n", address);
  if (data)
    *data = (data_t)0;

  if (!cache_seek(l1_data_cache, (uint64_t)address, data)) {
    cache_seek(l2_cache, (uint64_t)address, data);
  }
}

void memory_write(double* address, data_t *data)
{
  //printf("memory: write %p\n", address);

  if (!cache_write(l1_data_cache, (uint64_t)address, data)) {
    cache_write(l2_cache, (uint64_t)address, data);
  }
}

static cache_stats_t get_cache_stats(cache_t* cache)
{
  cache_stats_t stats = {
    .hits = cache_hits(cache),
    .misses = cache_misses(cache),
    .writes = cache_writes(cache),
  };
  return stats;
}

void memory_get_stats(memory_stats_t* stats)
{
  stats->l1_inst = get_cache_stats(l1_inst_cache);
  stats->l1_data = get_cache_stats(l1_data_cache);
  stats->l2 = get_cache_stats(l2_cache);
}

void memory_finish(void)
{
  fprintf(stdout, "Executed %lu instructions.\n", instr_count);
  fprintf(stdout, "L1 instr cache:\t%llu cache hits\n", cache_hits(l1_inst_cache));
  fprintf(stdout, "L1 instr cache:\t%llu cache misses\n", cache_misses(l1_inst_cache));
  fprintf(stdout, "L1 instr cache:\t%llu cache writes\n\n", cache_writes(l1_inst_cache));
  fprintf(stdout, "L1 data cache:\t%llu cache hits\n", cache_hits(l1_data_cache));
  fprintf(stdout, "L1 data cache:\t%llu cache misses\n", cache_misses(l1_data_cache));
  fprintf(stdout, "L1 data cache:\t%llu cache writes\n\n", cache_writes(l1_data_cache));
  fprintf(stdout, "L2 cache:\t%llu cache hits\n", cache_hits(l2_cache));
  fprintf(stdout, "L2 cache:\t%llu cache misses\n", cache_misses(l2_cache));
  fprintf(stdout, "L2 cache:\t%llu cache writes\n\n", cache_writes(l2_cache));

  memory_destroy();
}

void memory_destroy(void)
{
  /* Deinitialize memory subsystem here */
  destroy_cache(l1_inst_cache);
  destroy_cache(l1_data_cache);
  destroy_cache(l2_cache);
}
