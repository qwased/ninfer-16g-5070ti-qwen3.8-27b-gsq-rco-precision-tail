# PORT-REVIEW-PLAN — an independent code-review brief

Handoff for a **fresh review session** that has not seen the work. It states where the code is, what
was claimed, how to re-run each claim, and — most usefully — the decisions a reviewer should try to
falsify. Read this with `PORT-DOD.md` (the audit table) and `PORT-MEMORY.md` (the design memory) open.

> Standing honesty rule the port adopted: **a claim is only as good as a command that reproduces it.**
> Everything below lists the command. Much of the raw evidence lives under `.deps/` and `build-port/`,
> both **gitignored** — the docs carry the numbers, the untracked dirs carry the runs.

---

## 1. Repo state (as of `80e401f9`)

| | |
|---|---|
| Repo | `D:\ninfer\ninfer-precision-tail` — **local only, no git remote** |
| Branch | `main` @ `80e401f9`, working tree **clean** |
| Port base | `b06908ba` (last donor commit); **112 commits** of port work on top |
| Leftover worktrees | `.worktrees/{m5a,wp1,wp2,wp3}` on branches `port/{m5-instrument,wp1,wp9-docs,wp10-oracle}` |
| Merges into main | `964d7be6`, `a36746db`, `b691646e`, `e8b22921` |

**Reviewer tasks on the state itself:**

1. The four `port/*` branches are **stale** (their tips predate the merges). Confirm each merge commit
   actually contains its branch's work, then they can be pruned. Do **not** review the branch tips as
   the current state — `main` is.
2. `main` contains four commits titled `wip(unverified)` and one that records their status:
   `96043f9a`, `481ac4da`, `f19425f1`, `3037bb98`, and `a3359b27`. These were "park" commits made when
   a subagent hit its turn limit mid-task; the work was later rebased, merged and **verified by the
   main agent**. A reviewer should treat the `wip` commits as *history, not authority* and confirm that
   nothing in them survived unreviewed. `3037bb98` in particular is the KLD instrument that `0f554668`
   / `d78481cb` then verified.
3. `git diff --shortstat b06908ba..HEAD -- src include apps` → **40 files, +2167/−98**; tests
   **+1914/−64** over 6 files; docs **+611/−29** over 8 files. That is the whole surface.

---

## 2. What the feature is (one paragraph)

