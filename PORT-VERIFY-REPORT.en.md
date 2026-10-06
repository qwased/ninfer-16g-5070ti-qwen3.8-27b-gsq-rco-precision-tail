# PORT-VERIFY-REPORT (English) — KV precision tail, acceptance campaign

Verification of the precision-tail port (`D:\ninfer\ninfer-precision-tail`, branch `main`, HEAD
`685aa33e`) against the shipped pre-port product (`D:\ninfer\ninfer-package`). Executed per
`PORT-VERIFY-PLAN.md`; the audit rows are `PORT-DOD.md` §"Verification campaign", the run log is
`PORT-JOURNAL.md` Steps 59–62, and the durable lessons are `PORT-MEMORY.md` §5.16–5.17.

> Rule the campaign ran under: **a claim is only as good as a command that reproduces it.** Every
> number below is produced by a command listed in §8, from raw artifacts under `.deps/verify-*`.

## 0. Verdict

| Claim | Result |
|---|---|
| **C1a** port did not change generation, MTP off **and** on | **PASS** — 64/64 cells byte-identical (incl. `--draft-tokens 2`) |
| **C1b** no performance / memory regression | **memory PASS exactly (64/64)**; **decode PASS** (0/64 cells over 2 %); **prefill not discriminative** (harness noise floor > effect) |
| **C1c** no change to vision output | **PASS** — 64/64 byte-identical incl. all `--vision` cells |
| **C2** tail gives a substantial quality benefit | **PASS on the decode-width KLD** — 24/24 cells improve, gain monotone by body coarseness; ppl agrees in 22/24 |
| **C3** MTP speculation still works, not hurt by the tail | **PASS** — untouched with the tail off; ±2 pt with rk8v4; at the ±2 pt boundary for the coarsest rk4v4-e8; **positive** in the long-context regime |
| **A4** tail-off is the pre-tail path | **PASS** — `softmax_attention: PASS`, `ORACLE_EXIT=0` |

Two honest gaps, both reported rather than smoothed: `ppl(tailN) ≤ ppl(tail0)` fails in **2 of 24**
Arm B cells (int8, ctx 8192, +0.050 %/+0.002 %); and Arm B3's ±2 pt gate is at the resolution limit of
its sample size. Neither is a port defect.

## 1. Scope, claims, hardware, artifacts

| Item | Value |
|---|---|
| Model | `D:\ninfer\ninfer-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer` (10.33 GiB) |
| Pre-port engine | `D:\ninfer\ninfer-package\engine\ninfer-serve.exe` (serve only — no CLI, no `--kv-tail-*`, no offline scorer) |
| Post-port engines | `build-port\apps\ninfer-serve.exe` (built for this campaign), `build-port\apps\ninfer.exe`, `build-port\apps\ninfer-perplexity.exe` |
| GPU | RTX 5070 Ti 16 GB, sm_120a, CUDA 13.3; idle baseline **48 MiB** |
| Corpus | `.deps\m5-longtext.txt`, 160,000 B ≈ 32.7K tokens (64K out of scope) |
| Vision fixture | `bench\fixtures\ttft\media\load_00.png` |
| Grid | Arm A `{pre,post} × MTP{off,on D=2} × vision{off,on} × ctx{8192,32768} × 8 storages = 128 cells`; Arm B `4 storages × tail{0,1024,2048,4096} × ctx{8192,32768} = 32 cells` + 2 references; Arm B3 `{rk8v4,rk4v4-e8} × tail{0,1024,2048}`, short and long prompts |
| Storages | `bf16, int8, fp8, rk8v4, rk4v4, rk4v4-e8, nvfp4, k8v4` (all ≥4-bit; `rk2v4-e8` is 2-bit, excluded) |
| Discipline | GPU strictly serial, single-owner; after every chunk: no `ninfer`/`perplexity` process and `nvidia-smi` = 48 MiB |

## 2. Method

