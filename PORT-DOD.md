# PORT-DOD — completion audit checklist

Living audit of `precision-tail-port-plan.md` §7 (DoD, M1) plus the §4 milestones and §3 work
packages, each mapped to concrete evidence. **Update on every step.** Status values:
`DONE` (evidence verified) · `PARTIAL` · `TODO` · `BLOCKED`.

> Honest rule: a WP is `DONE` only with compile/test evidence. Memory-doc claims are not evidence.

## DoD (plan §7, M1 exit)

| # | Requirement | Status | Evidence / gap |
|---|---|---|---|
| 1 | `--kv-tail-tokens 1024` starts / decodes / prefills on `rk8v4`, `rk4v4-e8`, `nvfp4` bodies | **DONE** | All three bodies ran end-to-end at N=1024 on the 27B artifact: `ninfer-perplexity` loaded it and prefilled/scored 261,223 tokens with `PPL_EXIT=0` (`.deps/ppl-{rk8v4,rk4v4-e8,nvfp4}-t1024/`). Decode (small-T, width<=8) with the tail active is covered by the FP32 oracle (`.deps/oracle-run9.out`, `softmax_attention: PASS`). Decode at N=1024 is now also run end-to-end through the `ninfer` cli on the 27B artifact, all three bodies, `EXIT=0` (Step 36): `rk8v4` with and without `--spec mtp`, `rk4v4-e8`, `nvfp4` (`.deps/m3-*.err`, prefill ~145 tok/s, decode ~68 tok/s, MTP rounds 12) |
| 2 | Tail merge passes an FP32 oracle (incl. `p+1<=N`, empty-body window, C>1 masked rows) | **DONE** | `ORACLE_EXIT=0`, `softmax_attention: PASS` (`.deps/oracle-run8.out`/`-run9.out`, Steps 30/32). All tail cases pass with the tail verifiably active: BF16 ×5, tail-off regressions ×2, rk8v4 fused+cached, rk4v4-e8 fused+cached, nvfp4 weak check (`rel_l2_tail == rel_l2_tail_off`), batched masked `B=2 W=8 valid={6,0} tail=2`. The Step-27 failures were stale `small_t_i8_w5..w8` objects from an interrupted build, not product logic. **M5 guards added (Steps 49/50/54):** `run_fused_empty_body_append_case` (the quantized body cache holds `[0, window)` after an empty-body fused step), `run_fused_crossing_case` (multi-step: rows built while `window <= N` are later read by a `window > N` body), `run_prompt_ring_write_case` (a Prompt-route step's rows reach the ring, read back by a following small-T step; wired for bf16 and rk8v4 ×2 mappings) and `run_fused_chunked_ring_case` (a chunked fused launch whose second chunk starts above column 0). The last two fail before `b99ba8d5` and pass after; full re-run green (`.deps/m5-oracle-fix2.log`) |
| 3 | `apps/perplexity` shows ppl improvement vs `tail=0` on all three bodies | **DONE (scoped: the tail-capable storages)** | `nvfp4` is inert by design (deviation #2) and cannot move; for `bf16`, `int8`/`rk8v4` and `rk4v4-e8` the decode-width metric now shows the gain. **M5 execution (Steps 38-52) + the ring-population fix (Step 54)** — see `docs/performance.md` "Decode-width KLD" for the full tables. Post-fix, every tail-capable storage improves and the gain is monotone in body coarseness: mean KLD tail-on ≤ tail-off in **18/18** WP-B cells (ctx 8192/16384/32768 × N 1024/2048), gain 1.02–1.10× (int8) < 2.04–2.30× (rk8v4) < 2.97–4.04× (rk4v4-e8); `same_top` and ppl move the same way. The last open item — `bf16` **storage** + tail, which the earlier M5 pass recorded as breakage outside the WP-B matrix — is now fixed and measured as a controlled A/B against the *same* bf16-tail0 reference and corpus: mean KLD **0.13284577 → 0.00093566** (−142×), max 16.29 → 0.475, `same_top` 0.9004 → 0.9867 (ctx 2048, N=1024, W=8, 36,846 scored tokens); the rk8v4 cell of the same protocol is bit-identical pre/post fix (0.0011331605820924095), and the bf16 tail-off control stays KLD 0 / `same_top` 1.0. `docs/performance.md` carries the tables. *(Superseded history, kept for the audit trail: earlier in M5 the mechanism was first read as*) Two independent reasons, both by design (plan deviations #2 and #5). (a) The tail is merged only for `bf16` + the INT8 family, so `nvfp4` is inert. (b) For every storage the perplexity app scores the **prefill phase** (`apps/perplexity/main.cpp:94-97`) in 1024-wide query tiles (`score_tile_tokens`, `main.cpp:534`) -> **prompt** route, which plan §1.5 says "only writes, decode merges". The tail is never read during scoring, so all three measured `overall.perplexity` are bit-identical at tail=0 and tail=1024 (rk8v4 4.65880995706738, rk4v4-e8 4.675237004820881, nvfp4 4.657980442927839). A decode-width ppl (width<=8) is needed to observe the tail's effect and is not reachable without new app work. The tail's *quality* effect is instead evidenced on the decode route by the FP32 oracle plus the new `TAILGAIN` comparison (Step 33): tail-on rel-L2 is strictly below tail-off rel-L2 on all six wired cases, e.g. h24 rk8v4 N=6 6.8711e-02 vs 7.0782e-02. **M5 update (Steps 38-45) — resolved as NEGATIVE, defect found.** The new instrument exists: `apps/perplexity --score-width W` scores in width-`W` tiles (`W<=8` selects the small-T route that reads the tail) and `--save-topk`/`--kld-base` measure KLD against a persisted reference over top-K 100. With it, (a) the A4 gate passes (tail=0 ppl W=1 ≈ W=1024; tail=N ppl W=1 < W=1024); (b) when the tail covers the **whole** window (`ctx = N = 1024`) the tail is correct and improves (ppl 6.496149→6.464423, mean KLD 0.002647→0.000912; and ctx 2048/tail 2048 → KLD 0.001068 < tail0 0.002838); but (c) whenever a **body and the tail coexist** the merge is **wrong**: ctx 2048/rk8v4 tail1024 KLD 0.022539 vs tail0 0.002838; ctx 8192 int8/rk8v4 tail1024 ≈ 0.027 vs ≈ 0.001-0.003 tail0; and the tell is **bf16 body+tail vs a bf16 reference giving KLD 0.137** (same precision both sides ⇒ a correct merge would be ≈0). It reproduces at W=1 and W=8, for bf16/int8/rk8v4; the in-tree oracle (T=6, keys≤67, or a single fused append step) does not cover this regime. The external reference is decisive: llama.cpp `kvarn4` (tail merged inside FA at all widths) cuts mean KLD 0.001107→0.000702 (−37%) and max 0.188→0.078 on the same model family, so the benefit is real and ninfer's merge is the gap. See `docs/performance.md` "Decode-width KLD". **Root cause closed (Step 46, code-derived):** not the merge and not the partition — the fused-append small-T kernel writes the quantized cache **only from body splits**, and with `body_window == 0` the partition sets `body_active = 0` so every split returns before the append block; while the tail covers the whole window (`window ≤ N`) the rows are never quantized (the shadow kernel writes only the ring), so `ctx > N` bodies read a permanent hole. See `PORT-JOURNAL.md` Step 46 and `PORT-MEMORY.md` §5.13. |
| 4 | `MemorySummary` within ±5% of plan §2; C=1 tail ≈ 64 MiB at N=1024 | **DONE** | Measured C=1 (`.deps/ppl-*-t0` vs `-t1024/report.json`): `kv_exact_history_bytes` 0 → **67,108,864 = 64 MiB exactly** for every wired storage, matching plan §2 `round_up(N,64)×65,536×C` at **0% error**; `kv_rollback_reserve_bytes` 0 → 4,194,304 (4 MiB, one page, inside the plan's "+16 MiB/C" allowance); rk8v4 `runtime_reservation_bytes` +71,303,168 = exactly 68 MiB. The `memory` block needed a rebuild: the app's `main.cpp:499-521` had the fields but the binary was stale (Step 31) |
| 5 | CUDA Graph family sequence unchanged vs tail off | **DONE** | Assertion test added: `run_graph_family_stability_cases` in `tests/ops/softmax_attention/causal_cache.cpp` (Step 32), wired into the tail runner. 8 decode shapes × {tail off, tail on} assert the route family and the launch capacity (`grid.y`) are identical, and the family is the pinned small-T family. `.deps/oracle-run9.out`: `exact KV tail: decode graph family / launch shape is tail-independent (WP4)` then `... graph family=0 grid.y=8|16` for every pair, `softmax_attention: PASS` |
| 6 | `tail=0` output bit-identical to today (zero regression) | **DONE** | `tail_tokens == 0` is the identity partition (unchanged `body_limit`); the oracle's tail-off regression case builds the same BF16 output with the field defaulted and set explicitly to 0 and asserts bit-for-bit equality (`softmax_attention: PASS`). The N=0 ppl runs completed clean (`PPL_EXIT=0`), and the calculator's `tailBytes: N = 0 leaves the golden engine reservation untouched` check pins the N=0 reservation to the golden engine figure. The M5 fix is tail-on-only, confirmed twice at the KLD level on the fixed build: the bf16 tail-off candidate still scores mean KLD exactly 0 / `same_top` 1.0 against its own reference, and the rk8v4 tail-on cell of the same protocol is bit-identical to its pre-fix value (0.0011331605820924095) |
| 7 | Docs + `config-calculator.html` updated | **DONE** | `docs/cli.md`, `docs/serving.md`, `docs/perplexity.md` document the option and its storage scope (`11d67445`); `docs/performance.md` now carries the measured RTX 5070 Ti tail section (memory table, ppl-invariance finding, Step 32). The calculator models the tail (`tailRingPages`/`tailBytes` + a `--kv-tail-tokens` input); `node docs/config-calculator.test.mjs` → `PASS` 24/24 including seven tail checks |

## Milestones (plan §4)

- **M0** (KVarN decision gate): **DONE (first pass)** — open item **RESOLVED**: `llama-perplexity
  --cache-type-k kvarn4` runs with **no** speculative mode (`--spec-type none` is a server/cli flag;
  the offline scorer does not speculate); it logs `KVarN requires Flash Attention; enabling it` and
  scores. The harness was also fixed to pass the corpus (`-f`), which it previously omitted
  (`port-tools/run-experiments.sh`). Pinned baseline (`data/wikitext/00.txt`, ctx 4096, 4 chunks,
  tail 0): **f16 5.3580, q8_0 5.3577, kvarn4 5.3559**; kvarn4 tail=1024 **5.3622** (all ±0.136).
  **Conclusion:** on this slice KVarN shows no measurable ppl penalty vs f16/q8_0 and the tail is
  within noise, so there is no *quality-driven* case to borrow KVarN — ninfer's own rotated rk4v4-e8
  is likewise indistinguishable from exact at this granularity. A low-noise decision needs a longer
  chunk count (Phase-2 trigger, not a blocker). Artifacts under `port-tools/results/20261005-101*`.
- **M1** (static BF16 tail functional closure): **DONE** — the FP32 oracle is green for BF16 +
  the INT8 family (fused and cached) and the batched masked case, the graph-family assertion passes,
  the memory footprint matches plan §2 exactly, and the exit clause "quality improvement measurable"
  is met on the decode route by the `TAILGAIN` comparison (tail rel-L2 strictly below tail-off on
  all six wired cases, Step 33). The ppl row (§7.3) is `NEGATIVE` after M5 (Steps 38-45): the
  decode-width instrument was built (`--score-width`, KLD), and it shows the tail is correct and
  beneficial only when it covers the whole window, but is **mis-merged whenever a body contributes**
  (bf16 body+tail KLD 0.137 vs a bf16 reference; see the §7.3 row and `docs/performance.md`). This
  supersedes the earlier "harness limitation" reading and is a **defect to fix**, not a closure
  failure of the small-window oracle.
- **M2** (F16 default + graph stability): **DONE** — the exact ring's element type is now a config
  dimension (`KvTailType` / `--kv-tail-type bf16|f16`, F16 default) carried through the CLI, serve,
  planning and `DecoderStateSpec` chains and folded into the engine identity tag; both 16-bit, so
  the page geometry and `MemorySummary` are unchanged. The three tail kernels are element-generic
  through `ops/common/kv_tail_element.cuh` (`if constexpr`; the BF16 instantiation stays verbatim +
  `mma_bf16`, F16 converts BF16→float→half + `mma_f16`), dispatching on the ring tensor's dtype. The
  graph-family assertion (WP4/§7.5) is unchanged and still passes. `ORACLE_EXIT=0` with F16
  correctness cases; F16-vs-BF16 chosen by the `WIDETAIL` wide-tail comparison (F16 lower in all
  four, never worse, identical cost) — commit `b64b6b1e`, Step 35.
- **M3** (concurrency + speculative, tier decision): **DONE** — (a) per-C footprint: the exact pool
  is `(page_count(N)+1) * C` page groups; a new host test `tests/models/qwen3_5/test_exact_tail_capacity.cpp`
  asserts `payload_bytes == round_up(N,64)*65,536*C + C*4 MiB` **exactly** for C=1..8 and N∈{512,1024,2048}
  (commit `6d77e0b6`), matching plan §2 to the byte. (b) Draft tail=0: the MTP/draft cache is
  `layout.mtp_kv`, a `PagedKVCacheLayout` with **no tail member at all**, so a draft can never read or
  write the exact ring; the test asserts `mtp_kv` payload is byte-identical tail-on vs tail-off and that
  it carries no tail planes. (c) Cross-tier decision: exact pool is device-only (WP7/M1, no slab/LRU/disk).
  (d) End-to-end two-state re-test on the 27B artifact (Step 36): `--spec mtp` at N=1024 and N=0 both
  complete the same 12 rounds (~24-26% acceptance, ~1.8-1.9 tok/round, `EXIT=0`), and plain decode at
  N=1024 completes too — the speculation path is not regressed by the tail.
- **M4** (optional tier): out of scope for now.
- **M5** (tail *benefit* on rk4v4-e8 / rk8v4 / int8, + MTP impact): **DONE — the benefit is
  established, and two ring-write defects were found and fixed on the way.** Campaign executed per
  `PORT-M5-PLAN.md` (Steps 38–55). WP-A: the instrument (`--score-width`, KLD via
  `--save-topk`/`--kld-base`) landed and passes its own falsifiable gate. WP-B: post-fix, tail-on ≤
  tail-off in **18/18** cells and the gain is monotone in body coarseness (int8 1.02–1.10× < rk8v4
  2.04–2.30× < rk4v4-e8 2.97–4.04×) at every ctx and tail length. WP-C: llama.cpp `kvarn4` confirms the
  concept independently (−37% mean KLD). WP-D: D3 evidenced; D1/D2/D4 have no pre-port build tree (D4
  non-gating). WP-F: F1/F2/F5 established by analysis (no ring pollution on rejection), F3/F4
  measured on the fixed build. WP-E: docs updated. The two write-side defects were the reason the
  benefit was unmeasurable at first: the fused-append split never quantized the tail rows when
  `body_window == 0` (`56fc8384`) and the `Prompt` route never wrote the ring at all (`b99ba8d5`).
  See the §7.3 row and `docs/performance.md`.

## Work packages (plan §3)

| WP | Status | Evidence / gap |
|---|---|---|
| WP1 storage/exact pool | **DONE** | `PagedKVExactTailView` (`74297cba`); pool in `DecoderState` (`f15a4d72`); ctx plumbed (`d9c013fd`); views expose the pool (`75343cc2`). Runtime per-sequence ring *page leases* are not separately tracked — the ring is one contiguous per-sequence run sized by WP5, which is what the addressing assumes |
| WP2 fused dual write | **DONE** | Fused-append shadow write `causal_attention_small_t_tail_shadow_kernel` (`155fd1ab`), storage-independent; the cached entry gets its ring from `ops::kv_cache_append`. The fused path is the one the main model uses (the §5.7 correction) |
| WP3 attention merge | **DONE** | The merge IS the existing split reducer (no new merge kernel, §5.6); tail-partial kernel `small_t_tail.cuh` + partition sizing + dispatch wiring landed. FP32 oracle passes for BF16 ×5, rk8v4/rk4v4-e8 fused+cached, nvfp4 weak check, and batched masked (Steps 30/32, `ORACLE_EXIT=0`); the tail's decode-route quality gain is asserted by `run_tail_quality_gain` (Step 33) |
| WP4 graph/route family | **DONE** | §5.8: no graph-family change is needed and none was made (route family and node count are tail-independent). Now asserted by `run_graph_family_stability_cases` (DoD §7.5) |
| WP5 capacity/`MemorySummary` | **DONE** | Tail cost inside the curve constant + `kv_payload_bytes` (verified §5.5); split fields added (`fc325aeb`); measured to match plan §2 exactly at C=1 (DoD §7.4) |
| WP6 config chain | **DONE** | Option + identity + help (`64f32d3e`, `2f010b36`); `--kv-tail-tokens` on the perplexity app (`16130914`); draft caches are tail-free by construction |
| WP7 tier ownership | **DONE** | M1 decision: exact pool is device-only; `docs/maintainer/paged-kv-cache.md §4.5` updated |
| WP8 transaction/rollback | **DEFERRED** | Plan §3 requires the exact-pool write to commit *after* attention, roll back and mark degraded on failure, with reserve `R` for it; `R = 1` page exists in sizing (`startup.cpp:360-363`). Not in DoD §7 and not reachable in M1: the pool is device-only, `C=1`, and the functional closure exercises no failure/rollback path. It belongs with the transactional layer (`src/models/qwen3_5/program/transactions/`), so it is a deliberate deferral, not a silently dropped requirement |
| WP9 docs | **DONE** | `docs/cli.md`, `serving.md`, `perplexity.md`, `paged-kv-cache.md §4.5` updated; `docs/performance.md` measured sections added (Steps 32/35, incl. the F16-vs-BF16 ring table); `--kv-tail-type` documented in `cli.md`/`serving.md`; `config-calculator.html` models the tail and its node test passes 24/24 |
| WP10 verification/bench | **DONE** | Harness + oracle + `nvfp4`/`k8v4` reference quality landed; the oracle is green including the graph-family assertion, the `TAILGAIN` quality comparison and the F16 ring cases (Steps 30/32/33/35, `ORACLE_EXIT=0`); the M1 measurement is recorded in `docs/performance.md` (memory + ppl + the prefill-route finding) and the M2 `WIDETAIL` F16-vs-BF16 wider-tail comparison (Step 35) |

## Plan deviations (allowed by the objective; each is recorded in PORT-MEMORY)

1. **The tail must be wired on the fused-append path, not only the cached one** (§5.7). The plan
   assumed decode uses the cached small-T entry; the main model uses the fused-append entry, so the
   original WP2 shadow write (in `ops::kv_cache_append`) was inert. Corrected by adding the fused
   shadow write and tailing both entries.
2. **The exact tail is merged for `bf16` + the INT8 family only** (§5.10). `fp8`, `nvfp4` and `k8v4`
   reach reduce kernels in a Hadamard-rotated frame; merging the raw BF16 tail rows there would need
   the tail rows rotated first, which is not implemented. On those storages `--kv-tail-tokens`
   allocates the ring but no route reads or writes it (inert). Plan DoD §7.3 assumed `nvfp4` would
   improve; it cannot, so that row is narrowed.
3. **`body_active` must stop the body one split short of the reducer range** (§5.10). The plan's
   partition only clamped the body at `total_active`; the split-tier floor then dropped the newest
   keys entirely. Found by *running* the oracle, not by compiling it.
4. **WP8 (transaction/rollback) is deferred, not dropped.** The write ordering it protects
   (commit-after-attention) and the degraded-on-failure marking only matter where a failure can
   occur mid-step; M1's functional closure is device-only and `C=1` and exercises none. The reserve
   `R` is already carried in the WP5 sizing, so the accounting is in place when the transactional
   layer lands (M2/M3).
5. **DoD §7.3 ("ppl improvement") is unobservable through `apps/perplexity` by design** (Step 31).
   The app scores the prefill phase (a 1024-wide prompt-route pass); the tail is merged only on the
   small-T decode route (width <= 8). So ppl is bit-identical with the ring on or off, for every
   wired storage, regardless of tail quality. The plan presumed the parse would show a delta; it
   cannot. Quality is evidenced by the FP32 oracle instead, and a true ppl delta would require a
   decode-width scoring mode the app does not have. `MemorySummary` (§7.4) *does* move and matches
   plan §2 exactly, which is the tail's measurable footprint in this harness.
   **M5 resolves this (Steps 38-45):** the decode-width scoring mode now exists (`--score-width W`,
   plus KLD via `--save-topk`/`--kld-base`). Its first pass showed a defect (bf16 body+tail vs a bf16
   reference: KLD 0.137) and the follow-up work was done: **two ring-write defects were found, fixed
   and re-measured (Steps 47-54)**, and every tail-capable storage now improves (mean KLD tail-on ≤
   tail-off 18/18; bf16 0.13285 → 0.00094 at ctx 2048). The llama.cpp cross-check (same model family)
   confirms the concept independently (kvarn4 KLD 0.001107→0.000702 with the tail). See
   `docs/performance.md` "Decode-width KLD" and the §7.3 row.

6. **A chunked small-T step derives its exact/quantized boundary from the chunk, not the step**
   (Step 54, discovered while writing the oracle guard). The tail partition's `window` is the chunk's
   last position + 1, because the rows after the chunk have not been appended yet, so a chunk can only
   merge keys it already has. A chunk therefore merges a *superset* of the exact keys the documented
   "newest N" rule would pick — never less accurate, but it makes a multi-chunk step deviate from the
   strict oracle's model, so the oracle's chunked case keeps the whole step inside the tail
   (`run_fused_chunked_ring_case`). The single-chunk decode route (width ≤ 8 for 24 query heads,
   ≤ 6 for 16) — the tail's habitat — has no such deviation.

7. **The exact ring has two independent writers and no single owner** (Step 54). The fused small-T
   shadow kernel (`small_t_tail_shadow.cuh`) and the Prompt launcher (`prompt.cu`) both write it, and
   `ops::kv_cache_append` writes it for the single-sequence view. A route that appends but forgets to
   shadow is a silent correctness hole (that is exactly `b99ba8d5`). The invariant to hold when
   touching an append path: **every route that appends rows must write the ring for those rows**, and
   the source offset must match the body kernel's flat column base.

## Outstanding characterisation (non-blocking; not acceptance criteria)

- **Plan §5 "performance" row.** The end-to-end decode shape (prefill flat, decode a few percent
  slower at N=1024) is recorded in `docs/performance.md` from the Step-36 cli runs, but the
  body+tail dual-write scan of `bench/ops/kv_cache_append_bench.cu` is **not run**: benchmarks are
  behind `NINFER_BUILD_BENCHMARKS` (OFF) and turning them on reconfigures the whole tree for what is
  a cost characterisation, not a correctness gate. Left for a benchmark pass if a throughput number
  is needed.
- **M4** (plan §4, optional: exact pool on host/disk tier; per-request tail observability API) is not
  started, by design — the plan gates it on a real need after M3 ("仅在 M3 后确有需求时启动").
- **WP8** (transaction/rollback) remains deferred; see deviation #4.
- **Merge numeric fidelity — the tail's own floor (Step 55).** After the two write-side fixes the
  tail-on mean KLD against an exact reference settles at ~9.2e-4 (`same_top` ~0.986) even when the body
  is *empty* and every merged row is exact bf16 — i.e. the residual is the small-T path's fp32/bf16
  reduction order, not a data defect. It is the figure a future merge-fidelity change would have to
  beat, and it is what caps the tail's benefit for the coarsest bodies. Not a gate; recorded so it is
  not re-discovered as a defect.
- **Generation-route A/B is not reproducible pre-fix** (Step 55): the fix was rebuilt in place, so the
  pre-fix `ninfer.exe` no longer exists. The pre-fix contrast for that route is the scoring A/B, which
  shares it. Re-running a pre-port-style generation A/B would need a second build tree (as WP-D D1/D2
  would).

## Required-scope items from the objective itself

| Requirement | Status | Evidence |
|---|---|---|
| New project in `ninfer-precision-tail` | **DONE** | local clone at `D:\ninfer\ninfer-precision-tail` |
| Git version control | **DONE** | `main` (+ merged `port/wp1`, `port/wp10-oracle`); commits through `6d77e0b6` (M2 F16 `b64b6b1e`/`8284ea95`, M3 capacity `6d77e0b6`) |
| Memory doc written at every step | **DONE** | `PORT-MEMORY.md` (§5.1-5.11) + `PORT-JOURNAL.md` (Steps 0-32); `PORT-BEELLAMA-SPEC.md`; this file |
| Subagents + worktree used to spare main context | **DONE** | multiple delegations; worktrees `port/wp1`, `port/wp10-oracle`; per-TU harness `.deps/vcheck.py` |
| Original project/product not damaged | **DONE** | clone is separate, `origin` removed; donors (`ninfer-16g-...`, `ninfer-package`, `llamacpp`, `vcpkg`) only read; `.deps`/`build-port` inside our repo |
| Test discipline (no leaked processes) | **DONE** | every build/test/agent stopped and confirmed (`tasklist` clean) before the next step; the one orphaned debug build was stopped by PID |
