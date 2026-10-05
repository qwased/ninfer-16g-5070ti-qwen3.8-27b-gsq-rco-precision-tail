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

### A3 KLD
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