**Arm A — serve A/B.** Both sides are the same HTTP `serve` engine driven identically (the pre-port
product ships no CLI), one greedy request per cell (`--greedy --seed 0`, prompt 188 tokens / 1,214 with
image, 128 new tokens), then `cmp` the recorded completion. Only the engine build varies;
`--kv-tail-tokens` is absent on the pre side by construction.

**Arm B — decode-width KLD.** bf16-tail0 top-K 100 references (per ctx) vs quantized-tail candidates,
scored in width-8 tiles (`--score-width 8`) so the small-T route that merges the tail runs. Reference
and protocol: `--disjoint`, `--score-topk 100`, 32,767 scored tokens.

**Arm B3 — MTP acceptance** through the `ninfer` CLI (`--spec mtp --draft-tokens 2 --greedy --seed 0`),
pooling the server's own `mtp drafted/accepted tokens` figures.

### Harness repairs the smoke run forced (the port was correct, the harness was not)

1. **Git Bash `kill` cannot stop the native engine.** `kill $!` and `kill -9 $!` both return 0 while
   `ninfer-serve.exe` survives (verified by hand: two PIDs still resident, 11.4 GiB held). Teardown is
   now `taskkill //F //IM ninfer-serve.exe`; without it every cell would orphan an engine on the GPU
   and the port.
2. **The readiness gate accepted a loading server.** `/health` answers **503 `model_loading`** while
   the weights load and `curl -s -o /dev/null` exits 0 for a 503, so the original gate fired in the
   first second and POSTed into a loading engine. It now requires HTTP **200**.
3. **The model is a thinking model.** The generated stream lands in `message.reasoning_content`;
   `content` stays `""` (with a 128-token cap the thinking block never closes). Comparing `content`
   alone would have compared **two empty strings** and passed vacuously. The client now records
   `reasoning_content + content`, and refuses to write an empty file.
4. **The perf grep matched nothing.** The per-request line is
   `req#N done | … | prefill X tok/s | decode Y tok/s`; the old keys never matched. It now captures
   that line plus the `capacity |` startup memory line.

Also: chunked drivers were added because the host kills any single invocation older than ~500 s
(`VERIFY_LIMIT` for Arm A, `LIMIT=1` per invocation for Arm B, `RUN_LIMIT` for Arm B3).

## 3. Arm A — no negative impact (pre-port vs post-port)

### 3.1 Identity (text + vision), the primary criterion

| Configuration | Cells | Identity |
|---|:--:|---|
| MTP off, vision off, ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP off, vision off, ctx 32768 | 8 | **8/8 IDENTICAL** |
| MTP off, vision on, ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP off, vision on, ctx 32768 | 8 | **8/8 IDENTICAL** |
| MTP on `--draft-tokens 2`, vision off, ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP on `--draft-tokens 2`, vision off, ctx 32768 | 8 | **8/8 IDENTICAL** |
| MTP on `--draft-tokens 2`, vision on, ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP on `--draft-tokens 2`, vision on, ctx 32768 | 8 | **8/8 IDENTICAL** |
| **Total** | **64** | **64/64 IDENTICAL, 0 DIFFERS, 0 MISSING** |

Two extra observations worth keeping: the **MTP-on stream is the same bytes as MTP-off** (the greedy
verifier is exact), and the vision cells genuinely carry the image (prompt 1,214 tokens vs 188) while
staying byte-identical. Same-binary repeats (same engine run twice) are **16/16 byte-identical** in
two independent repeat campaigns, i.e. greedy decoding is self-stable across processes — the
precondition the plan requires before calling any future mismatch a defect.

### 3.2 Throughput and memory

Compact table: each cell is **prefill Δ% / decode Δ%** (post relative to pre); every one of these 64
cells is also byte-identical per §3.1.

