# PORT-M5-PLAN — rk4v4-e8 档 + 精度尾的性价比收口

Active campaign plan. Status values: `TODO` · `WIP` · `DONE` · `BLOCKED`.
Authority for M5; supersedes the M1 verdict only for the *benefit* question (mechanism closure is M1's).

## 0. Why (the gap this closes)

M0–M3 closed the **mechanism**: the FP32 oracle is green and the `TAILGAIN` line shows tail-on rel-L2
strictly below tail-off on the decode route. They did **not** establish the **benefit**, and we now
know why — empirically, not by inference:

- `apps/perplexity` scores the **prefill route** (1024-wide query tiles → `Prompt`), and by design the
  tail merges **only on the small-T decode route (width ≤ 8)**. So the exact ring is never read during
  scoring → measured ppl is **bit-identical** for tail=0 vs tail=N on every wired storage
  (`docs/performance.md`, tail section: rk8v4 4.65880995706738 both ways).
- The llamacpp side merges the tail **inside FA at all widths**, so its `llama-perplexity` *does* read
  the ring — the M0 numbers differ (kvarn4 tail0 5.3559 vs tail1024 5.3622), which is the tell.
- llamacpp judges quality by **KLD** (median/mean/P99/P99.9/max + Same-top%) against a persisted
  **BF16-KV logits baseline** (`beellama-features.md:543,699-730`) — a sharp metric. M0 used **PPL**,
  which is blunt. M0's null result is thus **blunt metric × ctx 4096 × good body (kvarn4)** stacked
  three ways; it is **not** evidence that the tail is worthless.

## 1. Decisions (user-set)

- **Route: A — decode-width scoring.** No change to the product merge math. Score tokens in width-`W`
  tiles with `W ≤ 8` so the small-T route (and its tail merge) is actually exercised. This measures the
  tail's real habitat: **generation**.
- **Metric: KLD vs a BF16-KV baseline**, matching llamacpp's channel, plus ppl.
- **Context: ctx ∈ {8K, 16K, 32K}.** 16K is upstream's proven point; ~8.2K is the iso-memory crossover.
- **Extra: WP-F — MTP speculative-decoding impact** (quality *and* correctness).

## 2. WP-A — the instrument (change points已探明)

### A1 `--score-width W` — **DONE** (`e3afebbe`)
- Call chain: `apps/perplexity/main.cpp:365-421` (WindowPlan → `engine.score_tokens(span, first_target)`).
- Engine: `Engine::score_tokens` (`engine.h:83-85`, `engine.cpp:355-384`) → `CausalScoreCore`
  (`causal_score_core.h:62/136`) → `ProgramImpl::causal_score` (`program_impl.cpp:588`).
- **Width is implied by the token span; there is no width parameter.** The actual attention query width
  is `prefill_chunk` via `nominal` (`program_impl.cpp:684`), **not** `kCausalScoreTile` (`startup.h:25`,
  which only bounds logits/output staging, `:634,716-730`).
- Change: parse in `main.cpp`; add an `EngineOptions` field; thread through planning
  (`startup.cpp:1360-1374`); consume at `program_impl.cpp:684` as `nominal = min(score_width, …)`.
  Scoring forbids CUDA graphs (`program_impl.cpp:594`) and `small_prefill` only affects width>16
  (`causal_softmax_attention.cpp:350-358`), so **no route/graph work is needed**.
- Trap: `score_width` must bypass `effective_prefill_chunk`'s 128-alignment (`startup.cpp:1339-1354`).

### A2 route/tail confirmation (已核)
- `causal_attention_resolve_route` (`causal_softmax_attention.cpp:347-402`): q_heads==24 `width≤8`→SmallT
  (`:392`); q_heads==16 `width≤6`→SmallT (`:394`).
- The tail is read only in small-T: `launch_tc_partial_bf16` reads `cache.tail` and launches
  `causal_attention_small_t_tail_bf16_kernel` (`small_t.cu:214,237-252`). The `Prompt` path
  (`causal_attention_prompt_attention_launch`) **never references `cache.tail`**.
- Trap: BF16 body with ≤128 visible keys still resolves to `Prompt` (`:366-367`) → the first ~128 keys
  of a window skip the tail. INT8-family/rk8v4 have prompt-limit 0, so no such gap. Document it.
- **Empirical (Step 40, run):** ctx=N=1024 rk8v4, W=8 tail=0 ppl 6.496148968946205 vs tail=1024
  6.464422630715829 → W=8 is still small-T. This model has **24 query heads**, so the small-T width
  ceiling is **W=8**, and W=8 is the cheapest valid decode width for WP-B.

### A3 KLD — **DONE** (`3037bb98`, verified Step 41)
Verified two ways: self-comparison gives KLD exactly 0 / same-top 1.0; rk8v4 vs a bf16-tail0 reference
at W=8 ctx1024 gives mean KLD 0.002738 (tail0) → 0.000912 (tail1024), same-top 0.9726 → 0.9853.
Report `kld` block; default K=100.
- Today `ScoreAggregate::add(std::span<const float>)` gets the **target-token logprob only**
  (`program_impl.cpp:661-676`, via `ops::target_logprobs`, `target_logprobs.h:44`). But the full
  `[vocab,C]` BF16 logits are **already materialized on device** before the reduction → top-k is reachable.
- Existing top-k machinery does **not** drop in: `first_token_top_logprobs` is generation-only, first
  token only, host-side (`prefill.cpp:1108-1113`, `core/token_logprobs.cpp:21`); `ops::linear_topk`
  (`linear_topk.h:48`) re-projects hidden and is bound to Q8/FP8 head geometries.
- Minimal path: add a device top-k over the existing logits (top-K, e.g. 100 — **not** full vocab),
  widen the `Engine::score_tokens` return (touches `engine.h:84`, `causal_score_core.h:62/136`,
  `program_impl.{h,cpp}`, `evaluation.h:30`, `main.cpp:419`).
- Baseline: run **bf16 body + tail 0** once to persist the reference top-k; run candidates with the same
  evaluation tokens/order.

### A4 instrument self-check (falsifiable gate) — **DONE, PASSES** (Step 39)
- tail=0: ppl at W=1 ≈ ppl at W=1024 (within tolerance; only reduction-order drift).
- tail=N: ppl at W=1 **<** ppl at W=1024. This is the "the ring is read" criterion.
- If A4 fails → stop and revisit the design; do not proceed to WP-B.

**Result (rk8v4, ctx=N=1024, 2046 scored tokens):** tail=0 W1024 6.485411508522421 / W1
6.488674471977547 (+0.05%, drift); tail=1024 W1024 6.485411508522421 (bit-identical to tail=0) / W1
**6.476584746346809** (strictly lower). Gate passes; `kv_exact_history_bytes` 64 MiB exactly.

## 3. WP-B — three-tier benefit campaign (ninfer)

- Storages `int8 / rk8v4 / rk4v4-e8` (+ `bf16` KLD baseline); `tail ∈ {0, 1024, 2048}`.
- Metrics: decode-width **KLD** (median/mean/P99/P99.9/max + Same-top%) and ppl.
- **Acceptance proposition**: tail-on error ≤ tail-off, and the gain grows **monotonically with body
  coarseness** (rk4v4-e8 > rk8v4 > int8). Non-monotonic ⇒ the gain does not come from body coarseness ⇒
  the motivation is questionable.
- Record the ctx dependence across {8K, 16K, 32K}.

## 4. WP-C — llamacpp cross-product comparison

- Same model-family `.gguf`, `--spec-type none`, `-b 2048 -ub 512`.
- Persist a BF16 KLD baseline (`--save-all-logits`), then run `kvarn4` t0/t1024 and `q8_0`/`f16`
  references (`--kl-divergence --kl-divergence-base`).
- Compare the **incremental** ΔKLD by byte tier (feasibility report §3.2), never absolute values.
- The report must state the semantic difference: llamacpp merges the tail at **all widths**; ninfer
  merges on the **decode route only**.

## 5. WP-D — port regression vs the pre-port baseline

- D1 correctness: HEAD with `--kv-tail-tokens 0` vs the pre-port commit, fixed prompt/greedy/seed →
  token-identical text.
- D2 perf: tail=0 prefill/decode within noise of baseline; tail=N decode cost ≤ threshold.
- D3 memory: tail=0 `runtime_reservation_bytes` bit-identical to the pre-port figure.
- D4 corroboration: read-only run of the `D:\ninfer\ninfer-package` product; compare output/accounting
  (cross-build, corroboration only — not a gate).

## 6. WP-F — MTP speculation × tail (added)

**Question**: does the tail affect MTP speculative decoding — both *quality/acceptance* and
*correctness* (does a rejected draft pollute the exact ring?).

**Status (Step 42):** **F1, F2, F5 DONE by code analysis (read-only).** F1 confirmed (mtp_kv has no
tail member; attach happens once on text_kv only → draft tail=0 by construction). **F2: NO ring
pollution** — acceptance is a frontier advance only, the ring is never rolled back (the 4 MiB reserve
is allocated only; WP8 deferred), but causal masking + same-round shadow rewrite means only the reading
round's rows are observable. **R-D does not trigger.** F5 confirmed (`paged_kv_window_rows` remaps
block tables only; never touches `cache.tail`). Caveat: MTP verification width = `verify_window+1` can
exceed 8, so those steps take the Prompt route and skip the tail. **F3 (acceptance) and F4 (perf)
remain** — GPU runs on the `ninfer` cli, after WP-B.

