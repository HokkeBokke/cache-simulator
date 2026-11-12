/** @file cpu.c
 *  @brief This code simulates a system with caching between the CPU and the
 *  main memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "memory.h"
#include "cache.h"

#define N 256  // Matrix size
#define CLS 64  // Cache Line Size
#define SM (CLS/sizeof(double))  // Nb of double in each cache line

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

// the data is never touched, only the addresses matter. Fixed addresses
// keep results identical between runs.
#define MATRIX_A_ADDRESS 0x10000000
#define MATRIX_B_ADDRESS 0x20000000
#define MATRIX_C_ADDRESS 0x30000000
#define INSTRUCTION_ADDRESS 0x08000000

#define L1_CACHE_SIZE 32768   // 32 KB
#define L2_CACHE_SIZE 524288  // 512 KB

void print_usage(char* program) {
    printf("Usage: %s [option value]...\n", program);
    printf("  --benchmark     naive | block                (default naive)\n");
    printf("  --l1-size       size of both L1 caches in bytes (default %d)\n", L1_CACHE_SIZE);
    printf("  --l2-size       size of the L2 cache in bytes   (default %d)\n", L2_CACHE_SIZE);
    printf("  --l1i-assoc     associativity of L1 instruction cache (default 4)\n");
    printf("  --l1d-assoc     associativity of L1 data cache        (default 8)\n");
    printf("  --l2-assoc      associativity of L2 cache             (default 8)\n");
    printf("  --line-size     line size of all caches in bytes      (default 64)\n");
    printf("  --replacement   lru | fifo | random (all caches)      (default lru)\n");
    printf("  --write-policy  write-back | write-through (L1 data and L2) (default write-back)\n");
}

// print a message about the command line and stop
void argument_error(char* program, char* message, char* argument) {
    printf("%s: %s\n\n", message, argument);
    print_usage(program);
    exit(-1);
}

replacement_policy_t parse_replacement(char* program, char* name) {
    if (strcmp(name, "lru") == 0) return REPLACEMENT_POLICY_LRU;
    if (strcmp(name, "fifo") == 0) return REPLACEMENT_POLICY_FIFO;
    if (strcmp(name, "random") == 0) return REPLACEMENT_POLICY_RANDOM;
    argument_error(program, "Unknown replacement policy", name);
    return REPLACEMENT_POLICY_LRU;
}

write_fn parse_write_policy(char* program, char* name) {
    if (strcmp(name, "write-back") == 0) return write_back;
    if (strcmp(name, "write-through") == 0) return write_through;
    argument_error(program, "Unknown write policy", name);
    return write_back;
}

// the number of sets must be a whole power of two
void check_cache_params(char* program, char* name, cache_params_t params) {
    if (params.cache_size == 0 || params.line_size == 0 || params.associativity == 0) {
        argument_error(program, "Size, line size and associativity must be larger than 0 for", name);
    }
    size_t line_bytes_per_set = params.line_size * params.associativity;
    if (params.cache_size % line_bytes_per_set != 0) {
        argument_error(program, "Size must be a multiple of line size * associativity for", name);
    }
    size_t num_sets = params.cache_size / line_bytes_per_set;
    if ((num_sets & (num_sets - 1)) != 0 || (params.line_size & (params.line_size - 1)) != 0) {
        argument_error(program, "Number of sets and line size must be powers of two for", name);
    }
}

// everything that can be set from the command line
typedef struct config {
    benchmark_t benchmark;
    cache_params_t l1_inst_params;
    cache_params_t l1_data_params;
    cache_params_t l2_params;
} config_t;

config_t default_config(void) {
    config_t config = {
        .benchmark = NAIVE,
        .l1_inst_params = {
            .cache_size = L1_CACHE_SIZE,
            .associativity = 4,
            .line_size = 64,
            .replacement_policy = REPLACEMENT_POLICY_LRU,
            .write_policy = NULL,
        },
        .l1_data_params = {
            .cache_size = L1_CACHE_SIZE,
            .associativity = 8,
            .line_size = 64,
            .replacement_policy = REPLACEMENT_POLICY_LRU,
            .write_policy = write_back,
        },
        .l2_params = {
            .cache_size = L2_CACHE_SIZE,
            .associativity = 8,
            .line_size = 64,
            .replacement_policy = REPLACEMENT_POLICY_LRU,
            .write_policy = write_back,
        },
    };
    return config;
}

benchmark_t parse_benchmark(char* program, char* name) {
    if (strcmp(name, "naive") == 0) return NAIVE;
    if (strcmp(name, "block") == 0) return BLOCK;
    argument_error(program, "Unknown benchmark", name);
    return NAIVE;
}

// set the part of the config that one option controls
void set_option(char* program, config_t* config, char* option, char* value) {
    if (strcmp(option, "--benchmark") == 0) {
        config->benchmark = parse_benchmark(program, value);
    } else if (strcmp(option, "--l1-size") == 0) {
        config->l1_inst_params.cache_size = atoi(value);
        config->l1_data_params.cache_size = atoi(value);
    } else if (strcmp(option, "--l2-size") == 0) {
        config->l2_params.cache_size = atoi(value);
    } else if (strcmp(option, "--l1i-assoc") == 0) {
        config->l1_inst_params.associativity = atoi(value);
    } else if (strcmp(option, "--l1d-assoc") == 0) {
        config->l1_data_params.associativity = atoi(value);
    } else if (strcmp(option, "--l2-assoc") == 0) {
        config->l2_params.associativity = atoi(value);
    } else if (strcmp(option, "--line-size") == 0) {
        config->l1_inst_params.line_size = atoi(value);
        config->l1_data_params.line_size = atoi(value);
        config->l2_params.line_size = atoi(value);
    } else if (strcmp(option, "--replacement") == 0) {
        config->l1_inst_params.replacement_policy = parse_replacement(program, value);
        config->l1_data_params.replacement_policy = parse_replacement(program, value);
        config->l2_params.replacement_policy = parse_replacement(program, value);
    } else if (strcmp(option, "--write-policy") == 0) {
        config->l1_data_params.write_policy = parse_write_policy(program, value);
        config->l2_params.write_policy = parse_write_policy(program, value);
    } else {
        argument_error(program, "Unknown option", option);
    }
}

void check_config(char* program, config_t config) {
    check_cache_params(program, "L1 instruction cache", config.l1_inst_params);
    check_cache_params(program, "L1 data cache", config.l1_data_params);
    check_cache_params(program, "L2 cache", config.l2_params);
}

// options come in pairs: --name value
config_t parse_arguments(int argc, char* argv[]) {
    config_t config = default_config();
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
        if (i + 1 >= argc) {
            argument_error(argv[0], "Missing value for", argv[i]);
        }
        set_option(argv[0], &config, argv[i], argv[i + 1]);
    }
    check_config(argv[0], config);
    return config;
}

void run_benchmark(benchmark_t benchmark) {
    double* a = (double*)MATRIX_A_ADDRESS;
    double* b = (double*)MATRIX_B_ADDRESS;
    double* c = (double*)MATRIX_C_ADDRESS;
    uint32_t* instruction_num = (uint32_t*)INSTRUCTION_ADDRESS;
    if (benchmark == NAIVE) {
        naive_mat_mul(a, b, c, instruction_num);
    } else {
        block_mat_mul(a, b, c, instruction_num);
    }
}

int main(int argc, char *argv[])
{
  config_t config = parse_arguments(argc, argv);

  srand(1);  // random replacement gives the same result every run
  memory_init(config.l1_inst_params, config.l1_data_params, config.l2_params);
  run_benchmark(config.benchmark);
  memory_finish();
  return 0;
}
