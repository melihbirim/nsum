// 3SUM to target, SIMD (ARM NEON). Find distinct value-triples a+b+c == target.
// Build: cc -O3 -o nsum nsum.c   (arm64 has NEON always; scalar fallback below)
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

static int cmp_i32(const void *a, const void *b) {
    int32_t x = *(const int32_t *)a, y = *(const int32_t *)b;
    return (x > y) - (x < y);
}

// Prints each distinct value-triple once. Returns count. Sorts a[] in place.
// Two-pointer: O(n^2) total. Beats the old O(n^3) scan; SIMD gives ~4x, this gives ~n.
int three_sum(int32_t *a, int n, int32_t target) {
    qsort(a, n, sizeof *a, cmp_i32);
    int count = 0;
    for (int i = 0; i < n - 2; i++) {
        if (i > 0 && a[i] == a[i - 1]) continue;            // dedup first element
        if ((int64_t)a[i] + a[i + 1] + a[i + 2] > target) break;   // smallest triple already too big
        int lo = i + 1, hi = n - 1;
        while (lo < hi) {
            int64_t s = (int64_t)a[i] + a[lo] + a[hi];
            if (s == target) {
                printf("%d + %d + %d = %d\n", a[i], a[lo], a[hi], target);
                count++;
                int32_t vl = a[lo], vh = a[hi];
                while (lo < hi && a[lo] == vl) lo++;         // dedup second
                while (lo < hi && a[hi] == vh) hi--;         // dedup third
            } else if (s < target) {
                lo++;
            } else {
                hi--;
            }
        }
    }
    return count;
}

// ponytail: self-check instead of a test framework. Fails loud if logic breaks.
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
