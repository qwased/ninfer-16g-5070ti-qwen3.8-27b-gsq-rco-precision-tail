# NInfer 16G · Qwen3.8-27B GSQ-RCO · KV precision-tail port

[简体中文](README.md) | **English**

**RTX 5070 Ti / RTX 5080 / RTX 5090 · Windows 16GB · local inference**

This repository is the **RTX 50-series Windows 16GB NInfer product build** (Qwen3.8-27B GSQ-RCO Q3, CUDA 13 Native engine, tray manager) **plus the completed "KV precision tail" port**.

This README documents the **port work**; for product installation, downloads and deployment see the [RTX 5070 Ti Windows guide](docs/rtx-5070ti-windows.en.md) and the [download instructions](docs/rtx-5070ti-windows-downloads.md).

- This repository: **[qwased/ninfer-16g-5070ti-qwen3.8-27b-gsq-rco-precision-tail](https://github.com/qwased/ninfer-16g-5070ti-qwen3.8-27b-gsq-rco-precision-tail)**
- Direct upstream (this branch's base): **[Ryan-gsq/ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco](https://github.com/Ryan-gsq/ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco)**
- Upstream consolidation: **[iamwavecut/ninfer-all](https://github.com/iamwavecut/ninfer-all)**; original inference engine: **[Neroued/ninfer](https://github.com/Neroued/ninfer)**

---

## TL;DR

- Ported beellama.cpp's **KV precision tail** (KVCPT, `--kv-tail-tokens`) into NInfer: the newest **N tokens of every sequence keep an exact (F16) K/V shadow ring**, and attention computes the quantized body and the exact tail as **two FP32 partials, then merges them with online-softmax**.
- **Zero regression**: 64/64 acceptance cells **byte-identical** (MTP speculation, vision, ctx 8192/32768, eight storages), **memory bit-reproducible**, and **0/64** decode cells over 2 %.
- **Benefit quantified**: decode-width KLD falls in **24/24** cells, and the gain **grows monotonically with body coarseness** (`int8` 1.02–1.09× < `rk8v4` 2.04–2.41× < `rk4v4` 2.26–3.14× < `rk4v4-e8` 2.97–5.01×).
- Cost: **64 MiB per sequence** at N=1024/C=1 plus a 4 MiB rollback reserve; with the tail on, decode is about **−6 %** (indicative), and the port itself (tail off by default) is **−0.18 %** median decode.
- Two **ring-write defects** (plus a ring-capacity guard) were found and fixed — they were the real reason the benefit was unmeasurable at first. See §5.

## 1. What the precision tail is

The newest rows of a quantized KV cache carry quantization error, and attention is **most sensitive to the newest tokens**. The precision tail keeps the newest N tokens' K/V **unquantized** in a **device-only exact pool** (a shadow ring); attention then:

1. **body partial** — one ordinary attention pass over the quantized K/V, giving `(acc_b, m_b, l_b)`;
2. **tail partial** — a second pass over the exact K/V, giving `(acc_t, m_t, l_t)`;
3. **FP32 merge** —

```
g   = max(m_b, m_t)
acc = acc_b·exp(m_b − g) + acc_t·exp(m_t − g)
l   = l_b  ·exp(m_b − g) + l_t  ·exp(m_t − g)
out = acc / l
```

The merge matches upstream `fattn-tail.cuh` exactly: **not normalized-and-added**, but an online-softmax merge of two unnormalized partials, both in the **FP32 domain** (the body's quantized domain is already dequantized and normalized inside the body FA).

The design choices that buy the zero regression:

- **No new Op family and no change to the CUDA Graph family sequence.** The tail merge lives **inside the existing small-T family**, so a session's family sequence (prompt → small-T) is identical with and without the tail — no extra graph recapture. `retention_tokens` (i.e. N) is part of the engine identity key, while **each query's dynamic window is only a runtime input**.
- **Fused dual write**: in the **same kernel** that writes the quantized body, the unquantized K/V is written a second time into the exact ring (one read, no double quantization); repeated writes to the same slot within one ubatch are deduplicated last-write-wins.
- **The exact pool is device-only**, outside the host/disk tiers, and the draft (MTP) cache is **tail-free by construction** — so `--kv-tail-tokens` sharpens only the *verifier*.

## 2. Scope and memory cost

| Item | Value |
|---|---|
| Storages the merge applies to | `bf16` and the **INT8 family**: `int8`, `rk8v4`, `rk4v4`, `rk4v4-e8`, `rk2v4-e8` |
| Storages allocated but **inert** | `fp8`, `nvfp4`, `k8v4` (their decoded key plane is in a rotated frame and tail-row rotation is not implemented; tail-on and tail-off are byte-identical there) |
| Route that merges | the **small-T decode route** (query width ≤ 8). The prompt/prefill route **writes only** |
| Ring element type | `--kv-tail-type f16` (default, 10-bit mantissa) or `bf16` (7-bit, the split-verification form); both are 16-bit, so pool size, page geometry and `MemorySummary` are identical |
| Exact ring resident | `round_up(N,64) × 65,536 × C` bytes (C = `--max-concurrency`, 1..8) |

| N | C=1 | C=2 | C=4 | C=8 |
|---:|---:|---:|---:|---:|
| 512 | 32 MiB | 64 MiB | 128 MiB | 256 MiB |
| 1024 | **64 MiB** | 128 MiB | 256 MiB | 512 MiB |
| 2048 | 128 MiB | 256 MiB | 512 MiB | 1024 MiB |

Measured (27B artifact, C=1, N=1024): `kv_exact_history_bytes` = 67,108,864 B (**exactly 64 MiB**, **0 % error** against the model), `kv_rollback_reserve_bytes` = 4,194,304 B (one 4 MiB page), and `runtime_reservation_bytes` grows by exactly **68 MiB**.

## 3. New interface

| Entry point | Option | Meaning | Default |
|---|---|---|---|
| `ninfer` / `ninfer-serve` | `--kv-tail-tokens N` | keep the newest `N` tokens of each sequence unquantized; `0` disables | `0` |
| `ninfer` / `ninfer-serve` | `--kv-tail-type bf16\|f16` | element type of the exact ring | `f16` |
| `ninfer-perplexity` | `--score-width W` | score in width-`W` attention query tiles; only `W ≤ 8` drives the small-T route and makes the tail's on/off difference visible | `1024` |
| `ninfer-perplexity` | `--save-topk <path>` / `--kld-base <path>` | persist this run's per-target next-token distribution as a KLD reference / load one and report KLD (top-K channel, matching llama.cpp) | — |

The engine identity tag gained a tail dimension (otherwise differently-tailed configurations would share one engine identity), and `config-calculator.html` plus startup capacity planning carry the tail term per concurrency.

## 4. Verification results

Full report: **[PORT-VERIFY-REPORT.en.md](PORT-VERIFY-REPORT.en.md)** ([中文](PORT-VERIFY-REPORT.zh.md)). The rule the campaign ran under: **a claim is only as good as a command that reproduces it**.

- Hardware: RTX 5070 Ti 16 GB, `sm_120a`, CUDA 13.3, idle baseline 48 MiB
- Model: `Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer` (10.33 GiB)
- Compared: the pre-port shipped engine (`ninfer-package`, serve only) vs the ported `build-port` engine
- Discipline: GPU strictly serial, single-owner; after every chunk `nvidia-smi` must be back at 48 MiB

### 4.1 Arm A — no negative impact (pre-port vs post-port)

| Configuration | Cells | Identity |
|---|:--:|---|
| MTP off × vision {off,on} × ctx {8192,32768} | 32 | **32/32 IDENTICAL** |
| MTP on `--draft-tokens 2` × vision {off,on} × ctx {8192,32768} | 32 | **32/32 IDENTICAL** |
| **Total (eight storages)** | **64** | **64/64 IDENTICAL, 0 DIFFERS, 0 MISSING** |

| Metric | Result |
|---|---|
| **Memory** | **identical pre/post in 64/64 cells** → PASS exactly |
| **decode Δ%** | median **−0.18**, spread −0.71…+0.15, **0/64 over 2 %** → PASS |
| prefill Δ% | median −1.25, spread −28.1…+20.4 → **not discriminative** (the same-binary repeat spread exceeds the pre/post spread; see report §3.2) |

Extra observations: the MTP-on stream is **the same bytes as MTP-off** (the greedy verifier is exact); the vision cells genuinely carry the image (prompt 1,214 vs 188 tokens) and are still byte-identical; and same-binary repeats are 16/16 byte-identical, i.e. greedy decoding is self-stable across processes.

### 4.2 Arm B — tail benefit (decode-width KLD, W=8, 32,767 scored tokens)

Reference: bf16-tail0 top-K 100 per ctx. **Gain ratios (tail0 / tailN on mean KLD):**

| ctx | tail | int8 | rk8v4 | rk4v4 | rk4v4-e8 |
|---|---:|---:|---:|---:|---:|
| 8192 | 1024 | 1.09× | 2.20× | 2.47× | 3.25× |
| 8192 | 2048 | 1.09× | 2.30× | 2.87× | 4.04× |
| 8192 | 4096 | 1.05× | 2.41× | 3.14× | 5.01× |
| 32768 | 1024 | 1.02× | 2.04× | 2.26× | 2.97× |
| 32768 | 2048 | 1.03× | 2.18× | 2.53× | 3.70× |
| 32768 | 4096 | 1.05× | 2.27× | 2.87× | 4.46× |

| # | Sub-criterion | Result |
|---|---|---|
| B1 | `mean KLD(tailN) ≤ mean KLD(tail0)` in every cell | **PASS — 0 violations / 24** |
| — | `same_top(tailN) ≥ same_top(tail0) − 0.002` | **PASS — 0 violations / 24** (it rises in every cell) |
| B2 | `ppl(tailN) ≤ ppl(tail0)` | **22/24** — 2 exceptions (int8/ctx8192: t2048 +0.050 %, t4096 +0.002 %, noise-scale) |
| B3 | gain monotone by body coarseness `int8 ≤ rk8v4 ≤ rk4v4 ≤ rk4v4-e8` | **PASS at all 6 (ctx × N) combinations** |

The tail also cuts the worst-case error hard: `rk4v4-e8` at ctx 8192 has KLD max **3.38 → 0.92 / 0.33 / 0.74**.

### 4.3 Arm B3 — MTP acceptance under the tail (post-port only)

| Regime | Result |
|---|---|
| **Tail off** | speculation is **untouched by the port**: byte-identical to the pre-port product *and* to the MTP-off output |
| Whole window, short prompts (4× sample) | `rk8v4` inside ±2 pt and slightly positive (+0.7/+0.8 pt); the coarsest `rk4v4-e8` settles at **−2.05/−1.92 pt** (the ±2 pt boundary, at the sample's resolution limit) |
| **Long prompts (the production body+tail regime)** | 6 of 8 cells positive, **+2 to +7 pt** (the two negatives are small: −1.31, −1.84 pt) |

Mechanism: the draft cache `mtp_kv` is **tail-free by construction**, so `--kv-tail-tokens` sharpens only the *verifier*; agreement can move either way and the movement scales with body coarseness — exactly the measured shape.

### 4.4 A4 — tail-off identity and the FP32 oracle

`run-oracle.bat` → `softmax_attention: PASS`, `ORACLE_EXIT=0` (94 s). All guards present: `fused-append empty-body cache write` ×4, `fused-append crossing build` ×4, `prompt-route ring write` ×8, `fused-append chunked ring write` ×2, `PATHPT` ×2, `TAILGAIN` ×12, `WIDETAIL` ×8, `graph family=` ×8. `tail = 0` bit parity and graph-family stability are asserted inside the same run.

### 4.5 Verdict

| Claim | Result |
|---|---|
| **C1a** the port did not change generation (MTP off and on) | **PASS** — 64/64 byte-identical |
| **C1b** no performance / memory regression | **memory PASS exactly**; **decode PASS**; prefill **not discriminative** |
| **C1c** vision output unchanged | **PASS** — every `--vision` cell byte-identical |
| **C2** the tail gives a substantial quality benefit | **PASS** — decode-width KLD improves in 24/24, monotone by body coarseness |
| **C3** speculation still works, not hurt by the tail | **PASS** — untouched with the tail off; positive in the long-context regime |
| **A4** tail-off is the pre-tail path | **PASS** — `ORACLE_EXIT=0` |

## 5. The ring-write defects found and fixed

The benefit was **unmeasurable at first**; the root cause was not the merge but the **write side of the ring** — two real defects plus a capacity guard, all of the "only visible once it runs" kind:

1. **The fused-append split never quantized the tail rows when `body_window == 0`** (`56fc8384`). While the exact tail covered the whole window the partition set `body_active = 0`, so every split returned before its fused-append block and those rows **never reached the quantized cache**; a later `window > N` body then read a permanent hole.
2. **The `Prompt` route never wrote the ring** (`b99ba8d5`). The prompt route only *writes*, but it never wrote the shadow ring, so a following small-T step could not read it.
3. **A ring-write capacity guard** (`205ea411`): the exact-tail ring write is kept within its capacity.

After the fix: a `bf16` body+tail against a `bf16` reference moves mean KLD **0.13284577 → 0.00093566 (−142×)**, max 16.29 → 0.475, `same_top` 0.9004 → 0.9867; the `rk8v4` cell of the same protocol is **bit-identical** pre/post fix, and the `bf16` tail-off control is still KLD 0 / `same_top` 1.0 — i.e. the fix **only affects tail-on** behaviour. New guard tests: `run_fused_empty_body_append_case`, `run_fused_crossing_case`, `run_prompt_ring_write_case`, `run_fused_chunked_ring_case`.

## 6. Milestones and work packages

| Milestone | Status | Content |
|---|---|---|
| **M0** KVarN decision | DONE | Same-model comparison (`llamacpp` vs `ninfer-package`, IQ3_XXS): on this slice KVarN shows no measurable ppl penalty vs f16/q8_0 and no quality-driven reason to borrow it; the test baseline is pinned to `sm_120a / 5070 Ti` |
| **M1** static BF16 tail, functional closure | DONE | FP32 oracle green for BF16 + the INT8 family (fused/cached) and the batched masked case; memory matches the model at **0 % error** |
| **M2** F16 default + graph stability | DONE | the ring element type becomes a config dimension (`--kv-tail-type`); F16 is never worse than BF16 across every `WIDETAIL` case, so it is **the default** |
| **M3** concurrency and speculation | DONE | `payload_bytes == round_up(N,64)*65,536*C + C*4 MiB` exactly for C=1..8; the draft cache is tail-free by construction; end-to-end re-test with and without speculation shows no regression |
| **M4** optional host/disk tier | out of scope | — |
| **M5** tail benefit + MTP impact | DONE | built the instrument (`--score-width` + KLD), found and fixed the two §5 defects, and the benefit holds across the full matrix |

Work packages WP1–WP7, WP9 and WP10 are all DONE; **WP8 (transaction/rollback) is deliberately deferred** (M1's functional closure is device-only and C=1, so no failure path is reachable), and its rollback reserve `R` is already carried in the WP5 sizing.

## 7. Usage

```bash
# serve / CLI with the tail on (f16 ring by default, N=1024)
ninfer-serve models/qwen3_8_27b.ninfer --kv-dtype rk8v4 --kv-tail-tokens 1024
ninfer       models/qwen3_8_27b.ninfer --kv-dtype rk4v4-e8 --kv-tail-tokens 1024 --spec mtp --draft-tokens 2

# offline: decode-width scoring + KLD (the top-K channel that matches llama.cpp)
ninfer-perplexity models/qwen3_8_27b.ninfer --kv-dtype bf16     --kv-tail-tokens 0    --save-topk out/bf16-t0.topk
ninfer-perplexity models/qwen3_8_27b.ninfer --kv-dtype rk4v4-e8 --kv-tail-tokens 1024 --kld-base out/bf16-t0.topk --score-width 8
```

The commands that reproduce the whole campaign are in [PORT-VERIFY-REPORT.en.md §8](PORT-VERIFY-REPORT.en.md).

## 8. Honest gaps

- **B2 ppl**: 2 of 24 cells fail (`int8`/ctx 8192, +0.050 %/+0.002 %), noise-scale — and the same two cells improve on the KLD and `same_top` instruments. `int8` has the smallest tail gain, so its ppl movement is below a 32,767-token perplexity's resolution.
- **B3's short-prompt ±2 pt gate** is at the sample's resolution limit; a single 256-token run's −5.47 pt did not survive a 4× larger sample (−2.05 pt).
- **B3's ctx axis is vacuous** with a short prompt set — the flag changes the allocation, not the workload; real context lengths need `--messages FILE`.
- **Prefill ≤2 %** cannot be evaluated with one request per cell at these prompt lengths; reported as not-discriminative. Decode (the steady-state figure) is stable.
- **64K context** remains out of scope: the only local corpus slice is ~32.7K tokens.
- **Numeric floor**: a `bf16` body + tail rests on a ~1e-3 mean-KLD floor (one bf16 ULP) — which is why `bf16` is the Arm B reference and not a candidate.
- **Performance characterisation is unfinished**: a body+tail dual-write sweep of `kv_cache_append_bench` and a longer decode benchmark remain outstanding from plan §5; the −6 % in §4.3 is a 24-token indicative figure.
- **`fp8` / `nvfp4` / `k8v4`** tails are inert by design; DoD §7.3 is narrowed to the tail-capable storages.

## 9. Change size and documentation index

Against direct upstream `b06908ba`: **116 commits**, **69 files**, **+10,920 / −191** lines.

Core additions/changes:

| Area | Files |
|---|---|
| exact-ring element genericity | `src/ops/common/kv_tail_element.cuh` |
| tail partial kernel | `src/ops/softmax_attention/dense/causal_cache/small_t_tail.cuh` |
| fused dual write (shadow) | `.../causal_cache/small_t_tail_shadow.cuh` |
| small-T family splits/merge and ring writes | `.../causal_cache/small_t*.cuh`, `prompt.cu`, `small_t.cu` |
| config chain | `apps/cli/{main,options}.{cpp,h}`, `src/serve/serve_options.{cpp,h}`, `include/ninfer/types.h`, `src/runtime/engine/` |
| scoring instrument | `apps/perplexity/{main,evaluation}.{cpp,h}`, `include/ninfer/ops/target_logprobs.h`, `src/ops/kernel/target_logprobs.cuh` |
| capacity / memory | `src/core/paged_kv_cache.{cpp,h}`, `src/models/qwen3_5/program/planning/startup.{cpp,h}` |
| tests | `tests/models/qwen3_5/test_exact_tail_capacity.cpp` (new), `tests/ops/softmax_attention/causal_cache.cpp`, `tests/test_perplexity_evaluation.cpp` |

Documentation index:

| Document | Contents |
|---|---|
| [PORT-VERIFY-REPORT.en.md](PORT-VERIFY-REPORT.en.md) / [.zh.md](PORT-VERIFY-REPORT.zh.md) | the **acceptance report** (the full §4 above plus reproduction commands) |
| [PORT-DOD.md](PORT-DOD.md) | DoD / milestone / work-package audit table (with the V0–V7 acceptance rows) |
| [PORT-JOURNAL.md](PORT-JOURNAL.md) | the run log in order, with commands and observed values |
| [PORT-MEMORY.md](PORT-MEMORY.md) | durable lessons and traps (harness, kill semantics, readiness gate, …) |
| [PORT-M5-PLAN.md](PORT-M5-PLAN.md) / [PORT-REVIEW-PLAN.md](PORT-REVIEW-PLAN.md) / [PORT-VERIFY-PLAN.md](PORT-VERIFY-PLAN.md) | the M5 benefit campaign, the independent code review, the acceptance plan |
| [PORT-BEELLAMA-SPEC.md](PORT-BEELLAMA-SPEC.md) | the port's algorithm reference (port the algorithm, never the code) |
| [precision-tail-port-plan.md](precision-tail-port-plan.md) | the implementation plan (design + WBS + milestones + verification matrix) |
| [kvarn-kv-tail-feasibility-report.md](kvarn-kv-tail-feasibility-report.md) | the feasibility report |
| [docs/performance.md](docs/performance.md) §"KV precision tail" | the published measurements (memory / ppl / F16-vs-BF16 / decode-width KLD) |

## 10. Quick start, downloads and licence

Product installation, Windows deployment, flags and model conversion are in the **[RTX 5070 Ti Windows guide](docs/rtx-5070ti-windows.en.md)**; the prebuilt engine and matching `.ninfer` model products are in the **[download instructions](docs/rtx-5070ti-windows-downloads.md)** (**[Quark Drive](https://pan.quark.cn/s/28b896c4b0c0)**). For building, see [AGENTS.md](AGENTS.md) and the [build system](docs/maintainer/build-system.md).

This repository's Windows / RTX 5070 Ti build, memory policies and manager come from the direct upstream [Ryan-gsq](https://github.com/Ryan-gsq/ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco); the upstream consolidation is [iamwavecut/ninfer-all](https://github.com/iamwavecut/ninfer-all) and the original engine is [Neroued/ninfer](https://github.com/Neroued/ninfer), with every change keeping its author's credit ([maintainer map](docs/maintainer/consolidated-line.md)). The precision-tail algorithm was ported from beellama.cpp's KVCPT (**port the algorithm, never the code**); see [PORT-BEELLAMA-SPEC.md](PORT-BEELLAMA-SPEC.md).

Licence: [LICENSE](LICENSE).
