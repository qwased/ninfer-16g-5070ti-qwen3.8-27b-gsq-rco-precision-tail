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

## 5.3 Plan §2 memory model — verified + reference ppl (Step 5)

`docs/performance.md:448-460` publishes (RTX 3090, Qwen3.8-27B, `--quick`, ctx/stride 4096/2048,
261,167 scored tokens) — an independent check of plan §2:

| KV | Bytes/token | KV @ 2048 tok | Perplexity |
|---|---:|---:|---:|
| `bf16` | 65,536 | 128.00 MiB | 4.343225 |
| `int8` | 33,792 | 66.00 MiB | 4.343263 |
| `fp8` | 33,024 | 64.50 MiB | 4.347181 |
| `rk8v4` | 26,112 | 51.00 MiB | 4.346811 |
| `k8v4` | 25,728 | 50.25 MiB | 4.347596 |
| `nvfp4` | 18,432 | 36.00 MiB | 4.358924 |
| `rk4v4` | 17,920 | 35.00 MiB | 4.352432 |

- **§2 model CONFIRMED:** bf16 = 65,536 B/token (16 full-attn layers × 4096) ⇒ 1024 tokens = 64 MiB,
  exactly the plan's N=1024/C=1 cell; 2048 tokens = 128.00 MiB matches the published table.
- **DoD §7.3 reference:** the table's `rk8v4` (4.346811) and `nvfp4` (4.358924) are the `tail=0`
  baselines our tail runs must improve toward `bf16` (4.343225), same protocol as our harness
  (`--quick`, 4096/2048). `rk4v4-e8` has no published ppl — we measure it fresh.
- Caveat: published on **RTX 3090/sm_86**; our pinned baseline is **5070 Ti/sm_120a**, so absolute
  values will differ. Use as a sanity reference, not a gate.

## 5.4 WP3 design decision — tail window & addressing (Step 6, pre-implementation)

WP3 is the critical path and biggest risk. Decision recorded before coding, grounded in verified facts.

**Question:** host-built descriptor (beellama's approach, see `PORT-BEELLAMA-SPEC.md` §B) vs device-computed
window from `positions` (what plan §1.4 assumed).

**Decision: device-computed window, because ninfer's addressing is already device-side.**
Beellama selects the tail window on the host only because *its* persistent slot assignment is
host-managed (`tail->commit` in `apply_ubatch`). NInfer addresses KV entirely from device tensors —
`paged_kv_physical_page(block_table, position)` + `paged_kv_element_offset(...)`
(`src/ops/kernel/paged_kv_address.cuh:42-45,55-69`) — with `positions` already a device input to the
append kernels. So no host round-trip is needed, and adding one would put a sync in the decode path.

**Concrete shape:**
- Exact pool = **ring buffer** of `round_up(N,64)` pages (page = 64, matching the main pool geometry).
- Tail page for absolute position `p` = `(p >> 6) & (pages - 1)` with `pages` a power of two, or
  `(p >> 6) % pages` otherwise. Tail block table is therefore **implicit** (derived), not stored.
- Per query row at absolute position `p`: tail rows are `[max(0, p+1-N), p]`; a row is valid iff its
  position is in that window. Non-aligned `N` leaves the oldest page partially valid → mask by
  position, mirroring beellama's "window+causal rides in the mask" (`set_input_kq_mask_tail_impl`,
  `src/llama-kv-cache.cpp:7227-7367`).
- Body window is `[0, max(0, p-N)-1]`; when `p+1 <= N` body is empty — still run the body partial with
  mask −∞ (upstream does exactly this, `fattn-tail.cuh:854-856`), so no family/branch switch occurs.

**Merge (reuse, not new):** the merge is the existing reducer pattern —
`causal_merge_split_statistics` (`small_t.cuh:183-210`) + `causal_attention_small_t_reduce_output_kernel`
(`:212-299`). Add a *tailed* branch **inside** the small-T family so the route family count stays 0/1/2
(`softmax_attention.h:212-216`) and the session family sequence is unchanged (plan §1.5). Workspace:
extend `SmallTWorkspace{acc,m,l}` (`causal_softmax_attention.cpp:269-284`) with a second partial set
(`acc_t,m_t,l_t`) written by a BF16 tail-partial kernel (`small_t_bf16.cuh:18-25` is the nearest
producer); merge kernel combines (acc_b,m_b,l_b) × (acc_t,m_t,l_t).

