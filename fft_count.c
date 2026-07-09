// EXACT 3SUM counting via FFT, O(U log U). Counts index-triples i<j<k, a[i]+a[j]+a[k]==T.
// Bounded integer range only (range U). Matches two-pointer counts, flat in n.
// Method: conv2 = P*P (# ordered pairs per sum); count = (S1 - 3A + 2B)/6 with
// inclusion-exclusion removing collisions where two or three indices coincide.
// Build: cc -O3 -o fft_count fft_count.c -lm
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#define UMAX (1 << 25)

static int next_pow2(int n) { int m = 1; while (m < n) m <<= 1; return m; }

static void fft(double complex *a, int n, int invert) {
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { double complex t = a[i]; a[i] = a[j]; a[j] = t; }
    }
    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * M_PI / len * (invert ? -1 : 1);
        double complex wlen = cos(ang) + I * sin(ang);
        for (int i = 0; i < n; i += len) {
            double complex w = 1;
            for (int k = 0; k < len / 2; k++) {
                double complex u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
    if (invert) for (int i = 0; i < n; i++) a[i] /= n;
}

// Returns exact count of index-triples i<j<k with a[i]+a[j]+a[k]==T. -1 if range too big.
long three_sum_count_fft(const int32_t *arr, int n, int64_t T) {
    int32_t lo = arr[0], hi = arr[0];
    for (int i = 1; i < n; i++) { if (arr[i] < lo) lo = arr[i]; if (arr[i] > hi) hi = arr[i]; }
    int U = (int)(hi - lo) + 1;
    if (U <= 0 || U > UMAX) { fprintf(stderr, "range %d too big for FFT\n", U); return -1; }

    // cnt[s] = multiplicity of shifted value s = value-lo
    long *cnt = calloc(U, sizeof *cnt);
    for (int i = 0; i < n; i++) cnt[arr[i] - lo]++;

    int m = next_pow2(2 * U);
    double complex *p = calloc(m, sizeof *p);
    for (int s = 0; s < U; s++) p[s] = cnt[s];
    fft(p, m, 0);
    for (int i = 0; i < m; i++) p[i] *= p[i];
    fft(p, m, 1);
    long *conv2 = malloc(m * sizeof *conv2);          // ordered pairs (index-independent) per shifted sum
    for (int t = 0; t < m; t++) conv2[t] = llround(creal(p[t]));

    int64_t Tt = T - 3 * (int64_t)lo;                 // shifted triple target: s_a+s_b+s_c = Tt
    long S1 = 0, A = 0, B = 0;
    for (int ci = 0; ci < U; ci++) {                  // pick third value c, need pair sum = Tt-ci
        if (!cnt[ci]) continue;
        int64_t t = Tt - ci;
        if (t >= 0 && t < m) S1 += cnt[ci] * conv2[t]; // ordered triples, indices independent
        int64_t two = Tt - 2 * (int64_t)ci;            // A: two indices equal (value ci), third = two
        if (two >= 0 && two < U) A += cnt[ci] * cnt[two];
    }
    if (Tt % 3 == 0) { long v = Tt / 3; if (v >= 0 && v < U) B = cnt[v]; }  // all three equal

    long count = (S1 - 3 * A + 2 * B) / 6;            // ordered-all-distinct / 3!
    free(cnt); free(p); free(conv2);
    return count;
}

static void demo(void) {
    int32_t a[] = {-1, 0, 1, 2, -1, -4};              // 3 index-triples sum to 0
    assert(three_sum_count_fft(a, 6, 0) == 3);
    int32_t b[] = {5, 5, 5};                          // (5,5,5) once
    assert(three_sum_count_fft(b, 3, 15) == 1);
    int32_t c[] = {1, 2, 3};                          // none
    assert(three_sum_count_fft(c, 3, 100) == 0);
    printf("demo ok\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { demo(); return 0; }
    int64_t T = argc > 2 ? atoll(argv[2]) : 0;
    // read whitespace-separated ints
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror(argv[1]); return 1; }
    int cap = 1 << 20, n = 0; int32_t *a = malloc(cap * sizeof *a), v;
    while (fscanf(f, "%d", &v) == 1) { if (n == cap){cap*=2;a=realloc(a,cap*sizeof *a);} a[n++]=v; }
    fclose(f);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    long c = three_sum_count_fft(a, n, T);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("%-12s n=%-7d T=%-6lld  fft-count=%-10ld %.4fs\n", argv[1], n, (long long)T, c, s);
    free(a);
    return 0;
}
