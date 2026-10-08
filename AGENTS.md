# AGENTS.md

These rules apply to the whole repository. Removed history, rationale and measured evidence are
archived verbatim in `docs/port-records/AGENTS-ARCHIVE-2026-10.md`, referenced below as **archive §X**.

## Objective and scope

Complete the user's explicit deliverable within the applicable product and external contracts.
Choose a coherent solution with functional and numerical correctness, clear ownership, strong
architecture, and maximum performance at the requested scope. Do not sacrifice these goals to
reduce the diff or implementation effort. Evaluate complexity, maintenance cost, and verification
risk as engineering tradeoffs, not reasons to retain a known inferior design.

Before substantial work, identify the deliverable and its completion conditions. Work is relevant
when it completes that deliverable, preserves an applicable contract, resolves a material
uncertainty, or checks a realistic regression. A necessary redesign is in scope; unrelated cleanup,
hardening, compatibility, and benchmark campaigns are not. Address incidental findings when they
block the outcome or are inseparable from the selected implementation.

For analysis or design, deliver the explanation or design. For diagnosis, establish the cause and
supporting evidence; implement a fix when requested. For implementation, complete the selected
design across its affected implementations, callers, tests, tools, and active documentation.

The current product and architecture govern ordinary work. An explicit task may change them;
update the affected contracts and implementation together instead of treating the current design
as an immutable prohibition. Skills provide task-specific methods, not additional deliverables or
approval requirements beyond the user's instructions and the actual execution environment.

## Product and architecture

NInfer is a from-scratch C++/CUDA inference engine for maximum single-GPU performance, with an
optional layer pipeline across several GPUs on Linux. It implements `Qwen3_5ForCausalLM` and
`Qwen3_5MoeForCausalLM`; official Qwen3.6/3.8 artifacts and user recipes use the same architecture,
binding and execution path. This fork is the **RTX 50-series Windows 16 GB "precision-tail" port**:
Qwen3.8-27B GSQ-RCO plus the `--kv-tail-tokens` precision-tail work. It targets **`sm_120a`** and is
tuned on **NVIDIA GeForce RTX 5070 Ti (16 GB)**, built with CUDA 13.3; `CMakeLists.txt` also admits
`sm_80`/`sm_86`/`sm_89` as compatibility targets. On a `120a` build the `mma.sync` compatibility route
is the tested one; upstream's native routes (`NINFER_SM120_NATIVE=ON`) are a separate, unqualified code
path, and upstream's schedules, route tables and published measurements come from a different card —
treat an upstream tuning constant as a hypothesis until measured on this one (provenance: archive §C).
The build environment is in "Windows build environment (RTX 5070 Ti / sm_120a port host)" below.

Generation uses one resident model on one GPU, or split into pipeline stages over up to eight
(`--devices`, Linux only; each stage owns whole layers with their KV and state, and the head,
round state and sampling stay on the first device; design in
`docs/maintainer/pipeline-parallel-plan.md`). It runs startup-fixed concurrency of one to eight
requests, bounded FIFO ingress, no active-request preemption, and one compact decode batch per
round. Tensor parallelism is not built.
Generation and offline CausalScoring use the same public `.ninfer` Engine route. Delivered
capabilities and commands are documented in `README.md`, the product guides, and executable
`--help`. New mathematical architectures, execution platforms, large-scale/preemptive continuous
batching, and priority/QoS require an explicit product change. Another training instance or mixture
of existing representations does not require a checkpoint-specific execution registration.

This is a local, single-owner project with trusted local models, generated artifacts, and
local workflow. Do not derive requirements from a different deployment or trust model.

Keep these ownership boundaries visible when selecting a design:

- v3 `.ninfer` is the only C++ product artifact; CLI, serving, and inference benchmarks use the public
  Engine. NInfer has no Python model-inference route or installed/exported C++ SDK.
- Core owns physical primitives and raw transfers; artifact owns generic framing and
  materialization; Ops own closed mathematical and state-transition implementations.
- Models own fixed mathematics, config interpretation, logical parameter binding, frontend
  semantics and finite execution composition. Immutable Model data owns selected weights and
  resources; native Parameters supply the actual operands to planning and Program execution.
  Program owns mutable state, workspace, context stores and CUDA Graphs. Programs share no mutable
  state or device allocation.
- Converter recipes choose sources, formats, packing and per-input activation permissions. The
  loader validates, uploads and binds the stored representation. Native preparation, resource
  queries and execution enforce actual Op support; there is no whole-artifact capability registry.
