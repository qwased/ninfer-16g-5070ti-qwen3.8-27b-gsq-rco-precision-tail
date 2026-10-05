# PORT-VERIFY-PLAN — acceptance: no regression + tail benefit

Handoff/runbook for a verification campaign that must produce **strong, command-reproducible** evidence
for two claims about the precision-tail port, on the shipped 27B artifact, across context lengths and
KV storages. Reuses the existing `.deps/` harness wherever possible (see §7). Companion docs:
`PORT-DOD.md`, `PORT-MEMORY.md`, `PORT-REVIEW-PLAN.md`.

## 1. Claims to evidence

| # | Claim | Arm |
|---|---|---|
| **C1a** | The port did **not** change generation behaviour: identical output to the pre-port product, with **MTP off *and* on** | A1 (text), A2 (vision) |
| **C1b** | The port did not regress performance (prefill/decode tok/s) or memory | A3 |
| **C1c** | The port did not change **vision (image) understanding** output | A2 |
| **C2** | The exact-tail mechanism (`--kv-tail-tokens N`) gives a **substantial quality benefit** | B1/B2 |
| **C3** | MTP speculation still works and is not hurt by the tail | A1, B3 |

The pre-port reference is the **shipped product** `D:\ninfer\ninfer-package` (its `engine\ninfer-serve.exe`).
The post-port reference is the tree built from `D:\ninfer\ninfer-precision-tail` (`main`).

## 2. Artifacts, model, hardware

