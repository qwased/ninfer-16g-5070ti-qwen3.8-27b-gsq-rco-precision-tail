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

## 2026-10-05 — Step 23: scope corrections, M1 assets, scoring-app option

- **Regression run (tail subset).** 15 of 16 selected tests ran under the toolchain env
  (`ctest -R` serial, single GPU owner): `kv_capacity`, `rmsnorm_pack_tail`, `paged_kv_window`,
  `softmax_attention`, `_nvfp4`, `_k8v4`, `_rk4v4_e8`, `_rk2v4_e8`, `_dflash2` (195 s),
  `_int8_prompt`, `_pack_gqa`, `kv_cache_append{,_nvfp4,_k8v4}` all **Passed**; the last,
  `softmax_attention_wide_test`, completed at 729.93 s. Final: **100% passed, 0 failed of 15**,
  total 1121.09 s, `CTEST_EXIT=0` — the tail-off regression baseline for the merged WP1/WP2/WP5/WP6
  increment.
  `ninfer_resident_memory_test` is a standalone exe not built by the `ninfer_tests` target.
  Two environment facts fixed the earlier all-`0xc0000135` run: the test harness needs the CUDA
  `bin` on PATH (source `.deps/env-port.bat`) and the `ninfer_stage_test_runtime_dlls` ALL target
  must run to place ffmpeg/curl DLLs next to the exes.
- **CRITICAL scope correction (PORT-MEMORY §5.7, commit `1999fe53`).** The main model's step
  attention is the **fused-append** entry (`ops::causal_softmax_attention(q,k,v,…)`,
  `text.cpp:965/974`); `causal_softmax_attention_cached` is MTP-only (`:571`). `ops::kv_cache_append`
  — where WP2 put the exact shadow write — is called only for `mtp_kv_` (`:523`) and the draft
  prefix, so **nothing writes `batch_text_kv_`'s ring** and a cached-only tail merge is inert for
  the main model. Correction recorded in `PORT-MEMORY` §5.7: the fused entry must shadow-write its
  step and run the tail partial (new keys from `input`, older from the ring).
- **MTP/draft tail is 0 by construction.** `decoder_state.cpp:255-263` attaches the exact pool to
  `text_kv` only; `mtp_kv` is a `PagedKVCache` with no tail, so its views carry `retention = 0`.
  That satisfies the WP6 requirement without an explicit guard.
- **WP7 (M1) is already satisfied.** The exact pool is a `DeviceKVPagePool` (no host/disk tier, slab,
  LRU, prefix digest or COW), and `docs/maintainer/paged-kv-cache.md` §4.5 documents exactly that,
  plus ring addressing and `round_up(N,64)+R` pages × `--max-concurrency`. The rollback reserve `R`
  is already present as the `+1` in `tail_ring_pages = page_count(N)+1`.
- **Scoring app had no tail switch** — the M1 quality gate (DoD 3) would have been unrunnable.
  Added `--kv-tail-tokens N` to `apps/perplexity/main.cpp` (usage text, parse, `EngineOptions`,
  `report.json`), verified by building target `ninfer-perplexity` (`BUILD_EXIT=0`), commit
  `16130914`. `docs/perplexity.md` documents the flag and the tail quality protocol.
- **M1 assets verified** (read-only): `ninfer-package/model/Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`
  (11.09 GB) and `eval/corpora/perplexity-1m/` (manifest + wikitext/pg19/zhwiki/ninfer streams).
  Helper scripts prepared: `.deps/build-target.bat`, `.deps/run-m1-ppl.bat <fmt> <N>`.
- **In flight.** `port/wp1` has the cached-path BF16 step committed (`80c00386`) and INT8
  uncommitted (compile-verifying); `port/wp10-oracle` is adding the tail oracle cases.

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
- Commit: `d8c5bdc89832f4c1ef667cc7aaccf9ac79f198e9` — "docs: add exact KV tail dimension to the
  config calculator" (4 files: `docs/config-calculator.html`, its two test harnesses,
  `PORT-JOURNAL.md`). The hash is recorded here in a following journal-only commit, because a commit
  cannot contain its own hash.

---

## 2026-10-05 — Step 24: WP10 — exact-tail oracle coverage in the causal-cache attention test