- Runtime owns common execution contracts and Engine publication policy; product/serving own input
  acquisition and protocol translation. Model code does not acquire media or own transport.

Detailed model/runtime responsibilities and source ownership are defined in
[Engine architecture](docs/maintainer/engine-architecture.md). Read the relevant boundary before
changing it. Prefer explicit implementations for supported architectures. Do not introduce generic model
graphs, family base classes, plugin discovery, string-driven execution, hidden device allocation,
runtime weight repacking, or placeholders for hypothetical targets without a product requirement.

## Change consistency

Project-owned APIs, CLIs, Python tools, fixtures, reports, formats, and documentation do not preserve
backward compatibility. When replacing behavior, remove superseded aliases, fallbacks, transition
branches, and their tests within the affected contract. Leave unrelated paths alone.

Advertised OpenAI and Anthropic protocol behavior is an external contract. Changes update the
affected schema tests and serving documentation together.

Keep stable requirements in their existing active reference. Temporary plans are useful only for
active work; remove them when completed or abandoned. Maintain one current authority rather than
parallel `final`, `v2`, or `new-design` documents.

## Verification and completion

Select evidence to support the changed behavior and material claims. Tests should protect supported
observable behavior, mathematical or state semantics, and realistic regressions, including plausible
boundary failures that have not occurred yet. Avoid tests that merely mirror implementation,
freeze private file/class organization, or increase coverage numbers.

For numerical changes, identify represented public inputs, the independent mathematical oracle,
semantic cast/quantization/state boundaries, output criteria, and relevant real model shapes. Each
floating-point Op uses a naive FP32/FP64 oracle; exact transforms/codecs use an exact oracle. Packed
inputs are independently decoded with their stored scales. Qualify production routes directly
against that oracle, not another kernel or plausible model output. Private arithmetic need not
reproduce unfused materializations unless an intermediate is an observable semantic boundary.
[Op development](docs/maintainer/op-development.md) defines the full qualification contract.

Measure performance at the claimed scope. An Op microbenchmark establishes an Op result, not an
end-to-end improvement. Use whole-inference profiling when an in-scope end-to-end attribution is
unresolved; use kernel profiling when an identified kernel question can change the decision. Reuse
applicable evidence and stop collecting once the relevant alternatives can be distinguished.

Choose the affected checks, rather than running this table as a checklist:

| Change                              | Typical evidence                                                            |
| ----------------------------------- | --------------------------------------------------------------------------- |
| Documentation                       | affected links/references and `git diff --check`                            |
| C++ runtime/API                     | affected build targets and behavioral tests                                 |
| Python tooling                      | `py_compile` with the selected interpreter and affected tests               |
| Artifact framing/binding/conversion | affected contract tests; real artifact when semantics require it            |
| CUDA mathematics                    | independent oracle at relevant shapes and route boundaries                  |
| Memory or lifetime                  | affected execution; sanitizer for a concrete lifetime question              |
| Performance                         | measurement at the claimed scope; profiling only for unresolved attribution |
| Serving                             | affected schema tests and observable request/stream behavior                |

Record the target, relevant hardware/toolchain, workload or command, and summarized result needed
to interpret a material claim. Hashes, clean worktrees, full command transcripts, raw report
inventories, and exact probabilistic outputs are not default requirements. Use exact comparison for
exact outputs, and appropriate numerical or behavioral criteria otherwise. State checks that could
not run and their implications.

Finish when the deliverable is usable, applicable contracts are satisfied, material claims have
sufficient evidence, relevant checks pass or their limitations are clear, and no known in-scope
issue blocks use. Supporting work is not an independent completion objective.

## Reporting and completion

Selective reporting and evidence gaming are prohibited, even when every disclosed
statement is individually true.最终向用户汇报时必须使用中文. For every implementation task:

1. Cover the entire agreed deliverable, its completion status, and all affected or
   evaluated dimensions: behavior, numerical semantics, interfaces, architecture,
   performance, resources, and maintenance. Distinguish completed, incomplete, and
   unverified work; never describe an unmeasured aspect as unchanged.

2. Put favorable and unfavorable findings in the final reply itself, including
   regressions, costs, rejected approaches, failures subsequently fixed, unresolved
   issues, and verification gaps. Explain their disposition. Group repetition
   without hiding distinct problems or exceptions. Small or unexplained adverse
   results must remain visible; attachments cannot substitute for disclosure.