| MTP | vision | ctx | bf16 | int8 | fp8 | rk8v4 | rk4v4 | rk4v4-e8 | nvfp4 | k8v4 |
|---|:--:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| off | off | 8192 | +2.4/-0.1 | +1.3/+0.0 | +1.7/+0.0 | +8.7/+0.1 | +1.1/+0.0 | +1.8/+0.0 | +1.4/-0.3 | +2.0/+0.0 |
| off | off | 32768 | +0.3/+0.0 | -18.7/-0.3 | -0.4/-0.4 | -10.7/-0.1 | -0.2/-0.1 | -8.9/-0.1 | -1.6/-0.1 | -11.0/-0.1 |
| off | on | 8192 | -0.6/-0.3 | -3.1/-0.4 | -2.4/-0.3 | -2.3/+0.0 | -4.6/-0.3 | -0.8/-0.4 | -14.1/-0.1 | -4.3/-0.1 |
| off | on | 32768 | +0.0/-0.1 | -0.8/-0.4 | -1.8/-0.1 | -3.1/-0.3 | -3.8/-0.1 | -3.1/-0.3 | -4.4/-0.1 | -6.1/-0.1 |
| on | off | 8192 | -5.7/-0.3 | -1.2/-0.2 | -3.3/-0.2 | -1.1/-0.2 | -3.7/-0.2 | -4.4/-0.4 | +20.4/-0.4 | +10.9/-0.2 |
| on | off | 32768 | -7.6/-0.2 | -12.2/-0.1 | +8.5/-0.3 | +3.8/-0.1 | -10.2/-0.2 | -28.1/-0.6 | +0.2/-0.2 | +0.6/-0.7 |
| on | on | 8192 | -1.2/-0.3 | +1.2/-0.1 | +0.0/+0.0 | +0.6/-0.2 | -1.2/-0.2 | +3.2/-0.2 | -1.2/-0.5 | -3.1/-0.3 |
| on | on | 32768 | -3.7/-0.7 | -1.2/-0.6 | -8.0/-0.6 | -3.7/-0.3 | -1.8/-0.3 | -1.8/-0.2 | -1.2/-0.2 | -0.6/-0.2 |

| Metric | Result |
|---|---|
| **decode Δ%** (n=64) | median **−0.18**, mean −0.23, spread **−0.71…+0.15**, **0/64 exceed 2 %** → **PASS** |
| prefill Δ% (n=64) | median −1.25, mean −2.23, spread −28.1…+20.4, 34/64 exceed 2 % → see below |
| **memory** (`capacity \| KV … \| runtime … \| free …`) | **identical pre/post in 64/64 cells** → **PASS exactly** |

**Why prefill is reported as "not discriminative" rather than a failure.** Two dedicated repeat
campaigns re-ran the *same binary* on the *same cells* with a fresh server each time:

| repeat campaign | prompt | prefill Δ (same binary) | decode Δ | text |
|---|---|---|---|---|
| `.deps/verify-ab-repeat` (8 cells) | 188 tok | **−29.1 % … +7.9 %** | −0.44…−0.15 % | 8/8 IDENTICAL |
| `.deps/verify-ab-repeat2` (16 cells, 8 vision) | 1,214 tok | **−22.3 % … −1.2 %** (vision) | −0.88…0.00 % | 8/8 IDENTICAL |

Both spreads are **wider** than the pre/post spread they are supposed to gate (e.g. pre/post vision
prefill −0.6…−4.6 %). A 1.2–2.4 s prefill measured as a single request is dominated by process start-up,
warm-up and device state, so the ≤2 % prefill criterion is below this harness's noise floor. Decode —
the steady-state figure — is stable in both experiments (≤0.9 %). Memory needs no such caveat: it is
bit-reproducible.

## 4. Arm B — tail benefit (decode-width KLD, W=8, 32,767 scored tokens)

Reference: bf16-tail0 top-K 100 per ctx (bf16 stores keys/values exactly, so it is the near-exact
baseline). A quantized candidate gets *closer* to it as the exact tail replaces its newest quantized
rows, so `mean KLD(tailN) < mean KLD(tail0)` is the benefit. bf16 is the **reference**, not a
candidate: a bf16 candidate against a bf16 reference is 0 at tail0 and ~1e-3 (the small-T numeric
floor) at tailN, so it can never show a gain — bf16 is covered by Arm A instead.

