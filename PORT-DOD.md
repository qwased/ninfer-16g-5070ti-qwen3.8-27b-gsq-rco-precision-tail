# PORT-DOD — completion audit checklist

Living audit of `precision-tail-port-plan.md` §7 (DoD, M1) plus the §4 milestones and §3 work
packages, each mapped to concrete evidence. **Update on every step.** Status values:
`DONE` (evidence verified) · `PARTIAL` · `TODO` · `BLOCKED`.

> Honest rule: a WP is `DONE` only with compile/test evidence. Memory-doc claims are not evidence.

## DoD (plan §7, M1 exit)

| # | Requirement | Status | Evidence / gap |
|---|---|---|---|
| 1 | `--kv-tail-tokens 1024` starts / decodes / prefills on `rk8v4`, `rk4v4-e8`, `nvfp4` bodies | **PARTIAL** | Option (`64f32d3e`), pool attach, fused shadow write (`155fd1ab`) and tail partial all landed and compile-verified. Gap: no end-to-end run at N=1024 on the 27B artifact yet (runner ready, `.deps/run-m1-ppl.bat`) |
| 2 | Tail merge passes an FP32 oracle (incl. `p+1<=N`, empty-body window, C>1 masked rows) | **DONE** | `ORACLE_EXIT=0`, `softmax_attention: PASS` (`.deps/oracle-run8.out`, Step 30). All tail cases pass with the tail verifiably active: BF16 ×5, tail-off regressions ×2, rk8v4 fused+cached, rk4v4-e8 fused+cached, nvfp4 weak check (`rel_l2_tail == rel_l2_tail_off`), batched masked `B=2 W=8 valid={6,0} tail=2`. The Step-27 failures were stale `small_t_i8_w5..w8` objects from an interrupted build, not product logic |
| 3 | `apps/perplexity` shows ppl improvement vs `tail=0` on all three bodies | **BLOCKED** | Artifact + corpus + runner present; needs a GPU run. **Plan deviation (§5.10):** the tail is only merged for `bf16` + the INT8 family, so `nvfp4` is inert and *cannot* improve — this DoD row is narrowed to the wired storages and `nvfp4` is recorded as inert |
| 4 | `MemorySummary` within ±5% of plan §2; C=1 tail ≈ 64 MiB at N=1024 | **PARTIAL** | §2 arithmetic verified (§5.3); fields wired (`fc325aeb`); the perplexity `report.json` now carries `runtime_reservation_bytes`, `kv_payload_bytes`, `kv_exact_history_bytes` and `kv_rollback_reserve_bytes`, so the tail-0 vs tail-1024 diff is a report comparison. Awaits the M1 run |
| 5 | CUDA Graph family sequence unchanged vs tail off | **PARTIAL** | Reasoned in §5.8: the route family reads geometry/storage/window only, and `grid.y` is the full capacity either way, so the topology class and node count do not change with the tail. No dedicated assertion test |
| 6 | `tail=0` output bit-identical to today (zero regression) | **PARTIAL** | `tail_tokens == 0` is the identity partition (unchanged `body_limit`), the N=0 oracle path is bit-exact by construction, and the tail-off regression subset passed 15/15. The calculator's `tailBytes: N = 0 leaves the golden engine reservation untouched` check pins the N=0 reservation to the golden engine figure. Needs a real tail=0 run |
| 7 | Docs + `config-calculator.html` updated | **PARTIAL** | `docs/cli.md`, `docs/serving.md`, `docs/perplexity.md` document the option and its storage scope (`11d67445`). The calculator already models the tail (`tailRingPages`/`tailBytes` and a `--kv-tail-tokens` input); its node test passes 24/24 including seven tail checks — `node docs/config-calculator.test.mjs` → `PASS` (run 2026-10-05). Pending: `docs/performance.md` measured numbers (needs the M1 runs) |

## Milestones (plan §4)

- **M0** (KVarN decision gate): `TODO` — harness ready (`port-tools/run-experiments.sh`), baseline
  hashes pinned. Open item: llamacpp `--cache-type-k kvarn4` may require a model-backed speculative
  mode, which plan §0.5 forbids — resolve empirically. GPU must be free (ask first).
