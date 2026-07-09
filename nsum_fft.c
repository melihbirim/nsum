// 3SUM decision via FFT. Values must be small-bounded integers (range U).
// Preprocess pair-sum spectrum in O(U log U); each target answered in O(U).
// Semantics: SET of values, repetition allowed (a,b,c may share a value).
// Build: cc -O3 -o nsum_fft nsum_fft.c -lm
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#define UMAX (1 << 25)   // ponytail: hard ceiling. Range bigger -> use two-pointer nsum.c instead.

static int next_pow2(int n) { int m = 1; while (m < n) m <<= 1; return m; }

// Iterative radix-2 Cooley-Tukey. invert=1 does the inverse (with 1/n scaling).
static void fft(double complex *a, int n, int invert) {
    for (int i = 1, j = 0; i < n; i++) {           // bit-reversal permutation
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

// Returns 1 if some a+b+c == target (values from set, repetition allowed), else 0.
// On hit, prints one witness triple. lo/hi are min/max of the input.
int three_sum_fft(const int32_t *arr, int n, int32_t target) {
    int32_t lo = arr[0], hi = arr[0];
    for (int i = 1; i < n; i++) { if (arr[i] < lo) lo = arr[i]; if (arr[i] > hi) hi = arr[i]; }
    int U = (int)(hi - lo) + 1;
    if (U <= 0 || U > UMAX) { fprintf(stderr, "range %d too big for FFT; use two-pointer\n", U); return -1; }

    int m = next_pow2(2 * U);
    double complex *p = calloc(m, sizeof *p);
    char *present = calloc(U, 1);
    for (int i = 0; i < n; i++) { int s = arr[i] - lo; present[s] = 1; p[s] = 1; }

    fft(p, m, 0);
    for (int i = 0; i < m; i++) p[i] *= p[i];      // square = self-convolution
    fft(p, m, 1);

    // has2[t] : two values (rep allowed) with shifted sum t = (a-lo)+(b-lo), i.e. a+b = t + 2*lo
    char *has2 = calloc(m, 1);
    for (int t = 0; t < m; t++) if (llround(creal(p[t])) > 0) has2[t] = 1;

    int found = 0;
    for (int ci = 0; ci < U && !found; ci++) {     // pick c, need a+b = target-c
        if (!present[ci]) continue;
        int32_t c = ci + lo;
        int64_t need = (int64_t)target - c;         // = a + b
        int64_t t = need - 2 * (int64_t)lo;         // shifted pair-sum index
        if (t < 0 || t >= m || !has2[t]) continue;
        // reconstruct one (a,b): O(U) once, only for the single witness
        for (int ai = 0; ai <= (int)t && ai < U; ai++) {
            int bi = (int)t - ai;
            if (bi < 0 || bi >= U) continue;
            if (present[ai] && present[bi]) {
                printf("%d + %d + %d = %d\n", ai + lo, bi + lo, c, target);
                found = 1; break;
            }
        }
    }
    free(p); free(present); free(has2);
    return found;
}

// ponytail: self-check. Fails loud if FFT/convolution logic breaks.
static void demo(void) {
    int32_t a[] = {-1, 0, 1, 2, -1, -4};
    assert(three_sum_fft(a, 6, 0) == 1);      // e.g. -1 + -1 + 2
    int32_t b[] = {5, 5, 5};
    assert(three_sum_fft(b, 3, 15) == 1);     // 5 + 5 + 5 (repetition allowed)
    int32_t c[] = {1, 2, 3};
    assert(three_sum_fft(c, 3, 100) == 0);
    int32_t d[] = {10, 20, 30, 40};
    assert(three_sum_fft(d, 4, 100) == 1);    // 30 + 30 + 40
    assert(three_sum_fft(d, 4, 5) == 0);
    printf("demo ok\n");
}

// Reads whitespace-separated ints from a file. Returns count.
static int read_ints(const char *path, int32_t **out) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(1); }
    int cap = 1 << 20, n = 0;
    int32_t *a = malloc(cap * sizeof *a), v;
    while (fscanf(f, "%d", &v) == 1) {
        if (n == cap) { cap *= 2; a = realloc(a, cap * sizeof *a); }
        a[n++] = v;
    }
    fclose(f);
    *out = a;
    return n;
}

int main(int argc, char **argv) {
    if (argc < 3) { demo(); return 0; }
    int32_t target = atoi(argv[1]);
    int32_t *a; int n;
    if (argc == 3) n = read_ints(argv[2], &a);            // ./nsum_fft target file
    else {                                                 // ./nsum_fft target n1 n2 ...
        n = argc - 2;
        a = malloc(n * sizeof *a);
        for (int i = 0; i < n; i++) a[i] = atoi(argv[i + 2]);
    }
    int r = three_sum_fft(a, n, target);
    printf(r > 0 ? "reachable\n" : r == 0 ? "not reachable\n" : "");
    free(a);
    return 0;
}