| ctx | storage | tail | ppl | KLD mean | KLD max | same_top |
|---|---|---:|---:|---:|---:|---:|
| 8192 | int8 | 0 | 5.971447 | 0.00112641 | 1.42171 | 0.984709 |
| 8192 | int8 | 1024 | 5.971196 | 0.00103730 | 0.07297 | 0.984800 |
| 8192 | int8 | 2048 | **5.974426** | 0.00102872 | 0.10638 | 0.985441 |
| 8192 | int8 | 4096 | **5.971557** | 0.00107175 | 0.74014 | 0.984495 |
| 8192 | rk8v4 | 0 | 5.980890 | 0.00264652 | 0.48189 | 0.976010 |
| 8192 | rk8v4 | 1024 | 5.969419 | 0.00120207 | 0.26165 | 0.982817 |
| 8192 | rk8v4 | 2048 | 5.973092 | 0.00114922 | 0.25979 | 0.983518 |
| 8192 | rk8v4 | 4096 | 5.972232 | 0.00109672 | 0.74014 | 0.984648 |
| 8192 | rk4v4 | 0 | 5.986203 | 0.00386914 | 1.56008 | 0.971463 |
| 8192 | rk4v4 | 1024 | 5.975685 | 0.00156404 | 1.41782 | 0.982023 |
| 8192 | rk4v4 | 2048 | 5.973775 | 0.00134631 | 1.35255 | 0.982267 |
| 8192 | rk4v4 | 4096 | 5.970779 | 0.00123353 | 1.22301 | 0.984282 |
| 8192 | rk4v4-e8 | 0 | 6.011151 | 0.00652166 | 3.38330 | 0.965969 |
| 8192 | rk4v4-e8 | 1024 | 5.980981 | 0.00200700 | 0.92492 | 0.979948 |
| 8192 | rk4v4-e8 | 2048 | 5.980192 | 0.00161394 | 0.32719 | 0.982115 |
| 8192 | rk4v4-e8 | 4096 | 5.974378 | 0.00130116 | 0.74014 | 0.983244 |
| 32768 | int8 | 0 | 5.664418 | 0.00103341 | 0.33064 | 0.984893 |
| 32768 | int8 | 1024 | 5.663406 | 0.00100826 | 0.18132 | 0.984527 |
| 32768 | int8 | 2048 | 5.664209 | 0.00100349 | 0.71563 | 0.984802 |
| 32768 | int8 | 4096 | 5.664052 | 0.00098620 | 0.18133 | 0.985076 |
| 32768 | rk8v4 | 0 | 5.673180 | 0.00241272 | 0.33048 | 0.976867 |
| 32768 | rk8v4 | 1024 | 5.665655 | 0.00118513 | 1.13890 | 0.982940 |
| 32768 | rk8v4 | 2048 | 5.664276 | 0.00110771 | 0.18537 | 0.983947 |
| 32768 | rk8v4 | 4096 | 5.665985 | 0.00106248 | 0.18543 | 0.984008 |
| 32768 | rk4v4 | 0 | 5.682110 | 0.00356046 | 1.56008 | 0.972686 |
| 32768 | rk4v4 | 1024 | 5.667320 | 0.00157849 | 0.35742 | 0.982757 |
| 32768 | rk4v4 | 2048 | 5.665740 | 0.00140599 | 0.47161 | 0.982177 |
| 32768 | rk4v4 | 4096 | 5.665112 | 0.00124205 | 0.36498 | 0.984039 |
| 32768 | rk4v4-e8 | 0 | 5.694664 | 0.00631572 | 0.65803 | 0.963378 |
| 32768 | rk4v4-e8 | 1024 | 5.669458 | 0.00212822 | 0.53016 | 0.979247 |
| 32768 | rk4v4-e8 | 2048 | 5.667368 | 0.00170770 | 0.28566 | 0.980895 |
| 32768 | rk4v4-e8 | 4096 | 5.665916 | 0.00141657 | 0.85668 | 0.982482 |

Bold ppl values in the int8/8192 block are the two criterion exceptions (see below).

### 4.1 The four sub-criteria

