# PORT-MEMORY — ninfer KV cache precision tail

> Living memory document. **Update on every step.** Read this first before doing any work.
> Companion: `PORT-JOURNAL.md` (append-only chronological log).

## 1. Objective

Implement the plan in `precision-tail-port-plan.md` (KV-cache precision tail port for ninfer),
inside **this** repository (`D:\ninfer\ninfer-precision-tail`), under git version control.

Hard constraints from the user:
- **Never modify the original project** `D:\ninfer\ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco`
  or its products (`D:\ninfer\ninfer-package`, `D:\ninfer\llamacpp`). This repo is a *local clone*;
  its `origin` remote was removed to make accidental push-back impossible.
- Persist memory to disk at every step (this file + `PORT-JOURNAL.md`).
- Use subagents and git worktrees heavily so the main agent context window is not exhausted.
- Reference report: `kvarn-kv-tail-feasibility-report.md` (v2), sections 5.2/5.3/6.

## 2. Repository baseline

- Cloned 2026-10-05 from the original repo at HEAD `b06908ba`
  (`feat(manager): allow direct access and LAN engine endpoints`), branch `main`.
- Local git identity set (repo-local only): `precision-tail <precision-tail@local>`.
- No `origin` remote.
- `precision-tail-port-plan.md` and `kvarn-kv-tail-feasibility-report.md` copied into repo root.

### Donor / reference trees (read-only, never modify)
- `D:\ninfer\ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco` — original ninfer source.
- `D:\ninfer\beellama.cpp` — upstream reference for KVCPT/`--kv-tail-tokens` design only (no port).
- `D:\ninfer\llamacpp` — llama.cpp-family product (KVarN + KVCPT) used as M0/M1 external baseline.
- `D:\ninfer\ninfer-package` — ninfer product (engine + model) used as M0/M1 baseline.

## 3. Hardware / build baseline (per plan §0.5)

- GPU: RTX 5070 Ti 16 GB, sm_120a, CUDA 13.3 (`nvcc` on PATH).
- Note: repo `AGENTS.md` still claims sm_86/3090 (upstream fork text). Per plan §0.5 the test
  baseline is **pinned to sm_120a / 5070 Ti**; the doc contradiction is not a blocker.
