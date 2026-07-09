// 3SUM to target via SIMD membership scan (ARM NEON). Reference / teaching version.
// SUPERSEDED by nsum.c (two-pointer): this is O(n^3), two-pointer is O(n^2).
// Kept to show the NEON intrinsics and why vectorizing a cubic loses to a better algorithm.
// Build: cc -O3 -o nsum_simd nsum_simd.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

static int cmp_i32(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}

// Does `need` appear in sorted a[lo..hi)? Array ascending, so stop once a[k] > need.
// NEON compares 4 int32 lanes per instruction; scalar tail handles the remainder.
static int contains(const int32_t *a, int lo, int hi, int32_t need) {
    int k = lo;
#ifdef __ARM_NEON
    int32x4_t v = vdupq_n_s32(need);            // broadcast needle to all 4 lanes
    for (; k + 4 <= hi; k += 4) {
        int32x4_t chunk = vld1q_s32(a + k);      // load 4 haystack values
        uint32x4_t eq = vceqq_s32(chunk, v);     // 4 comparisons at once
        if (vmaxvq_u32(eq)) return 1;            // horizontal max: any lane matched?
        if (a[k + 3] > need) return 0;           // sorted: nothing further can match
    }
#endif
    for (; k < hi; k++) {                         // scalar tail (and full path if no NEON)
        if (a[k] == need) return 1;
        if (a[k] > need) return 0;
    }
    return 0;
}

// Prints each distinct value-triple once. Returns count. Sorts a[] in place. O(n^3).
int three_sum(int32_t *a, int n, int32_t target) {
    qsort(a, n, sizeof *a, cmp_i32);
    int count = 0;
    for (int i = 0; i < n - 2; i++) {
        if (i > 0 && a[i] == a[i - 1]) continue;          // dedup first element
        for (int j = i + 1; j < n - 1; j++) {
            if (j > i + 1 && a[j] == a[j - 1]) continue;   // dedup second
            int32_t need = target - a[i] - a[j];
            if (need < a[j]) break;                         // need must sit at k>j
            if (contains(a, j + 1, n, need)) {
                printf("%d + %d + %d = %d\n", a[i], a[j], need, target);
                count++;
            }
        }
    }
    return count;
}

// Self-check. Fails loud if the NEON scan or dedup logic breaks.
static void demo(void) {
    int32_t a[] = {-1, 0, 1, 2, -1, -4};
    assert(three_sum(a, 6, 0) == 2);   // (-1,-1,2) and (-1,0,1)
    int32_t b[] = {5, 5, 5};
    assert(three_sum(b, 3, 15) == 1);
    int32_t c[] = {1, 2, 3};
    assert(three_sum(c, 3, 100) == 0);
    printf("demo ok\n");
}

int main(int argc, char **argv) {
    if (argc < 3) { demo(); return 0; }
    int32_t target = atoi(argv[1]);
    int n = argc - 2;
    int32_t *a = malloc(n * sizeof *a);
    for (int i = 0; i < n; i++) a[i] = atoi(argv[i + 2]);
    int c = three_sum(a, n, target);
    printf("%d triple(s)\n", c);
    free(a);
    return 0;
}
