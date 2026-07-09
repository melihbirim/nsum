// algs4 ThreeSum benchmark: count index-triples i<j<k with a[i]+a[j]+a[k]==0.
// Matches Sedgewick/Wayne ThreeSum semantics (distinct indices, no value dedup).
// Build: cc -O3 -o bench bench.c
// Run:   ./bench 1Kints.txt [--brute]
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

static int cmp_i32(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}

// O(n^3) reference. Only for small n / validation.
static long brute(const int32_t *a, int n, int64_t T) {
    long c = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            for (int k = j + 1; k < n; k++)
                if ((int64_t)a[i] + a[j] + a[k] == T) c++;
    return c;
}

// O(n^2) two-pointer, counts index-triples, duplicate-safe via block counting.
static long fast(int32_t *a, int n, int64_t T) {
    qsort(a, n, sizeof *a, cmp_i32);
    long c = 0;
    for (int i = 0; i < n - 2; i++) {
        int64_t target = T - a[i];
        int lo = i + 1, hi = n - 1;
        while (lo < hi) {
            int64_t s = (int64_t)a[lo] + a[hi];
            if (s < target) lo++;
            else if (s > target) hi--;
            else if (a[lo] != a[hi]) {                 // two distinct value-blocks
                int cl = 1, ch = 1;
                while (a[lo + cl] == a[lo]) cl++;
                while (a[hi - ch] == a[hi]) ch++;
                c += (long)cl * ch;
                lo += cl; hi -= ch;
            } else {                                    // whole a[lo..hi] equal: choose 2
                long m = hi - lo + 1;
                c += m * (m - 1) / 2;
                break;
            }
        }
    }
    return c;
}

// O(n^2) hash-map counter. No sort. Per i, count pairs (j,k), i<j<k, a[j]+a[k]==-a[i].
// Open addressing, versioned stamps so per-i reset is O(1) not O(cap).
static long hashcount(const int32_t *a, int n, int64_t T) {
    int cap = 1; while (cap < 2 * n) cap <<= 1;
    int mask = cap - 1;
    int32_t *key = malloc(cap * sizeof *key);
    long *cnt = malloc(cap * sizeof *cnt);
    int *stamp = calloc(cap, sizeof *stamp);
    long total = 0;
    for (int i = 0, ver = 0; i < n - 2; i++) {
        ver++;
        for (int k = i + 1; k < n; k++) {
            int64_t need = T - (int64_t)a[i] - a[k];
            // lookup need
            if (need >= INT32_MIN && need <= INT32_MAX) {
                uint32_t h = (uint32_t)need * 2654435761u & mask;
                while (stamp[h] == ver && key[h] != (int32_t)need) h = (h + 1) & mask;
                if (stamp[h] == ver) total += cnt[h];       // pairs seen with value == need
            }
            // insert a[k]
            uint32_t h = (uint32_t)a[k] * 2654435761u & mask;
            while (stamp[h] == ver && key[h] != a[k]) h = (h + 1) & mask;
            if (stamp[h] != ver) { stamp[h] = ver; key[h] = a[k]; cnt[h] = 0; }
            cnt[h]++;
        }
    }
    free(key); free(cnt); free(stamp);
    return total;
}

static int read_ints(const char *path, int32_t **out) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(1); }
    int cap = 1024, n = 0;
    int32_t *a = malloc(cap * sizeof *a), v;
    while (fscanf(f, "%d", &v) == 1) {
        if (n == cap) { cap *= 2; a = realloc(a, cap * sizeof *a); }
        a[n++] = v;
    }
    fclose(f);
    *out = a;
    return n;
}

static double secs(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s file [target] [--brute]\n", argv[0]); return 1; }
    int64_t T = 0;
    int do_brute = 0;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--brute")) do_brute = 1;
        else T = atoll(argv[i]);          // numeric arg = target sum
    }
    int32_t *a; int n = read_ints(argv[1], &a);
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);      // hash first: runs on unsorted input
    long ch = hashcount(a, n, T);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("%-14s n=%-7d T=%-9lld hash  count=%-8ld %.4fs\n", argv[1], n, (long long)T, ch, secs(t0, t1));

    clock_gettime(CLOCK_MONOTONIC, &t0);      // fast sorts a[] in place
    long cf = fast(a, n, T);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("%-14s n=%-7d T=%-9lld 2ptr  count=%-8ld %.4fs  %s\n", argv[1], n, (long long)T, cf,
           secs(t0, t1), ch == cf ? "MATCH" : "MISMATCH");

    if (do_brute) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        long cb = brute(a, n, T);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        printf("%-14s n=%-7d T=%-9lld brute count=%-8ld %.4fs  %s\n",
               argv[1], n, (long long)T, cb, secs(t0, t1), cb == cf ? "MATCH" : "MISMATCH");
    }
    free(a);
    return 0;
}
