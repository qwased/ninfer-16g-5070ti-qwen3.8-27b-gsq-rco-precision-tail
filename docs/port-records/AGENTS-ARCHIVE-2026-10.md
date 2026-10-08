# AGENTS.md 归档（2026-10）

> **性质**：本文件是 `AGENTS.md` 瘦身时的**逐字搬运**目标（只搬运、不改写）。其中只收**历史、
> 依据、实测证据与 provenance**——**不收任何仍生效的规定**。现行规则在 `AGENTS.md` 对应位置，
> 各节标题下注明搬运来源的原始行号。
>
> **回退**：如需还原，把本文件对应小节逐字贴回 `AGENTS.md` 的指针处即可。

---

## §A 构建环境与工具链

**搬运来源**：`AGENTS.md`「Windows build environment (RTX 5070 Ti / sm_120a port host)」小节（原 229–242、256–258、260–265 行）。

### A-1 工具链版本与路径（原表）

Toolchain (all put on `PATH` by `.deps/env-port.bat`):

|                           | version        | path                                                             |
| ------------------------- | -------------- | ---------------------------------------------------------------- |
| MSVC (VS 2022 BuildTools) | `14.44.35207`  | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
| CUDA                      | `13.3`         | `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3`       |
| CMake                     | `3.31.6-msvc6` | bundled with the VS 2022 BuildTools CMake directory              |
| Ninja                     | (bundled)      | bundled with the VS 2022 BuildTools CMake directory              |

### A-2 已失效的旧陷阱（历史）

Only VS 2022 BuildTools is installed; there is no VS 2026 on this host, so the older "two MSVC
toolchains, the wrong one first on `PATH`" trap no longer applies. Importing the vcvars environment
is still required before building — `nvcc` needs `INCLUDE`/`LIB` from vcvars, and adding only the
compiler's `bin` to `PATH` leaves the standard-library headers missing
(`fatal error C1083: Cannot open include file: 'cstdint'`).

### A-3 `env-port.bat` 的逐项设置

`env-port.bat` sets the VS 2022 vcvars (`-vcvars_ver=14.44`), the CUDA 13.3 `bin`, the VS
CMake/Ninja `bin`, `VCPKG_ROOT=.deps/vcpkg-root` (`x64-windows`), `CUDACXX`, and
`CL=/D_USE_MATH_DEFINES`. It builds cleanly and is the supported path.

### A-4 独立 `.cu` 探针为何用 `cmd`/PowerShell

For a standalone `.cu` probe outside the build tree, use `cmd`/PowerShell rather than Git Bash
(which mangles MSVC-style flags, e.g. `/wd4819` becomes a path), and pass the port architecture:

```bat
nvcc -O3 -arch=sm_120a probe.cu -o probe.exe
```

---

## §B 图谱实测案例与工具边界

**搬运来源**：`AGENTS.md`「Codebase memory (indexed graph)」小节（原 294–299、300–304、307–310 行，以及 2026-10-08 新增的门禁① sweep 记录）。

### B-1 `trace_path` 对 field 恒为 0；`USAGE`/`WRITES` 按名解析

**`trace_path` resolves callables only**: an inbound trace on a *field* qualified name returns
`callers_total: 0` even when readers exist. Field consumers need `query_graph` Cypher over the
`USAGE`/`WRITES` edge types, and those edges are **name-resolved** — they conflate same-named
fields across classes (a `kvarn_bits` read comes back against whichever declaring class the
resolver picked) and carry no access-site line, only the enclosing symbol. Treat a field-level
negative claim as *indicative* and reconcile it with grep before recording it.

### B-2 `KvarnBits` 消费方：图谱与 grep 一致（0 个图谱独有文件）

The graph *can* find readers grep misses (visitors, serialisers, macros), but on the one measured
C++ field case (`KvarnBits`, 2026-10-08) its consumer set was exactly grep's — so use it to *check*
the sweep, not to replace it.

### B-3 reduce-output 唯一性（门禁① 的实证案例）

The measured case is the 2026-10-08 KVarN reduce race: grep and `racecheck` localized the defect,
but only `search_graph` established that this was the *only* one of the four reduce-output kernels
carrying it.

**2026-10-08 复核（WP6.3 门禁① sweep）**：`search_graph(name_pattern=".*reduce_output.*")` ⇒ 全仓
**恰好 4 个** reduce-output kernel —— `causal_attention_small_t_reduce_output_kernel`
（`small_t.cuh:261`）、`..._k8v4_...`（`small_t_k8v4.cuh:591`）、`..._nvfp4_...`
（`small_t_nvfp4.cuh:609`）、`kvarn::detail::reduce_output_hadamard_kernel`
（`decode_kernel.cuh:1084`，入度 0 / 出度 7）。⇒ 这正是 grep 给不了的**穷尽性**证据。

### B-4 命名撞车：`exact_tail` 同属 kvarn 与 q8

`src/ops/linear_pair/q8/q8_pair_plan.cpp` 有**既有的、与精度尾无关的** `launch_exact_tail`
（`:507-541`）/ `is_exact_tail_schedule`（`:116-129`），属 q8 线性对特性。同理 `KvarnBits`
与别的 `*Bits`。⇒ **同名不同物按前缀 / 精确路径区分**。

### B-5 `search_graph` 的 `file_pattern` 不是正则

`file_pattern="src/ops/kvarn/tail_partial.cuh"` **命中 3 节点**，而 `file_pattern=".*tail_partial.*"`
**返回 0**（同理 `.*causal_cache.*` 返回 0，而该目录节点确实存在）。**`name_pattern` /
`qn_pattern` 才是正则**。⇒ 按路径过滤一律写**全仓库相对路径**；要按文件名做正则，改用
`name_pattern` 或 `qn_pattern`。

---

## §C 产品 / 上游 provenance

**搬运来源**：`AGENTS.md`「Product and architecture」小节（原 38–41 行）。

Upstream (`Neroued/ninfer`) targets `sm_120a` on RTX 5090; its schedules, route tables and published
measurements come from that card, so treat an upstream tuning constant as a hypothesis until measured
on this one.
