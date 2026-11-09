/** @file memory.c
 *  @brief Implements starting point for a memory hierarchy with caching and
 * RAM.
 *  @see memory.h
 */

#include "memory.h"
#include "cache.h"

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define L1_CACHE_SIZE 32768   // 32 KB
#define L2_CACHE_SIZE 524288  // 512 KB

static unsigned long instr_count;

static cache_t* l1_inst_cache;
static cache_t* l1_data_cache;
static cache_t* l2_cache;

void memory_init(void)
{
  /* Initialize memory subsystem here. */
  
  l2_cache = create_cache(L2_CACHE_SIZE, 64, 8/*-way*/, replacement_fifo, write_back, NULL);
  l1_inst_cache = create_cache(L1_CACHE_SIZE, 64, 4/*-way*/, replacement_fifo, write_back, l2_cache);
  l1_data_cache = create_cache(L1_CACHE_SIZE, 64, 8/*-way*/, replacement_fifo, write_back, l2_cache);
  
  instr_count = 0;
}

void memory_fetch(uint32_t* address, data_t *data)
{
  printf("memory: fetch %p\n", address);
  if (data)
    *data = (data_t)0;

  if (!cache_seek(l1_inst_cache, (uint64_t)address, data)) {
    cache_seek(l2_cache, (uint64_t)address, data);
  }
  
  instr_count++;
}

void memory_read(double* address, data_t *data)
{
  printf("memory: read %p\n", address);
  if (data)
    *data = (data_t)0;

  if (!cache_seek(l1_data_cache, (uint64_t)address, data)) {
    cache_seek(l2_cache, (uint64_t)address, data);
  }
}

void memory_write(double* address, data_t *data)
{
  printf("memory: write %p\n", address);

  if (!cache_write(l1_data_cache, (uint64_t)address, data)) {
    cache_write(l2_cache, (uint64_t)address, data);
  }
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

  /* Deinitialize memory subsystem here */
  destroy_cache(l1_inst_cache);
  destroy_cache(l1_data_cache);
  destroy_cache(l2_cache);
}