An **exact KV tail** (`--kv-tail-tokens N`): the newest `N` keys/values of a sequence are kept
unquantized in a per-sequence BF16/F16 ring (page 64, `HeadMajor`, addressed as
`page (p/64) % ring_pages`, offset `p%64`) instead of the quantized KV cache. Attention merges the
ring through the **existing** small-T split reducer — no new merge kernel — so a decode step scores the
quantized body and the exact tail together. It is wired for **`bf16` + the INT8 family**
(`int8`, `rk8v4`, `rk4v4-e8`); `fp8`, `nvfp4`, `k8v4` allocate the ring but no route reads or writes it
(rotate-frame limitation, deviation #2). The perplexity app gained a decode-width scoring mode
(`--score-width W`) plus a KLD instrument (`--score-topk`, `--save-topk`, `--kld-base`) because the
default prefill-route scoring never reads the tail.

---

## 3. Review scope

### 3.1 The merge itself (highest value — read these first)

| File | Δ | What to check |
|---|---:|---|
| `src/ops/softmax_attention/dense/causal_cache/small_t_tail.cuh` | +406 | the exact-tail partial kernel and its split/neutral logic |
| `src/ops/softmax_attention/dense/causal_cache/small_t.cuh` | +48 | `CausalSmallTTailPartition` + `causal_small_t_tail_partition` (l.139/147) |
| `src/ops/softmax_attention/dense/causal_cache/small_t_tail_shadow.cuh` | +66 | the ring shadow-write (ring source index / `column_base`) |
| `src/ops/common/kv_tail_element.cuh` | +76 | BF16/F16 element genericity (`if constexpr`) |
| `.../small_t.cu` | +70 | tail-partial launcher, grid/workspace |
| `.../small_t_bf16.cuh`, `.../small_t_i8.cuh`, `.../small_t_i8_launch.cuh` | ±~50 each | the body side of the partition + fused append |
| `.../prompt.cu` | +40 | the Prompt-route ring write (fix `b99ba8d5`) |

### 3.2 Supporting surface

- Pool/lifecycle: `src/core/paged_kv_cache.{h,cpp}` (`PagedKVExactTailView`),
  `src/models/qwen3_5/state/decoder_state.{h,cpp}`, `.../program/planning/startup.{h,cpp}` (sizing).
- Config chain: `include/ninfer/types.h` (`ScoredTarget`/`ScoreTopKEntry`/`kMaxScoreTopK=128`, l.364/373/380),
  `include/ninfer/engine.h`, `apps/cli/options.*`, `src/serve/serve_options.*`.
- Scoring path: `src/models/qwen3_5/program/program_impl.{h,cpp}` (`score_tile`, l.740),
  `src/runtime/engine/causal_score_core.h`, `src/ops/kernel/target_logprobs.cuh` (+185),
  `apps/perplexity/{main,evaluation}.{cpp,h}` (+230/+308).
- Tests: `tests/ops/softmax_attention/causal_cache.cpp` (**+1499** — the oracle, incl. the new guards),
  `tests/models/qwen3_5/test_exact_tail_capacity.cpp` (+171), `tests/test_perplexity_evaluation.cpp`.
- Docs: `docs/performance.md` (+282), `docs/perplexity.md`, `docs/config-calculator.*`, `docs/cli.md`,
  `docs/serving.md`, `docs/maintainer/paged-kv-cache.md`.

---

## 4. Claims to falsify

Each row: the claim, where its evidence is, and the command that reproduces it. All GPU commands must
be run **serially** (single owner) and each must end with no `ninfer`/`perplexity` process and
`nvidia-smi` back to the 48 MiB idle baseline.

| # | Claim | Evidence | Reproduce |
|---|---|---|---|
| 1 | Merge is correct: BF16 + INT8 family, fused **and** cached, batched masked | `causal_cache.cpp` cases (1)/(3)/(3b)/(4) | `run-oracle.bat` → `softmax_attention: PASS`, `ORACLE_EXIT=0` |
| 2 | `tail=0` is bit-identical to pre-tail (zero regression) | `run_tail_off_regression` | same run; case asserts bit parity |
| 3 | Graph family / launch shape is tail-independent | `run_graph_family_stability_cases` | same run; asserts route family + `grid.y` per pair |
| 4 | C=1 footprint = 64 MiB exactly @N=1024, ±5% of plan §2 | `test_exact_tail_capacity.cpp`, `ppl-*/report.json` | build+run that test; or read `.deps/ppl-*-t{0,1024}/report.json` |
| 5 | Fix A (`56fc8384`): rows are quantized while the tail covers the window | `run_fused_empty_body_append_case`, `run_fused_crossing_case` | `run-oracle.bat`; both were added because they **fail** on the pre-fix kernel |
| 6 | Fix B (`b99ba8d5`): the Prompt route writes the ring | `run_prompt_ring_write_case`, `run_fused_chunked_ring_case` | `run-oracle.bat` |
| 7 | Benefit on the tail-capable storages | `docs/performance.md` matrix; raw `.deps/m5-wpb-*/`, `.deps/m5-ctx-*/` | `run-m5-wpb.bat` (resumable) + `summarize-wpb.py` |
| 8 | The residual ~9.2e-4 KLD floor is numeric (one bf16 ULP), not a defect | `run_path_parity_case` (asserts `rel_l2(tail,body) ≤ 2×criterion`) | `run-oracle.bat` → `PATHPT rel_l2=1.86e-03 max_abs=4.8828e-04` |
| 9 | `fp8`/`nvfp4`/`k8v4` are inert by design | deviation #2; `run_quantized_tail_case` (nvfp4: `rel_l2_tail == rel_l2_tail_off`) | `run-oracle.bat` → the `TAIL … nvfp4` line |
| 10 | MTP speculation is not regressed | `docs/performance.md` WP-F table; `.deps/wpf2/` | `run-m5-wpf2.sh` |

---

## 5. Soft spots — attack these (the point of the review)

1. **The partition invariant.** `causal_small_t_tail_partition` (`small_t.cuh:139-180`) claims
   *body ∪ tail == `[0, window)`, disjoint and adjacent*, via a `body_active` that floors at 1 and
   `total_active − 1` for the tail. Try to construct `(window, N, tokens, launch_capacity)` where the
   union misses a key or double-counts one — including the claimed-unreachable `total_active == 1`
   case, and the `Int8` token-count tiers (which are *not monotonic in window*, the reason the floor
   exists). The oracle asserts this only for the shapes it runs.
2. **The chunk-local boundary deviation** (`docs/performance.md` "Ring population", deviation #6). A
   chunked launch derives its exact/quantized boundary from the *chunk's* last position, so a chunk
   merges a **superset** of the newest-`N` keys. The doc claims the engine is therefore *never less
   accurate* than the documented boundary. Verify that claim, and verify that
   `run_fused_chunked_ring_case`'s `tail_tokens ≥ tokens` restriction really is the only case the
   strict oracle cannot cover.
3. **Reducer agreement.** The body partial, the tail partial and the reducer each recompute a split
   count (`causal_small_t_active_splits` / `_tail_partition`). Confirm the tail's split *indices*
   `[body_active, total_active)` always match what the reducer reads for `active_split_count`, for
   every storage and width — a mismatch would drop or double a partial silently.
4. **Shadow ring source index.** `small_t_tail_shadow.cuh` writes the ring from `input.k/v` with a flat
   `column_base = column_begin + batch*full_width`. This was itself a defect (`b99ba8d5`). Re-derive it
   for chunked and batched steps; the guards cover h24 × {bf16, rk8v4} — check h16 and the other widths.
5. **Prompt-route write shape.** The `prompt.cu` fix reuses the small-T shadow kernel with
   `grid.y = q.ne[3]` and passes `q.ne[2]` as both `batch` and `batch_size`. Confirm the batch/`kv_head`
   mapping and that `positions`/`valid_columns` are the tensors a **prompt** step actually carries
   (the pre-fix bug was that this route's `block_table` gating differed from the batch view).
6. **`--score-width` bypasses `effective_prefill_chunk` on purpose** (`program_impl.cpp:740`). The
   design claim is that a `W ≤ 8` stays `≤ 8` and selects the small-T route. Check that no `W` value
   can produce an invalid/unallocated tile, and that the *cap* by `prefill_chunk` is the only clamp.
7. **M2 F16 ring.** `kv_tail_element.cuh` claims the BF16 instantiation is verbatim pre-M2
   (`mma_bf16`) with F16 added via `if constexpr`. Diff the BF16 path against the M1 commit to confirm
   it is unchanged; a silent change would move the tail-off bit-identical guarantee (§4 row 2).
8. **Capacity accounting.** `MemorySummary` is asserted to match plan §2 byte-for-byte
   (`round_up(N,64)*65,536*C + C*4 MiB`). The `+1` rollback reserve page is **allocate-only** (WP8 is
   deferred). Confirm no runtime path writes it, and that the per-C pool really is `(pages+1)*C`.
9. **MTP caveat.** The draft cache is structurally tail-free (correct), but the verification width
   `verify_window+1` can exceed 8 → those steps take `Prompt` and skip the tail. Decide whether that
   matters for the acceptance numbers in §4 row 10.
10. **The KLD instrument itself** (`target_logprobs.cuh`, `apps/perplexity/evaluation.cpp`). The
    support is the union of both top-K sets plus the target, renormalized. Check the math, the
    `kMaxScoreTopK=128` bound (is `--score-topk 128` safe?), and what `--disjoint` changes.
11. **Docs vs artifacts.** Spot-check `docs/performance.md`'s numbers against `.deps/*/report.json`.
    The doc is the port's only durable evidence for the end-to-end claims.

---

## 6. Build & run (the harness)

Everything is driven through `.deps/*.bat` (gitignored). Toolchain: VS2022 BuildTools MSVC 14.44
**before** VS2026 on PATH, CUDA 13.3, `arch=compute_120a`; build dir `build-port` (CMake/Ninja).

```
.deps\env-port.bat                       # source the toolchain (NEVER redirect this call)
.deps\build-target.bat <target> [jobs]   # one target; default ninfer-perplexity
.deps\build-target.bat ninfer_tests      # the oracle + unit tests (bundle exe ninfer_tests.exe)
.deps\run-oracle.bat                     # -> tests\ninfer_tests.exe ninfer_softmax_attention_test
.deps\m5-check-host.bat                  # fast host-only syntax check of causal_cache.cpp (/Zs)
.deps\run-m5-wpb.bat                     # the WP-B KLD matrix (resumable; writes .deps/wpb.done)
```

Traps that cost real time here (see `PORT-MEMORY.md` §5.13/§5.14):

- **Never `call env-port.bat >nul 2>&1`** — redirecting it breaks the environment (`cl`/`cmake`/`ninja`
  "not recognized"). `m5-check-host.bat` uses the absolute `cl.exe` path for exactly this reason.
- **The donor model path is `D:\ninfer\ninfer-package\model\...`** — `ninfer`, with the leading `n`.
  A retyped `infer-package` fails as `CreateFileW: Win32 error 3`, which looks like a missing file.
  Copy the path; never retype it.
- Builds may be **silently killed** if they run long — build in foreground chunks and re-invoke; the
  build is incremental so re-running resumes.
- `tools/ninfer-multi-gpu-probe` has a **pre-existing, unrelated** link failure, so the default `all`
  target fails; build named targets instead.
- After **every** GPU run: `tasklist | grep -iE "ninfer|perplexity"` empty and `nvidia-smi` = 48 MiB.

---

## 7. Explicitly out of scope / deferred

- **WP8** (transaction/rollback: commit-after-attention, degraded-on-failure) — deferred; not reachable
  in M1 (device-only pool, C=1, no failure path). Deviation #4.
- **M4** (exact pool on host/disk; per-request tail observability) — not started, gated on a real need.
- **WP-D D1/D2** (pre-port build-tree A/B) — cannot be done: no pre-port tree exists. D4 is non-gating.
- **Generation-route pre-fix A/B** — not reproducible: the fix rebuilt `ninfer.exe` in place.
- **Merge numeric fidelity** — characterised (one bf16 ULP), not improved. Lowering it needs a
  higher-precision probability path in the *body* kernel too.

---

## 8. Suggested review order

1. State (§1) → 2. `small_t_tail.cuh` + `small_t.cuh` partition (§3.1, soft spot 1/3) → 3. the two
fixes and their guards (§4 rows 5/6, soft spots 2/4/5) → 4. run the oracle yourself (§6) →
5. the scoring/KLD plumbing (§3.2, soft spots 6/10) → 6. docs vs `.deps` artifacts (§4 row 11).

Record anything found as a new row in `PORT-DOD.md` (with a reproducing command) and a `Step` in
`PORT-JOURNAL.md`, per the port's rule: **a WP is `DONE` only with command-reproduced evidence.**
