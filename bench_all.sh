#!/usr/bin/env bash
# Builds everything, runs the benchmarks on the committed algs4 inputs, prints
# markdown to stdout. CI pipes this into the release notes, so every number in a
# release is produced by a real run on the runner, not typed by hand.
set -euo pipefail
cd "$(dirname "$0")"

cc -O3 -o nsum       nsum.c
cc -O3 -o nsum_fft   nsum_fft.c   -lm
cc -O3 -o fft_count  fft_count.c  -lm
cc -O3 -o bench      bench.c

echo "# Benchmark results"
echo
echo "Machine: \`$(uname -m) $(uname -s)\`  |  $(cc --version | head -1)  |  $(date -u +%FT%TZ)"
echo

echo "## Self-checks (assertions must pass, or CI fails)"
echo '```'
./nsum       | tail -1     # demo ok
./fft_count               # demo ok
echo '```'
echo

echo "## Counting zero-sum triples: brute + hash + two-pointer"
echo "Counts cross-validated: every row must print MATCH."
echo '```'
for f in 1K 2K 4K 8K; do ./bench ${f}ints.txt --brute; done
for f in 16K 32K;      do ./bench ${f}ints.txt;         done
echo '```'
echo

echo "## FFT exact count, O(U log U)"
echo '```'
for f in 1K 2K 4K 8K 16K 32K; do ./fft_count ${f}ints.txt 0; done
echo '```'