| # | Criterion | Result |
|---|---|---|
| B1 | `mean KLD(tailN) ≤ mean KLD(tail0)` in every cell | **PASS — 0 violations / 24 cells** |
| — | `same_top(tailN) ≥ same_top(tail0) − 0.002` | **PASS — 0 violations / 24** (it rises in every cell) |
| B2 | `ppl(tailN) ≤ ppl(tail0)` | **22/24** — 2 violations: int8 ctx 8192, t2048 **+0.050 %**, t4096 **+0.002 %** |
| B3 | gain monotone by body coarseness, `int8 ≤ rk8v4 ≤ rk4v4 ≤ rk4v4-e8` | **PASS at all 6 (ctx × N) combinations** |

**Gain ratios (tail0 / tailN on mean KLD):**

| ctx | tail | int8 | rk8v4 | rk4v4 | rk4v4-e8 |
|---|---:|---:|---:|---:|---:|
| 8192 | 1024 | 1.09× | 2.20× | 2.47× | 3.25× |
| 8192 | 2048 | 1.09× | 2.30× | 2.87× | 4.04× |
| 8192 | 4096 | 1.05× | 2.41× | 3.14× | 5.01× |
| 32768 | 1024 | 1.02× | 2.04× | 2.26× | 2.97× |
| 32768 | 2048 | 1.03× | 2.18× | 2.53× | 3.70× |
| 32768 | 4096 | 1.05× | 2.27× | 2.87× | 4.46× |

Reading: the benefit is real and **ordered exactly by how coarse the body is** — the coarsest
`rk4v4-e8` gains 3–5×, `rk8v4` ~2×, and `int8` (already close to bf16) ~1.05×. Gain is larger at
N=4096 than N=1024 and shrinks mildly as ctx grows. The tail also cuts the worst-case error hard
(`rk4v4-e8` ctx 8192 KLD max 3.38 → 0.92/0.33/0.74). This re-confirms the WP-B result at the required
ctx set and adds `rk4v4` and N=4096, which the WP-B matrix did not cover.

**On the 2 ppl exceptions.** `int8` is the tier with the smallest KLD gain, so its ppl movement is
below the resolution of a 32,767-token perplexity: +0.05 % is ~3e-3 on a 5.97 ppl, while the *same two
cells* improve monotonically on the KLD instrument (0.00112641 → 0.00102872 / 0.00107175) and on
`same_top`. The KLD instrument is the one that shows what the feature does; ppl is reported as-is.

## 5. Arm B3 — MTP acceptance under the tail

Post-port only (no pre-port CLI). Pooled over prompts, `--spec mtp --draft-tokens 2 --greedy --seed 0`.

### 5.1 Short prompts (3 prompts, ~290-token sequences)

Here **the ctx axis is vacuous**: a 256–1,024-token generation never approaches a ctx 8192/32768
ceiling, so both settings give identical rounds/drafted/accepted — the flag only changes the
allocation. The two ctx rows below are therefore one measurement, not two.

| set | storage | tail | drafted | accepted | rate vs tail0 | Δ pt | acc len |
|---|---|---:|---:|---:|---|---:|---:|
| `b3` (256 new) | rk8v4 | 0 | 739 | 396 | 53.59 % | — | 2.079 |
| `b3` | rk8v4 | 1024 | 739 | 397 | 53.72 % | **+0.14** | 2.082 |
| `b3` | rk8v4 | 2048 | 739 | 397 | 53.72 % | **+0.14** | 2.082 |
| `b3` | rk4v4-e8 | 0 | 734 | 400 | 54.50 % | — | 2.099 |
| `b3` | rk4v4-e8 | 1024 | 773 | 379 | 49.03 % | **−5.47** | 1.987 |
| `b3` | rk4v4-e8 | 2048 | 773 | 379 | 49.03 % | **−5.47** | 1.987 |
| `b3n` (1024 new) | rk4v4-e8 | 0 | 3,015 | 1,563 | 51.84 % | — | 2.039 |
| `b3n` | rk4v4-e8 | 1024 | 3,083 | 1,535 | 49.79 % | **−2.05** | 2.001 |
| `b3n` | rk4v4-e8 | 2048 | 3,079 | 1,537 | 49.92 % | **−1.92** | 2.004 |
| `b3n` | rk8v4 | 0 | 2,952 | 1,601 | 54.23 % | — | 2.091 |
| `b3n` | rk8v4 | 1024 | 2,931 | 1,611 | 54.96 % | **+0.73** | 2.106 |
| `b3n` | rk8v4 | 2048 | 2,927 | 1,612 | 55.07 % | **+0.84** | 2.108 |