- **F1 structural**: the draft cache `layout.mtp_kv` has **no tail member** (asserted in M3); re-confirm
  the draft reads only `mtp_kv` and can never read the exact ring.
- **F2 correctness (the sharp one)**: verification writes `k+1` exact rows and the tail merge reads them.
  **If a draft is rejected, are its exact-ring rows invalidated/rolled back?** (Plan WP8 rollback was
  deferred — this is exactly where it surfaces.) Test:
  - same prompt: spec+tail vs spec+no-tail vs non-spec+no-tail — compare output and acceptance;
  - construct a rejection-heavy prompt and look for drift in already-accepted tokens;
  - assert the `keep_last_writes` / commit-after-attention semantics hold for the ring (rows commit after
    attention; the current round reads the graph-local source).
- **F3 acceptance**: tail0 vs tailN over many prompts × many rounds; report acceptance rate and
  tokens/round. Hypothesis: the tail does **not** reduce acceptance (expected ≥, since the draft is
  near-exact and the verifier moves closer to exact with the tail on).
- **F4 perf**: spec-on decode tok/s, tail0 vs tailN; isolate the extra verification-pass cost.
- **F5 interaction**: `--mtp-attention-window` (draft sees first 64 + newest N before its query) vs the
  tail; re-confirm the draft's forced tail=0.

