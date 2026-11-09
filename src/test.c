#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "cache.h"

static int failures = 0;
static int checks = 0;

#define EXPECT(cache, h, m, w) expect(#cache, __LINE__, cache, h, m, w)

// compare a cache's counters against the expected hits, misses and writes
static void expect(const char* name, int line, cache_t* cache, uint64_t hits, uint64_t misses, uint64_t writes) {
    checks++;
    if (cache_hits(cache) == hits && cache_misses(cache) == misses && cache_writes(cache) == writes)
        return;
    failures++;
    printf("FAIL line %d: %s expected %llu/%llu/%llu (h/m/w), got %llu/%llu/%llu\n", line, name,
           (unsigned long long)hits, (unsigned long long)misses, (unsigned long long)writes,
           (unsigned long long)cache_hits(cache), (unsigned long long)cache_misses(cache),
           (unsigned long long)cache_writes(cache));
}

// same lookup order as memory_read: try L1, go to L2 on miss
static void read_through(cache_t* l1, cache_t* l2, uint64_t address) {
    if (!cache_seek(l1, address, NULL))
        cache_seek(l2, address, NULL);
}

// same lookup order as memory_write
static void write_through_hierarchy(cache_t* l1, cache_t* l2, uint64_t address) {
    if (!cache_write(l1, address, NULL))
        cache_write(l2, address, NULL);
}

// cold miss, then hits anywhere in the same 64 byte line
static void test_spatial_locality(void) {
    cache_t* c = create_cache(1024, 64, 2, REPLACEMENT_POLICY_LRU, write_back, NULL);
    cache_seek(c, 0x1000, NULL);
    cache_seek(c, 0x1008, NULL);
    cache_seek(c, 0x103f, NULL);
    EXPECT(c, 2, 1, 0);
    // first byte of the next line is a new miss
    cache_seek(c, 0x1040, NULL);
    EXPECT(c, 2, 2, 0);
    destroy_cache(c);
}

// sequential scan of 64 doubles touches 8 lines: 8 misses, 56 hits
static void test_sequential_scan(void) {
    cache_t* c = create_cache(4096, 64, 4, REPLACEMENT_POLICY_LRU, write_back, NULL);
    for (int i = 0; i < 64; i++)
        cache_seek(c, 0x10000 + i * sizeof(double), NULL);
    EXPECT(c, 56, 8, 0);
    destroy_cache(c);
}

// direct mapped: addresses one cache size apart share a set and evict each other
static void test_direct_mapped_conflict(void) {
    cache_t* c = create_cache(256, 64, 1, REPLACEMENT_POLICY_LRU, write_back, NULL);
    for (int i = 0; i < 3; i++) {
        cache_seek(c, 0x1000, NULL);
        cache_seek(c, 0x1100, NULL);
    }
    EXPECT(c, 0, 6, 0);
    // addresses in different sets coexist
    cache_t* d = create_cache(256, 64, 1, REPLACEMENT_POLICY_LRU, write_back, NULL);
    for (int i = 0; i < 3; i++) {
        cache_seek(d, 0x1000, NULL);
        cache_seek(d, 0x1040, NULL);
    }
    EXPECT(d, 4, 2, 0);
    destroy_cache(c);
    destroy_cache(d);
}

// 2-way associativity absorbs the conflict that breaks the direct mapped cache
static void test_associativity(void) {
    cache_t* c = create_cache(512, 64, 2, REPLACEMENT_POLICY_LRU, write_back, NULL);
    for (int i = 0; i < 3; i++) {
        cache_seek(c, 0x1000, NULL);
        cache_seek(c, 0x1100, NULL);
    }
    EXPECT(c, 4, 2, 0);
    destroy_cache(c);
}

// single set, 2 ways: A B A C A. LRU keeps A, FIFO evicts it
static void test_lru_vs_fifo(void) {
    uint64_t seq[] = { 0x1000, 0x2000, 0x1000, 0x3000, 0x1000 };
    cache_t* lru = create_cache(128, 64, 2, REPLACEMENT_POLICY_LRU, write_back, NULL);
    cache_t* fifo = create_cache(128, 64, 2, REPLACEMENT_POLICY_FIFO, write_back, NULL);
    for (int i = 0; i < 5; i++) {
        cache_seek(lru, seq[i], NULL);
        cache_seek(fifo, seq[i], NULL);
    }
    EXPECT(lru, 2, 3, 0);
    EXPECT(fifo, 1, 4, 0);
    destroy_cache(lru);
    destroy_cache(fifo);
}

// looping over one more line than fits thrashes both policies completely
static void test_capacity_thrash(void) {
    cache_t* lru = create_cache(256, 64, 4, REPLACEMENT_POLICY_LRU, write_back, NULL);
    cache_t* fifo = create_cache(256, 64, 4, REPLACEMENT_POLICY_FIFO, write_back, NULL);
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < 5; i++) {
            cache_seek(lru, 0x1000 + i * 0x40, NULL);
            cache_seek(fifo, 0x1000 + i * 0x40, NULL);
        }
    }
    EXPECT(lru, 0, 10, 0);
    EXPECT(fifo, 0, 10, 0);
    destroy_cache(lru);
    destroy_cache(fifo);
}