3. Make comparisons representative and comparable. State the baseline, workload,
   conditions, metrics, coverage, outcome distribution, worst changes, and exceptions.
   Distinguish new capability, fallback replacement, and improvement to an optimized
   implementation. Keep claims within the measured scope; neither a best case nor
   an average may stand in for the full results.

4. Apply the same evidence standard to gains and regressions. Label uncertainty;
   do not dismiss slowdowns as noise without evidence. Explain changes to scope,
   baselines, methods, or acceptance criteria and preserve earlier adverse findings.
   Never change these choices to manufacture a favorable conclusion.

5. Reuse sufficient evidence. Additional or repeated checks must satisfy required
   verification, replace invalidated evidence, or resolve a concrete question that
   could change implementation or acceptance. Once the deliverable and acceptance
   conditions are satisfied, stop and report. Report review checks existing work
   and findings; it must not become a new audit, sweep, or reporting-tool project.
   Disclose remaining uncertainty without silently making it a new requirement.
   Disclosure does not excuse unmet completion conditions.

## Durable progress record

Long-running work must keep its progress on disk, not only in the conversation. Context is
summarized and lost between sessions, so a conclusion that exists only in a chat transcript is not
durable. For any multi-step effort — a port, a migration, a multi-work-package plan — maintain one
progress record in the repository alongside the plan it serves, and treat it as part of the
deliverable:

1. **One record, one authority.** A single file next to the plan holds the live state; do not
   scatter progress across several notes. The plan names the record, and the record names the plan.

2. **Log every step, including the ones that hurt.** Failures, rejected approaches, measurements
   that contradict the plan, and unresolved items are recorded with the same care as successes.
   An omitted adverse result is a lost result.

3. **Update at every work-package boundary.** Refresh a status snapshot, append a dated entry with
   the measured numbers, the exact commands, and the disposition, then write back to the plan any
   conclusion that changes acceptance criteria, open decisions, or risks.

4. **Measured reality wins.** When a measurement contradicts the plan, correct the plan and mark the
   superseded claim rather than deleting it. Never narrow scope or reinterpret a criterion after
   seeing an unfavorable result.

5. **An unupdated record means the work package is not complete.**

The current instance is the KVarN port: `kvarn-port-progress.md` is the record, and
`kvarn-port-into-precision-tail-plan.md` §0.5 states the per-work-package protocol.

## Reference navigation

Read the authority relevant to the current decision; this is not a mandatory reading list.

| Decision                                                                    | Entry point                                                                                                                                                                                                                          |
| --------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Product capabilities and exact commands                                     | `README.md`, executable `--help`; `docs/cli.md`, `docs/serving.md`, `docs/perplexity.md`                                                                                                                                             |
| Execution, model/runtime ownership, scheduling, transactions, graphs        | `docs/maintainer/engine-architecture.md`                                                                                                                                                                                             |
| Context resources, checkpoints, replicas; physical KV                       | `docs/maintainer/resource-scheduling-and-context-cache.md`; `docs/maintainer/paged-kv-cache.md`                                                                                                                                      |
| Artifact, layout, codec, conversion, or model mathematics                   | model/artifact references and conversion guide linked from `docs/README.md`                                                                                                                                                          |
| Op contracts, implementation ownership, numerical/performance qualification | `docs/maintainer/op-development.md`                                                                                                                                                                                                  |
| Test/benchmark commands and published performance                           | `tests/README.md`, `bench/README.md`, `docs/performance.md`                                                                                                                                                                          |
| Build system, toolchain and configuration options                           | `docs/maintainer/build-system.md`; host details in "Windows build environment" below                                                                                                                                                 |
| KVarN port plan, progress record and precision-tail records                 | `kvarn-port-into-precision-tail-plan.md` (active plan; §0.5 record protocol), `kvarn-port-progress.md` (active progress record); archived records in `docs/port-records/` (`PORT-BEELLAMA-SPEC.md`, `PORT-MEMORY.md`, `PORT-DOD.md`) |
| In-tree C++ interface                                                       | `include/ninfer/engine.h`, `include/ninfer/types.h`                                                                                                                                                                                  |

[Documentation map](docs/README.md) routes to narrower authorities when needed.

## Local operations

### Windows build environment (RTX 5070 Ti / sm_120a port host)

