#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include "common.h"
#include "memory.h"

#define N 256  // Matrix size
#define CLS 64  // Cache Line Size
#define SM (CLS/sizeof(double))  // Nb of double in each cache line

// access latencies in cycles, used to compute average memory access time
#define L1_LATENCY 4
#define L2_LATENCY 12
#define MEMORY_LATENCY 200

// the data is never touched, only the addresses matter. Fixed addresses
// keep results identical between runs (malloc addresses change with ASLR).
#define MATRIX_A_ADDRESS 0x10000000
#define MATRIX_B_ADDRESS 0x20000000
#define MATRIX_C_ADDRESS 0x30000000
#define INSTRUCTION_ADDRESS 0x08000000

typedef enum {
    NAIVE,
    BLOCK
} benchmark_t;

typedef enum {
    LRU,
    FIFO,
    RANDOM
} replacement_t;

typedef enum {
    WRITE_BACK,
    WRITE_THROUGH
} write_t;

typedef struct config {
    size_t l1_size;
    size_t l2_size;
    uint8_t l1_inst_associativity;
    uint8_t l1_data_associativity;
    uint8_t l2_associativity;
    size_t line_size;
    replacement_t replacement;
    write_t write;
} config_t;

// one parameter and the values it is tested with
typedef struct sweep {
    const char* parameter;
    int num_values;
    int values[5];
} sweep_t;

static sweep_t sweeps[] = {
    { "l1_size", 3, { 16384, 32768, 65536 } },      // 16KB, 32KB, 64KB
    { "l2_size", 3, { 262144, 524288, 1048576 } },  // 256KB, 512KB, 1024KB
    { "associativity", 5, { 1, 2, 4, 8, 16 } },
    { "line_size", 4, { 16, 32, 64, 128 } },
    { "write_policy", 2, { WRITE_BACK, WRITE_THROUGH } },
    { "replacement", 3, { LRU, FIFO, RANDOM } },
};

