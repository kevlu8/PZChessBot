Speed experiment on `optimize/engine-speed`, starting from `11bd212`.

Both baseline and optimized engines were built from clean objects with GCC
15.2.0, `-O3 -flto=auto -march=native` and benchmark-trained PGO. The host reports
AMD Ryzen 7 170 with Radeon Graphics (AVX2/BMI2). The network is unchanged:
SHA-256 `69808c8aafeaf8e69ea9f3503111619ccd95809c699a1631893bf1a43e98488b`.

The comparison runs engines sequentially on logical CPU 2, warms up each,
alternates execution order and rejects differing benchmark node counts. Speedup
is the geometric mean of paired NPS ratios; intervals use a normal approximation
on log ratios. Results describe this machine and network.

| Bench depth | Paired runs | Baseline NPS | Optimized NPS | Paired speedup | Approx. 95% interval |
| --- | ---: | ---: | ---: | ---: | --- |
| 12 (default) | 20 | 1,025,412 | 1,197,407 | **+16.80%** | +15.14% to +18.49% |
| 14 | 8 | 1,041,941 | 1,197,045 | **+14.86%** | +12.77% to +16.98% |

Both engines search **2,254,747 nodes** at depth 12 and **5,193,900 nodes** at depth 14.

The retained optimizations:

- Defer threat discovery until evaluation needs an incremental accumulator.
- Keep accumulator tiles in SIMD registers across all threat additions and removals.
- Use common occupancy to find affected attackers with half the sliding-attack lookups.
- Skip king attacks and attacks on kings, which have no threat-network features.
- Combine source-square offsets with attack indices in a table half the original size.
- Reuse the previous accumulator's king buckets.
- Replace locked node-counter increments with atomic loads/stores; each counter has one writer.
- Advise Linux to back the aligned weight table with transparent huge pages.

Sparse first-layer evaluation and larger unrolling/accumulator tiles were tested
and discarded because they did not improve speed reliably. Search parameters,
move ordering and network arithmetic are unchanged.

`bench.py` now reads PZChessBot summaries from either output stream, retains
Stockfish summary support, pins the entire engine process before its threads
start, reports failures clearly and requires no third-party Python packages.
The PGO target also rebuilds instrumented objects instead of silently reusing
objects from another build mode.

Validation:

- Exact baseline search-output equality at depths 6, 12, 14 and 16, including
  scores, PVs, best moves and intermediate node counts. Node totals are 58,755;
  2,254,747; 5,193,900; and 12,113,859 respectively.
- 360,192 threat-index comparisons against the original indexing formula.
- 36,391 incremental accumulator comparisons against full rebuilds, including
  skipped evaluations, sibling searches, mirror crossings, promotions, en passant,
  standard castling and Chess960 castling with overlapping origin/destination squares.
- AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer passed the
  accumulator tests. The full debug engine also passed `bench 6` with ASan/UBSan.
  LeakSanitizer was run outside the sandbox because its thread inspection is
  restricted inside it.
- Perft: start position depth 5 = 4,865,609; Kiwipete depth 4 = 4,085,603;
  rook/pawn endgame depth 5 = 674,624.
- UCI searches with 1, 2 and 4 threads completed successfully.
- AVX-512 and VNNI SIMD backends compiled. ARM NEON was not compiled
  or executed on this host.
- Benchmark parser and CLI checked for PZ/Stockfish output, constant rates,
  missing summaries, differing node counts and engine failures.

Reproduce locally (the original baseline binary is preserved in `build/`):

```sh
make -j8
make test
make test-debug
python3 bench.py build/pz-base ./pzchessbot 20 --cpu 2
python3 bench.py build/pz-base ./pzchessbot 8 --cpu 2 --depth 14
```

Raw measurements and detailed verification logs are in the ignored `build/`
directory. No playing-strength improvement is claimed; the changes preserve
search behavior in these checks.
