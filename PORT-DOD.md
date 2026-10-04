# PORT-DOD — completion audit checklist

Living audit of `precision-tail-port-plan.md` §7 (DoD, M1) plus the §4 milestones and §3 work
packages, each mapped to concrete evidence. **Update on every step.** Status values:
`DONE` (evidence verified) · `PARTIAL` · `TODO` · `BLOCKED`.

> Honest rule: a WP is `DONE` only with compile/test evidence. Memory-doc claims are not evidence.

## DoD (plan §7, M1 exit)

| # | Requirement | Status | Evidence / gap |
|---|---|---|---|
| 1 | `--kv-tail-tokens 1024` starts / decodes / prefills on `rk8v4`, `rk4v4-e8`, `nvfp4` bodies | **PARTIAL** | Option exists (`port/wp1 64f32d3e`); tail pool attach in flight; no kernel yet → cannot run |
| 2 | Tail merge passes an FP32 oracle (incl. `p+1<=N`, empty-body window, C>1 masked rows) | **TODO** | No merge kernel, no test. Test shape: extend `tests/ops/softmax_attention/causal_cache.cpp` `run_a1_case` (L2552-2640) + FP64 `ideal_attention` (L1519) with tail rows |
| 3 | `apps/perplexity` shows ppl improvement vs `tail=0` on all three bodies | **BLOCKED** | Product ships no perplexity binary; needs our build. Published sm_86 tail=0 refs: `rk8v4` 4.346811, `nvfp4` 4.358924, `bf16` 4.343225 (PORT-MEMORY §5.3) |
| 4 | `MemorySummary` within ±5% of plan §2; C=1 tail ≈ 64 MiB at N=1024 | **PARTIAL** | §2 arithmetic verified (PORT-MEMORY §5.3); `MemorySummary` fields + wiring in flight |
| 5 | CUDA Graph family sequence unchanged vs tail off | **TODO** | Design fixed (§1.5, PORT-MEMORY §5.4: merge inside small-T family); needs the WP4 assertion test |
| 6 | `tail=0` output bit-identical to today (zero regression) | **PARTIAL** | All landed changes are default-disabled; needs a real tail=0 run to confirm |
| 7 | Docs + `config-calculator.html` updated | **TODO** | WP9 untouched |

## Milestones (plan §4)

- **M0** (KVarN decision gate): `TODO` — harness ready (`port-tools/run-experiments.sh`), baseline
  hashes pinned. Open item: llamacpp `--cache-type-k kvarn4` may require a model-backed speculative
  mode, which plan §0.5 forbids — resolve empirically. GPU must be free (ask first).
- **M1** (static BF16 tail functional closure): `IN PROGRESS` — see DoD above.
- **M2** (F16 default + graph stability): `TODO`.
- **M3** (concurrency + speculative, tier decision): `TODO`.
- **M4** (optional tier): out of scope for now.

## Work packages (plan §3)

| WP | Status | Evidence / gap |
|---|---|---|
| WP1 storage/exact pool | **DONE (scaffolding)** | `PagedKVExactTailView` (`74297cba`); pool planned in `DecoderState` (`f15a4d72`); ctx plumbed (`d9c013fd`); views expose the pool (`75343cc2`). Compile-verified per TU. NOT done: per-sequence ring page leases at runtime |
| WP2 fused dual write | **TODO** | Anchor: `src/ops/kv_cache/append/launch.cu` (201/219, template 16-121), `kernel.cuh` BF16 kernel 69-104; upstream semantics in PORT-BEELLAMA-SPEC §C |
| WP3 attention merge | **TODO** (design fixed) | Merge reuse: `small_t.cuh` `causal_merge_split_statistics` 183-210 + `..._reduce_output_kernel` 212-299; design in PORT-MEMORY §5.4 |
| WP4 graph/route family | **TODO** | N goes in the tail identity (§5.2 correction 2); dynamic window excluded |
| WP5 capacity/`MemorySummary` | **PARTIAL** | Tail cost already inside the curve constant + `kv_payload_bytes` (verified, PORT-MEMORY §5.5 — plan premise was wrong). Optional split fields TODO |
| WP6 config chain | **DONE** | Option + identity + help (`64f32d3e`, `2f010b36`); draft caches are tail-free by construction (`d9c013fd`) |
| WP7 tier ownership | **TODO** | M1 decision: exact pool device-only; update `docs/maintainer/paged-kv-cache.md §4.5` |
| WP8 transaction/rollback | **TODO** | Commit-after-attention + reserve `R` |
| WP9 docs | **TODO** | `docs/cli.md`, `serving.md`, `config-calculator.html`, `paged-kv-cache.md §4.5`, `performance.md` |
| WP10 verification/bench | **PARTIAL** | Harness + baseline manifest + oracle reference landed; oracle test + measurements TODO |

## Required-scope items from the objective itself

| Requirement | Status | Evidence |
|---|---|---|
| New project in `ninfer-precision-tail` | **DONE** | local clone at `D:\ninfer\ninfer-precision-tail` |
| Git version control | **DONE** | `main` + `port/wp1`; commits `cc8c529a`, `3d91febd`, `271fa8fb`, `9cc18f50`, `43bb5565`, `fc6152e9`, `1a7d281d`, `f2b31336`, `9833368f` |
| Memory doc written at every step | **DONE** | `PORT-MEMORY.md` + `PORT-JOURNAL.md` (Steps 0-9); `PORT-BEELLAMA-SPEC.md`; this file |
| Subagents + worktree used to spare main context | **DONE** | 5 delegations; worktree `port/wp1` (manual — see journal Step 2) |
| Original project/product not damaged | **DONE** | clone is separate, `origin` removed; donors only read; `.deps`/`build-port` inside our repo |
