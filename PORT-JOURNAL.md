# PORT-JOURNAL — append-only step log

Newest entries appended at the bottom. Each entry: date, step, what changed, evidence, next.

---

## 2026-10-05 — Step 0: project scaffold

- Cloned original ninfer repo (`D:\ninfer\ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco`,
  HEAD `b06908ba`) into `D:\ninfer\ninfer-precision-tail` via `git clone --local` (114 MB working
  tree, full history preserved). Original left untouched.
- Removed `origin` remote from the clone so no push can reach the original tree.
- Set repo-local git identity `precision-tail <precision-tail@local>`.
- Copied `precision-tail-port-plan.md` and `kvarn-kv-tail-feasibility-report.md` into the repo root
  so the repository is self-contained.
- Created task list: scaffold(#1), build env(#2), WP1(#3), WP2(#4), WP3(#5), WP5(#6), WP6(#7),
  WP4(#8), WP7/8(#9), WP9(#10), WP10(#11).
- Created `PORT-MEMORY.md` (living state) and this journal.
- Environment probe: `nvcc` = CUDA 13.3 present; `cmake`/`ninja` not on PATH; VS 2022 installed.

Evidence: `git log --oneline -1` → `b06908ba feat(manager): allow direct access and LAN engine endpoints`;
`git remote -v` → empty.

Next: commit Step 0; then task #2 locate cmake and produce a baseline Release CUDA configure/build.

---

## 2026-10-05 — Step 1: build environment recon + WP1-3 code dossier

Two read-only subagents were used (main context kept clean). No source files changed.

### Build environment (task #2)
- cmake: `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe` 3.31.6-msvc6.
  ninja: bundled next to it (`...\CMake\Ninja\ninja.exe`). Neither on PATH.
- MSVC: only VS 2022 BuildTools (x86) v143 14.44.35207. VS2026/18 absent (no v145 trap).
- nvcc: CUDA 13.3.33 (12.3 also present). GPU: RTX 5070 Ti, compute capability sm_120; driver 617.14.
- `nvcc -arch=sm_120a` probe compiled and ran a real kernel on the GPU → CUDA toolchain proven.
- **BLOCKER:** `cmake/Dependencies.cmake:22` requires `find_package(FFMPEG REQUIRED)` on Windows, resolved
  from a vcpkg triplet tree at `$ENV{VCPKG_ROOT}/installed/x64-windows` (see `cmake/FindFFMPEG.cmake`:
  needs `lib/{avformat,avcodec,avutil,swscale}.lib` + `include/libavformat/avformat.h`).
  `D:\ninfer\vcpkg` exists but is **not bootstrapped** (no `vcpkg.exe`, no `installed/`).
  No prebuilt vcpkg/FFmpeg tree exists anywhere under `D:\ninfer`. CURL is only required when
  `NINFER_BUILD_PRODUCT_SUPPORT` is ON.
- Presets `release`/`dev` do not set the vcpkg toolchain and leave arch at 86 → not usable as-is;
  must pass `-DCMAKE_CUDA_ARCHITECTURES=120a -DNINFER_SM120_NATIVE=ON` and a vcpkg/in-workspace tree.
- Planned workaround (do NOT bootstrap the shared `D:\ninfer\vcpkg`): create a self-contained triplet
  tree at `D:\ninfer\ninfer-precision-tail\.deps\vcpkg-root\installed\x64-windows` and export
  `VCPKG_ROOT`/`VCPKG_TARGET_TRIPLET=x64-windows`; only FFMPEG (+CURL if product support kept) needed.

### Code dossier (WP1/WP2/WP3) — plan anchors corrected
- `DeviceKVPagePoolSpec` is in `src/core/paged_kv_cache.h:71` (NOT `paged_kv_storage.h`);
  `PagedKVPlaneOrder` is `src/core/paged_kv_cache.h:58` (NOT `types.h`).
- Everything else verified: `KvCacheStorage` types.h:49-73; `EngineOptions` 353-486 (kv_cache@422,
  max_concurrency@415); `MemorySummary` 1294-1331; `PagedKVLayerView` paged_kv_cache.h:22-31 (k_pages,
  v_pages, k_scale_pages, v_scale_pages, block_table); `paged_kv_storage_layout` paged_kv_storage.h:58-121.
- DFlash second-pool precedent: `startup.cpp:247-287` (`paged_kv_storage_layout(BFloat16,..)`,
  `KVPageGeometry` HeadMajor, `DeviceKVPagePoolSpec{page_group_count=physical_pages}`); reusable
  `plan_cache()` in `decoder_state.cpp:76-80`; `mtp_kv` decl `decoder_state.h:114,128`.
- Write path: `kv_cache_append_launch` launch.cu:201-217; `_batch_launch` 219-251; central template
  `launch_full<...>` 16-121 dispatches FP8/INT8/BF16; BF16 kernel `kernel.cuh:69-104` is the natural
  shadow-write template; INT8 kernels `:167-255`/`:262-363`.
- Attention merge: `small_t.cuh` **`causal_merge_split_statistics` L183-210** and
  **`causal_attention_small_t_reduce_output_kernel` L212-299** are the exact online-softmax merge to
  mirror; `partial_acc/m/l` written in `small_t_i8.cuh:938-975`; BF16 partial producer
  `small_t_bf16.cuh:18-25`; `SmallTWorkspace{acc,m,l}` + `allocate_small_t_workspace` at
  `causal_softmax_attention.cpp:269-284`; route resolve 347-402; route family hook softmax_attention.h:212-216.
- Tests: `tests/ops/softmax_attention/causal_cache.cpp` `run_a1_case` L2552-2640, FP64 reference
  `ideal_attention` L1519, `verify_attention`, codec oracles `tests/ops/kv_cache_lloyd4_oracle.h`,
  `tests/ops/kv_cache_e8_root_host.h`; extend `HostCache`/`ideal_attention` with tail rows for a merge oracle.

### User directive (subagent model)
- User asked to use "deepseek-flash, reasoning high" for subagents. The Agent tool in this session
  exposes **no model/effort parameter** and no agent-profile config file was found; user chose
  "continue with defaults". Recorded here so the limitation is not re-litigated.

Next: task #2 get a working configure/build (in-workspace FFMPEG tree); in parallel start WP1/WP3.

---

## 2026-10-05 — Step 2: parallel agents (build unblock + Increment 1)

- Launched background agent **BUILD** (no worktree): resolve the FFMPEG/vcpkg blocker by creating a
  self-contained triplet tree at `D:\ninfer\ninfer-precision-tail\.deps\vcpkg-root\installed\x64-windows`
  (prebuilt MSVC-linkable FFmpeg via `lib.exe /def:` if possible, else an in-workspace vcpkg),
  then configure + build `ninfer_tests` (+`ninfer-perplexity`). Must not touch the shared `D:\ninfer\vcpkg`.
- Launched background agent **INCR1**: WP1 (tail page pool spec + optional view component + planning),
  WP6 subset (`--kv-tail-tokens`, EngineOptions field, identity tag, draft tail=0), WP5 accounting
  (`MemorySummary.kv_exact_history_bytes` / `kv_rollback_reserve_bytes`, `SequenceCapacityCurve` →
  constant + linear). No kernels; inert when tail=0.
- **Isolation lesson:** the Agent tool's `isolation: worktree` FAILED with
  `Failed to resolve base branch "HEAD": git rev-parse failed` because the session's primary working
  directory (`D:\ninfer`) is not a git repository — the mechanism derives the repo from cwd, not from
  the target path. Workaround adopted: create the worktree manually and point the agent at it:
  `git worktree add .worktrees/wp1 -b port/wp1` (from the repo root), then instruct the agent to
  work in `.worktrees/wp1`. `.worktrees/` added to `.gitignore` (commit `cc77009a`).
- Commit `cc77009a`: ignore `.worktrees/`.

Next: on agent completion, review INCR1 diff, fold BUILD findings into §3, then compile and iterate.

---

## 2026-10-05 — Step 3: baseline product CLI evidence (WP6 naming / WP10 readiness)

While the two background agents (BUILD, INCR1) ran, gathered read-only evidence from the two
baseline products (no GPU used, no files changed in donor trees):

- `D:\ninfer\ninfer-package\engine\ninfer-serve.exe --help`: `--kv-dtype` accepts
  `bf16, int8, fp8, rk8v4, rk4v4, rk4v4-e8, rk2v4-e8, nvfp4, k8v4` → all three DoD tiers valid.
  Also `--spec mtp|dflash|dflash2`, `--max-concurrency N` (1..8), `--device-snapshot-slots N`.
- **The ninfer product has no `ninfer-perplexity` binary** (only `ninfer-serve.exe` +
  `NInferManager.exe`). Consequence: M1 quality runs (plan §5) require building `ninfer-perplexity`
  from this repo — the product alone cannot produce ppl. Recorded in PORT-MEMORY §2.
- `D:\ninfer\llamacpp\llama-perplexity.exe --help` confirms the upstream KVCPT surface:
  `--kv-tail-tokens` (`0|auto|N|positional list|named group list`) and `--kv-tail-type`
  (`f16|bf16`; default `bf16` standard caches, `f16` for KVarN), KVarN types `kvarn2..kvarn8`,
  `--kvarn-window-chunk` default 65536; KVarN always retains an intrinsic 128-token exact suffix.
  → WP6 naming should mirror `--kv-tail-tokens` / `--kv-tail-type` exactly.
- Both baseline models present as the plan states.

Both agents still running at end of this step (WP1 worktree diff empty, `.deps` not yet created).

---

## 2026-10-05 — Step 4: experiment harness + verified beellama algorithm spec

Two independent artifacts (read-only w.r.t. donor trees; no GPU compute used).

### 4a. Reproducibility harness (WP10 / plan §0.5 item 6)
- Added `port-tools/run-experiments.sh` (syntax-checked, `bash -n` OK). Subcommands:
  `manifest` (hashes only, no GPU), `llamacpp-kvarn4` (baseline KVarN+tail perplexity),
  `ninfer-tiers` (rk8v4 / rk4v4-e8 / nvfp4 × tail 0/N on our build), `all`.
  Each run writes a timestamped dir with the exact argv and full stdout/stderr; `port-tools/results/`
  is gitignored.
- Ran the `manifest` cell successfully (no GPU compute). Pinned hashes committed to
  `port-tools/baseline-manifest.txt`:
  - GPU: RTX 5070 Ti, compute_cap 12.0, driver 617.14.
  - llamacpp model `...IQ3_XXS-mtp.gguf` sha256 `63f29a21…93262`;
    ninfer model `...IQ3_XXS-vision-bf16-mtp.ninfer` sha256 `edb3279e…77a33`.
  - corpus `eval/corpora/perplexity-1m/manifest.json` sha256 `b5be6783…6b2e8`.
  - llama-perplexity.exe `4f153c21…1700`, llama-server.exe `31970509…69a1`,
    ninfer-serve.exe `3e084bf8…95f7`; ninfer-perplexity = MISSING (needs our build).
- Fixed a path typo in the harness default (`infer-package` → `ninfer-package`).
- Open items recorded in-script: KVarN target-cache use may require a model-backed speculative mode
  on the llamacpp fork (the help text says so) — must be resolved empirically at M0, since plan §0.5
  forbids MTP on that side. Corpus alignment between the two products is the plan §3.2 gap.

### 4b. beellama KVCPT algorithm spec (`PORT-BEELLAMA-SPEC.md`, new)
Read-only subagent verified `D:\ninfer\beellama.cpp` @ `58a162927`. Recorded as the authoritative
reference for WP2/WP3/WP4/WP5. Highlights and **three corrections to our plan** (see PORT-MEMORY §5.2):
1. Tail window is **host-selected** (newest-N finite/causal slot descriptor), not device-computed
   from positions → contradicts plan §1.4.
2. Upstream **includes `retention_tokens` (N) in the tail graph key** → contradicts plan WP4
   ("长度不进 key"). Excluded is the *dynamic window* (runtime inputs, shape-validated).
3. Memory accounting splits `native_exact_bytes` / `exact_tail_bytes` / `rollback_reserve_bytes` /
   `transient_estimate_bytes` — our WP5 names align.
4. Confirms risk R1 resolution: body FA dequantizes to FP32, tail dequantizes to FP32, merge is
   FP32↔FP32 online-softmax `g=max(m_b,m_t); out=(O_b·l_b·e^{m_b-g}+O_t·l_t·e^{m_t-g})/(l_b·e^{m_b-g}+l_t·e^{m_t-g})`.
   Boundary: body-empty → pure tail with sinks to the tail pass; tail covers all → body mask −∞.
5. Write path: one fused `SET_ROWS`-style op writing body+shadow from the same F32 row;
   `keep_last_writes` dedup; commit-after-attention; rollback reserve `R`, `history_stride=N+R`.

BUILD and INCR1 agents still running at end of this step.
