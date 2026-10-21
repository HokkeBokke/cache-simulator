# Cache Simulator

This is a group assignment.

In this assignment, you will implement a cache simulator for a memory system with a level-1 read-only instruction cache, a level-1 data cache, and a unified level-2 cache. The latter two should support both reads and writes.

The pre-code provides a CPU simulator that will perform memory accesses against your memory subsystem. Your goal is to use the simulator to find the best cache design for two matrix multiplication benchmarks. To do this, you will measure the cache hit and miss ratios and experiment with different caches' parameters.

## Cache specifications

As a starting point, you can use the following starting parameter (AMD Zen1):

|                     | L1 Instruction cache | L1 Data cache       | L2 Unified cache    |
|---------------------|----------------------|---------------------|---------------------|
| Cache size          | 32 KB                | 32 KB               | 512 KB              |
| Cache associativity | 4-way associativity  | 8-way associativity | 8-way associativity |
| Inclusive policy    |       N/A            |       N/A           | Inclusive           |
| Replacement policy  | Approximated LRU     | Approximated LRU    | Approximated LRU    |
| Line size           | 64 bytes             | 64 bytes            | 64 bytes            |
| Bus width           | 32 bytes             | 32 bytes            | 32 bytes            |
| Write policy        | Write-back           | Write-back          | Write-back          |


## Cache simulator implementation

As a starting point for your simulator implementation, we provide a pre-code that implements the API of a memory subsystem and a CPU using this API. The simulator executes instruction fetch, data load, and data store memory operations based on a simulated matrix multiplication benchmark. You are required to write an implementation of a memory hierarchy over the memory subsystem API containing modules for the caches.

### Simplifications

1. You can assume that each data access is within the boundary of a cache line.
2. All instruction fetch are of a fixed size and addresses are word-aligned. So effectively you may ignore the last two bits of the accessed memory addresses to pretend that accesses are aligned.
3. You do not have to implement reads and writes that actually transfer data. Just count cache hits and misses.

## Methodology

You will evaluate the cache by using the memory access from the simulation of two implementations of a matrix multiplication: the naive matrix multiplication and block matrix multiplication. You will use this to evaluate the cache parameters to find the best cache design for your benchmark. Note that the size of the matrix should be set to 256 in your final results (so that the total data size is larger than the L2 cache). But during development, you can set the matrix to a lower size to have your program execute faster. 

### Benchmarks
The implementation of naive and block matrix multiplications are available in cachesim.c. The naive matrix multiplication uses three for loops, that multiply the lines of the first matrix with the columns of the second matrix. The whole lines and columns are crossed at each iteration of the third loop. It is a simple and intuitive implementation, but doesn't take advantage of cache locality: addresses of the second matrix called on subsequent iterations of the third loop don't belong to the same cache line.  

The block matrix multiplication takes advantage of cache locality by cache line size into consideration in its algorithm. A detailed explanation of this algorithm can be found in the begining of Section 6.2.1 from [What Every Programmer Should Know About Memory](https://people.freebsd.org/~lstewart/articles/cpumemory.pdf).

### Pre-code
The pre-code will initialize your memory subsystem by calling *memory_init()*. Then, memory access is done by calling one of the following functions:
* *memory_fetch()* - if the memory access is an instruction fetch
* *memory_read()* - if the memory access is a data read
* *memory_write()* - if the memory access is a data write

### Integration test
To ensure that your cache simulator behaves correctly, you must design integration tests with a known number of cache hits or miss. You must ensure that your simulator produces the expected number of cache hits/misses from the test.

## Evaluation
Your goal is to find the cache configuration that gives your benchmark the best performance at the lowest hardware implementation cost for both benchmarks. To do this, you will start with the default parameters listed in the _Cache specifications_-section. Then you should change one parameter at a time and measure the changes in hit and miss ratio for each cache. The parameters you will change for each cache are:
- Cache size
* Associativity
* Line size
* Write-policy (write-back, write-through)
- Replacement policy (random, LRU, temporal/spatial)

Plus you must study the above according to the benchmark: naive, block matrix multiplication

At the minimum you must report the number cache hits, cache misses, and write for each configuration.

Your report must present the results from your measurements and provide a discussion about the cost of implementing your final cache design in hardware. It must also compare and discuss results obtained for both naive and block matrix multiplication implementations.

You are also allowed to change additional parameters. Note that the number of sets in a given cache is a dependent variable given from the total cache size, line size, and associativity (number of ways = number of lines per set). 

## Deliverables
The final submission must follow the same structure as the pre-code:
1. The src/ folder containing the implementation of your simulator with a README describing how to run the simulator
2. The report/ folder containing your report named report.pdf. One report per group

### Report
The report's goal is to convince a computer architecture expert that your cache design is the best for your particular benchmark. You should assume that the expert may want to repeat your experiments to verify your results. *You therefore do not need to explain how caches work, or other basic concepts described in the textbook*. The report should contain all the information required to redo your work, including the following parts:

1. Introduction
2. Methodology: where you describe how to do the experiments, including:
    * Cache simulator implementation: where you provide the details for the expert to re-implement your cache simulator.
    * How you count cache hits and misses for each cache.
    * The design of your correctness test cases
3. Results: where you present the evaluation results.
4. Discussion: where you discuss the most interesting result with respect to hardware cost.
5. Conclusion: where you write a summary of the best cache design. 