void naive_mat_mul(double* a, double* b, double* c, uint32_t* instruction_num) {
    memory_fetch(instruction_num, NULL);
    for (int i = 0; i < N; i++) {
        memory_fetch(instruction_num+1, NULL);
        for (int j = 0; j < N; j++) {
            memory_fetch(instruction_num+2, NULL);
            for (int k = 0; k < N; k++) {
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

config_t default_config(void) {
    config_t config = {
        .l1_size = 32768,
        .l2_size = 524288,
        .l1_inst_associativity = 4,
        .l1_data_associativity = 8,
        .l2_associativity = 8,
        .line_size = 64,
        .replacement = LRU,
        .write = WRITE_BACK,
    };
    return config;
}

// set one parameter of the config, parameter names are the ones in sweeps
void set_parameter(config_t* config, const char* parameter, int value) {
    if (strcmp(parameter, "l1_size") == 0) {
        config->l1_size = value;
    } else if (strcmp(parameter, "l2_size") == 0) {
        config->l2_size = value;
    } else if (strcmp(parameter, "associativity") == 0) {
        config->l1_inst_associativity = value;
        config->l1_data_associativity = value;
        config->l2_associativity = value;
    } else if (strcmp(parameter, "line_size") == 0) {
        config->line_size = value;
    } else if (strcmp(parameter, "write_policy") == 0) {
        config->write = value;
    } else if (strcmp(parameter, "replacement") == 0) {
        config->replacement = value;
    }
}

replacement_policy_t replacement_policy(replacement_t replacement) {
    if (replacement == FIFO) return REPLACEMENT_POLICY_FIFO;
    if (replacement == RANDOM) return REPLACEMENT_POLICY_RANDOM;
    return REPLACEMENT_POLICY_LRU;
}

write_fn write_policy(write_t write) {
    return write == WRITE_THROUGH ? write_through : write_back;
}

const char* replacement_name(replacement_t replacement) {
    if (replacement == FIFO) return "fifo";
    if (replacement == RANDOM) return "random";
    return "lru";
}

const char* write_name(write_t write) {
    return write == WRITE_THROUGH ? "write_through" : "write_back";
}

const char* benchmark_name(benchmark_t benchmark) {
    return benchmark == NAIVE ? "naive" : "block";
}

// run one benchmark on a fresh memory hierarchy and return its statistics
memory_stats_t run_benchmark(benchmark_t benchmark, config_t config) {
    cache_params_t l1_inst_params = {
        .cache_size = config.l1_size,
        .associativity = config.l1_inst_associativity,
        .line_size = config.line_size,
        .replacement_policy = replacement_policy(config.replacement),
        .write_policy = NULL,  // instruction cache is read only
    };
    cache_params_t l1_data_params = {
        .cache_size = config.l1_size,
        .associativity = config.l1_data_associativity,
        .line_size = config.line_size,
        .replacement_policy = replacement_policy(config.replacement),
        .write_policy = write_policy(config.write),
    };
    cache_params_t l2_params = {
        .cache_size = config.l2_size,
        .associativity = config.l2_associativity,
        .line_size = config.line_size,
        .replacement_policy = replacement_policy(config.replacement),
        .write_policy = write_policy(config.write),
    };

    srand(1);  // random replacement gives the same result every run
    memory_init(l1_inst_params, l1_data_params, l2_params);

    double* a = (double*)MATRIX_A_ADDRESS;
    double* b = (double*)MATRIX_B_ADDRESS;
    double* c = (double*)MATRIX_C_ADDRESS;
    uint32_t* instruction_num = (uint32_t*)INSTRUCTION_ADDRESS;
    if (benchmark == NAIVE)
        naive_mat_mul(a, b, c, instruction_num);
    else
        block_mat_mul(a, b, c, instruction_num);

    memory_stats_t stats;
    memory_get_stats(&stats);
    memory_destroy();
    return stats;
}

double miss_rate(cache_stats_t cache) {
    return (double)cache.misses / (cache.hits + cache.misses);
}

// average memory access time in cycles per L1 access. Writes arriving at L2
// (write-through stores and dirty evictions) cost an L2 access each.
double amat(memory_stats_t stats) {
    uint64_t l1_accesses = stats.l1_inst.hits + stats.l1_inst.misses + stats.l1_data.hits + stats.l1_data.misses;
    uint64_t l1_misses = stats.l1_inst.misses + stats.l1_data.misses;
    uint64_t cycles = l1_accesses * L1_LATENCY
                    + l1_misses * L2_LATENCY
                    + stats.l2.misses * MEMORY_LATENCY
                    + stats.l2.writes * L2_LATENCY;
    return (double)cycles / l1_accesses;
}

void write_csv_header(FILE* file) {
    fprintf(file, "sweep,benchmark,l1_size,l2_size,l1_inst_assoc,l1_data_assoc,l2_assoc,line_size,replacement,write_policy,");
    fprintf(file, "l1_inst_hits,l1_inst_misses,l1_inst_writes,l1_data_hits,l1_data_misses,l1_data_writes,l2_hits,l2_misses,l2_writes,");
    fprintf(file, "l1_inst_miss_rate,l1_data_miss_rate,l2_miss_rate,amat\n");
}

void write_csv_row(FILE* file, const char* sweep, benchmark_t benchmark, config_t config, memory_stats_t stats) {
    fprintf(file, "%s,%s,%zu,%zu,%d,%d,%d,%zu,%s,%s,", sweep, benchmark_name(benchmark), config.l1_size, config.l2_size,
            config.l1_inst_associativity, config.l1_data_associativity, config.l2_associativity, config.line_size,
            replacement_name(config.replacement), write_name(config.write));
    fprintf(file, "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,",
            (unsigned long long)stats.l1_inst.hits, (unsigned long long)stats.l1_inst.misses, (unsigned long long)stats.l1_inst.writes,
            (unsigned long long)stats.l1_data.hits, (unsigned long long)stats.l1_data.misses, (unsigned long long)stats.l1_data.writes,
            (unsigned long long)stats.l2.hits, (unsigned long long)stats.l2.misses, (unsigned long long)stats.l2.writes);
    fprintf(file, "%.6f,%.6f,%.6f,%.4f\n", miss_rate(stats.l1_inst), miss_rate(stats.l1_data), miss_rate(stats.l2), amat(stats));
}

// run one config, store it in the csv and return its amat
double run_and_save(FILE* file, const char* sweep, benchmark_t benchmark, config_t config) {
    memory_stats_t stats = run_benchmark(benchmark, config);
    write_csv_row(file, sweep, benchmark, config, stats);
    fflush(file);
    return amat(stats);
}

int main(int argc, char* argv[]) {
    mkdir("results", 0777);
    FILE* results = fopen("results/benchmark.csv", "w");
    FILE* evaluation = fopen("results/evaluation.csv", "w");
    if (results == NULL || evaluation == NULL) {
        printf("Could not open files in results/\n");
        exit(-1);
    }
    write_csv_header(results);
    fprintf(evaluation, "benchmark,parameter,best_value,default_amat,best_amat\n");

    int num_sweeps = sizeof(sweeps) / sizeof(sweeps[0]);
    for (benchmark_t benchmark = NAIVE; benchmark <= BLOCK; benchmark++) {
        config_t best_config = default_config();
        double default_amat = run_and_save(results, "default", benchmark, best_config);
        printf("%s default: amat %.4f\n", benchmark_name(benchmark), default_amat);

        for (int s = 0; s < num_sweeps; s++) {
            sweep_t sweep = sweeps[s];
            int best_value = sweep.values[0];
            double best_amat = -1;

            for (int v = 0; v < sweep.num_values; v++) {
                config_t config = default_config();
                set_parameter(&config, sweep.parameter, sweep.values[v]);
                double config_amat = run_and_save(results, sweep.parameter, benchmark, config);
                if (best_amat < 0 || config_amat < best_amat) {
                    best_amat = config_amat;
                    best_value = sweep.values[v];
                }
            }

            // the best value of each parameter makes up the combined config
            set_parameter(&best_config, sweep.parameter, best_value);
            fprintf(evaluation, "%s,%s,%d,%.4f,%.4f\n", benchmark_name(benchmark), sweep.parameter, best_value, default_amat, best_amat);
            printf("%s %s: best value %d, amat %.4f\n", benchmark_name(benchmark), sweep.parameter, best_value, best_amat);
        }

        // parameters affect each other, so the combination has to be measured too
        double combined_amat = run_and_save(results, "combined", benchmark, best_config);
        printf("%s combined: amat %.4f\n", benchmark_name(benchmark), combined_amat);
    }

    fclose(results);
    fclose(evaluation);
    return 0;
}
