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

---

## 2026-10-05 — Step 5: plan §2 memory model verified against published numbers

Read `docs/performance.md:448-460` (published KV-format table, RTX 3090, Qwen3.8-27B, same
`--quick` 4096/2048 protocol our harness uses):
- bf16 = **65,536 B/token** → independent confirmation of plan §2 (16 full-attn layers × 4096 B).
  1024 tokens = 64 MiB (the plan's N=1024/C=1 cell) and 2048 tokens = 128.00 MiB as published.
- Published tail=0 references for DoD §7.3: `rk8v4` ppl 4.346811, `nvfp4` 4.358924, `bf16` 4.343225
  (target direction). `rk4v4-e8` unpublished — measure fresh.
- Caveat recorded: values are sm_86/3090; our baseline is sm_120a/5070 Ti, so absolute numbers differ.
- Recorded in PORT-MEMORY §5.3.

Still no BUILD/INCR1 completion notification; `.deps` was at 184 MB with `vcpkg-root` underway.

---

## 2026-10-05 — Step 6: WP3 window/addressing design decision (pre-implementation)

Resolved the main WP3 design fork before coding, from verified facts (no code changed):
- **Chose device-computed window** (plan §1.4) over beellama's host-selected descriptor, because
  ninfer already addresses KV from device tensors via `paged_kv_address.cuh:42-45,55-69` with
  `positions` on device; beellama's host selection is an artifact of its host-managed slot allocator.
- **Exact pool = ring buffer** of `round_up(N,64)` pages, tail page implicit from `p>>6`; rows masked
  by `[max(0,p+1-N), p]`. Body-empty case still runs the body partial with mask −∞ (upstream does the
  same) so the route family never switches.
- **Merge reuses** `causal_merge_split_statistics` + `causal_attention_small_t_reduce_output_kernel`
  inside the small-T family; workspace gains a second partial set (acc_t,m_t,l_t).
- N in the tail identity is fine (startup-fixed); the dynamic window must stay out of the graph key.
- Recorded in PORT-MEMORY §5.4.

Agents still running (no BUILD/INCR1 completion notification at end of step).

---

## 2026-10-05 — Step 7: WP6 config plumbing implemented (port/wp1), INCR1 agent stopped

- **Stopped the INCR1 background agent**: after ~11 minutes its transcript file was still 0 bytes and
  its worktree had zero changes — it was not producing. Took the increment over directly.
- **Implemented WP6 (config subset)** on branch `port/wp1` in `.worktrees/wp1`, commit `64f32d3e`
  (4 files, +27):
  - `include/ninfer/types.h`: `std::int32_t kv_tail_tokens = 0;` on `EngineOptions`.
  - `apps/cli/options.cpp`: `--kv-tail-tokens` parsed via a new `parse_kv_tail_tokens`
    (`parse_u32(..., allow_zero=true)` → zero allowed, negatives rejected, range-checked) + help text.
  - `src/serve/serve_options.cpp`: same flag via `parse_nonnegative_int` + help text.
  - `src/runtime/engine/model_instance.cpp`: `hybrid_cache_fingerprint` gains `;kvt=<N>` so different
    tail configurations cannot share one engine identity.
- NOT yet done in WP6: forcing the speculative draft context to tail = 0 — belongs with the pool
  planning in WP1 (noted as a TODO; plan §3 WP6 last bullet).
- Code lives on `port/wp1`; memory docs stay on `main`. Merge to `main` once the BUILD agent confirms
  the tree configures/compiles, so the change is actually compiled.

BUILD agent still running (`.deps/vcpkg-root/installed/x64-windows` now holds av*-63/61/10 ffmpeg
libs+headers+DLLs, curl and zlib). Not yet a completion notification.

---

## 2026-10-05 — Step 8: WP1 view interface landed; build CONFIGURE SUCCEEDED

- **Build blocker resolved (configure):** `build-port/CMakeCache.txt` (38 KB) and
  `build-port/build.ninja` (2.4 MB) now exist, with `compile_commands.json`, `CTestTestfile.cmake`,
  `apps/`, `src/`, `tests/`, `tools/` — i.e. the in-workspace FFMPEG triplet tree satisfied
  `FindFFMPEG.cmake` and CMake configured the sm_120a Release build at 03:23. The BUILD agent is now
  presumably compiling (CUDA build is long); no completion notification yet.
- **WP1 interface landed** on `port/wp1`, commit `74297cba` (2 files, +18):
  `PagedKVExactTailView` (BF16 k/v ring pages, implicit `(p/64)%page_count` addressing, `retention`
  N, `enabled()`), added as a default-disabled trailing member of `PagedKVLayerView` and
  `PagedKVBatchLayerView`; propagated in `single_row_paged_kv_batch_view`. All other construction
  sites use designated initializers ending at `storage`, so they compile unchanged.
  Tail design follows PORT-MEMORY §5.4 (device-side ring addressing, no stored block table).
- Still open in WP1: attaching a real exact pool in `PagedKVCache`/`DecoderStateLayout`
  (`decoder_state.cpp:191 kv_payload_bytes` sums text_kv + mtp_kv and must add the tail), the
  `startup.cpp:247-287`-style pool planning, `MemorySummary` fields, and the draft tail=0 forcing.

---

## 2026-10-05 — Step 9: delegated tail-pool attachment + accounting

- Build still compiling at step start (446 `.obj`, `.ninja_log` growing, `ninfer_ops` target) — no
  executables yet, no BUILD completion notification.
- Launched a background agent in the **existing** `port/wp1` worktree (continuing from `74297cba`) for
  the next increment: attach the exact tail pool (WP1) + capacity/`MemorySummary` accounting (WP5) +
  the speculative-draft tail=0 forcing (WP6 leftover). Explicitly NOT the WP2/WP3 kernels.
- Brief includes: verified anchors, PORT-MEMORY §5.4 ring-pool design, the §2 byte formula, and a
  required syntax-check procedure using `build-port/compile_commands.json` with `/c`→`/Zs` so the agent
  gets real compile feedback WITHOUT disturbing the running ninja build.
- Isolated in `port/wp1` so it cannot conflict with the BUILD agent's work in the main checkout.

Next: on completion, review + syntax/compile verify, then merge `port/wp1` → `main` once the BUILD
agent's full build finishes (incremental rebuild will recompile the touched TUs).

---

## 2026-10-05 — Step 10: completion-audit checklist created

- Added `PORT-DOD.md`: maps every plan §7 DoD item, §4 milestone, §3 work package, and every
  objective-level requirement to concrete evidence + honest status (DONE/PARTIAL/TODO/BLOCKED).
  This is the artifact the final audit will be performed against; it prevents claiming completion
  from intent rather than verified state.
- Current honest summary: WP6 config + WP1 view interface landed; WP1 pool attach and WP5 accounting
  in flight; WP2/WP3/WP4/WP7/WP8/WP9 still TODO; DoD items 1-7 all PARTIAL/TODO/BLOCKED.
- Build at 479 objects, still compiling.

---

## 2026-10-05 — Step 11: WP9 partial — document the new option

- Documented `--kv-tail-tokens N` in the option tables of `docs/cli.md` and `docs/serving.md`
  (row added directly after `--kv-dtype`), matching the implemented semantics: newest-N unquantized
  exact pool, device-only, `0` disables. `git diff --check` clean.
- Done on `main` (docs are not touched by the pool agent and are not compiled), so no collision.
- Remaining WP9: `config-calculator.html` tail rows, `docs/maintainer/paged-kv-cache.md §4.5`
  ownership boundary, `docs/performance.md` measured tail numbers.

---

## 2026-10-05 — Step 12: WP7/WP9 partial — exact-pool ownership boundary documented

- `docs/maintainer/paged-kv-cache.md §4.5 Page payload` gains a paragraph defining the exact tail pool
  (BF16, HeadMajor, 2 planes/layer, page 64, its own `PageBytes`) and its M1 ownership boundary: device
  only — no host/disk tier, slab, LRU, prefix digest or COW; ring addressing by position, no block
  table; capacity `round_up(N,64)+R` pages per sequence × `--max-concurrency`. `git diff --check` clean.
- WP9 now: cli.md, serving.md, paged-kv-cache §4.5 done. Remaining: `config-calculator.html` tail rows
  and `docs/performance.md` measured numbers (need the build + runs).

---

## 2026-10-05 — Step 13: exact tail pool planned (WP1 attach) — implemented directly

- **Stopped the POOL agent** (second agent to stall: 0 file changes after ~6 min, same pattern as the
  first). Implemented the increment directly in `.worktrees/wp1`.
- Commit `f15a4d72` (2 files, +77) — the tail pool is now planned:
  - `DecoderStateSpec`: `kv_tail_tokens`, `kv_tail_physical_page_groups`.
  - `plan_decoder_state`: when `kv_tail_tokens > 0`, plans a second pool — BF16 `HeadMajor`, two planes
    (K/V) per full-attention layer, placed on each layer's own rank (mirrors the DFlash pool pattern at
    `startup.cpp:253-286`), `page_group_count = kv_tail_physical_page_groups`, **no execution table**
    (ring addressing by position).
  - `ExactTailCacheLayout {pages, retention, layers, payload_bytes()}`; `DecoderStateLayout::exact_tail`
    and `DecoderState::exact_tail` (`optional<DeviceKVPagePool>`), constructed in place, plus
    `exact_tail_pool()` accessors; `kv_payload_bytes()` now includes the tail.
  - Fail-closed: throws if the tail is enabled with zero page groups. Default 0 = unchanged.
