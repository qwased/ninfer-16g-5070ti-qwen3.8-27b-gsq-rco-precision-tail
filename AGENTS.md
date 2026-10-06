# AGENTS.md

These rules apply to the whole repository.

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
optional layer pipeline across several GPUs on Linux. It implements
`Qwen3_5ForCausalLM` and `Qwen3_5MoeForCausalLM`; official Qwen3.6/3.8 artifacts and user recipes
use the same architecture, binding and execution path.
This fork is the **RTX 50-series Windows 16 GB "precision-tail" port**: Qwen3.8-27B GSQ-RCO plus the
`--kv-tail-tokens` precision-tail work. It targets **`sm_120a`** and is tuned on **NVIDIA GeForce RTX
5070 Ti (16 GB)**, built with CUDA 13.3. `CMakeLists.txt` also admits `sm_80`/`sm_86`/`sm_89` as
compatibility targets; on a `120a` build the `mma.sync` compatibility route is the tested one, while
upstream's native routes (`NINFER_SM120_NATIVE=ON`) are a separate, unqualified code path. Upstream
(`Neroued/ninfer`) targets `sm_120a` on RTX 5090; its schedules, route tables and published
measurements come from that card, so treat an upstream tuning constant as a hypothesis until measured
on this one. The build environment is in "Windows build environment (RTX 5070 Ti / sm_120a port
host)" below.

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

| Change | Typical evidence |
|---|---|
| Documentation | affected links/references and `git diff --check` |
| C++ runtime/API | affected build targets and behavioral tests |
| Python tooling | Python 3.11 `py_compile` and affected tests |
| Artifact framing/binding/conversion | affected contract tests; real artifact when semantics require it |
| CUDA mathematics | independent oracle at relevant shapes and route boundaries |
| Memory or lifetime | affected execution; sanitizer for a concrete lifetime question |
| Performance | measurement at the claimed scope; profiling only for unresolved attribution |
| Serving | affected schema tests and observable request/stream behavior |

Record the target, relevant hardware/toolchain, workload or command, and summarized result needed
to interpret a material claim. Hashes, clean worktrees, full command transcripts, raw report
inventories, and exact probabilistic outputs are not default requirements. Use exact comparison for
exact outputs, and appropriate numerical or behavioral criteria otherwise. State checks that could
not run and their implications.

Finish when the deliverable is usable, applicable contracts are satisfied, material claims have
sufficient evidence, relevant checks pass or their limitations are clear, and no known in-scope
issue blocks use. Expand or repeat verification only for new changes, failures, or unresolved risks
that could change the result. Supporting work is not an independent completion objective.

## Reference navigation

Read the authority relevant to the current decision; this is not a mandatory reading list.

| Decision | Entry point |
|---|---|
| Product capabilities and exact commands | `README.md`, executable `--help`; `docs/cli.md`, `docs/serving.md`, `docs/perplexity.md` |
| Execution, model/runtime ownership, scheduling, transactions, graphs | `docs/maintainer/engine-architecture.md` |
| Context resources, checkpoints, replicas; physical KV | `docs/maintainer/resource-scheduling-and-context-cache.md`; `docs/maintainer/paged-kv-cache.md` |
| Artifact, layout, codec, conversion, or model mathematics | model/artifact references and conversion guide linked from `docs/README.md` |
| Op contracts, implementation ownership, numerical/performance qualification | `docs/maintainer/op-development.md` |
| Test/benchmark commands and published performance | `tests/README.md`, `bench/README.md`, `docs/performance.md` |
| In-tree C++ interface | `include/ninfer/engine.h`, `include/ninfer/types.h` |

[Documentation map](docs/README.md) routes to narrower authorities when needed.

## Local operations

### Windows build environment (RTX 5070 Ti / sm_120a port host)

**Use the existing `build-port/` tree. Do not reconfigure and do not delete it.** It is already
configured for this host: Ninja, Release, `CMAKE_CUDA_ARCHITECTURES=120a`,
`NINFER_SM120_NATIVE=ON`, `NINFER_BUILD_APPS=ON`, `BUILD_TESTING=ON`,
`NINFER_BUILD_BENCHMARKS=OFF`. Reconfiguring is the one thing that actually breaks the build here;
there is nothing to gain by redoing it. The tree registers 259 tests
(`ctest --test-dir build-port -N`).

Toolchain (all put on `PATH` by `.deps/env-port.bat`):

| | version | path |
|---|---|---|
| MSVC (VS 2022 BuildTools) | `14.44.35207` | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
| CUDA | `13.3` | `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3` |
| CMake | `3.31.6-msvc6` | bundled with the VS 2022 BuildTools CMake directory |
| Ninja | (bundled) | bundled with the VS 2022 BuildTools CMake directory |

Only VS 2022 BuildTools is installed; there is no VS 2026 on this host, so the older "two MSVC
toolchains, the wrong one first on `PATH`" trap no longer applies. Importing the vcvars environment
is still required before building — `nvcc` needs `INCLUDE`/`LIB` from vcvars, and adding only the
compiler's `bin` to `PATH` leaves the standard-library headers missing
(`fatal error C1083: Cannot open include file: 'cstdint'`).

Build from any shell by sourcing the port environment first. `.deps/env-port.bat` is the supported
entry point; do not inline vcvars from Git Bash (MSYS rewrites `>nul` into a path and breaks the
`&&` chain):

```bat
call D:\ninfer\ninfer-precision-tail\.deps\env-port.bat
cmake --build D:\ninfer\ninfer-precision-tail\build-port -j    # everything
cmake --build D:\ninfer\ninfer-precision-tail\build-port --target <name> -j
ctest --test-dir D:\ninfer\ninfer-precision-tail\build-port -j2 --output-on-failure
ctest --test-dir D:\ninfer\ninfer-precision-tail\build-port -R <regex> --output-on-failure
```

`env-port.bat` sets the VS 2022 vcvars (`-vcvars_ver=14.44`), the CUDA 13.3 `bin`, the VS
CMake/Ninja `bin`, `VCPKG_ROOT=.deps/vcpkg-root` (`x64-windows`), `CUDACXX`, and
`CL=/D_USE_MATH_DEFINES`. It builds cleanly and is the supported path.

For a standalone `.cu` probe outside the build tree, use `cmd`/PowerShell rather than Git Bash
(which mangles MSVC-style flags, e.g. `/wd4819` becomes a path), and pass the port architecture:

```bat
nvcc -O3 -arch=sm_120a probe.cu -o probe.exe
```

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

## Commits

Use `cmake --build <build-dir> -j` by default. Adjust parallelism when actual resource pressure
causes failures or interferes with the task, and briefly explain why.

Select the Python interpreter explicitly: this host has both Python 3.12 (`python`) and 3.11
(`py -3.11`), and the default may differ from what a tool expects.
Normal resources are the `build-port/` tree and the model artifact under the sibling
`ninfer-precision-tail-package/model/` (for example
`Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`); the local toolchain is CUDA 13.3.
Select model artifacts by explicit path, never glob order, modification time, or unqualified
“latest”. Source checkpoints and large artifacts are prerequisites; download or regenerate them
only when that work is in scope. Install or upgrade dependencies only when the task needs it.

Create commits only when requested. Use Conventional Commit subjects with concise lowercase types
such as `feat`, `fix`, `perf`, `bench`, `test`, `build`, `refactor`, `docs`, or `chore`.