Worktree `.worktrees/wp3`, branch `port/wp10-oracle`. Only
`tests/ops/softmax_attention/causal_cache.cpp` and the port memory docs changed; no `src/`,
`include/` or `apps/` file touched (the CUDA attention agent works in another worktree), no ninja
build, no GPU run. Compile-verified with the per-TU harness (below).

**Harness — all in `tests/ops/softmax_attention/causal_cache.cpp`**
- `AttentionCase::kv_tail_tokens` and `BatchAttentionCase::kv_tail_tokens`, default `0`
  (`:225-229`, `:3146-3150`). `0` leaves the pre-tail path byte-for-byte unchanged.
- Ring helpers `tail_page_count` / `tail_ring_elements` / `tail_ring_index` (`:383-406`) resolve the
  `(p/64) % page_count`, page-64, HeadMajor addressing to the append kernel's
  `paged_kv_element_offset<D, kv_heads>` (`src/ops/kv_cache/append/kernel.cuh:99-103`,
  `launch.cu:22-37`).
- `HostCache` gains `tail_retention` / `tail_page_count` / `tail_k_bf16` / `tail_v_bf16` (`:656-664`).
- `populate_tail` fills the ring from the same BF16 k/v rows `append_cache` consumes; `tail_covers`
  + `cache_value_with_tail` (`:1557-1600`) make `ideal_attention` read exact newest rows and the
  stored representation for older ones. With the tail off this is exactly `cache_value`, so every
  untailed case keeps its prior oracle.
- `DeviceCache` / `BatchDeviceCache` allocate a BF16 ring `[D, 64, kv_heads, page_count]`, set
  `PagedKVExactTailView` on `view()` / `batch_view()`, and expose `upload_tail` / `verify_tail`
  (`:1857-1905`, `:2143-2158`).
- `run_a1_case` (both overloads) populates the ring with one `ops::kv_cache_append(tk, tv, tp,
  cache.view(), …)` — the existing shadow write — and checks it via `verify_tail` (`:2785-2791`,
  `:2888-2891`). `run_a3_case` (cached, read-only) uploads the generated ring (`:2994-3008`).
  `run_batch_case` uploads the one shared ring and requires a single writer sequence
  (`batch_tail_writer`, `:3311-3328`). `case_label` appends `tail=N`.
- `run_a1_case` gained an optional captured-output pointer; the storage overload also gained
  `assert_oracle` so the rotated-oracle plans can be measured against the exact oracle.

**Cases (`run_tail_cases`, wired into `run_softmax_attention_causal_cache_tests`)**
1. BF16 primary merge against the plain causal FP32 oracle: `{6, base=61, N=2}` (p+1 > N),
   `{6, 61, N=6}` and `{1, 128, N=1}` (boundary p+1 <= N, body window empty), `{130, 0, N=129}`
   (multi-64-page ring wrap with a one-key body).
2. `run_tail_off_regression`: the same case with `kv_tail_tokens` defaulted vs explicitly `0`, BF16
   outputs asserted bit-for-bit equal.
3. Quantized hybrid (rk8v4 fused+cached, rk4v4-e8 fused+cached) at N=2 / N=129 / N=65, judged by the
   existing criterion against the tail-aware oracle.
4. NVFP4 weaker check `run_quantized_tail_case`: result finite and `rel_l2` vs the exact oracle no
   worse than the tail-off run.
5. C>1 masked batched rows with the tail on:
   `BatchAttentionCase{8, {63,63}, {6,0}, {0,0}, Fragmented, …, tail=2}`.

**Compile evidence.** The shared `.deps/ptcheck.py` rewrites the repo prefix to `.worktrees/wp1`;
to verify *this* worktree without disturbing the concurrent wp1 CUDA agent, the script was copied to
`.deps/ptcheck_wp3.py` (same rewrite to wp3, output `.deps/ptcheck_wp3.bat`, scratch
`.deps/ptcheck-wp3/`). Run from `D:\ninfer\ninfer-precision-tail`:
- `python .deps/ptcheck_wp3.py tests/ops/softmax_attention/causal_cache.cpp`
  -> `wrote .deps/ptcheck_wp3.bat with 3 commands`.
- `cmd /c .deps\ptcheck_wp3.bat`
  -> `=== tests/ops/softmax_attention/causal_cache.cpp ===` / `causal_cache.cpp` / `DONE`;
     grep for `SYNTAXFAIL|error|warning` is empty; `.deps/ptcheck-wp3/causal_cache.obj` (2,071,694
     bytes) is produced. No `cl.exe` left alive; the only `nvcc.exe` seen is the other worktree's
     concurrent build.