- NOT yet wired: `startup.cpp` must compute `kv_tail_physical_page_groups` from
  `round_up(N,64)+R` × `max_concurrency` and pass `kv_tail_tokens` into `DecoderStateSpec`; the views
  are not yet attached (`PagedKVLayerView.tail`) — that lands with WP3, which consumes it; `MemorySummary`
  population still TODO.
- Build at 519 objects, still compiling.

---

## 2026-10-05 — Step 14: tail tokens plumbed through planning; tail pool sized

- Commit `d9c013fd` on `port/wp1` (2 files, +13):
  - `SequencePlanningInputs` and `SequencePlanImpl` gain `kv_tail_tokens`
    (`src/models/qwen3_5/program/planning/startup.h`).
  - `startup.cpp`: `impl->kv_tail_tokens = inputs.kv_tail_tokens;` and the engine-options bridge sets
    `.kv_tail_tokens = options.kv_tail_tokens`; the `DecoderStateSpec` now receives
    `kv_tail_tokens` and `kv_tail_physical_page_groups`.
  - Tail sizing: `round_up(N,64) + 1` pages per sequence (the `+1` is the rollback reserve R), times
    `max_concurrency` — the plan §2 model, now expressed in code.
  - Speculative draft contexts are tail-free **by construction**: the MTP cache and the DFlash pool
    are planned through `plan_cache`/their own layout blocks and never receive the tail fields, so the
    plan §3-WP6 "draft tail = 0" requirement holds without a special case.