## 7. WP-E — conclusion and docs

- Update the `docs/performance.md` tail section with decode-width KLD/ppl and the MTP impact.
- Change DoD §7.3 from `BLOCKED` to evidenced; if the result is negative, state plainly that the tail is
  not worth it at this tier.

## 9. Outcome (M5 executed, Steps 38-45)

**WP-A — DONE.** A1 `--score-width W` (`e3afebbe`); A2 confirmed; A3 KLD top-K instrument (`3037bb98`,
verified: self-KLD 0, and rk8v4 tail0→tail1024 0.002738→0.000912 at ctx=N=1024); A4 gate **PASSES**
(tail=0 ppl W1≈W1024; tail=N ppl W1<W1024). Small-T ceiling measured: W≤8 (24 q-heads).

**WP-B — EXECUTED, NEGATIVE (defect).** At ctx=N=1024 the tail is correct and improves. But whenever a
**body** and the tail coexist the merge is **wrong**: ctx2048/rk8v4 tail1024 KLD 0.022539 vs tail0
0.002838; ctx8192 int8/rk8v4 tail1024 ≈0.027 vs ≈0.001-0.003; the tell is **bf16 body+tail vs a bf16
reference = KLD 0.137** (same precision ⇒ should be ≈0). Reproduces at W=1 and W=8, for bf16/int8/rk8v4.
The in-tree oracle (T=6, keys≤67, single fused append) does not cover this regime. **The benefit cannot
be claimed; the merge must be fixed.** The **full matrix is complete** — 3 storages × tail {0,1024,2048}
× ctx {8192,16384,32768}, plus rk8v4 at ctx 2048 and the whole-window clean cases. Every cell degrades;
the degradation is 24×/21×/14× (int8), 10×/9×/6× (rk8v4), 4×/4×/2.4× (rk4v4-e8) at 8K/16K/32K, and
tail-on KLD is ~storage-independent while tail-off spans 6× — the defect signature. **(Resolved by the
fix: the post-fix matrix is in §10.4 — tail-on ≤ tail-off 18/18, monotone by coarseness.)**