**Not covered / not run.** No GPU run, so no runtime pass is claimed. The fused-path tail merge
read is being wired by a separate agent and is not present in this worktree (`src/ops/softmax_attention`
reads no `.tail` here); the cached entry is MTP-only. NVFP4/K8V4 have no complete hybrid oracle —
the kernel evaluates in a Hadamard-rotated frame the raw tail rows are not expressed in — so they get
the weaker no-worse-than-tail-off check, documented in `run_quantized_tail_case`.

- Commit: `3d6228a039356a27a5503f497d2927ecaadc541e` — "test(ops): exact KV tail oracle
  coverage in causal-cache attention" (3 files: `tests/ops/softmax_attention/causal_cache.cpp`,
  `PORT-JOURNAL.md`, `PORT-MEMORY.md`). The hash is recorded here in a following journal-only
  commit, because a commit cannot contain its own hash.
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

---

## 2026-10-05 — Step 25: WP3 tail merge wired on BOTH entries (cached + fused), WP2 fused shadow write

- **`port/wp1` step 2 (`0832e257`) and step 3 (`155fd1ab`), merged into `main` as `b691646e`.**
  After the BF16 cached step (`80c00386`) the separate agent stalled on slow per-TU verification of
  six INT8 wrapper TUs; I stopped it, reviewed the diff (a faithful INT8 twin of the BF16 change:
  `causal_small_t_tail_partition<Geometry,true>`, `launch_tail()` on the cached path, and a
  `small_t_tail_retention<CacheInput>` gate), committed it, and verified it with the independent
  `.deps/vcheck.py` harness (`small_t.cu` OK; `small_t_i8_w8_h24_cached.cu` OK, `out.obj` 4.6 MB;
  only `#177-D`/`#128-D` warnings).
- **Fused-path wiring (`155fd1ab`).** Per the §5.7 correction: added
  `causal_attention_small_t_tail_shadow_kernel` (`small_t_tail_shadow.cuh`, BF16, storage
  independent) and launch it from `causal_attention_small_t_launch_for` when
  `CacheInput::writes_cache` and the tail is enabled, before the tail partial reads the ring; dropped
  the `!writes_cache` term from `causal_small_t_tail_retention` so both entries shorten the body and
  merge the tail. `tail_tokens == 0` remains the identity partition, so a tail-free launch is
  unchanged. Verified: `small_t.cu` compiles (`out.obj` 25.2 MB, no `SYNTAXFAIL`).
- **Quick-start helpers** added under `.deps/` (untracked): `vcheck.py` (a parameterised per-TU
  compile harness with a per-tree scratch dir, so concurrent agents do not clobber each other's
  `ptcheck.bat`), `build-target.bat`, `run-m1-ppl.bat`, `run-tail-tests.bat`.
- **`port/wp10-oracle` merged as `e8b22921`** (one `PORT-MEMORY.md` conflict resolved by renumbering
  the incoming WP10 section to §5.9; `PORT-JOURNAL.md` union-merged). It adds `kv_tail_tokens` to the
  test cases: BF16 merge vs the plain FP32 oracle (p+1>N and the empty-body boundary), an N=0
  bit-exact regression, the INT8-family hybrid oracle (rk8v4 / rk4v4-e8), an NVFP4
  no-worse-than-tail-off check and a C>1 masked batched case — all **compile-verified only** so far.
- **Superseded note:** §5.9's "fused tail read is absent" is no longer true; the fused read now exists
  on `main`, so the merged oracle cases must be RUN to become evidence. Next: the incremental build
  of `ninfer_tests` + `ninfer-perplexity`, then the oracle run and the M1 `tail 0 vs N` ppl runs.
- **Discipline:** every build/test/agent process was checked to have terminated; the only compiler
  left alive at any point was the one actively building, and no `ctest`/`ninfer_tests` was orphaned.

## Step 26 - WP10 oracle RUN on the merged tree: the tail cases FAIL

The incremental build of the merged `main` finished (`BUILD_EXIT=0`; `ninfer_tests.exe` and
`ninfer-perplexity.exe` relinked; no build/compiler process left alive). The WP10 cases were then
run, and the first real execution of the tail code does **not** pass:

```
tests\ninfer_tests.exe ninfer_softmax_attention_test
...
softmax_attention: FAIL
ORACLE_EXIT=1
```