- Build progress: `ninfer_core.lib`, `ninfer_xgrammar.lib` linked; test dispatch sources generated;
  521 objects. Still compiling, no executables yet.
- Remaining immediately: `MemorySummary` fields + `kv_capacity` population (WP5), then merge the whole
  `port/wp1` branch into `main` for a real compile of everything.

---

## 2026-10-05 — Step 15: real per-TU compile verification (2 bugs found and fixed)

- Built a per-TU compile harness (`.deps/ptcheck.py` + `.deps/ptcheck.bat`) that compiles individual
  changed TUs from `build-port/compile_commands.json` retargeted at the worktree, without disturbing
  the running ninja build. First attempt failed with `D8022 cannot open ....obj.modmap` (a MSVC
  response-file argument that does not exist until the module scan runs) — stripping `@...modmap`
  fixed the harness.
- Result: **`decoder_state.cpp`, `startup.cpp`, `paged_kv_cache.cpp`, `options.cpp`, `serve_options.cpp`,
  `generation_service.cpp`, `model_instance.cpp` all compile cleanly.** `main.cpp` is blocked only by the
  build-generated `ninfer_build_id.h`.
- **Two real bugs found and fixed** (commit `2f010b36`): `kv_tail_tokens` was added to `EngineOptions`
  but not to the products' own option structs. Added to `apps/cli/options.h` (`cli::Options`) and
  `src/serve/serve_options.h` (`serve::ServeOptions`), and mapped into `EngineOptions` in
  `apps/cli/main.cpp` and `src/serve/generation_service.cpp`.