**WP-C — DONE.** llama.cpp `kvarn4` merges inside FA at all widths and cuts mean KLD 0.001107→0.000702
(−37%), max 0.188→0.078 — the benefit is real; ninfer's merge is the gap.

**WP-D — PARTIAL.** D3 evidenced: `node docs/config-calculator.test.mjs` → **PASS**, incl. "N=0 leaves
the golden engine reservation untouched" and the 262144-token int8 27B golden matching the engine's
refusal figure. D1/D2/D4 **not run**: a pre-port build tree (b06908ba) does not exist here and
reconfiguring one is a multi-hour, >100 GB cost; the shipped product exposes only `ninfer-serve.exe`
(D4 is explicitly non-gating). Partial support: `tail=0` is the identity partition (oracle N=0
bit-exact, DoD §7.6 DONE), the calculator pins N=0 to the golden reservation, and WP-F measured HEAD
`tail=0` decode at 82.6 tok/s.

**WP-F — F1/F2/F5 DONE (analysis), F3/F4 measured (confounded).** No ring pollution on rejection (R-D
does not trigger). Acceptance 24.1%→20.3%, decode 82.6→70.8 tok/s with the tail — but confounded by the
merge defect; repeat after the fix.

**WP-E — DONE.** `docs/performance.md` gained the decode-width KLD / llamacpp / speculation sections;
`PORT-DOD.md` §7.3 `BLOCKED`→`NEGATIVE` with the M5 evidence.

**Follow-up (the real remaining work) — root cause now closed (Step 46).** The defect is not the merge
and not the split partition. The fused-append small-T kernel writes the quantized cache **only from
body splits**; when `body_window == 0` the partition sets `body_active = 0` and both body kernels
return before their fused-append block, so while the tail covers the whole window (`window ≤ N`) the
rows are never quantized into the body cache (the shadow kernel writes only the ring). Those rows are
appended only at their own step, so they are a permanent hole; once `window > N` the body reads
`[0, window−N)` and hits it. This explains `ctx = N` clean, every `ctx > N` broken, the bf16 0.137
(self-precision) tell, the storage-independent tail-on KLD, and why the oracle (which drives the
*cached* entry via `kv_cache_append`, with no `body_active == 0` skip) never sees it. Fix: make the
append own the newest rows independent of the body split count; then add a large-window oracle case
(window ≫ 67 keys, `ring_pages ≥ 3`, `body_window > 0`, earliest rows built through the fused path) and
re-run WP-B/C/F. Until then the tail's benefit is unproven and `--kv-tail-tokens` should not be
recommended.

## 8. Discipline and risks

- Single-GPU serial owner; confirm every process terminates after each run; donor trees read-only;
  subagents + worktrees; write memory docs at every step.
- **R-A**: if threading `score_width` touches planning in more than one place, cost rises.
- **R-B**: width≤8 scoring costs O(1024/W) more (one full-stack pass per W tokens per window) → use a
  short ctx and a bounded scored-token count.