| Item | Path / value |
|---|---|
| Model (baseline) | `D:\ninfer\ninfer-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer` (10.33 GiB) |
| Pre-port engine | `D:\ninfer\ninfer-package\engine\ninfer-serve.exe` (serve only; **no** CLI, **no** `--kv-tail-*`, **no** offline scorer) |
| Post-port CLI | `build-port\apps\ninfer.exe` (`--messages` multimodal, `--spec mtp`, `--greedy`) |
| Post-port serve | `build-port\apps\ninfer-serve.exe` (**must be built once**: `.deps\build-target.bat ninfer-serve`) |
| Post-port scorer | `build-port\apps\ninfer-perplexity.exe` (KLD/ppl; the tail's decode-width instrument) |
| GPU | RTX 5070 Ti 16 GB, sm_120a, CUDA 13.3; idle baseline **48 MiB** |
| Corpus | `.deps\m5-longtext.txt` — a WikiText slice, **≈32,764 tokens** (enough for 8K and 32K only) |
| Vision fixture | `bench\fixtures\ttft\media\load_00.png` (56 PNGs available) |

## 3. Scope matrix

- **Context**: `8192` (8K) and `32768` (32K). **64K is dropped**: the only local corpus slice is
  ≈32.7K tokens, which a 64K window cannot fill. Re-adding 64K needs a ≥131K-token text (e.g. full
  WikiText-2 test, ≈245K tokens, fetchable via `beellama.cpp\scripts\get-wikitext-2.sh`) — out of
  scope unless a longer corpus is supplied.
- **Storages** (`--kv-dtype`, **≥4-bit only**; `rk2v4-e8` is 2-bit → excluded):
  `bf16, int8, fp8, rk8v4, rk4v4, rk4v4-e8, nvfp4, k8v4`.
  - **Tail-capable** (the tail actually merges): `bf16, int8, rk8v4, rk4v4, rk4v4-e8`
    (`tail_merge_wired`, `causal_cache.cpp:1621-1623`; int8 family per `d256_profile.h:71-77`). In
    Arm B, `bf16` is the **reference** and the benefit **candidates** are the four quantized ones
    (int8, rk8v4, rk4v4, rk4v4-e8); bf16 itself is covered by Arm A (see §5).
  - **Tail-inert by design**: `fp8, nvfp4, k8v4` (rotate-frame; no tail code) — appear in Arm A only.
- **Tail lengths** `N`: `0, 1024, 2048, 4096`.
- **MTP**: off; on with **`--draft-tokens 2`** (per requirement).
- **Vision**: off; on (`--vision`) with one image.
- **Concurrency**: `C=1` (single-owner; the tail's C>1 capacity is unit-tested separately).

## 4. Arm A — no negative impact (pre-port vs post-port)

Both sides are driven identically so the only variable is the engine build. **Use HTTP `serve` for both**
(the pre-port product ships no CLI), so build the port's `ninfer-serve` first.

| Sub | Input | Grid | Pass criterion |
|---|---|---|---|
| **A1** text consistency | a fixed text prompt (`--messages`/chat body), `--greedy --seed 0` | {pre,post} × {MTP off, MTP on D=2} × ctx {8192,32768} × storages {8 ≥4-bit} | completion text **byte-identical** pre vs post (for each MTP setting) |
| **A2** vision consistency | same + one image part (`load_00.png`, base64 data URI) | same grid, `--vision` on | completion text **byte-identical** pre vs post |
| **A3** performance | the A1/A2 requests; server logs `summary prefill speed` / `decode speed` | same grid | Δprefill and Δdecode **≤ 2 %**; memory figures equal when tail off |
| **A4** (supporting, already have) | tail off is the pre-tail path | oracle + `N=0` KLD references | `softmax_attention: PASS`; tail=0 output bit-identical (`.deps/oracle-*.out`) |

Notes:
- A1/A2 prove **consistency**; a divergence is only a real defect if it persists after ruling out
  cross-build FP-reduction noise (re-run the *same* binary twice; if a single binary is self-stable but
  differs from the other, investigate the specific token).
- The pre-port serve is launched from a copied config with the *same* flags as the post-port side;
  `--kv-tail-tokens` is simply absent on the pre side (it does not exist there).

## 5. Arm B — tail benefit (post-port, tail 0 vs N)

The benefit is only *visible* on the **decode-width route** (`--score-width ≤ 8` selects the small-T
route that merges the tail; the default 1024-wide prefill tiles never read it). Use the existing KLD
instrument.

| Sub | Method | Grid | Pass criterion |
|---|---|---|---|
| **B1** decode-width KLD | per (ctx, storage) save a `bf16`-tail0 top-K reference (`--save-topk`), then score candidates (`--kld-base`) | storages {int8,rk8v4,rk4v4,rk4v4-e8} × tails {0,1024,2048,4096} × ctx {8192,32768}, `W=8` | mean KLD **tailN ≤ tail0** in every cell; gain **monotone by body coarseness** (int8 ≤ rk8v4 ≤ rk4v4 ≤ rk4v4-e8); `same_top` not lower |
| **B2** ppl | `report.json` `overall.perplexity` from the same runs | same | ppl tailN ≤ tail0 |
| **B3** MTP | acceptance % / tokens-per-round, tail0 vs tailN | ctx {8192,32768} × {rk8v4, rk4v4-e8}, `--spec mtp --draft-tokens 2`, a fixed prompt set | tailN acceptance **within noise** of tail0 (not materially lower) |

**Why `bf16` is the reference, not a candidate.** In Arm B the reference is the **bf16 body with the
tail off** — the near-exact baseline (bf16 stores keys/values exactly; only the storage rounding of
the quantized tiers is absent). A quantized candidate gets *closer* to that reference as the exact
tail replaces its newest quantized rows, so `mean KLD(tailN) < mean KLD(tail0)` is the benefit. A
`bf16` *candidate* against a bf16-tail0 reference is KLD exactly 0 at tail0 and ≈1e-3 (the small-T
numeric floor, Step 56) at tailN, so it can never show a positive gain — bf16 is therefore covered by
Arm A (no-regression) and used only as the reference here. `bf16` + tail is a *correctness* property
(no defect), evidenced by the oracle and the Step-56 parity floor, not a benefit.

Baseline expectation from the prior campaign (ctx 8192/16384/32768): the tail-capable storages improve
and the gain is monotone (int8 ≈1.0–1.1× < rk8v4 ≈2.0–2.3× < rk4v4-e8 ≈3.0–4.0×). This plan re-confirms
it at the required ctx set, including `rk4v4` and `bf16` (whole-window) which the WP-B matrix did not.

## 6. Numeric pass thresholds (summary)

- **Consistency (A1/A2)**: exact string equality of the completion; MTP on and off both.
- **Performance (A3)**: |Δ| ≤ 2 % on prefill and decode tok/s; identical `kv cache payload` / peak mem
  at tail off.
- **Benefit (B1/B2)**: `mean KLD(tailN) ≤ mean KLD(tail0)` for every (storage, ctx, N) cell, with the
  coarseness ordering int8 ≤ rk8v4 ≤ rk4v4 ≤ rk4v4-e8 on the gain ratio; `same_top(tailN) ≥
  same_top(tail0) − 0.002`. (bf16 is excluded from this criterion by construction; see §5.)
- **MTP (A1/B3)**: identical stream (A1); acceptance within ±2 points (B3).

## 7. Tool reuse map (nothing new unless marked)

| Need | Existing tool | Reuse |
|---|---|---|
| Save a top-K reference | `.deps\run-m5-save.bat <fmt> <tail> <w> <ctx> <text> <ref>` | **as-is** (ctx is already a parameter) |
| Score + KLD | `.deps\run-m5-kld.bat <fmt> <tail> <w> <ctx> <text> <ref> <outdir>` | **as-is** |
| Matrix runner | `.deps\run-m5-wpb2.bat` (resumable via `report.json` sentinel) | adapt → `.deps\run-verify-matrix.bat` |
| Summarize / acceptance | `.deps\summarize-wpb2.py` | adapt → `.deps\summarize-verify.py` |
| Tail-off / oracle parity | `.deps\run-oracle.bat` | **as-is** (A4) |
| Build a target | `.deps\build-target.bat <target>` | **as-is** (`ninfer`, `ninfer-serve`, `ninfer-perplexity`, `ninfer_tests`) |
| MTP generation (post only) | `.deps\run-m5-wpf2.sh` pattern (`ninfer` cli) | pattern reuse (B3 post side) |
| **Serve A/B client** | — | **new**: `.deps\verify-serve.py` + `.deps\run-verify-serve-ab.sh` |
| **Corpus / fixture check** | — | **new**: `.deps\verify-corpus.bat` (length check, fixture presence) |

## 8. Prerequisites (before any GPU run)

1. `nvidia-smi` idle at 48 MiB; no `ninfer*` process (`tasklist | grep -i ninfer`).
2. Build the serve target once: `.deps\build-target.bat ninfer-serve` (and `ninfer`, `ninfer-perplexity`
   are already built).
3. Corpus present and long enough: `.deps\verify-corpus.bat` (asserts `m5-longtext.txt` scores
   ≥ 32768 tokens at ctx 32768).
4. Vision fixture present: `bench\fixtures\ttft\media\load_00.png`.
5. Pre-port serve runnable from `D:\ninfer\ninfer-package` with the model + `--vision --spec mtp`.
6. Raw evidence dirs: `.deps\verify-*` (gitignored); no donor tree is written.

## 9. Runbook (commands)

```bat
:: 0) environment + build once
.deps\env-port.bat
.deps\build-target.bat ninfer-serve
.deps\verify-corpus.bat

:: A) no-regression A/B (pre vs post serve, greedy, MTP off/on, vision off/on, ctx 8K/32K, 8 storages)
bash .deps\run-verify-serve-ab.sh

:: B) tail benefit — references (bf16 tail0, per ctx)
.deps\run-m5-save.bat bf16 0 8 8192  .deps\m5-longtext.txt .deps\verify-ref-bf16-t0-w8-c8192.ptk
.deps\run-m5-save.bat bf16 0 8 32768 .deps\m5-longtext.txt .deps\verify-ref-bf16-t0-w8-c32768.ptk

:: B) tail benefit — the matrix (resumable; re-invoke to resume)
.deps\run-verify-matrix.bat
python .deps\summarize-verify.py

:: B3) MTP under the tail (post only; tail0 vs tailN)
bash .deps\run-m5-wpf2.sh       :: existing WP-F harness, re-point at ctx 8192/32768

:: A4) tail-off identity
.deps\run-oracle.bat
```

All GPU work is **serial, single-owner**. After **every** run: `tasklist | grep -iE "ninfer|perplexity"`
empty and `nvidia-smi` back to 48 MiB.

## 10. Risks and limitations

- **Cross-build numeric divergence.** Greedy decode is deterministic for one binary, but two builds can
  differ by an FP reduction order and flip a rare token. A1/A2 therefore compare **token streams** and,
  on a mismatch, must first prove a single binary is self-stable before calling it a regression.
- **Pre-port app asymmetry.** The pre-port product ships only `serve`; the A/B uses `serve` on both
  sides. The post-port `ninfer` CLI is not used for A (only for B3).
- **Serve determinism.** Use `--greedy --seed 0`, `--temperature 0`; avoid the Manager (which injects
  its own defaults) — launch the engine directly with explicit flags.
- **Coverage.** 64K is untested (corpus); `C>1` and the host/disk tier are out of scope (device-only,
  unit tested); `fp8/nvfp4/k8v4` cannot show benefit (inert) and are only in Arm A.
- **Reference numbers.** The shipped `docs\rtx-5070ti-windows.en.md` performance table is for a different
  model (Swift XXS/S) and build; it is a sanity reference, not the A/B baseline.
- **Corpus quality.** WikiText-2 test is the intended corpus; the local slice is a subset, so absolute
  ppl is not comparable to published numbers — only the tail0↔tailN **relative** change is claimed.