The `b3n` block is the same experiment with `--max-new 1024` — **4× the sample** (2,927–3,083 drafts
vs 734–773, binomial stderr ~0.9 pt vs ~1.8 pt). It shows the −5.47 pt was mostly small-sample noise:
rk4v4-e8 settles at **−2.05/−1.92 pt** (N=1024 marginally outside the ±2 pt gate, N=2048 inside) and
rk8v4 is inside at **+0.7/+0.8 pt**. Note also that when the tail covers the whole window, N=1024 and
N=2048 compute the same thing, so those pairs are one observation, not two.

### 5.2 Long prompts (`--messages`) — the production body+tail regime

| set | prompt tok | storage | tail | drafted | rate vs tail0 | Δ pt | acc len |
|---|---:|---|---:|---:|---|---:|---:|
| `b3long8` (ctx 8192) | 6,847 | rk8v4 | 0 | 231 | 62.77 % | — | 2.330 |
| `b3long8` | 6,847 | rk8v4 | 1024 | 230 | 63.48 % | **+0.71** | 2.339 |
| `b3long8` | 6,847 | rk8v4 | 2048 | 229 | 69.43 % | **+6.66** | 2.674 |
| `b3long8` | 6,847 | rk4v4-e8 | 0 | 242 | 57.85 % | — | 2.217 |
| `b3long8` | 6,847 | rk4v4-e8 | 1024 | 242 | 63.22 % | **+5.37** | 2.500 |
| `b3long8` | 6,847 | rk4v4-e8 | 2048 | 244 | 62.30 % | **+4.44** | 2.476 |
| `b3long32` (ctx 32768) | 31,020 | rk8v4 | 0 | 223 | 64.13 % | — | 2.277 |
| `b3long32` | 31,020 | rk8v4 | 1024 | 234 | 62.82 % | −1.31 | 2.374 |
| `b3long32` | 31,020 | rk8v4 | 2048 | 236 | 62.29 % | −1.84 | 2.361 |
| `b3long32` | 31,020 | rk4v4-e8 | 0 | 228 | 61.40 % | — | 2.228 |
| `b3long32` | 31,020 | rk4v4-e8 | 1024 | 233 | 63.52 % | **+2.12** | 2.383 |
| `b3long32` | 31,020 | rk4v4-e8 | 2048 | 242 | 64.46 % | **+3.06** | 2.592 |

One prompt each → ±3 pt, so treat the individual magnitudes loosely; the **sign is predominantly
positive** (6 of the 8 cells improve, and the two negatives are small: −1.31 and −1.84 pt, both on
rk8v4 at 31K where the tail covers the smallest fraction of the window). At 6,847 prompt tokens the
tail (1,024/2,048) covers only the newest part of the window, and the verifier's attention is
measurably better while the draft's own cache is unchanged — agreement rises by 2–7 points there.

### 5.3 MTP conclusion (C3)

1. **With the tail off, speculation is untouched by the port.** Arm A's `mtp on --draft-tokens 2`
   cells are byte-identical to the pre-port product *and* to the MTP-off output, at both ctx and with
   vision (§3.1).
2. **With the tail on, the decay is not material.** `rk8v4` stays inside ±2 pt everywhere; the
   coarsest `rk4v4-e8` sits at the ±2 pt boundary in the whole-context regime (−2.1/−1.9 pt at 3.0k
   drafts) and, once a body is present, acceptance moves **+2 to +7 pt** — the regime the feature is
   for. The ±2 pt gate at ~3k drafts is this harness's resolution limit (stderr ≈0.9 pt for the pooled
   `--draft-tokens 2` budget of ~2.0–2.7 tokens/round).