- **R-C**: KLD needs an engine return-type change — not a small change; top-K (not full vocab) is the
  pragmatic path.
- **R-D**: if **F2** exposes ring pollution on rejection, tail × spec needs WP8 rollback first — a
  product change; treat as a stop-and-decide.

## 10. Fix plan — close the body+tail cache hole (Step 47+)

**Defect.** The fused-append small-T kernel writes the quantized body cache only from *body* splits;
with `body_window == 0` the partition sets `body_active = 0` and both body kernels return before their
fused-append block, so while the exact tail covers the whole window (`window ≤ N`) no row is quantized
into the body cache. Rows are appended only at their own step, so they are a permanent hole that a
later `window > N` body reads from `[0, window−N)`.

**Design (minimal, 3 files, no new kernel):**

1. `small_t.cuh` — `causal_small_t_tail_partition`: floor `body_active` at 1 whenever `tail_keys > 0`,
   including `body_window == 0`, so one body split always exists to own the append. It scores nothing
   (neutral partial) and the tail takes `total_active − 1`.
2. `small_t_bf16.cuh` + `small_t_i8.cuh` — hoist the fused-append block above the
   `if (split_start >= split_end) { write_neutral(); return; }` early return, and change the
   append-owner predicate from `(split_start < body_window && split_end == body_window)` to
   `split == active_split_count - 1` (the last body split). The two are equivalent for
   `body_window > 0`; the new form also fires for the append-only split when `body_window == 0`.