Two distinct symptom classes, both systematic (every fused, cached and quantized tail case):

1. `... exact-tail-k: exact mismatch at index 768` / `exact-tail-v: exact mismatch at index 768`
   (the same index for d256-h24-kv4 and d256-h16-kv2, i.e. it is not head-strided; 768 = 3*256 =
   page_offset 3, d=0 for both). Also seen at index 131584 for the T=130/keys=130/tail=129 cases.
2. `... reduction criterion failed at index <big> actual=<a> reference=<b>` - the merged small-T
   output disagrees with the plain FP32 / hybrid oracle.

The C>1 masked batched case throws before running: `causal_softmax_attention: invalid execution
envelope or table`. `packed_softmax_attention` and `context_softmax_attention` PASS.

Facts established while reconnoitring (so the fix does not have to re-derive them):

- Read side `causal_cache_index<Geometry>(physical_page, kv_head, d, page_offset)` and write side
  `paged_kv_element_offset<D, Geometry::KVHeads>(ring, kv_head, page_offset, d)` both expand to
  `LeadingExtent*64*(head + KVHeads*physical_page) + LeadingExtent*page_offset + leading`, so the two
  addressing helpers already agree; `physical_page = batch*ring_pages + ((pos>>6) % ring_pages)` on
  both sides.
- The fused shadow write is `small_t_tail_shadow.cuh`; the fused/cached tail partial is
  `small_t_tail.cuh`; the split partition is `causal_small_t_tail_partition` in the same file.
- The oracle is `cache_value_with_tail` (`tests/ops/softmax_attention/causal_cache.cpp:1593`) and the
  exact-compare helper is `verify_exact` (`tests/ops/op_tester.h:277`).

This is exactly the class of defect WP10 existed to expose: the code was compile-verified only and
never executed. Root cause and fix are being investigated next; §7.2 is **not** satisfied yet.

## Step 27 - WP10 oracle root cause (body swallowed the whole split range) and partial fix

**Root cause (product bug, `small_t.cuh` `causal_small_t_tail_partition`).** The partition clamped
the body to the reducer's range: `body_active = active_splits(body_window)` then
`if (body_active > total_active) body_active = total_active`. But `causal_small_t_active_splits` has
a tier floor (`kMinSplits`) that makes `active_splits(body_window) == active_splits(window)` for
nearly every short window, so `body_active == total_active` and `tail_active == 0`. The tail partial
then launched no split: the newest `tail_keys` keys sat in **neither** partial (the body only covers
`[0, body_window)` and the tail covered nothing), so neither the `(m, l)` merge nor the weighted
accumulator ever saw them. This is why every tail case failed, including the `exact-tail-k/v` row
mismatch: the ring rows are fine, but the oracle expects them to be *read* and they were not.

**Fix.** When the tail has keys the body stops one split short of the reducer range and a non-empty
body keeps at least one split:

```cpp
int body_active = 0;
if (body_window > 0) {
    body_active = causal_small_t_active_splits<Geometry, Int8>(body_window, launch_capacity,
                                                               tokens, wave_splits);
    const int body_limit = tail_keys > 0 ? total_active - 1 : total_active;
    if (body_active > body_limit) { body_active = body_limit; }
    if (body_active < 1) { body_active = 1; }
}
```

`tail_tokens == 0` is unchanged (`body_limit == total_active`), so the tail-free path is untouched.

**Verified by running** (`.deps/oracle-run2.txt`, `.deps/oracle-run2-stats.txt`; the exe at
`build-port\tests\ninfer_tests.exe` 05:19): all five BF16 tail cases now pass the existing criterion
(`use` 0.22-0.31 of the limit) and no `exact-tail-*` mismatch is reported for them. The test fixture
was also corrected by the same pass (`zero_tail_ring` gives the ring the same deterministic zero
baseline the host fixture has; `make_tail_fixture` generates the whole retention window from 0 so a
wrapped ring is exercised; the batched `table_rows` is `max(rows_addressed, batch)` to satisfy the
Op's batched contract; the fused batched expectation is the writer's own appended rows because the
fused entry shadow-writes rather than calling `kv_cache_append`). A `tail_merge_wired(storage)` gate
stops the oracle asserting the ring for fp8-e4m3 / nvfp4-g16 / k8v4: those routes were **confirmed
by inspection to contain no tail/shadow code at all** (`grep -E "tail|shadow"` on `small_t_nvfp4.cu`,
`small_t_k8v4.cu`, `small_t_fp8.cu` is empty), because their partials live in a Hadamard-rotated frame
the raw BF16 tail rows are not expressed in. The exact tail is therefore wired for **BF16 and the
INT8 family only** -- a scope limitation to be stated in the docs, not a silent no-op.