**Interaction with §5.2 correction 2:** N IS part of the tail identity (upstream puts
`retention_tokens` in the key) — fine for us: N is startup-fixed, so one graph per N. The *dynamic*
window (per-row tail length) must NOT be in the graph key; it is a runtime input.

**Open for implementation:** does the exact ring pool need its own `block_table` for tier/prefix paths
(WP7 says device-only in M1, so no), and exact rollback reserve `R` page count (`history_stride = N+R`).


### Per-TU compile verification (no need to wait for the whole build)
`build-port/compile_commands.json` (897 entries) lets one TU be compiled against the worktree without
touching the running ninja build. Harness: `.deps/ptcheck.py` -> `.deps/ptcheck.bat`.
It rewrites the repo prefix to `.worktrees/wp1/`, drops the `/Fo...` and the `@...obj.modmap`
response-file argument (absent until the module scan runs), adds the generated `build-port/apps`
include, and compiles into `.deps/ptcheck/` under the VS2022 v143 vcvars environment.
The harness also handles CUDA TUs: for nvcc commands it only retargets `-o`, so individual `.cu`
files (and the `.cuh` headers they include) can be compiled and checked too — this is how WP2 was
verified. Invoke as `python .deps/ptcheck.py <repo-relative paths...>`. Two real errors were found and fixed this way
(`kv_tail_tokens` missing from `cli::Options` / `serve::ServeOptions`).


## 5.5 Plan correction — WP5 capacity curve (verified in code)

Plan §3 WP5 says `SequenceCapacityCurve` is "线性单系数" and must become "常量项 + 线性项".
**That premise is wrong.** Verified in `src/runtime/contract/resources.h:392-398` and
`src/runtime/engine/kv_capacity.cpp:57-69`:
`reservation_bytes(p) = minimum_device_reservation_bytes + (p - minimum_main_page_groups) *
bytes_per_additional_main_page_group` — it already carries a constant term AND a linear term.
And `startup.cpp:1216-1218` sets `device_reservation_bytes = persistent.bytes + workspace.capacity +
graph_allowance`, so the sequence candidate's constant already contains `persistent.bytes`, which
contains `DecoderStateLayout::kv_payload_bytes()` — which now includes the exact tail pool.
=> **The tail is already fully accounted** as a per-sequence constant in the capacity curve and in
`MemorySummary.kv_payload_bytes`; no curve change is needed. Only the optional explicit split fields
(`kv_exact_history_bytes` / `kv_rollback_reserve_bytes`) remain unimplemented; the plan's named
`kv_exact_history_bytes` split is a reporting nicety, not a capacity-correctness requirement.


## 5.6 WP3 simplification — the split reducer already IS the merge (verified)

`causal_merge_split_statistics` (`src/ops/softmax_attention/dense/causal_cache/small_t.cuh:183-210`)
iterates every split, takes `maximum = max_i m_i`, and weights each split by `exp(m_i - maximum)`
before summing `l` — which is exactly the body x tail online-softmax merge of PORT-BEELLAMA-SPEC §A
(`g = max(m_b, m_t); out = (num_b*e^{m_b-g} + num_t*e^{m_t-g}) / (l_b*e^{m_b-g} + l_t*e^{m_t-g})`).
Therefore **WP3 needs no new merge kernel**: write the body partials into splits `[0, s_b)` and the
tail partials into splits `[s_b, s_b + s_t)` of the SAME workspace, then run the existing
`causal_attention_small_t_reduce_output_kernel` (`:212-299`). Work that remains is therefore:
1. a tail-partial kernel that reads the tail ring (window `[max(0,p+1-N), p]`, mask by position) and
   writes `partial_acc/m/l` at split indices offset by the body's split count;
2. workspace sizing so `splits` covers body + tail (`SmallTWorkspace` + `allocate_small_t_workspace`,
   `causal_softmax_attention.cpp:269-284`);
3. dispatch: in the cached small-T path, when `cache.tail.enabled()`, also run (1) and pass the total
   split count to the reducer. The route family is unchanged (still small-T), satisfying §1.5.
This is the concrete WP3 plan; the merge math is already proven in-tree.

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
