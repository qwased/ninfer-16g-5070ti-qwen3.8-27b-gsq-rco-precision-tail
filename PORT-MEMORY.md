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

## 3. Hardware / build baseline (per plan §0.5)

- GPU: RTX 5070 Ti 16 GB, sm_120a, CUDA 13.3 (`nvcc` on PATH).
- Note: repo `AGENTS.md` still claims sm_86/3090 (upstream fork text). Per plan §0.5 the test
  baseline is **pinned to sm_120a / 5070 Ti**; the doc contradiction is not a blocker.
- Toolchain status: `nvcc` = CUDA 13.3 found. `cmake`/`ninja` **not on PATH**; VS 2022 installed
  under `C:\Program Files\Microsoft Visual Studio`. Exact cmake path TO BE LOCATED (task #2).

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
- Next: task #2 build environment verification (baseline compile before touching code).

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