**Invariants to hold:** exactly one split appends each current-step row exactly once; the union of
append ranges is `[0, window)`; `tail_tokens == 0` stays bit-identical; the cached entry
(`writes_cache == false`), the prompt route, and fp8/nvfp4/k8v4 are untouched; `grid.y` is unchanged
(DoD #5).

**Accepted cost:** tail-on `window ≤ N` loses bit-identity (one split moves tail→body, fp32 reduction
order); bounded ~`S·ε` ≈ 1e-6…8e-6, three-to-four orders below the defect — see Step 47.

**Verification:**
- `ptcheck` syntax on the touched TUs; full build of the small-T objects.
- Existing FP32 oracle suite (`tests/ops/softmax_attention/causal_cache.cpp`) must stay `PASS`,
  including the tail-off bit-exact regression and the empty-body-window tail cases.
- New case: a fused-append sequence whose earliest rows are built while `window ≤ N` and which then
  crosses `window > N`; the attention output must match the exact/quantized reference (today it
  reads the hole). Plus a direct invariant check that the quantized cache holds `[0, window)` after
  an empty-body fused step.
- Re-run WP-B/C/F only after the fix lands and the oracle passes.

### 10.1 Execution outcome (Step 48)

The fix landed as `56fc8384` and the acceptance re-run passed: rk8v4/int8 KLD drops 20-42x (0.022539 →
0.001133 at W=8; 0.038912 → 0.000933 at W=1, ctx 2048, N=1024), tail-on now beats the tail-off control
(0.002416), the whole-window case is unchanged (0.001068 → 0.001096) and `tail=0` stays bit-identical
(oracle `N=0` bit parity PASS). The `window ≤ N` ppl moved −0.16% (the fp32 split-order price, an
improvement). **One separate defect remains:** `bf16` storage + tail is broken at scale independent of
this hole and of the ring element type (whole-window vs `bf16 tail0` = 0.207) — tracked as a new open
item, not a regression. `apps/perplexity` now exposes `--kv-tail-type bf16|f16`. See `PORT-JOURNAL.md`
Step 48 for the raw numbers.

### 10.2 Unit guard (Step 49)

The plan's third verification bullet — the direct invariant check that the quantized body cache holds
`[0, window)` after an empty-body fused step — landed as `run_fused_empty_body_append_case` in
`tests/ops/softmax_attention/causal_cache.cpp`. It runs `run_a1_case` with the harness's ring-priming
`ops::kv_cache_append` *dropped* (`prime_tail_body = false`), so the fused append is the only writer of
the six rows, and reuses the existing `verify_cache` comparison of the device `k_`/`v_` planes against the
host fixture. Wired over both geometries × `{bf16, rk8v4}`, fragmented mapping; four
`TOPTEST fused-append empty-body cache write` lines. `ninfer_tests` links (`BUILD_EXIT=0`) and the full
oracle re-runs green (`ORACLE_EXIT=0`, `softmax_attention: PASS`) with the new lines present. This
completes the verification list (touched-TU build, FP32 oracle suite incl. `N=0` bit parity, new case).
The empirical fail-without-fix reversal was deferred — reverting `small_t.cuh` forces a full small-T
rebuild (~2 h); the Step 48 KLD re-run already is the system-level fail-without-fix evidence. See
`PORT-JOURNAL.md` Step 49.

### 10.3 Crossing case — the multi-step consequence (Step 50)

The empty-body case guards the *missing append* directly. The plan's first half of bullet 3 — a
fused-append sequence whose earliest rows are built while `window ≤ N` and which then crosses
`window > N`, with the attention output held to the reference — needs **multiple steps**, because a
single fused step writes all of `[0, window)` (`append_end = window` on the last body split); a row is
only ever written at its own step, so the hole is a multi-step artifact. `run_fused_crossing_case`
drives that: one persistent `DeviceCache`, 24–25 sequential fused launches of 8 tokens each
(`window 192–200`), the early steps entirely inside the tail (`body_window == 0`), the last scoring a
body of 8. The host reference appends every step's rows to the fixture in order and populates the ring
once from the whole sequence, then the **final** step's output plus the whole cache and ring are
compared. Wired for `d256-h24-kv4` + `rk8v4` × `{fragmented, offset}` × `{N=192 (window 200, ring
wraps), N=129 (window 192, no wrap)}` — four `TOPTEST fused-append crossing build` lines.
`softmax_attention: PASS`, `ORACLE_EXIT=0`.

Only rk8v4 is used: its decode prompt cutoff is 0 keys, so every step routes to the small-T fused
kernel at any window; bf16's 256-key cutoff sends the early steps to the writing-only prompt route,
which appends correctly and could never build the hole. The case is structurally discriminating — the
body region `[0, window − N)` is exactly the rows the early empty-body steps own, so before the fix
the final step scored the fixture and both the output and the cache planes disagreed (the same
comparison already caught the harness's own token-major slicing bug during development, `cache-v` +
`exact-tail` mismatches). The fail-without-fix reversal is deferred for the same build cost as §10.2.

### 10.4 Post-fix WP-B matrix — the benefit, measured (Step 51)

The full post-fix campaign — `.deps/run-m5-wpb2.bat`, **27/27** cells (storages `{int8, rk8v4,
rk4v4-e8}` × tails `{0, 1024, 2048}` × ctx `{8192, 16384, 32768}`, W=8) — is complete; the same
32,764-token corpus and the **same** per-ctx bf16-tail0 references as the pre-fix run, so it is a
controlled A/B.

- **Controls bit-identical.** All three tail-0 ctx-8192 cells equal their pre-fix reports byte-for-byte
  in ppl and KLD mean → the fix is tail-on-only.
- **Defect removed.** Tail-on ctx-8192 mean KLD improves vs pre-fix by **26.0×/27.1×** (int8),
  **22.5×/24.4×** (rk8v4), **13.6×/17.3×** (rk4v4-e8); 0.027–0.028 → 0.0010–0.0021.
- **Acceptance proposition met (§3):**
  1. tail-on ≤ tail-off — **18/18** cells.
  2. gain monotone with body coarseness — **YES** at all 3 ctx × both tail lengths
     (int8 1.02–1.10× < rk8v4 2.04–2.30× < rk4v4-e8 2.97–4.04×).
  3. ctx dependence mild, monotone decreasing (a fixed ring is a smaller fraction of a larger window).
- `same_top` and ppl move the same way (rk4v4-e8 8K: ppl 6.0112→5.9802, same_top 0.966→0.982).

| mean KLD | tail0 | tail1024 | tail2048 | gain 1024/2048 |
|---|---|---|---|---|
| ctx 8192 int8 | 0.00112641 | 0.00103730 | 0.00102872 | 1.09× / 1.09× |
| ctx 8192 rk8v4 | 0.00264652 | 0.00120207 | 0.00114922 | 2.20× / 2.30× |
| ctx 8192 rk4v4-e8 | 0.00652166 | 0.00200700 | 0.00161394 | 3.25× / 4.04× |
| ctx 16384 int8 | 0.00112316 | 0.00104790 | 0.00101761 | 1.07× / 1.10× |
| ctx 16384 rk8v4 | 0.00259360 | 0.00119415 | 0.00115986 | 2.17× / 2.24× |
| ctx 16384 rk4v4-e8 | 0.00648249 | 0.00213736 | 0.00174379 | 3.03× / 3.72× |
| ctx 32768 int8 | 0.00103341 | 0.00100826 | 0.00100349 | 1.02× / 1.03× |
| ctx 32768 rk8v4 | 0.00241272 | 0.00118513 | 0.00110771 | 2.04× / 2.18× |
| ctx 32768 rk4v4-e8 | 0.00631572 | 0.00212822 | 0.00170770 | 2.97× / 3.70× |

**Remaining open (not a WP-B result):** bf16 *storage* + tail at scale (§10.1) is untouched by this
campaign (bf16 appears only as the tail-0 reference); it is the last correctness item before
`--kv-tail-tokens` can be declared safe for bf16 storage. WP-C (llamacpp) already shows the external
concept works; the three quantized tiers now show ninfer's own merge works too.

### 10.5 Problem B is broader than bf16 — the Prompt route never writes the ring (Step 52)

Supersedes the "bf16 only" framing of §10.1. Root cause (two read-only subagents + my own re-reads):

- bf16's nonzero `prompt_limit` (`causal_softmax_attention.cpp:367`: 128 at W≤4, 256 at W5–8) sends
  early rows to `Prompt` (`:390`). `Prompt` appends via the **batched** `kv_cache_append_batch_launch`
  (`prompt.cu:288`), whose ring-shadow write is gated by
  `if constexpr (requires(const CacheView& c){ c.block_table; })` (`ops/kv_cache/append/launch.cu:25`)
  — false for `PagedKVBatchLayerView` (`block_tables`, plural) → **compiled out**. Ring writers are only
  the single-row `ops::kv_cache_append` (product caller = `mtp_kv_` only, `text.cpp:523`) and the
  small-T shadow kernel (`small_t.cu:339`).
- **Correction:** the tail is read only by **bf16 and the int8 family** (int8/rk8v4/rk4v4/rk4v4-e8/
  rk2v4-e8). fp8/k8v4/nvfp4 small-T launchers contain **no tail code** → the feature is **inert** there.
- **B is therefore not bf16-only.** A prefill chunk > 64 keys (default 1024; `small_prefill` covers only
  width 17–64) routes to `Prompt` (`:401`) for **every** storage, so the ring is unwritten for prefill
  rows; the tail then reads the newest `min(N, window)` keys by absolute index and, for the first `g < N`
  generated tokens, reads unwritten slots → **corrupt in generation for every tail-capable storage** with
  a prompt > 64 tokens. Unseen by WP-B (W=8 from position 0 → small-T throughout) and WP-F (~20-token
  prompts → shadow writes the ring); the oracle drives the cached entry, which writes the ring.

**Consequence:** the fix is **required**, not optional — a guard would disable the tail whenever a long
prompt is prefilled, i.e. in its main habitat. Fix = write the ring from the batched append path (a
per-row kernel keyed on `block_tables`/`table_rows`, or launch the existing storage-independent
`causal_attention_small_t_tail_shadow_kernel` from `causal_attention_prompt_launch`). It subsumes the
bf16 scoring case, so one fix + one rebuild (which also refreshes the pre-fix `ninfer.exe`) closes both,
together with the WP-F re-run.
