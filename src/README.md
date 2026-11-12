# Cache simulator

Simulates an L1 instruction cache, an L1 data cache and a unified L2 cache
running a 256x256 matrix multiplication (naive or block). It prints the number
of hits, misses and writes of each cache.

## Build

```
make            # builds ./cachesim
make test       # integration tests
make benchmark  # parameter sweeps, writes results/benchmark.csv and results/evaluation.csv
make clean
```

## Run

```
./cachesim [--option value]...
```

Without options the AMD Zen1 configuration from the assignment is simulated.

| Option | Values | Default |
|---|---|---|
| `--benchmark` | `naive`, `block` | `naive` |
| `--l1-size` | bytes, both L1 caches | 32768 |
| `--l2-size` | bytes | 524288 |
| `--l1i-assoc` | ways per set, L1 instruction cache | 4 |
| `--l1d-assoc` | ways per set, L1 data cache | 8 |
| `--l2-assoc` | ways per set, L2 cache | 8 |
| `--line-size` | bytes, all caches | 64 |
| `--replacement` | `lru`, `fifo`, `random` (all caches) | `lru` |
| `--write-policy` | `write-back`, `write-through` (L1 data and L2) | `write-back` |

`./cachesim --help` prints the same list. The number of sets, `size / (line size * associativity)`,
and the line size must be powers of two, otherwise the program stops with an error.

Examples:

```
./cachesim --benchmark block
./cachesim --benchmark block --l2-size 1048576
./cachesim --l1i-assoc 2 --l1d-assoc 2 --l2-assoc 2
./cachesim --line-size 128 --write-policy write-through
./cachesim --replacement random
```

Results are the same on every run: the matrices are at fixed addresses and the
random replacement is seeded.