- Technique recorded in PORT-MEMORY so later steps can verify without a full build.
- Build still running (529 objects).

---

## 2026-10-05 — Step 16: WP5 premise verified and corrected (plan amended)

Verified the capacity chain in `port/wp1`:
- `SequenceCapacityCurve::reservation_bytes` (`kv_capacity.cpp:57-69`) is already
  `constant + linear`; `startup.cpp:1216-1218` folds `persistent.bytes` (which contains
  `DecoderStateLayout::kv_payload_bytes()`, now tail-inclusive) into `minimum_device_reservation_bytes`.
- Therefore the exact tail is **already accounted** as a per-sequence constant in both the capacity
  curve and `MemorySummary.kv_payload_bytes`. Plan §3 WP5's "convert the single-coefficient curve"
  rests on a wrong premise and is **not needed** — the plan is amended (allowed by the updated
  objective). Remaining WP5 item is only the optional reporting split
  `kv_exact_history_bytes` / `kv_rollback_reserve_bytes`.
- Recorded as PORT-MEMORY §5.5 and updated PORT-DOD WP5.

---

## 2026-10-05 — Step 17: tail pool exposed through the layer views (WP1 attach complete)

- Commit `75343cc2` (3 files, +35): `PagedKVCache::attach_exact_tail(pool, retention, ring_pages)`;
  `layer_view`/`batch_layer_view` populate `PagedKVExactTailView` from the pool's per-layer K/V planes
  (2 planes per layer, `tail_base = layer*2`); `DecoderState` binds the tail right after constructing
  it; `startup.cpp` passes `kv_tail_ring_pages`. Verified by compiling `decoder_state.cpp` and
  `startup.cpp` with the per-TU harness — both clean.
- WP1 scaffolding is now complete: option -> planning -> sizing -> pool -> accounting in the curve
  constant and `kv_payload_bytes` -> views. What remains for WP1 is the runtime per-sequence ring page
  lease (materializing `ring_pages` contiguous pages per sequence), which WP2 needs.
- Remaining feature work is WP2 (fused dual write) and WP3 (tail partial + FP32 merge) — the actual
  numerical feature — plus WP4/WP7/WP8/WP9/WP10.

---

## 2026-10-05 — Step 18: MemorySummary split fields (WP5 complete for M1)

- Commit `fc325aeb` on `port/wp1`: `MemorySummary` gains `kv_exact_history_bytes` /
  `kv_rollback_reserve_bytes`; computed in `startup.cpp` from the planned tail pool
  (`(tail_payload / physical_pages) * max_concurrency` is the one-page-per-sequence reserve), carried
  through `PersistentLayout` and `ProgramImpl`, and published in the summary. Zero when the tail is off.
- Compile-verified per TU with `.deps/ptcheck.py`: `startup.cpp` and `program_impl.cpp` clean; all other
  changed TUs clean. Only `apps/cli/main.cpp` remains unverifiable (build-generated `ninfer_build_id.h`).
- WP5 is complete for M1: the tail's memory is accounted in the capacity curve constant (verified, §5.5)
  and reported explicitly in `MemorySummary`.
- Build still running (537 objects, ninja log active). No executables yet.

---

## 2026-10-05 — Step 19: CUDA per-TU verification + WP2 exact tail write

- Extended `.deps/ptcheck.py` to handle CUDA TUs (nvcc: retarget only `-o`). Confirmed on an
  unmodified `launch.cu`. **This gives per-kernel compile verification** without waiting for the
  full build — recorded in PORT-MEMORY §5.
