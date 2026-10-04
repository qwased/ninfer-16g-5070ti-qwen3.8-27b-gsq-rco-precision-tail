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

## 2026-10-05 — WP3 step 1: exact-tail merge in the BF16 small-T partial

Worktree `.worktrees/wp1` (branch `port/wp1`). Code + this entry committed together (hash recorded
at the head of the next WP3 entry, and in the final entry).

### What changed
- **New kernel** `src/ops/softmax_attention/dense/causal_cache/small_t_tail.cuh` (389 lines):
  `causal_attention_small_t_tail_bf16_kernel<Geometry, TokenTile, WarpsPerCta, Int8>` plus
  `kCausalSmallTTailWarps<TokenTile>`. Storage-independent exact-tail partial: reads K/V from the
  BF16 ring (`physical_page = batch * ring_pages + ((key >> 6) % ring_pages)`, then
  `causal_cache_index<Geometry>`), owns the absolute keys `[body_window, window)`, writes
  `partial_m/l/acc` at the *global* split index `body_active + blockIdx.y` with `tokens` as the
  split stride, and is launched with `grid = (KVHeads, splits, batch)` so `gridDim.y` equals the
  launch capacity the reducer receives. A split it owns but that no key falls into publishes a
  neutral partial (`m = -inf, l = 0, acc = 0`); splits past `tail_active` write nothing (the
  reducer never reads them). Modeled on `small_t_bf16.cuh`: same 128-thread `__launch_bounds__`,
  MMA producer/consumer, `qkv_s`/`p_s` staging and `causal_small_t_tc_swz`/`_swz32` swizzles.
- **Shared partition helper** `small_t.cuh:134-166`: `CausalSmallTTailPartition` +
  `causal_small_t_tail_partition<Geometry, Int8>(window, tail_tokens, launch_capacity, tokens,
  wave_splits)`, used by body and tail alike so the two cannot disagree:
  `total_active = causal_small_t_active_splits(...)` (unchanged), `tail_keys = min(N, window)`,
  `body_window = window - tail_keys`, `body_active = active_splits(body_window)` (0 when
  `body_window == 0`, clamped to `total_active`), `tail_active = total_active - body_active`.
- **Body substitution** `small_t_bf16.cuh:18-25` (signature) and `:125-141`: new
  `std::int32_t tail_tokens` parameter; the split tier, the tile/unit mapping and the mask now use
  `body_window`/`body_active` instead of `window`/`active_split_count`. With `tail_tokens == 0` the
  helper returns `body_window == window` and `body_active == total_active` -> bit-identical.
- **Dispatch** `small_t.cu`: includes the new header; `launch_tc_partial_bf16` (`:203-243`) takes
  `tail_tokens` and, after the body launch, launches the tail kernel with the same grid and block
  when `tail_tokens > 0 && cache.tail.page_count > 0`; `causal_attention_small_t_launch_for`
  (`:284-306`) resolves the effective retention. The reducer call is untouched.

### Deviations from the plan text (deliberate; see PORT-MEMORY section 5.6)
1. `causal_attention_split_capacity` untouched: the capacity still follows the full window and
   `total_active <= launch_capacity` holds exactly as before, so no workspace or grid change.
2. `body_active` is 0 for an empty body and clamped to `total_active`. The bare plan formula
   returns `launch_capacity` for `body_window == 0` (the helper's non-positive-window default), and
   the INT8 token-count tiers are not monotonic in the window, so an unclamped `body_active` can
   exceed `total_active` and starve the tail.
3. The retention passed on the *fused-append* entry (`CacheInput::writes_cache`) is 0: see "not
   done" below.
4. The tail uses the uniform split mapping over `tail_keys`, not nvfp4's proportional
   `split * logical_tiles / active` variant. Any partition of the tail key range is correct for the
   merge (the reducer weights each split by its own `exp(m_i - max)`), so the tail need not mirror
   the body's tile ownership.

### Not done (the fused-append entry; step 4 of the task, deferred)
`causal_attention_small_t_launch` (the fused-append entry, `small_t.cu:410-455`) is not tailed. Its
partial kernel appends the new K/V rows itself in the loop guarded by `p_tok >= split_start &&
p_tok < split_end` (`small_t_i8.cuh:244-342`, `small_t_bf16.cuh:156-176`), so shortening the body
range would leave the newest N rows unwritten in the quantized body cache; and the ring is not
written on that path at all (WP2 hangs `kv_cache_append_tail_bf16_kernel` off
`kv_cache_append_launch`/`_batch_launch`, `src/ops/kv_cache/append/launch.cu:25-38`, which the
text-layer decode path never calls: `src/models/qwen3_5/execution/text.cpp:965-985` goes through
the fused attention entry, and the only standalone `ops::kv_cache_append` on a text-shaped cache is
the MTP one at `:523`). Enabling the merge there without fixing both would silently drop the newest
tokens. The wired path (standalone append + `causal_softmax_attention_cached`) is exactly WP2's
single-sequence scope.

### Evidence
```
python .deps/ptcheck.py src/ops/softmax_attention/dense/causal_cache/small_t.cu \
                         src/ops/softmax_attention/dense/causal_cache/causal_softmax_attention.cpp
cmd //c .deps\ptcheck.bat
```
-> `=== small_t.cu ===`, `=== causal_softmax_attention.cpp ===`, `DONE`; no `SYNTAXFAIL` and no
nvcc diagnostic. The first run reported one real error (a missing `scale` argument in the body
launch) which was fixed; the clean run above is the recorded one.

Environment note: new files written by the editor tool in this worktree flicker in and out of the
filesystem view (two nvcc runs reported `C1083` for a header that the very next listing showed).
The new header was therefore staged with `git add` the moment it was visible, and copies of all
four touched files are kept outside the tree at `.deps/wp3-backup/`. Another agent is working in
`.worktrees/wp2` and building/running tests in `build-port` concurrently; `.deps/ptcheck.bat` is a
shared file, so the file list is regenerated before every run.