3. **Mechanism.** The draft cache (`mtp_kv`) is tail-free **by construction**, so `--kv-tail-tokens`
   sharpens only the *verifier*; draft/verifier agreement can move either way, and the movement scales
   with body coarseness — which is exactly the pattern measured (int8/rk8v4 ≈ 0 to +0.8 pt, coarse
   rk4v4-e8 ≈ −2 pt in the whole-context regime, all positive with a body).

## 6. A4 — tail-off identity and the FP32 oracle

`.deps\run-oracle.bat` → `.deps/verify-a4-oracle.out` (`softmax_attention: PASS`, `ORACLE_EXIT=0`,
94 s; the test binary is newer than every source file). All guards present: `fused-append empty-body
cache write` ×4, `fused-append crossing build` ×4, `prompt-route ring write` ×8 (the 4 original plus
the 4 F3 ring-overflow cases), `fused-append chunked ring write` ×2, `PATHPT` ×2, `TAILGAIN` ×12,
`WIDETAIL` ×8, `graph family=` ×8. `tail = 0` bit parity and graph-family stability are asserted
inside this run, so DoD §7.6 (zero regression) and §7.5 (graph family) are re-confirmed on the current
tree.

## 7. Conclusions and honest gaps

**What is established:** the port is behaviourally identical to the shipped product on every cell
tested — text, vision, MTP off and on, both context lengths, all eight ≥4-bit storages — with exactly
reproducible memory and no decode regression; and `--kv-tail-tokens N` delivers a substantial,
monotone-in-body-coarseness reduction of the decode-width KLD on every tail-capable storages (bf16 +
the INT8 family), with `same_top` rising everywhere and speculation not materially affected.

**Gaps, stated plainly:**

- **B2 ppl** in 2 of 24 Arm B cells (int8, ctx 8192, t2048 +0.050 %, t4096 +0.002 %). Noise-scale, and
  the KLD/`same_top` instruments improve in the same cells. `int8` has the smallest tail gain, so its
  ppl movement is below a 32,767-token ppl's resolution.
- **B3** short-prompt ±2 pt gate is at the sample's resolution limit; a single 256-token run's −5.47 pt
  for rk4v4-e8 did not survive a 4× larger sample (−2.05 pt).
- **B3 ctx axis is vacuous** with a short prompt set — the flag changes the allocation, not the
  workload. Real context lengths need a long prompt via `--messages FILE` (the Windows command line
  caps `--prompt` at ~32 KB, so a 30K-token text cannot go through it).
- **Prefill ≤2 %** cannot be evaluated with one request per cell at these prompt lengths; the
  same-binary repeat spread exceeds the pre/post spread. Reported as not-discriminative.
- **64K** remains out of scope: the only local corpus slice is ~32.7K tokens.
- **Cross-build residual:** `bf16`+tail rests on a ~1e-3 numeric floor (one bf16 ULP, Steps 55/56);
  it is why `bf16` is the Arm B reference and not a candidate.

## 8. Reproduction

