/** @file memory.h
 *  @brief Public API of memory hierarchy.
 *  @see memory.c
 */

#ifndef MEMORY_H
#define MEMORY_H
#include <stdint.h>

/* data_t can be any 64-bit type, such as (uint64_t), (void *).
 */
typedef uint64_t data_t;

/** Initialize memory hierarchy.
 */
void memory_init(void);

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

/** Clean up and deinitialize memory hierarchy.
 */
void memory_finish(void);

#endif
