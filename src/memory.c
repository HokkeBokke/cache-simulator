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

#define L1_CACHE_SIZE 32'000
#define L2_CACHE_SIZE 512'000

static unsigned long instr_count;

static cache_t* l1_inst_cache;
static cache_t* l1_data_cache;
static cache_t* l2_cache;

void memory_init(void)
{
  /* Initialize memory subsystem here. */
  
  l1_inst_cache = create_cache(L1_CACHE_SIZE, 64, 4/*-way*/, NULL, NULL);

  instr_count = 0;
}

void memory_fetch(uint32_t* address, data_t *data)
{
  printf("memory: fetch %p\n", address);
  if (data)
    *data = (data_t)0;

  cache_seek(l1_inst_cache, (uint64_t)address, data);
  
  instr_count++;
}

void memory_read(double* address, data_t *data)
{
  printf("memory: read %p\n", address);
  if (data)
    *data = (data_t)0;

  instr_count++;
}

void memory_write(double* address, data_t *data)
{
  printf("memory: write %p\n", address);

  instr_count++;
}

void memory_finish(void)
{
  fprintf(stdout, "Executed %lu instructions.\n\n", instr_count);

  /* Deinitialize memory subsystem here */
}
