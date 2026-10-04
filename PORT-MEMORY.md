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

### Baseline products — verified CLI surface (Step 3 evidence)
- `D:\ninfer\ninfer-package\engine\ninfer-serve.exe`:
  - `--kv-dtype T` accepts `bf16 (default), int8, fp8, rk8v4, rk4v4, rk4v4-e8, rk2v4-e8, nvfp4, k8v4`
    → all three DoD §7.1 tiers (`rk8v4` / `rk4v4-e8` / `nvfp4`) are valid.
  - `--spec mtp|dflash|dflash2`, `--max-concurrency N` (1..8), `--device-snapshot-slots N`.
  - **The product ships only `ninfer-serve.exe` + `NInferManager.exe` — there is NO
    `ninfer-perplexity.exe`.** So ninfer-side ppl (M1 quality, plan §5) requires building
    `ninfer-perplexity` from this repo.
- `D:\ninfer\llamacpp\llama-perplexity.exe` (baseline side):
  - `--kv-tail-tokens SPEC` = `0 | auto | N | positional list | named group list`;
    `--kv-tail-type TYPE` = `f16 | bf16` (default `bf16` for standard caches, **`f16` for KVarN**).
  - KVarN cache types `kvarn2..kvarn8`; `--kvarn-window-chunk N` (default 65536).
  - Note: "KVarN always retains an intrinsic 128-token exact suffix".
  - → Confirms WP6 flag naming: mirror `--kv-tail-tokens` and (M2) `--kv-tail-type bf16|f16` exactly.
- Both products contain the plan's baseline models
  (`...IQ3_XXS-vision-bf16-mtp.ninfer` 11.09 GB; `...IQ3_XXS-mtp.gguf` 10.44 GB).

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
- Next: task #2 get a working configure/build via an in-workspace FFMPEG triplet tree;
  in parallel start WP1 storage and WP3 merge work.

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

## 5.2 Plan corrections (verified against beellama — see `PORT-BEELLAMA-SPEC.md`)

Three plan assumptions were checked against the real upstream source and **corrected**:
1. **§1.4 tail window:** upstream builds the per-query tail window on the **host** (newest-N finite,
   causal slot list in a flat descriptor), NOT on device from `positions`. Our plan said the opposite
   ("不能回读主机"). WP3 must decide host-descriptor vs device-window; host descriptor is proven.
2. **§1.5/WP4 graph key:** upstream **includes** `retention_tokens` (N) in the tail graph identity.
   Our plan said "长度不进 key" — wrong. Excluded is the *dynamic window* (n_tail, slot table, masks),
   which are runtime inputs validated by shape. Startup-fixed N in the key costs nothing for us.
3. **WP5 field names:** upstream splits `native_exact_bytes` / `exact_tail_bytes` /
   `rollback_reserve_bytes` / `transient_estimate_bytes`; our `kv_exact_history_bytes` /
   `kv_rollback_reserve_bytes` align with that split.
Confirmed as planned: two-partial online-softmax merge in a common FP32 domain (risk R1 resolved:
body FA already dequantizes to FP32), fused single-read dual write, `keep_last_writes` dedup,
commit-after-attention + rollback reserve.

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
