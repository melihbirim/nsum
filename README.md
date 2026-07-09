# nsum — 3SUM, four ways

Find/count triples `a + b + c == target` in an integer array, benchmarked across
four algorithms. Built to answer one question: *which approach wins, and when?*

## TL;DR

| Algorithm | Complexity | Job | Wins when |
|---|---|---|---|
| Brute force | O(n³) | count | never (reference/validation only) |
| Hash map | O(n²) | count | you want no sort; simple |
| **Two-pointer** | O(n²) | count | **default** for general integers |
| FFT (decision) | O(U log U) | decide + witness | bounded range, "is target reachable?" |
| FFT (counting) | O(U log U) | **exact count** | bounded range + huge `n` — beats two-pointer |

There is **no** sub-quadratic general 3SUM (3SUM conjecture). FFT is the only escape,
and only for **bounded integer ranges**. It is not decision-only: with count histograms +
inclusion-exclusion it counts every triple exactly in O(U log U) (see `fft_count.c`).

## Which one do I use?

```
Need a count or all triples?  ── yes ──►  two-pointer   (or hash if you can't sort)
        │ no — just yes/no
        ▼
Values dense & small range  AND  n huge (≳1M)?  ── yes ──►  FFT
        │ no
        ▼
        two-pointer
```

- **Two-pointer (O(n²)) — default.** Counting or listing, any integers, n up to ~100K.
  Fastest counter measured (~2.8× over hash). Needs a sort (free in the noise). Use this
  unless a specific reason says otherwise.
- **Hash map (O(n²)) — niche.** Only when you can't sort (must keep original order/indices)
  or want the simplest no-sort code. Same complexity, ~2.8× slower (cache misses + probing).
- **FFT (O(U log U)) — escape hatch.** Use when **both** hold: (1) values in a small bounded
  range so `U log U ≪ n²`; (2) `n` large, where O(n²) hurts. Works for **decision** (`nsum_fft.c`)
  *and* **exact counting** (`fft_count.c`). Miss either condition → don't.

One line: **two-pointer for general integers; FFT when values are bounded and n is large
(it both decides and counts). Hash only when you can't sort.**

## Files

- `nsum.c` — two-pointer 3SUM to a target. Prints distinct value-triples. Self-test via `./nsum`.
- `nsum_simd.c` — NEON reference version (O(n³) membership scan). Superseded by `nsum.c`; kept to show the intrinsics.
- `nsum_fft.c` — FFT decision: "is target reachable as a+b+c?" + one witness. Bounded-range only.
- `fft_count.c` — FFT **exact counter** (O(U log U)): counts all index-triples via convolution + inclusion-exclusion. Bounded-range only; beats two-pointer at large n.
- `bench.c` — benchmark harness. Counts zero-sum index-triples via brute / hash / two-pointer, cross-validates counts, times each.
- `*ints.txt` — Sedgewick/Wayne **algs4** `ThreeSum` benchmark inputs (1K…32K downloaded; 64K synthetic).

## Build & run

```sh
cc -O3 -o nsum nsum.c
cc -O3 -o nsum_fft nsum_fft.c -lm
cc -O3 -o bench bench.c

./nsum                          # self-test
./nsum 0 -1 0 1 2 -1 -4         # triples summing to 0
cc -O3 -o nsum_simd nsum_simd.c && ./nsum_simd   # NEON reference version
./bench 8Kints.txt              # hash + two-pointer, cross-checked
./bench 4Kints.txt --brute      # add O(n³) reference
./bench 16Kseq.txt 1000000      # custom target
./nsum_fft 0 8Kints.txt         # FFT decision on a file
cc -O3 -o fft_count fft_count.c -lm
./fft_count 64Kints.txt 0       # FFT exact count, O(U log U)
```

Large inputs are gitignored (regenerate locally):

```sh
# 64K algs4-style, range ±1M
python3 -c "import random;print(' '.join(str(random.randint(-1000000,1000000)) for _ in range(64000)))" > 64Kints.txt
# 10M in a bounded range (the FFT crossover demo)
python3 -c "import random;print(' '.join(str(random.randint(-5000000,5000000)) for _ in range(10_000_000)))" > 10Mints.txt
# dense sequence 1..16000 (impossible-target demo)
python3 -c "print(' '.join(str(i) for i in range(1,16001)))" > 16Kseq.txt
```

## Benchmark results

Counting zero-sum index-triples `i<j<k, a[i]+a[j]+a[k]==0`. Counts cross-validated
across methods; 1K–32K match algs4's published reference numbers.

**Counts**

| n | zero-triples |
|---|---|
| 1K | 70 |
| 2K | 528 |
| 4K | 4,039 |
| 8K | 32,074 |
| 16K | 255,181 |
| 32K | 2,052,358 |
| 64K | 16,377,796 |

**Times (seconds), Apple arm64, -O3**

| n | brute O(n³) | hash O(n²) | two-pointer O(n²) | FFT decision |
|---|---|---|---|---|
| 1K | 0.035 | 0.003 | 0.0013 | 0.45 |
| 2K | 0.267 | 0.013 | 0.005 | 0.53 |
| 4K | 2.074 | 0.053 | 0.021 | 0.47 |
| 8K | 16.22 | 0.230 | 0.083 | 0.48 |
| 16K | ~130* | 1.009 | 0.337 | 0.51 |
| 32K | ~1040* | 4.262 | 1.367 | 0.49 |
| 64K | ~8300* | 17.40 | 5.539 | 0.49 |

