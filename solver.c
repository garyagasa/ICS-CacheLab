/*
 * solver.c - Reverse-engineer a mystery cache.
 *
 * solve() is called once per mystery cache. The only allowed calls are the
 * cache API in solver.h (cache_load, cache_store, cache_flush, report);
 * everything else is plain integer code so solver.o contains no symbols
 * outside test-solver's whitelist.
 *
 * Discovery order:
 *   1. b  -- block offset bits (first power-of-two offset that misses).
 *   2. E  -- associativity (a set holds at most E distinct blocks).
 *   3. s  -- set index bits (which single-bit addresses land back in set 0).
 *   4. wa -- write-allocate (does a store miss bring the block in?).
 *   5. policy -- LRU / FIFO / RANDOM (only graded when E > 1).
 */
#include "solver.h"

/* Tag-only address in set 0: bit 16 and above is always tag because
 * b <= 7 and s <= 9, so every block offset / set index bit is below 16. */
static uint32_t T(int j)
{
    return (uint32_t)((j + 1) << 16);
}

static int find_b(void)
{
    int b = 0;
    cache_flush();
    cache_load(0);
    while (b < 31 && cache_load(1u << b))
        b++;
    return b;
}

/* True (1) iff a set can hold at least n distinct blocks. Robust against
 * every replacement policy: if n <= E all n blocks are still resident, and
 * if n > E at least one of them has been evicted. */
static int holds(int n)
{
    int j, all = 1;
    cache_flush();
    for (j = 0; j < n; j++)
        cache_load(T(j));
    for (j = 0; j < n; j++) {
        if (!cache_load(T(j))) {
            all = 0;
            break;
        }
    }
    return all;
}

static int find_E(void)
{
    int n;
    for (n = 1; n <= 16; n++) {
        if (!holds(n))
            return n - 1;
    }
    return 16;
}

/* True (1) iff x maps to the same set as address 0. Fill set 0 completely,
 * then load x; if x lives in set 0 it evicts exactly one of the filler
 * blocks, so at least one filler block is no longer resident. */
static int same_set(int E, uint32_t x)
{
    int j;
    cache_flush();
    for (j = 0; j < E; j++)
        cache_load(T(j));
    cache_load(x);
    for (j = 0; j < E; j++) {
        if (!cache_load(T(j)))
            return 1;
    }
    return 0;
}

static int find_s(int b, int E)
{
    int k;
    for (k = 0; k < 9; k++) {
        if (same_set(E, (uint32_t)(1u << (b + k))))
            return k;
    }
    return 9;
}

static int find_wa(void)
{
    cache_flush();
    cache_store(0);
    return cache_load(0);
}

/* One LRU-vs-FIFO probe. Returns POLICY_LRU, POLICY_FIFO or -1.
 *
 * Fill a set, touch the first block (so LRU would evict the second block
 * next), then force one eviction and scan for which block disappeared.
 * The scan stops at the first miss, so reloading that block cannot corrupt
 * the remaining checks. */
static int classify_policy(int E, int t)
{
    int i, j;
    int victim = -1;

    cache_flush();
    for (i = 0; i < E; i++)
        cache_load(T(i + t * 3));
    cache_load(T(t * 3));            /* make the first block most-recent */
    cache_load(T(E + t * 3));        /* force exactly one eviction */

    for (j = 0; j < E; j++) {
        if (!cache_load(T(t * 3 + j))) {
            victim = j;
            break;
        }
    }

    if (victim == 1)
        return POLICY_LRU;           /* the block we just touched survived */
    if (victim == 0)
        return POLICY_FIFO;          /* the oldest inserted block was evicted */
    return -1;                       /* some other block: random-ish */
}

static int find_policy(int E)
{
    int t, c;
    int lru = 0, fifo = 0, other = 0;

    if (E <= 1)
        return POLICY_LRU;

    for (t = 0; t < 20; t++) {
        c = classify_policy(E, t);
        if (c == POLICY_LRU)
            lru++;
        else if (c == POLICY_FIFO)
            fifo++;
        else
            other++;
    }

    if (other == 0 && fifo == 0)
        return POLICY_LRU;
    if (other == 0 && lru == 0)
        return POLICY_FIFO;
    return POLICY_RANDOM;
}

void solve(void)
{
    int b = find_b();
    int E = find_E();
    int s = find_s(b, E);
    int wa = find_wa();
    int policy = find_policy(E);
    report(b, s, E, policy, wa);
}