**Still failing (not yet root-caused):**
- INT8 family. `rk8v4` fused tail=2/tail=6 fail the gross bound marginally; the `cached` rk8v4
  (tail=6 `use=34`, tail=65 `use=84`) and `cached` rk4v4-e8 (tail=129 `use=33-38`) cases are grossly
  wrong, while `rk4v4-e8` fused cases pass. The cached entry's ring is uploaded by the test rather
  than shadow-written, so the suspicion is the cached INT8 route's ring/append or the
  `active_split_count == body_active` handshake, not the partition.
- Batched BF16: `... batch ... cache-k/v: exact mismatch at index 197376` -- the fused batched shadow
  write puts rows in the ring the oracle does not expect (or vice versa).
- Debug `printf`s (`DBGBODY`, `DBGTAIL`, `DBGRED`) and a probe rebuild were left in the working tree
  when the investigating subagent exhausted its turn budget; they are **instrumentation only** and
  must be deleted before this work is finished. The build it started was stopped and no compiler
  process was left alive.

## Step 27b - DoD 7.4 evidence plumbing + doc scope notes

- `apps/perplexity/main.cpp` now records a `memory` object in `report.json`
  (`runtime_reservation_bytes`, `kv_payload_bytes`, `kv_exact_history_bytes`,
  `kv_rollback_reserve_bytes`, `minimum_runtime_reservation_bytes`, `cuda_graph_measured_bytes`) from
  `Engine::memory_summary()` (declared `include/ninfer/engine.h:110`). This turns DoD 7.4 from a log
  scrape into a report diff between `--kv-tail-tokens 0` and `1024`. **Compile-pending**: the edit is
  one `const ninfer::MemorySummary` copy plus one JSON key and it will be built with the next
  `ninfer-perplexity` link, but it has not been compiled yet (a build is deliberately not started now
  to avoid contending with the agent's build of `build-port`).
- `docs/perplexity.md`, `docs/cli.md`, `docs/serving.md` now state the tail's real storage scope:
  merged for `bf16` + the INT8 family (`int8`, `rk8v4`, `rk4v4`, `rk4v4-e8`, `rk2v4-e8`), and
  allocated-but-inert for `fp8`, `nvfp4`, `k8v4` (`11d67445`).
- `PORT-DOD.md` refreshed to the run-verified state, including the three recorded plan deviations
  (fused-path wiring, storage scope, and the `body_active` partition fix) (`08a77d9d`).

- **Verified** (Step 27c): the `report.json` `memory` object compiles — `python .deps/check-host-tu.py apps/perplexity/main.cpp` returns `SYNTAX_EXIT 0` (`cl /Zs`, no object written, ninja state untouched). That closes the "compile-pending" gap above; a full link still happens with the M1 build. New untracked helper `.deps/check-host-tu.py` does per-TU host syntax checks the same way `vcheck.py` does for CUDA TUs.

## Step 27d - WP9 config-calculator verified (was mis-audited as "untouched")

`node docs/config-calculator.test.mjs` → `PASS`, 24/24 checks, including the tail ones:
`tailRingPages` (0 disables; `ceil(N/64)+1` pages), `tailBytes` (N=0 exactly zero on both models,
`(ceil(N/64)+1)` pages of the measured BF16 body cache per sequence, BF16 whatever the body format
is, N=0 leaves the golden engine reservation untouched, a tail consumes context headroom
page-consistently). So the calculator half of DoD 7.7 is **DONE and verified**, and the N=0 golden
reservation check is additional real evidence for DoD 7.6. `PORT-DOD.md` corrected accordingly; the
only outstanding 7.7 item is the measured `docs/performance.md` numbers.

## Step 27e - WP8 recorded as a deliberate deferral

Read plan §3 WP7/WP8. WP7 was already satisfied (device-only pool, `§4.5` updated). WP8 asks that the
exact-pool write commit after attention with rollback + degraded marking and reserve `R`; the plan's
acceptance is a post-rollback exact-vs-body consistency test. This is **not in DoD §7** and **not
reachable in M1**: the pool is device-only, `C=1`, and the functional closure exercises no failure
path. `R = 1` page is already carried in the WP5 sizing (`startup.cpp:360-363`), so the accounting is
in place for when the transactional layer lands. Recorded as deviation 4 in `PORT-DOD.md` rather than
left as a silent gap.

## Step 27f - preserved diagnostic leads for the two open failure classes

So a take-over does not re-derive them:

- **INT8 `cached`, grossly wrong (`use` 33-84).** The fused BF16/fused-INT8 cases pass, so the
  partition arithmetic is right. The cached entry differs in two ways: its ring is uploaded by the
  test (`upload_tail`) and its body partial comes from the `small_t_i8_w*_h*_cached.cu`
  instantiations, whose `causal_attention_small_t_launch_for` is a *different* instantiation than
  the `*_append.cu` ones. Prime suspect: the cached path's `launch_tail()` passes an argument the
  tail kernel keys off — `logical_capacity` (the tail kernel bails when `last_pos >=
  logical_capacity`, which would silently drop the newest `tail_keys` keys and give exactly this
  "missing newest keys" magnitude) or `wave_splits`/`split_count` (a disagreement with the body's
  `active_split_count = tail_partition.body_active` would overlap or gap the split ranges). Compare
  the two `launch_tail` call sites against the fused one, and check the `DBGBODY`/`DBGTAIL` prints
  for a `win`/`sc`/`ws` mismatch between body and tail.
- **Batched BF16 ring mismatch (index 197376).** The fused batched shadow write lands rows the oracle
  does not expect. Check the per-sequence arithmetic in `causal_attention_small_t_tail_shadow.cuh`:
  `column_base = column_begin + batch*full_width` and `ring = batch*ring_pages + ((position>>6) %
  ring_pages)`, against the test's batched expectation (`populate_tail` with the writer's own rows,
  `batch_tail_writer`, `tail_row`) — in particular whether `full_width` is the right per-sequence row
  stride in the batched view (the batched append also skips the ring shadow, so the oracle and the
  kernel must agree on which columns belong to which sequence).