`*` extrapolated (brute infeasible past 8K).

Notes:
- Brute ×8 per doubling (n³). Dead by 16K.
- Hash and two-pointer both ×4 per doubling (n²); two-pointer stays ~2.7–3× faster.
- FFT is flat in `n` — its cost tracks value range `U`, not `n`. On these files `U ≈ 2M`
  (m = 2²²), so ~0.5s regardless of size.

### FFT counting vs two-pointer

`fft_count.c` counts triples **exactly** (all seven counts above reproduced), flat ~0.5s.
On the algs4 inputs (range `U ≈ 2M`) it overtakes two-pointer around 20–25K:

| n | two-pointer | FFT-count | winner |
|---|---|---|---|
| 8K | 0.083s | 0.52s | two-pointer |
| 16K | 0.337s | 0.53s | two-pointer |
| 32K | 1.367s | 0.49s | **FFT 2.8×** |
| 64K | 5.539s | 0.45s | **FFT 12×** |

Past the crossover, FFT pulls away forever (n² vs U log U). Correctness note: FFT counting is
**not** decision-only — index-distinctness is handled by inclusion-exclusion `(S1 − 3A + 2B)/6`.

### The FFT crossover

At **10M** values in a bounded range `[-5M, 5M]`:

| method | time | memory |
|---|---|---|
| two-pointer | ~36 h (extrapolated) | tiny |
| FFT decision | **5.65 s** | 608 MB |

And on a *dense* array `1..16000` with an impossible target `1,000,000`:

| method | verdict | time |
|---|---|---|
| hash | count 0 | 0.461 s |
| two-pointer | count 0 | 0.075 s |
| **FFT** | not reachable | **~0.00 s** |

Range is only 16,000 → m = 2¹⁵ → the FFT is nearly free and proves non-existence
in one convolution, no pair scanning. FFT's dream case: small range + impossible target.

### Where FFT fails

1. **Wide value range** — cost is O(U log U); sparse/large ints blow up `U`. Dies when `U log U > n²`.
2. **Precision** — double-precision convolution rounds counts; huge counts risk a wrong `llround` (fix: integer NTT). Validated exact up to 64K here.
3. **Non-additive keys** — needs values → bounded integer index and a *sum* (linear convolution). Strings/floats/similarity → useless.
4. **Overhead** — power-of-two padding wastes up to 2×; setup constant loses to a trivial loop at small `n`.

---

## Technical: the two-pointer method

The heart of the fast counter. Requires a **sorted** array.

Fix the smallest element `a[i]`. Now you need two more elements from `a[i+1..n-1]`
summing to `target - a[i]`. Put one pointer at each end of that suffix:

```
lo = i+1, hi = n-1
while lo < hi:
    s = a[lo] + a[hi]
    if s < target: lo++      # too small — only way up is a bigger low end
    elif s > target: hi--    # too big — only way down is a smaller high end
    else: record; advance both
```

Why it's correct: the array is sorted, so `a[lo]` is the smallest available low value
and `a[hi]` the largest available high value. If their sum is too small, **no** `hi`
pairs with the current `lo` (every other candidate is smaller), so `lo` can only move
right. Symmetric for too-big. Each step eliminates one element permanently → the inner
loop is O(n), and the outer loop over `i` makes the whole thing **O(n²)**, plus an
O(n log n) sort that vanishes into the noise.

Duplicate handling (for counting): when a match is found and `a[lo] != a[hi]`, count the
full block of equal lows × block of equal highs, then skip both blocks. When the whole
`[lo, hi]` window is one repeated value, the pairs are "choose 2" = `m(m-1)/2`.

Why it beats the hash map (same O(n²)): purely sequential memory access from both ends —
cache-friendly, no hashing, no probing, no random loads. The hash map pays a cache miss
and probe per element. Measured ~2.8× gap.

## Technical: SIMD (ARM NEON)

`nsum_simd.c` accelerates a membership scan with SIMD: for each pair `(i, j)`, search the
array for `target - a[i] - a[j]`. NEON compares 4 `int32` lanes per instruction:

```c
int32x4_t v     = vdupq_n_s32(need);   // broadcast target value to all 4 lanes
int32x4_t chunk = vld1q_s32(a + k);    // load 4 array elements
uint32x4_t eq   = vceqq_s32(chunk, v); // lane-wise equality → 0xFFFFFFFF or 0
if (vmaxvq_u32(eq)) return 1;          // horizontal max: any lane matched?
```

`vdupq_n_s32` broadcasts the needle into a 128-bit register; `vld1q_s32` loads 4 haystack
values; `vceqq_s32` does four comparisons at once; `vmaxvq_u32` reduces the 4 lanes to a
single "did anything match" test. Roughly a 4× constant-factor speedup on the scan (8× with
`int16`, 16× with `int8`, since more lanes fit in 128 bits).

**Why we dropped it.** That membership-scan structure is O(n) per pair → **O(n³)** overall.
SIMD gives a ~4× constant; the two-pointer swap gives an ~`n/6`× *algorithmic* win. Polishing
a cubic never beats fixing the exponent. The lesson worth keeping: **pick the algorithm first,
then vectorize the inner loop of the right algorithm.** SIMD is a multiplier on the constant,
never a substitute for complexity class.

(The two-pointer inner loop is inherently sequential — each step's direction depends on the
previous comparison — so it doesn't vectorize cleanly. The right move there is the better big-O,
not lanes.)