- Toolchain (resolved, Step 1): cmake 3.31.6 + ninja at
  `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\`
  (subdirs `CMake\bin\cmake.exe`, `Ninja\ninja.exe`); MSVC v143 14.44.35207 (VS2022 BuildTools only);
  nvcc CUDA 13.3.33. Configure must override arch to `120a` + `NINFER_SM120_NATIVE=ON`.
- **Build blocker:** Windows requires FFMPEG from a vcpkg triplet tree
  (`$VCPKG_ROOT/installed/x64-windows`, see `cmake/FindFFMPEG.cmake`). No such tree exists on disk and
  `D:\ninfer\vcpkg` is not bootstrapped. Plan: build a self-contained tree under
  `D:\ninfer\ninfer-precision-tail\.deps\vcpkg-root\` (never bootstrap/modify the shared `D:\ninfer\vcpkg`).
  CURL needed only if `NINFER_BUILD_PRODUCT_SUPPORT=ON`.

## 4. Plan shape (summary — authoritative text is `precision-tail-port-plan.md`)

Work packages: WP1 storage/exact pool · WP2 fused dual write · WP3 attention merge (key path,
biggest risk R1) · WP4 CUDA graph/route family · WP5 capacity + MemorySummary · WP6 config chain ·
WP7 tier ownership · WP8 transaction/rollback · WP9 docs · WP10 verification.

Milestones: M0 decision gate (KVarN 收口 experiment on the two products) → M1 static BF16 tail
functional closure on bodies `rk8v4` / `rk4v4-e8` / `nvfp4` → M2 F16 default + graph stability →
M3 concurrency & speculative → M4 optional tier.

Key design decisions already fixed by the plan:
- Tail = second same-geometry exact pool (BF16 at M1, F16 target), HeadMajor, 2 planes.
- Merge inside the **small-T family** (no new Op family, no new route family) → graph-safe.
- Merge semantics = two FP32 partials combined by online-softmax `(m,l)`, **not** normalized add.
- M1: prefill writes but does not merge; decode merges. Exact pool is device-only in M1.

## 5. Current state

- **Step 0 (done):** scaffold — clone, remote removed, identity set, plan+report copied,
  task list created, memory docs created. See `PORT-JOURNAL.md`.
- **Step 1 (done):** build env recon + WP1–3 code dossier (two read-only subagents).
  Toolchain resolved; FFMPEG/vcpkg is the build blocker; plan anchors corrected
  (`DeviceKVPagePoolSpec` and `PagedKVPlaneOrder` live in `src/core/paged_kv_cache.h`).
- **WP1 (done, `port/wp1`):** `PagedKVExactTailView` attached as `.tail` on both layer views
  (`src/core/paged_kv_cache.h:27-34,46,59`), the exact pool planned in
  `decoder_state.cpp:118-163` (`BFloat16`, `HeadMajor`, two planes per full-attention layer) with
  `retention = kv_tail_tokens` and `ring_pages`, wired at `PagedKVCache::layer_view`
  (`decoder_state.cpp:212-219`).
- **WP2 (done, `port/wp1`):** `kv_cache_append_tail_bf16_kernel` (`append/kernel.cuh:69-104`)
  shadow-writes the ring, launched from `append/launch.cu:25-38` for a single-sequence view.
  **Caveat found in WP3:** it hangs off `kv_cache_append_launch`/`_batch_launch`, and the
  text-layer decode path does not call those (it fuses the append into the small-T partial), so no
  production path currently fills the ring.
- **WP3 step 1 (done, `port/wp1`, 2026-10-05):** exact-tail partial kernel
  (`causal_cache/small_t_tail.cuh`), the shared `causal_small_t_tail_partition` helper
  (`small_t.cuh:134-166`) and the body/`body_window` substitution + dispatch for the BF16 storage
  path. Compile-verified with `.deps/ptcheck.py` on `small_t.cu` +
  `causal_softmax_attention.cpp` (clean). Details and deviations: PORT-JOURNAL + section 5.6.
- Next: WP3 steps 2-3 (INT8 family, then nvfp4/fp8/k8v4); the fused-append entry stays untailed
  until the write path covers the newest rows.

## 5.1 Corrected anchor map (from Step 1 dossier)

- Merge primitives to reuse: `src/ops/softmax_attention/dense/causal_cache/small_t.cuh`
  `causal_merge_split_statistics` (L183-210) + `causal_attention_small_t_reduce_output_kernel`
  (L212-299). Partial writers: `small_t_i8.cuh:938-975`, `small_t_bf16.cuh:18-25`.
  Workspace: `causal_softmax_attention.cpp:269-284`. Route: `causal_softmax_attention.cpp:347-402`.
- Second-pool precedent: `startup.cpp:247-287`, `decoder_state.cpp:76-80`, `decoder_state.h:114,128`.
- Write path: `src/ops/kv_cache/append/launch.cu` (201/219; template 16-121), `kernel.cuh` (BF16 69-104).
- Storage types: `src/core/paged_kv_cache.h` (PagedKVLayerView 22-31, PagedKVPlaneOrder 58,
  DeviceKVPagePoolSpec 71); `src/core/paged_kv_storage.h` (`paged_kv_storage_layout` 58-121).
- Test harness: `tests/ops/softmax_attention/causal_cache.cpp` (`run_a1_case` 2552-2640,
  FP64 `ideal_attention` 1519).

## 6. Working protocol (how we operate here)
1. One work package per branch/worktree. Subagents do the reading + editing; the main agent keeps
   only summaries. Never let a subagent modify the original trees.
2. Every step: (a) do the work, (b) update `PORT-JOURNAL.md` with an entry, (c) update §5 of this
   file, (d) `git commit` the code **and** the memory update together.
3. Build/verify commands and raw outputs are recorded in the journal, not re-run needlessly.
4. A work package is "done" only with evidence: compile success and, where applicable, a test/oracle
   result — not intent.

## 7. Open questions / risks to resolve

- R1 cross-domain merge (INT8 body × BF16 tail): first falsifiable step of M1 (plan §6 R1).
- Exact cmake path + a working configure preset for a CUDA Release build (#2).
- Whether M0's product-level experiments can run on this host now (products present; needs GPU idle).

## 8. WP3 merge design as implemented (exact tail)

- Split partition (shared, device side, `small_t.cuh:134-166`):
  `total_active = causal_small_t_active_splits(window, launch_capacity, tokens[, wave_splits])`
  (unchanged, identical to what the reducer computes);
  `tail_keys = min(retention, window)`;
  `body_window = window - tail_keys`;
  `body_active = body_window <= 0 ? 0 : min(active_splits(body_window), total_active)`;
  `tail_active = total_active - body_active`.
- Body partial: splits `[0, body_active)` cover keys `[0, body_window)` (the plan substitution).
  With `retention == 0`, `body_window == window` and `body_active == total_active`: bit-identical.
- Tail partial (`small_t_tail.cuh`): splits `[body_active, total_active)` cover keys
  `[body_window, window)` read from the exact BF16 ring
  (`page = batch * ring_pages + ((key >> 6) % ring_pages)`), written at the global split index
  `body_active + blockIdx.y`. Launched with the body grid `(KVHeads, launch_capacity, batch)`.
- Reducer: untouched (`causal_attention_small_t_reduce_output_kernel`,
  `causal_merge_split_statistics`). It merges `[0, total_active)`; no merge kernel is added.
- Workspace/capacity, route family and grid: untouched.
- Deviations from the plan text: (a) `body_active` for an empty body is 0 rather than the
  non-positive-window default of `causal_small_t_active_splits` (which is `launch_capacity`), and it
  is clamped to `total_active` because the INT8 tiers are not monotonic in the window; (b) the tail
  uses the uniform split mapping (`units_per_split` over `tail_keys`), not nvfp4's proportional
  variant -- any partition of the tail key range is correct because the reducer weights each split
  by its own `exp(m_i - max)`.
- Not wired: the fused-append entry (`CacheInput::writes_cache`), the launch that both attends and
  appends. Its append range follows the split range, so a shortened body would never write the
  newest N rows to the quantized body cache, and the ring is not written on that path at all.
  Retention 0 keeps the entry bit-identical. Wiring it requires the fused append to shadow-write its
  new rows into the ring and to decouple its write range from its read range.