- **WP2 (single-sequence) landed**, commit `52b36257` (2 files, +54):
  - `kv_cache_append_tail_bf16_kernel` in `kernel.cuh`: reads the same unquantized BF16 source row
    (no re-quantization) and stores it at ring page `(position/64) % ring_pages` using the same
    `paged_kv_element_offset` addressing as the body.
  - `launch_full` launches it right before the body dispatch when `cache.tail.enabled()`, guarded by
    `if constexpr (requires(const CacheView& c) { c.block_table; })` so only the single-sequence view
    runs it — batched launches need a per-row sequence base (deferred to M3).
  - Known limitation documented in-code: the ring page is used directly as the physical page, which
    is correct for one sequence (M1's C=1 target) and needs the per-sequence arena base for C>1.
  - Verified: `launch.cu` (which includes `kernel.cuh`) compiles cleanly with the harness.
- Build still running.

---

## 2026-10-05 — Step 20: WP3 reduced to "add splits" (verified in-tree)

- Verified `causal_merge_split_statistics` (`small_t.cuh:183-210`) implements exactly the upstream
  body x tail online-softmax merge: max over splits, `exp(m_i - max)` weights, summed denominators.
- Consequence: **WP3 needs no new merge kernel.** Write body partials into splits `[0,s_b)` and tail
  partials into `[s_b, s_b+s_t)` of the same workspace and call the existing
  `causal_attention_small_t_reduce_output_kernel`. Remaining work is a tail-partial kernel, workspace
  split sizing, and dispatch wiring — route family unchanged (still small-T), so §1.5 holds.
- Recorded in PORT-MEMORY §5.6; PORT-DOD updated (WP2 PARTIAL/DONE-single-seq, WP3 design proven).

---

## 2026-10-05 — Step 21: port/wp1 merged into main (single authoritative branch)

- `git merge --no-ff port/wp1` -> `964d7be6`, 18 files, +246. `main` now carries the whole increment:
  WP1 tail pool + views + accounting, WP2 single-sequence shadow write, WP5 `MemorySummary` split,
  WP6 option/identity across CLI and serve.
- Rationale: the objective wants the new project under git as one authority, and earlier the code lived
  on `port/wp1` while memory docs lived on `main`; the merge removes that split. The worktree
  `.worktrees/wp1` remains for further increments.
- Verification status unchanged: every changed TU (C++ and CUDA) compiles individually via
  `.deps/ptcheck.py`; a full-tree ninja build has not yet been observed to complete.

---

## 2026-10-05 — Step 22: full-tree build completes; MSVC portability fix

- Diagnosis: the earlier full build had not hung but died. `ninja`/`nvcc` were gone, objects frozen at
  799/897, no test binary. Restarting resumed the 799 done objects and immediately surfaced the real
  failure:
  `tests/ops/test_hadamard_transform.cpp(45): error C3861: "__builtin_popcount" unknown identifier`.
  `__builtin_popcount` is a GCC/Clang builtin absent in MSVC; a grep confirmed it is the only such use
  outside `third_party/` (the vendored libraries guard their own).
- Fix (`b5b71c84`): `#include <bit>` and use C++20 `std::popcount` in the oracle. The build already
  targets `-std:c++20`, so `std::popcount` is the portable equivalent; no behaviour change.
- Evidence: `cmd //c .deps\build-port.bat ninfer_tests 10` -> `[89/89] Linking CXX executable
  tests\ninfer_tests.exe`, `BUILD_EXIT=0`, 884 objects, `build-port/tests/ninfer_tests.exe` present.
  This is the first observed complete full-tree build of the port; it compiles the whole `small_t`
  attention family (`ninfer_ops.lib` links) with the WP1/WP2/WP5/WP6 increments merged in `main`.
- Test inventory (for the tail=0 regression run): 516 registered CTest entries; the tail-relevant set
  is `ninfer_kv_cache_append{,_k8v4,_nvfp4}_test`, `ninfer_softmax_attention{,_{rk2v4_e8,rk4v4_e8,
  k8v4,nvfp4,int8_prompt,wide}}_test`, `ninfer_paged_kv_window_test`, `ninfer_kv_capacity_test`,
  `ninfer_rmsnorm_pack_tail_test`, `ninfer_resident_memory_test`. GPU run pending device confirmation.

---

## 2026-10-05 — Step 23: WP9 — exact-tail dimension in the fit calculator

Worktree `.worktrees/wp2`, branch `port/wp9-docs`. Docs + the self-contained calculator only; no
`src/`, `include/`, `apps/`, `tests/` or `cmake/` file touched, no build, no GPU run.

- `docs/config-calculator.html`: new `#tail` input (`--kv-tail-tokens N`, default `0` = off) with a
  hint, a `--seg-tail` bar segment and legend row (light/dark palette entries added), and new
  `tailRingPages()`/`tailBytes()` helpers. The tail enters `total` and the fit verdict as its own
  term, is subtracted once from the max-context room (`render()`, `room = usable - solveFixed -
  tail`), and is taken off `buildKvTable()`'s context budget with a matching table caption. The
  dynamic `#tail-hint` and `#ctx-hint` report the per-sequence cost and ring size. Method notes gain
  a bullet for the one non-linear term; the "does not model" concurrency paragraph now says the tail
  scales with `--max-concurrency`, unlike the shared KV pool.
  - **Formula implemented (per sequence):**
    `tail_bytes = (ceil(N / 64) + 1) * 64 * model.kv.bf16`.
    Verified in the code, not guessed:
    `src/models/qwen3_5/program/planning/startup.cpp:146-151` gives
    `tail_ring_pages = page_count(kv_tail_tokens) + 1` where `page_count(N) = 1 + (N-1)/64 =
    ceil(N/64)` (`startup.cpp:71-74`), i.e. round-up to the physical page **plus one rollback
    page**, and `tail_physical_pages = tail_ring_pages * plan.max_concurrency`.
    `src/models/qwen3_5/state/decoder_state.cpp:124-136` plans the pool as `KvCacheStorage::BFloat16`,
    `device_plane_order = HeadMajor`, `page_tokens = kPagedKVPageSize = 64`
    (`src/core/paged_kv_cache.h:19`), **two planes (K then V) per full-attention layer** with
    `leading_extent = head_dim`, `head_extent = kv_heads`, `dtype = BF16`
    (`src/core/paged_kv_storage.h:66-67`), so one plane page is `64 * kv_heads * head_dim * 2` bytes
    and the layer-and-plane factor is exactly one 64-token page of the BF16 body cache. That is what
    makes `model.kv.bf16` (the already-measured BF16 per-token body cost) the right multiplier.
    Single-lane page => engine's `--max-concurrency` factor is 1.
  - The tail is BF16 whatever body format is selected, and `N = 0` yields exactly 0 bytes, so the
    previous numbers are reproduced byte for byte when disabled. **No tail figure has been measured
    against the engine**, so the page derives it from the measured BF16 row + the layout formula and
    adds no golden cross-check; the method note says so rather than claiming a measurement.
- `docs/config-calculator.test.mjs`: `tailRingPages`/`tailBytes` extracted from the page and covered
  — ring is `page_count(N) + 1` (anchored at N=1/64/65/2048/40960, pinning the extra rollback page),
  `N = 0` is exactly zero on both models, the cost is BF16 regardless of body format, and adding
  `tailBytes(model, 0)` leaves the golden 262,144-token int8 engine reservation at `9197389568`.
- `docs/config-calculator.render.test.mjs`: the tail is now a fourth loop dimension
  (0/2048/65536) over all model×spec×kv×ctx combinations; it further asserts that a bigger tail
  never fits more context, that `N = 0` restores the exact previous readouts (max context, sub,
  hints, segment width, legend), that the segment and legend row exist, and that an impossible tail
  reports `Does not fit` instead of being silently capped. The speculation-only block resets the
  tail to 0 so its printed figures keep their old meaning.
- Both test files' `<script>` extractor is now `/<script>\r?\n/`. The committed blob is LF, but Git
  for Windows' default `core.autocrlf=true` checks this page out CRLF, so the previous bare `\n`
  anchor matched nothing on this host — a **pre-existing** failure, reproduced on the unmodified
  HEAD via `git stash`. No git config was changed; the optional `\r` keeps both checkouts working.
- Evidence (run from `.worktrees/wp2`):
  - `node docs/config-calculator.test.mjs` -> `ok` × 23 (4 pageRoundUp, 7 tail, 4 decodeAtDepth,
    3 golden engine cross-checks, 5 model/table checks), then `PASS`, exit 0.
  - `node docs/config-calculator.render.test.mjs` -> `render()+buildKvTable() over 1260
    combinations: 0 threw`, the three 35b speculation rows (`none`/`mtp3+head` 262,144 capped,
    `dflash-3` 240,192), then `PASS`, exit 0.
  - `git diff --check` clean.
