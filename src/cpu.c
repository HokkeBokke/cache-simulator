/** @file cpu.c
 *  @brief This code simulates a system with caching between the CPU and the
 *  main memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "memory.h"

#define N 256  // Matrix size
#define CLS 64  // Cache Line Size
#define SM (CLS/sizeof(double))  // Nb of double in each cache line

#define BENCHMARK       NAIVE

typedef enum {
    NAIVE,
    BLOCK
} benchmark_t;

void naive_mat_mul(double* a, double* b, double* c, uint32_t* instruction_num) {
	memory_fetch(instruction_num, NULL);
	for(int i = 0; i < N; i++) {
		memory_fetch(instruction_num+1, NULL);
		for(int j = 0; j < N; j++) {
			memory_fetch(instruction_num+2, NULL);
			for(int k = 0; k < N; k++) {
				//c[N*i + j] += a[N*i + k] * b[N*k + j];
				memory_fetch(instruction_num+3, NULL);
				memory_read(a+N*i + k, NULL);
				memory_fetch(instruction_num+4, NULL);
				memory_read(b+N*k + j, NULL);
				memory_fetch(instruction_num+5, NULL);
				memory_read(c+N*i + j, NULL);
				memory_fetch(instruction_num+6, NULL);
				memory_write(c+N*i + j, NULL);
				memory_fetch(instruction_num+7, NULL);
			}
    }
  }
}

void block_mat_mul(double* a, double* b, double* c, uint32_t* instruction_num) {
	int i, j, k, i2, k2, j2;
	double *rres, *rmul1, *rmul2;
	memory_fetch(instruction_num, NULL);
	for (i = 0; i < N; i += SM) {
		memory_fetch(instruction_num+1, NULL);
		for (j = 0; j < N; j += SM) {
			memory_fetch(instruction_num+2, NULL);
			for (k = 0; k < N; k += SM) {
				memory_fetch(instruction_num+3, NULL);
				for (i2 = 0, rres = &c[i*N+j], rmul1 = &a[i*N+k]; i2 < SM; ++i2, rres += N, rmul1 += N) {
					memory_fetch(instruction_num+4, NULL);
					for (k2 = 0, rmul2 = &b[k*N+j]; k2 < SM; ++k2, rmul2 += N) {
						memory_fetch(instruction_num+5, NULL);
						for (j2 = 0; j2 < SM; ++j2) {
							// rres[j2] += rmul1[k2] * rmul2[j2];
							memory_fetch(instruction_num+6, NULL);
							memory_read(rmul1+k2, NULL);
							memory_fetch(instruction_num+7, NULL);
							memory_read(rmul2+j2, NULL);
							memory_fetch(instruction_num+8, NULL);
							memory_read(rres+j2, NULL);
							memory_fetch(instruction_num+9, NULL);
							memory_write(rres+j2, NULL);
							memory_fetch(instruction_num+10, NULL);
						}
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
  memory_init();
	double* a = malloc(N*N * sizeof(double));
	double* b = malloc(N*N * sizeof(double));
	double* c = malloc(N*N * sizeof(double));
	static uint32_t instruction_num[16];
	if(BENCHMARK == NAIVE) {
		naive_mat_mul(a, b, c, instruction_num);
	} else {
		block_mat_mul(a, b, c, instruction_num);
	}
  memory_finish(); /* Deinitialize the memory subsystem */
  return 0;
}