**Use the existing `build-port/` tree. Do not reconfigure and do not delete it.** It is already
configured for this host: Ninja, Release, `CMAKE_CUDA_ARCHITECTURES=120a`,
`NINFER_SM120_NATIVE=ON`, `NINFER_BUILD_APPS=ON`, `BUILD_TESTING=ON`,
`NINFER_BUILD_BENCHMARKS=OFF`. Reconfiguring is the one thing that actually breaks the build here;
there is nothing to gain by redoing it. The tree registers 259 tests
(`ctest --test-dir build-port -N`).

Toolchain (all on `PATH` by `.deps/env-port.bat`): MSVC `14.44.35207` (VS 2022 BuildTools), CUDA
`13.3`, and the VS-bundled CMake `3.31.6-msvc6` + Ninja. Only VS 2022 BuildTools is installed, so the
older "two MSVC toolchains, the wrong one first on `PATH`" trap no longer applies. Importing the
vcvars environment is still required before building — `nvcc` needs `INCLUDE`/`LIB` from vcvars, and
adding only the compiler's `bin` to `PATH` leaves the standard-library headers missing
(`fatal error C1083: Cannot open include file: 'cstdint'`). Versions, paths and the retired trap:
archive §A.

Build from any shell by sourcing the port environment first. `.deps/env-port.bat` is the supported
entry point; do not inline vcvars from Git Bash (MSYS rewrites `>nul` into a path and breaks the
`&&` chain). Its full setting list is in archive §A.

```bat
call D:\ninfer\ninfer-precision-tail\.deps\env-port.bat
cmake --build D:\ninfer\ninfer-precision-tail\build-port -j    # everything
cmake --build D:\ninfer\ninfer-precision-tail\build-port --target <name> -j
ctest --test-dir D:\ninfer\ninfer-precision-tail\build-port -j2 --output-on-failure
ctest --test-dir D:\ninfer\ninfer-precision-tail\build-port -R <regex> --output-on-failure
```

For a standalone `.cu` probe outside the build tree, use `cmd`/PowerShell rather than Git Bash
(which mangles MSVC-style flags, e.g. `/wd4819` becomes a path) with the port architecture:
`nvcc -O3 -arch=sm_120a probe.cu -o probe.exe` (why: archive §A).

Other host facts:

- GPU: RTX 5070 Ti 16 GB, compute capability 12.0; idle VRAM baseline is ~48 MiB.
- Profilers are installed under `C:\Program Files\NVIDIA Corporation\`: Nsight Systems 2026.1.3
  (and 2023.3.3), Nsight Compute 2026.2.0. `ncu` needs GPU performance counters, which are
  admin-only by default — run it from an elevated shell rather than changing
  `RmProfilingAdminOnly`, which needs two reboots and loosens a system-wide setting.
- Compilation needs no GPU, so building is always possible. **Executing** CUDA tests, benchmarks
  or perplexity runs needs free VRAM, and the user often has the server loaded — ask before
  assuming the device is free.
- `nvcc` writes `.exp`/`.lib` next to any `-o` target; keep probe builds out of the repo root.

### Codebase memory (indexed graph)

This repository is indexed as graph project `D-ninfer-ninfer-precision-tail`; the sibling repos under
`D:\ninfer` are indexed separately. Use the `codebase-memory` skill for structural work.

- **`list_projects` is authoritative, not the SessionStart hook.** When the workspace root is
  `D:\ninfer` (the parent of this repo) the hook reports "no indexed graph project matched this
  working directory", because it matches only when an indexed project root is an *ancestor* of the
  working directory. Call `list_projects` and continue; the graph is available.
- Before relying on a file, `check_index_coverage` it. `metadata_changed` is normal on a dirty
  worktree and is not a verdict; `parse_partial` / `not_indexed` ranges mean read those lines
  directly and qualify the conclusion.
- **Structural, negative and exhaustive claims must come from the graph**, not grep alone: "who
  calls X", "X is unused", "no other caller", "nothing else reads this field", "the only place".
  Use `trace_path` inbound for callables (plus coverage). **`trace_path` resolves callables only** —
  a *field* needs `query_graph` Cypher over `USAGE`/`WRITES`, whose edges are **name-resolved** (they
  conflate same-named fields across classes and carry no access-site line), so treat a field-level
  negative claim as *indicative* and reconcile it with `grep` before recording it. **State the query
  that was run** whenever the claim is negative or exhaustive.
- **Changing a shared type is a graph task.** Before editing a struct or enum in
  `include/ninfer/types.h` or another shared header, enumerate its consumers, and use the result to
  *check* a `grep` sweep rather than to replace it.
- Literal lookups (one flag, one string, one config value) stay with `grep`.

**Measured cases and tool boundaries** (the `reduce_output` uniqueness evidence, the `KvarnBits`
consumer comparison, the `exact_tail` name collision, and the fact that `file_pattern` is not a
regex): archive §B.

**The graph does not cover kernel interiors.** A local `__shared__` array inside a kernel body is not
a graph node, and a `<<<>>>` launch is not a `CALLS` edge (the kernel's inbound degree stays 0 with a
caller present). Positive claims about synchronization, scratch reuse and race freedom come from the
source plus `compute-sanitizer --tool racecheck`.

**Change flow (checklist; the graph gates above are its static half).** **Tier A** — a shared
cross-kernel contract, a shared type, or cross-file behavior — runs the whole flow. **Tier B** — a
single-site local change (one function body, a literal, a config value, docs) — runs only A7–A9. Make
the tier explicit: running the full ceremony on Tier B work is what caused gate ① to be skipped on
2026-10-08.

- **A1 index freshness.** `index_status`: reindex only when `indexed_at` predates the working tree's
  latest change; otherwise reuse the previous package's boundary index (`metadata_changed` /
  `parse_partial` on a dirty tree are why a stale index looks untrustworthy).
- **A2 contract enumeration (graph).** `search_graph` (`name_pattern`) for every sibling
  implementation of the contract; narrow a same-named family with `qn_pattern` or an **exact
  full-path** `file_pattern`. Distinguish same-named different things by prefix.
- **A3 launch-site sweep (grep).** Enumerate every `<<<...>>>` launch of each sibling by kernel name;
  the graph does not model launches.
- **A4 coverage.** `check_index_coverage` on every file relied on; read the flagged `parse_partial` /
  `not_indexed` ranges directly and qualify the conclusion. Open an unfamiliar file with
  `get_file_outline`, not a whole-file read.
- **A5 plan with the change manifest.** Write the goal and acceptance criteria, then the per-file
  change points (symbol, lines) with, per item, **feasible / doubtful / rejected** + risk + rollback.
  Keep "quasi-code" at the interface-and-invariant level — do not paste implementation bodies into a
  durable document. Iterate every *doubtful* item back through A2–A4 until it is resolved or rejected.
- **A6 state-transition matrix (before implementing).** Cross the state dimensions (e.g. `N=0`,
  `N≤ring`, `N≥width`, cross-group(128)/ring(64) boundary, checkpoint recovery) with the observation
  faces (numeric oracle, bit-exact, capacity, observable text). A blank cell is **uncovered**: record
  it as a declared gap, never as satisfied.
- **A7 implement** the manifest item by item.
- **A8 build + dynamic verification.** Build by target (never the whole tree): the registered
  oracle/gate tests, `compute-sanitizer --tool racecheck` for synchronization and scratch reuse, an
  independent FP64 oracle for numeric changes, and a cross-route consistency check (a change confined
  to one route must move only that route's margins).
- **A9 close the boundary.** Append the progress §3 entry (command, raw output, verdict), refresh the
  §0 snapshot, write conclusions back to the plan, run **`index_repository`**, and `git diff --check`.

**What the graph cannot give.** It is a static structure tool: it supplies *structure and
completeness*, never feasibility, risk or correctness. Those come from reading the source and from
domain reasoning.

### Build, run and resources

Use `cmake --build build-port -j` by default. Adjust parallelism when actual resource pressure
causes failures or interferes with the task, and briefly explain why. Benchmarks are off in
`build-port`; enabling them needs one reconfigure with `-DNINFER_BUILD_BENCHMARKS=ON`.

Select the Python interpreter explicitly: this host has both Python 3.12 (`python`) and 3.11
(`py -3.11`), and the default may differ from what a tool expects. Python 3.11 is the maintained
environment (see `docs/maintainer/build-system.md`).

Normal resources are the `build-port/` tree and the model artifact under the sibling
`ninfer-precision-tail-package/model/` (for example
`Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`). Select model artifacts by explicit path,
never glob order, modification time, or unqualified “latest”. Source checkpoints and large
artifacts are prerequisites; download or regenerate them only when that work is in scope. Install
or upgrade dependencies only when the task needs it.

## Commits

Create commits only when requested. Use Conventional Commit subjects with concise lowercase types
such as `feat`, `fix`, `perf`, `bench`, `test`, `build`, `refactor`, `docs`, or `chore`.