- The three debug `printf`s (`DBGBODY`, `DBGTAIL`, `DBGRED`) are still in the working tree
  (committed in `f19425f1` as instrumentation) and must be removed before the work is finished.

## Step 27g - the body and the tail call the partition with DIFFERENT arguments (lead, unconfirmed)

`causal_small_t_active_splits` is genuinely token-count-tiered for the INT8 family
(`small_t.cuh:104-122`: `tokens == 5 && window in (128,512]` and `tokens >= 6 && window in
(128,160]` and `... window in (5000,8198]` each take their own branch, else
`causal_small_t_default_splits`), and `wave_splits` is a second live input (it rounds `splits` down
to whole waves when positive). So the partition depends on both `tokens` and `wave_splits` -- yet the
call sites do not agree:

| site | call |
|---|---|
| `small_t_bf16.cuh:131` (body) | `tail_partition<Geometry, false>(window, tail_tokens, split_count, TokenTile)` -- omits `wave_splits` (defaults 0) |
| `small_t_i8.cuh:219` (body) | `tail_partition<Geometry, true>(window, tail_tokens, split_count, TokenTile, wave_splits)` |
| `small_t_tail.cuh:116` (tail) | `tail_partition<Geometry, Int8>(window, tail_tokens, split_count, tokens, wave_splits)` -- passes the runtime `tokens`, not `TokenTile` |

The body passes the compile-time `TokenTile`; the tail passes the runtime width `tokens`; the BF16
body also drops `wave_splits`. Whenever the route's `TokenTile` exceeds the step's actual width, or
whenever the route's `wave_splits` is positive, the body and the tail compute different
`total_active`/`body_active`, so the body fills splits `[0, body_active_body)` while the tail fills
`[body_active_tail, total_active_tail)` -- overlapping or leaving a gap, which is exactly a gross
reduction error, and it would hit the routes whose `wave_splits`/tile differ from the fused norm.
**Unconfirmed** (no build was run for this); the fix is to make all three sites pass the same
`tokens` (the runtime width, i.e. `min(TokenTile, invocation.width)`) and the same `wave_splits`, and
add a cheap static assert or a unit check that the three agree. This supersedes the `launch_tail`
grid hypothesis -- `launch_tail` already uses the same `splits` grid (`small_t_i8_launch.cuh:108`).