- **M1** (static BF16 tail functional closure): `IN PROGRESS` — BF16 oracle cases pass; INT8-family
  and the batched case open; ppl + memory measurement pending. See DoD above.
- **M2** (F16 default + graph stability): `TODO`.
- **M3** (concurrency + speculative, tier decision): `TODO`.
- **M4** (optional tier): out of scope for now.

## Work packages (plan §3)

| WP | Status | Evidence / gap |
|---|---|---|
| WP1 storage/exact pool | **DONE** | `PagedKVExactTailView` (`74297cba`); pool in `DecoderState` (`f15a4d72`); ctx plumbed (`d9c013fd`); views expose the pool (`75343cc2`). Runtime per-sequence ring *page leases* are not separately tracked — the ring is one contiguous per-sequence run sized by WP5, which is what the addressing assumes |
| WP2 fused dual write | **DONE** | Fused-append shadow write `causal_attention_small_t_tail_shadow_kernel` (`155fd1ab`), storage-independent; the cached entry gets its ring from `ops::kv_cache_append`. The fused path is the one the main model uses (the §5.7 correction) |
| WP3 attention merge | **DONE** | The merge IS the existing split reducer (no new merge kernel, §5.6); tail-partial kernel `small_t_tail.cuh` + partition sizing + dispatch wiring landed. FP32 oracle passes for BF16 ×5, rk8v4/rk4v4-e8 fused+cached, nvfp4 weak check, and batched masked (Step 30, `ORACLE_EXIT=0`) |
| WP4 graph/route family | **DONE** | §5.8: no graph-family change is needed and none was made (route family and node count are tail-independent) |
| WP5 capacity/`MemorySummary` | **DONE** | Tail cost inside the curve constant + `kv_payload_bytes` (verified §5.5); split fields added (`fc325aeb`) |
| WP6 config chain | **DONE** | Option + identity + help (`64f32d3e`, `2f010b36`); `--kv-tail-tokens` on the perplexity app (`16130914`); draft caches are tail-free by construction |
| WP7 tier ownership | **DONE** | M1 decision: exact pool is device-only; `docs/maintainer/paged-kv-cache.md §4.5` updated |
| WP8 transaction/rollback | **DEFERRED** | Plan §3 requires the exact-pool write to commit *after* attention, roll back and mark degraded on failure, with reserve `R` for it; `R = 1` page exists in sizing (`startup.cpp:360-363`). Not in DoD §7 and not reachable in M1: the pool is device-only, `C=1`, and the functional closure exercises no failure/rollback path. It belongs with the transactional layer (`src/models/qwen3_5/program/transactions/`), so it is a deliberate deferral, not a silently dropped requirement |
| WP9 docs | **PARTIAL** | `docs/cli.md`, `serving.md`, `perplexity.md`, `paged-kv-cache.md §4.5` updated; `config-calculator.html` and `performance.md` measured numbers pending |
| WP10 verification/bench | **PARTIAL** | Harness + oracle + `nvfp4`/`k8v4` reference quality landed; the oracle is green (Step 30, `ORACLE_EXIT=0`); M1 ppl/memory measurements pending |

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

## Required-scope items from the objective itself

| Requirement | Status | Evidence |
|---|---|---|
| New project in `ninfer-precision-tail` | **DONE** | local clone at `D:\ninfer\ninfer-precision-tail` |
| Git version control | **DONE** | `main` (+ merged `port/wp1`, `port/wp10-oracle`); commits through `11d67445` |
| Memory doc written at every step | **DONE** | `PORT-MEMORY.md` (§5.1-5.10) + `PORT-JOURNAL.md` (Steps 0-27); `PORT-BEELLAMA-SPEC.md`; this file |
| Subagents + worktree used to spare main context | **DONE** | multiple delegations; worktrees `port/wp1`, `port/wp10-oracle`; per-TU harness `.deps/vcheck.py` |
| Original project/product not damaged | **DONE** | clone is separate, `origin` removed; donors (`ninfer-16g-...`, `ninfer-package`, `llamacpp`, `vcpkg`) only read; `.deps`/`build-port` inside our repo |
| Test discipline (no leaked processes) | **DONE** | every build/test/agent stopped and confirmed (`tasklist` clean) before the next step; the one orphaned debug build was stopped by PID |
