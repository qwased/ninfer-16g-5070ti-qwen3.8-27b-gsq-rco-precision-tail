# PORT-VERIFY-HANDOFF — prompt for the execution session

Paste the block below into a **fresh** conversation window (it has no memory of the session that wrote
these docs). Everything it needs is on disk in `D:\ninfer\ninfer-precision-tail`.

---

## Prompt (copy from here)

You are executing a verification campaign for the **KV precision-tail port** in the repo
`D:\ninfer\ninfer-precision-tail` (branch `main`, local only, no git remote).

**Read first, in this order:** `PORT-VERIFY-PLAN.md` (the runbook you will execute — matrix, pass
criteria, tool reuse map, commands), `PORT-DOD.md` (acceptance checklist — append a row per result),
`PORT-MEMORY.md` §5 (design memory and the traps already paid for), `PORT-REVIEW-PLAN.md` (the
independent review brief), `PORT-JOURNAL.md` (the append-only log; the last Step is 58). Also
`AGENTS.md` governs how you work.

**Goal.** Produce **strong, command-reproducible** evidence for:
1. **No negative impact** from the port: generation output **byte-identical** to the shipped pre-port
   product (`D:\ninfer\ninfer-package\engine\ninfer-serve.exe`), with **MTP off *and* on
   (`--draft-tokens 2`)**, **with and without vision (`--vision`)**; and no performance/memory
   regression.
2. **Substantial tail benefit**: `--kv-tail-tokens N` lowers the decode-width KLD (and ppl) on the
   tail-capable quantized storages, monotone by body coarseness.

**Hardware / artifacts.**
- GPU: RTX 5070 Ti 16 GB (sm_120a, CUDA 13.3). Idle baseline **48 MiB**.
- Model (baseline): `D:\ninfer\ninfer-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`.
- Pre-port engine: `D:\ninfer\ninfer-package\engine\ninfer-serve.exe` (serve only; no CLI, no
  `--kv-tail-*`, no offline scorer).
- Post-port: build `ninfer-serve` once (`.deps\build-target.bat ninfer-serve`); `ninfer.exe` and
  `ninfer-perplexity.exe` are already built in `build-port\apps\`.

**Scope (decided — do not expand without asking).** ctx **8192 and 32768 only**; 64K is out (the local
corpus `.deps\m5-longtext.txt` is ≈32.7K tokens and cannot fill a 64K window; re-adding 64K needs a
≥131K-token text). Storages: all `--kv-dtype` ≥4-bit (`bf16,int8,fp8,rk8v4,rk4v4,rk4v4-e8,nvfp4,k8v4`);
`rk2v4-e8` is 2-bit → excluded. Arm B candidates are the **quantized** tail-capable storages
(int8/rk8v4/rk4v4/rk4v4-e8) against a per-ctx **bf16 tail0** reference — **bf16 is the reference, not a
candidate** (a bf16 candidate can never show gain; it is covered by the no-regression arm). Tails
{0,1024,2048,4096}; MTP `--draft-tokens 2`; concurrency C=1.

**Tools are already prepared — reuse them, do not rewrite:**
- `.deps\run-verify-matrix.bat` — Arm B KLD matrix (resumable; writes `.deps\verify.done`).
  Calls the existing `.deps\run-m5-save.bat` / `.deps\run-m5-kld.bat` **unchanged**.
- `.deps\summarize-verify.py` — Arm B table + acceptance (tailN ≤ tail0; monotone by coarseness).
- `.deps\verify-serve.py` + `.deps\run-verify-serve-ab.sh` — Arm A pre/post serve A/B (greedy, MTP,
  vision).
- `.deps\verify-corpus.bat` — corpus/fixture readiness. `.deps\run-oracle.bat` — tail-off identity.
- `.deps\build-target.bat <target>` — one target, incremental.

**Execution order.**
0. `nvidia-smi` idle 48 MiB, `tasklist | grep -i ninfer` empty. `.deps\build-target.bat ninfer-serve`.
   `.deps\verify-corpus.bat` (expect `CORPUS_OK`).
1. Arm A: `bash .deps\run-verify-serve-ab.sh` — but **first do a smoke subset** (e.g. ctx 8192,
   rk8v4, MTP off/on, vision off/on) to prove the harness end-to-end before the full grid; fix the
   scripts if the serve handshake or flag surface differs. Report each cell IDENTICAL / DIFFERS, plus
   the `.perf` deltas.
2. Arm B: save the two references, then `run-verify-matrix.bat`, then `python
   .deps\summarize-verify.py`. This is a long GPU batch — run it in **foreground chunks** and
   re-invoke to resume (it is resumable); do not leave a detached batch running.
3. Arm B3: MTP tail0-vs-tailN acceptance with the `ninfer` CLI (`.deps\run-m5-wpf2.sh` pattern),
   ctx 8192/32768, rk8v4 + rk4v4-e8.
4. `.deps\run-oracle.bat` for the tail-off identity supporting row.

**Standing rules (from `PORT-MEMORY.md` §6 and `AGENTS.md`).**
- **Never write to the donor trees** `D:\ninfer\ninfer-package`, `D:\ninfer\llamacpp`,
  `D:\ninfer\beellama.cpp`, `D:\ninfer\ninfer-16g-...`. Read only. All output goes under `.deps\` /
  `build-port\` (both gitignored).
- **The donor path is `ninfer-package`** — with the leading `n`. Never retype it; copy it. A wrong
  name fails as `CreateFileW: Win32 error 3`, which looks like a missing file.
- **Never `call env-port.bat >nul 2>&1`** — redirecting it breaks the environment.
- **GPU runs are serial, single-owner.** After **every** run check `tasklist | grep -iE
  "ninfer|perplexity"` is empty and `nvidia-smi` is back to 48 MiB before the next run. Long batches
  are killed after ~3 min if detached — run them in foreground chunks.
- Builds/tests may be silently killed when long; the build is incremental, so re-invoke.
- Use subagents/worktrees to keep the main context small.
- **Record as you go:** one row in `PORT-DOD.md` and one `Step` in `PORT-JOURNAL.md` per result; a
  claim counts only with a command that reproduces it. Update `PORT-MEMORY.md` §5 if a design fact
  changes. Commit code+docs together when the user asks.

**Deliverable.** The run artifacts under `.deps\verify-*`, the Arm A identity table, the Arm B KLD/ppl
table with the two acceptance verdicts, the MTP verdict, and the DOD/JOURNAL records. If any cell
fails, state the exact command and the observed numbers; do not soften it.

## Prompt (end)

---

### Facts the execution session must not have to re-derive

- HEAD at handoff: `205ea411` (the F3 ring-overflow fix — `kv_tail_row_in_ring` in
  `ops/common/kv_tail_element.cuh` — plus the §8 review record). `softmax_attention` oracle green
  (`ORACLE_EXIT=0`).
- The tail merges only on the **decode-width** route (`--score-width ≤ 8`); the default 1024-wide
  prefill tiles never read it (that is why plain `ninfer-perplexity` scores are tail-invariant).
- Tail-capable = `bf16` + INT8 family (`int8, rk8v4, rk4v4, rk4v4-e8`); `fp8/nvfp4/k8v4` are inert by
  design (rotate-frame) — Arm A only.
- Prior campaign (ctx 8K/16K/32K) already showed: tail-on ≤ tail-off in 18/18 cells, gain monotone
  int8 < rk8v4 < rk4v4-e8; the merge's numeric floor is one bf16 ULP (`run_path_parity_case`).
- Pre-port has **no CLI and no scorer** → Arm A must use `serve` on both sides; Arm B uses the port's
  `ninfer-perplexity`.