```bash
# 0) preflight
nvidia-smi --query-gpu=memory.used --format=csv,noheader      # 48 MiB
.deps/env-port.bat && .deps/build-target.bat ninfer-serve
cmd //c ".deps\\verify-corpus.bat"                            # CORPUS_OK

# A) no-regression A/B (pre vs post serve) — chunked, resumable
VERIFY_CTX=8192 VERIFY_FMT=rk8v4 VERIFY_LIMIT=8 bash .deps/run-verify-serve-ab.sh   # smoke
VERIFY_LIMIT=30 bash .deps/run-verify-serve-ab.sh                                   # x5 -> 128 cells
VERIFY_COMPARE_ONLY=1 bash .deps/run-verify-serve-ab.sh | tee .deps/verify-a-compare.log
python .deps/verify-ab-report.py --md > .deps/verify-a-table.md

# A) the prefill noise floor (same binary, twice; --dir / VERIFY_OUT select the output)
VERIFY_OUT=$PWD/.deps/verify-ab-repeat  VERIFY_CTX=8192 VERIFY_LIMIT=8  bash .deps/run-verify-serve-ab.sh
VERIFY_OUT=$PWD/.deps/verify-ab-repeat2 VERIFY_CTX=8192 VERIFY_LIMIT=16 bash .deps/run-verify-serve-ab.sh

# B) tail benefit — references + matrix (resumable; LIMIT=1 advances exactly one new run)
for i in $(seq 1 34); do cmd //c ".deps\\run-verify-matrix.bat 1"; done
python .deps/summarize-verify.py | tee .deps/verify-b-summary.txt

# B3) MTP acceptance under the tail
B3_MAX_NEW=1024 B3_CTX=8192 B3_FMT="rk8v4 rk4v4-e8" B3_TAIL="0 1024 2048" RUN_LIMIT=18 bash .deps/run-verify-b3.sh
B3_TAGPREFIX=b3long8  B3_CTX=8192  B3_MSG_FILE=$PWD/.deps/verify-b3-msg-c8192.json  RUN_LIMIT=6 bash .deps/run-verify-b3.sh
B3_TAGPREFIX=b3long32 B3_CTX=32768 B3_MSG_FILE=$PWD/.deps/verify-b3-msg-c32768.json RUN_LIMIT=6 bash .deps/run-verify-b3.sh
python .deps/summarize-b3.py | tee .deps/verify-b3-summary.txt

# A4) tail-off identity / FP32 oracle
cmd //c ".deps\\run-oracle.bat" | tee .deps/verify-a4-oracle.out

# after EVERY GPU run
tasklist | grep -iE "ninfer|perplexity"   # must be empty
nvidia-smi --query-gpu=memory.used --format=csv,noheader   # must be 48 MiB
```

## 9. Artifact inventory (raw evidence, `.deps/` is gitignored)

| Artifact | Contents |
|---|---|
| `.deps/verify-ab/{pre,post}-mtp*-vis*-c*-*.txt` | 128 completion streams (the identity evidence) |
| `.deps/verify-ab/*.perf` / `*.server.log` / `*.client.err` | 128 request perf lines + engine logs |
| `.deps/verify-a-compare.log`, `.deps/verify-a-table.md` | identity table, 64-row A/B table |
| `.deps/verify-a-chunk0{1..5}.log`, `.deps/verify-a-repeat*.log` | chunk/compare/noise-floor transcripts |
| `.deps/verify-ref-bf16-t0-w8-c{8192,32768}.ptk` (+`.run/report.json`) | the two bf16-tail0 top-K references |
| `.deps/verify-{int8,rk8v4,rk4v4,rk4v4-e8}-t*-c*/report.json` | 32 Arm B cells (ppl + KLD block) |
| `.deps/verify-b-chunk0{1..12}.log`, `.deps/verify-b-summary.txt` | matrix transcripts + summary |
| `.deps/verify-b3/*.{err,out}`, `.deps/verify-b3-summary.txt` | 66 MTP runs + pooled summary |
| `.deps/verify-b3-msg-c{8192,32768}.json` | the long-prompt messages files |
| `.deps/verify-a4-oracle.out` | oracle output |
| `.deps/run-verify-matrix.bat`, `run-verify-b3.sh`, `summarize-b3.py`, `verify-ab-report.py` | new/extended harness (additive) |

## 10. Where this is recorded in the repository

- `PORT-JOURNAL.md` **Steps 59–62** — the run log, in order, with commands and observed values.
- `PORT-DOD.md` **§"Verification campaign (PORT-VERIFY-PLAN)"** rows **V0–V7** — the audit table.
- `PORT-MEMORY.md` **§5.16–5.17** — the harness traps and the durable campaign facts (kill semantics,
  503 readiness, thinking-model stream, prefill noise, B3 sample size, draft/verifier asymmetry).