// a write miss counts as a miss but not as a write, the following write hits
static void test_write_miss(void) {
    cache_t* c = create_cache(1024, 64, 2, REPLACEMENT_POLICY_LRU, write_back, NULL);
    cache_write(c, 0x1000, NULL);
    EXPECT(c, 0, 1, 0);
    cache_write(c, 0x1000, NULL);
    EXPECT(c, 1, 1, 1);
    destroy_cache(c);
}

// write-back: parent only sees a write when a dirty line is evicted
static void test_write_back(void) {
    cache_t* l2 = create_cache(4096, 64, 4, REPLACEMENT_POLICY_LRU, write_back, NULL);
    cache_t* l1 = create_cache(64, 64, 1, REPLACEMENT_POLICY_LRU, write_back, l2);
    read_through(l1, l2, 0x1000);
    write_through_hierarchy(l1, l2, 0x1000);
    // store hit is absorbed by L1
    EXPECT(l1, 1, 1, 1);
    EXPECT(l2, 0, 1, 0);
    // evicting the dirty line writes it back to L2
    read_through(l1, l2, 0x2000);
    EXPECT(l1, 1, 2, 1);
    EXPECT(l2, 0, 2, 1);
    // evicting the now clean line does not
    read_through(l1, l2, 0x1000);
    EXPECT(l2, 1, 2, 1);
    destroy_cache(l1);
    destroy_cache(l2);
}

// write-through: every store hit in L1 is forwarded to L2, evictions are free
static void test_write_through(void) {
    cache_t* l2 = create_cache(4096, 64, 4, REPLACEMENT_POLICY_LRU, write_through, NULL);
    cache_t* l1 = create_cache(64, 64, 1, REPLACEMENT_POLICY_LRU, write_through, l2);
    read_through(l1, l2, 0x1000);
    write_through_hierarchy(l1, l2, 0x1000);
    write_through_hierarchy(l1, l2, 0x1000);
    EXPECT(l1, 2, 1, 2);
    EXPECT(l2, 2, 1, 2);
    // line is never dirty, so eviction adds no writes
    read_through(l1, l2, 0x2000);
    EXPECT(l1, 2, 2, 2);
    EXPECT(l2, 2, 2, 2);
    destroy_cache(l1);
    destroy_cache(l2);
}

// L1 hits never reach L2, L1 misses always do
static void test_hierarchy_lookup(void) {
    cache_t* l2 = create_cache(4096, 64, 4, REPLACEMENT_POLICY_LRU, write_back, NULL);
    cache_t* l1 = create_cache(64, 64, 1, REPLACEMENT_POLICY_LRU, write_back, l2);
    read_through(l1, l2, 0x1000);
    read_through(l1, l2, 0x1000);
    EXPECT(l1, 1, 1, 0);
    EXPECT(l2, 0, 1, 0);
    // A is evicted from the tiny L1 but is still in L2
    read_through(l1, l2, 0x2000);
    read_through(l1, l2, 0x1000);
    EXPECT(l1, 1, 3, 0);
    EXPECT(l2, 1, 2, 0);
    destroy_cache(l1);
    destroy_cache(l2);
}

// separate L1 instruction and data caches share the unified L2
static void test_split_l1_shared_l2(void) {
    cache_t* l2 = create_cache(4096, 64, 4, REPLACEMENT_POLICY_LRU, write_back, NULL);
    cache_t* l1i = create_cache(1024, 64, 4, REPLACEMENT_POLICY_LRU, write_back, l2);
    cache_t* l1d = create_cache(1024, 64, 8, REPLACEMENT_POLICY_LRU, write_back, l2);
    read_through(l1i, l2, 0x1000);
    // data cache has not seen the line, but L2 has
    read_through(l1d, l2, 0x1000);
    EXPECT(l1i, 0, 1, 0);
    EXPECT(l1d, 0, 1, 0);
    EXPECT(l2, 1, 1, 0);
    destroy_cache(l1i);
    destroy_cache(l1d);
    destroy_cache(l2);
}

// column walk with a stride of one cache size maps every access to one set
static void test_strided_access(void) {
    cache_t* c = create_cache(4096, 64, 4, REPLACEMENT_POLICY_LRU, write_back, NULL);
    // 8 rows, 4 ways: the second pass misses everywhere
    for (int pass = 0; pass < 2; pass++)
        for (int row = 0; row < 8; row++)
            cache_seek(c, 0x100000 + row * 4096, NULL);
    EXPECT(c, 0, 16, 0);
    destroy_cache(c);
}

int main(void) {
    test_spatial_locality();
    test_sequential_scan();
    test_direct_mapped_conflict();
    test_associativity();
    test_lru_vs_fifo();
    test_capacity_thrash();
    test_write_miss();
    test_write_back();
    test_write_through();
    test_hierarchy_lookup();
    test_split_l1_shared_l2();
    test_strided_access();

    printf("%d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
