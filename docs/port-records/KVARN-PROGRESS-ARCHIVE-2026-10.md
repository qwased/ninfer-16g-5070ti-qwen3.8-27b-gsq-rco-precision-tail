# KVARN-PROGRESS-ARCHIVE-2026-10 — KVarN 进度日志早期内容归档

- 归档日期：2026-10-08
- 来源：`../../kvarn-port-progress.md`（归档前的提交状态 `6aea191b`；行号均指该文件）
- 内容（**逐字搬运**）：§3 记录 **2026-10-07-1 … 2026-10-07-23**（原第 260–1312 行）、
  §0 快照的「最后更新」散文段（16–34）、§0 WP 状态表（36–47）、§0 工作树改动清单（49–131）、
  §0 已关闭的未决项 1 / 5 / 8 / 9 / 10（135–142 / 146–167 / 177–192）、§4.1 任务清单（1574–1607）、
  §4.4 历史 WP 状态（1615–1648）
- 归档原因：进度日志 §3 是**追加式**记录，早期条目描述的工作包（WP0–WP4）已全部收尾，
  其结论已进入计划书（§1 A1–A8、§7 WP 状态、§9 风险、§10 D1–D6）与本日志 §0/§4。
  日志正文保留 **2026-10-08-1 … -5** 近期条目 + §1 环境速查（操作手册）+ §2 事实索引 + §5 handoff。
- 体例：下文每个「逐字归档」小节都是源文件的**字节级副本**，未改写数字、命令、结论或措辞；
  未决项 9 的「未实现」等表述**按原样保留**（已被 08-02/08-04 的记录取代，见下方 superseded 标注）。
- 记录规则：见计划书 §0.5；本文件不改变任何验收口径。

## 索引 A：§3 记录条目（标题逐字）

| 条目号 | 原小节标题（逐字） |
|---|---|
<!-- vbegin @@S3ROWS@@ -->
| 2026-10-07-1 | 侦察与路线确认 |
| 2026-10-07-2 | WP0：基线构建发现先存缺陷（与本工作无关） |
| 2026-10-07-3 | WP1：kvarn ops 原样移植，零告警编译通过 |
| 2026-10-07-4 | WP1：测试移植与首跑（30 项中 1 项容差未过） |
| 2026-10-07-5 | WP0.5-C：构型口径声明（D6 决议） |
| 2026-10-07-6 | 建立本进度日志 |
| 2026-10-07-7 | 信息落盘制度写入 AGENTS.md（仓库级） |
| 2026-10-07-8 | WP0.5-A：MTP parity 测试移植完成；**实跑发现 A3 失败（非 kvarn 相关）** |
| 2026-10-07-9 | A3 差分诊断（进行中）：上游 vs TAIL 的代码溯源 |
| 2026-10-07-10 | A3 差分实测：**上游同样失败 ⇒ 缺陷先于 precision-tail 存在**（推翻 07-09 的假设） |
| 2026-10-07-11 | A3 上游调研（GitHub + 三仓代码/文档）：**不是上游 bug，而是 Fork B 私有合同** |
| 2026-10-07-12 | A3 定案：报告复核 + 计划调整（**回写计划书 v4**） |
| 2026-10-07-13 | **WP2 完成**：页几何 + 存储枚举 + parser/名字/指纹；**261 测试全量回归 + 基线对照** |
| 2026-10-07-14 | WP3 起手：模型接入（`kvarn:k4v2` **短上下文端到端跑通**；长上下文阻塞已精确定位） |
| 2026-10-07-15 | WP0.5-B 起手：**首个 KVarN 准入 KLD 数据点（正向）**；但准入门被 **WP3 页几何**前置 |
| 2026-10-07-16 | **WP3① 完成**：地址空间页几何按存储贯穿；`kvarn:k4v2` ctx8192 / 229,348 token 全量跑通 |
| 2026-10-07-17 | WP0.5-B 补测启动：**229k 同字节臂缺失面**（结果未出） |
| 2026-10-07-18 | WP0.5-B 补测**完成**：229k 同字节矩阵齐备 ⇒ 代理门禁**明确为正**（WP4 GO） |
| 2026-10-07-19 | WP4 起手：位宽站点测绘（WP4.1）+ **记录布局泛化为函数**（WP4.2 第一步，构建绿） |
| 2026-10-07-20 | WP4.2 主体完成：核按 `(kb,vb)` 模板化 + `bits` 贯穿到 launcher（构建绿） |
| 2026-10-07-21 | WP4.2 端到端验收（测试扩 4/5/6 实跑全绿）+ WP4.4 parser 发档 + 两处规划期拒绝 |
| 2026-10-07-22 | **WP4 收尾：正式 WP0.5-B 三档全过**（发布档准入达成）+ 测试补 5/6 位宽注意力覆盖 + 文档 |
| 2026-10-07-23 | KVarN「本质」核对：报告描述 vs 本项目实现（三方审计，无代码改动） |

<!-- vend @@S3ROWS@@ -->

## 索引 B：§0 / §4 其余归档件

| 归档件 | 源行号 | 一句话内容 | 状态 |
|---|---|---|---|
| §0 快照散文（最后更新段） | 16–34 | 08-05 / 08-04 / 08-03 三轮的实测摘要（P2b 仪器、GPU 收尾四件、纯 CPU 三件） | 现行值已压缩进 §0；全文见本文件 §B |
| §0 WP 状态表 | 36–47 | WP0…WP9 每包的详细状态与实测数字（含发布档三档 KLD、`flips 1/65`、`cached_tokens=851`） | 现行值已压缩进 §0；全文见本文件 §C |
| §0 工作树改动清单 | 49–131 | WP1–WP5/P1 的逐文件改动清单（新增/修改，含关键函数与守卫点） | 已随 `beda920a`/`fe76ad42`/`6aea191b` 提交；全文见本文件 §D |
| §0 未决项 1 | 135–142 | `ninfer_kvarn_test` 4-bit K oracle 容差：07-21 收口、08-03 形式化、余量待 GPU 首跑 | **已关闭**（08-04 余量实测 65×） |
| §0 未决项 5 | 146–167 | A3 判据不成立的定案依据（#265/#80、`small_t.cu:383-395` 机制、Fork B 私有合同、O1 采纳） | **已裁决**（采纳 O1，回写计划 v4 附录 D-6） |
| §0 未决项 8 | 177–182 | WP3 长上下文阻塞：`kv_pages_for_*` 硬写 64 ⇒ `create_active` 返 `nullopt`；修方法与验收 | **已解决**（07-16 WP3①，计划附录 D-9） |
| §0 未决项 9 | 183–187 | 「WP3 未做（续列尾）：`state_image` kvarn 镜像与 capture/activate 未实现」 | **已被取代**（08-02 实现、08-04 host+device+e2e 全绿）；原文按原样保留 |
| §0 未决项 10 | 188–192 | `kvarn + --mtp-attention-window` 不兼容；`validate_target_options` 一并拒绝 `--kv-tail-tokens` | **已实现**（07-21）；残余（MTP 激励 / A3 相对判据）已由 08-04/08-05 关闭 |
| §4.1 任务清单 | 1574–1607 | P1 续列尾 / P2 MTP 激励与仪器 / P3a-c / P4a-b / P5 的原始「目标·内容·验收·风险·估时」 | P0/P1/P2/P4a 已完成；未完成任务的现行表述在日志 §4 |
| §4.4 历史 WP 状态 | 1615–1648 | WP0.5-A/B/C、WP1–WP4 当时的状态与剩余项（含 FORK/TAIL API 差异三点） | 已被 §0 现行表取代；全文见本文件 §J |

---

## 索引 C：计划书附录 D 的对应关系

§3 早期条目与计划书附录同源：**07-13→D-7、07-14→D-8 前置、07-15→D-8、07-16→D-9、07-17/-18→D-10、
07-19/-20/-21/-22→D-11、07-12→D-6**；附录 D-1…D-9 的逐字归档在
`./KVARN-PLAN-APPENDIX-ARCHIVE.md`，D-10…D-13 仍留在计划书正文。

---

## §A 逐字归档：§3 记录 2026-10-07-1 … 2026-10-07-23

<!-- 以下自「### 2026-10-07-1」起为源文件第 260–1312 行的逐字副本 -->

<!-- vbegin @@S3BODY@@ -->
### 2026-10-07-1 — 侦察与路线确认

- 确认 TAIL 工作树干净，HEAD = `c8d905ef`（"docs: archive the completed port records…"），
  前序提交含 `955ff44b`（加入 v3 计划）。
- 确认 TAIL `src/ops/kvarn/` **不存在**；FORK `D:\ninfer\ninfer-rtx5090-mobile\src\ops\kvarn\`
  有完整 12 文件（2980 行）。
- 确认 `outdate/kvarn-port-into-precision-tail-plan-review.md`（v2 审阅报告）在 `D:\ninfer\outdate\`。
- 计划书 §2 的路线结论（只有 R2 = 在 TAIL 内移植）与资产拓扑复核无误，继续按 R2 推进。

### 2026-10-07-2 — WP0：基线构建发现先存缺陷（与本工作无关）

- **整树 `cmake --build build-port -j` 不通过**。唯一失败目标 `tools/ninfer-multi-gpu-probe.exe`：
  `LNK2019` 未解析 `ninfer::core::current_resident_memory(void)`。
- **根因**：`d92f0abb`（`feat(cuda): add strict and mixed memory policies with 5070 ti docs`）
  令 `src/core/device.cu:68` 的 `cuda_check` 调用 `core::current_resident_memory()`（定义在
  `src/core/resident_memory.cpp:550`，声明 `src/core/resident_memory.h:83`），
  但 `tools/CMakeLists.txt:13-18` 的 probe **刻意只编 `multi_gpu_probe.cu` + `src/core/device.cu`、
  不链 `resident_memory.cpp` / `ninfer_core`**（其注释仍假设 "device.cu only includes core/device.h"）。
- **判定**：**先于本次工作存在的缺陷，与 kvarn 无关**。不影响 `ctest`（不需要该目标）。
  **不修**（超出本交付面），改用按目标构建绕过。
- 副作用：ninja 遇首个失败即停，所以整树构建会在该点中断，后续目标不会构建——**不是**编译错误。

### 2026-10-07-3 — WP1：kvarn ops 原样移植，零告警编译通过

- 复制 FORK `src/ops/kvarn/` 12 文件 + `include/ninfer/ops/{kvarn.h,kvarn_attention.h}` 入 TAIL 同路径。
- `src/ops/CMakeLists.txt` 增 `include("${CMAKE_CURRENT_LIST_DIR}/kvarn/sources.cmake")`。
- 构建 `ninfer_ops`：**零告警、零错误**，三个 TU（`attention.cu`/`codec.cu`/`decode.cu`）全部编过，
  `ninfer_ops.lib` 链接成功。⇒ **一手印证计划 §4.1「设备侧接口兼容」**：含 `launch.h`
  （FORK 127 行 / TAIL 142 行，`causal_attention_prompt_launch` 多一个必填 `bool fast`）与
  `small_t.cuh`（`causal_small_t_active_splits` 多默认参 `wave_splits=0`，且函数体新增钳制块）
  在内的差异面**均不影响 kvarn 编译**。
- CMake 源清单口径（FORK `sources.cmake`，仅 3 个 `.cu`）：
  `codec.cu`、`attention.cu`、`decode.cu`。

### 2026-10-07-4 — WP1：测试移植与首跑（30 项中 1 项容差未过）

- 复制 FORK `tests/ops/test_kvarn.cpp`（1710 行）→ TAIL `tests/ops/`，
  在 `tests/ops/tests.cmake` 注册 `ninfer_kvarn_test`（`LIBRARIES ninfer_ops`），
  位置与 FORK 一致（`linear_swiglu/tests.cmake` 之后）。
- **移植期唯一编译错误**：MSVC 无 GCC 内建 `__builtin_popcount`（11 处，均在主机 FP64 Hadamard
  oracle 中：`(__builtin_popcount((unsigned)(row & col)) & 1)`）。
  **修法**：全部替换为 C++20 `std::popcount` + 增 `#include <bit>`。语义等价（同一内建语义）。
  TAIL 既有同类用法：`src/runtime/engine/context_cache/resource_manager.h:2373` 用 `std::popcount`。
- 编译链接通过，`ninfer_tests.exe` 内注册成功（`build-port/tests/CTestTestfile.cmake` 有引用）。
- **实跑**（RTX 5070 Ti；采样确认独显显存 48 → 281 MiB、利用率峰值 ~10%）：

  ```
  KVarN K official oracle: relative_l2=0.00060882 limit=0.0003 max_abs=0.216104
  FAIL kvarn correctness
  ```

  `main()`（`tests/ops/test_kvarn.cpp:1633`）**累计约 30 项用例**（codec / hadamard / publication
  settlement / append-attention oracle / cache lifecycle / 27B / 27B grouped decode / 35B / prefill
  slab boundary / batched / tail staging / speculative boundary），全部跑完后统一报失败数。
  输出**只有上述 2 行** ⇒ **只有 1 项检查失败**。
- **性质判定（重要）**：紧随该检查之后的 `KVarN K stored-bit decode`
  （`test_kvarn.cpp:350`，限 **2.0e-7**）**没有报错** ⇒「设备反量化」与「用设备自己存的
  codes/scales/zeros 在主机重算的表示」在 2e-7 内一致 ⇒ **设备 codec 自洽**。
  故 6.09e-4 只能来自 **FP32 设备 Sinkhorn 与 FP64 主机 oracle 在 4-bit 量化桶边界的少量码翻转**，
  属**容差标定**问题（**WP5 职责**：`3.0e-4` 只对 4-bit 有效这一说法在本机不成立），
  **不是**接线/移植缺陷。
- **未隔离（如实记录）**：该漂移来自 MSVC libm（oracle 侧 `std::exp/log/sqrt`）还是
  nvcc 13.3 libdevice（device 侧 `expf/logf`），**尚未定位**；**亦未证明** FORK 在 Linux 上该项曾通过。
- **WP1 验收未完全达成**：`ninfer_kvarn_test` 通过这一条待 WP5 定容差后再判。
- **WP1 欠项**：kvarn bench（FORK `bench/ops/kvarn_attention_bench.cu`、`kvarn_codec_bench.cu`）
  未移植——需 `-DNINFER_BUILD_BENCHMARKS=ON` 重配，与 AGENTS.md「不要重配」冲突，**标为待决**。

### 2026-10-07-5 — WP0.5-C：构型口径声明（D6 决议）

- `build-port/CMakeCache.txt` 实测：`CMAKE_CUDA_ARCHITECTURES=120a`、`NINFER_SM120_NATIVE=ON`、
  `NINFER_BUILD_APPS=ON`、`BUILD_TESTING=ON`、`NINFER_BUILD_BENCHMARKS=OFF`。
- 由 `CMakeLists.txt:33-37`（`if(arch MATCHES ^(80|86|89)$ OR NOT NINFER_SM120_NATIVE)`）⇒
  **`NINFER_COMPAT_PATH=OFF`** ⇒ 本构建走 **native（本仓自述 unqualified）路径**，
  **不是** AGENTS.md 所说「120a 上已测的 `mma.sync` 兼容路径」。
- **取 native 口径**，理由：AGENTS.md「使用既有 `build-port`，**不要重配**」为硬约束，
  且重配是本机唯一会真正破坏构建的操作。⇒ **A1/A8 的噪声底、route 表、split-capacity 常量的
  判断一律属于 native 口径**，必须在结论里写明。
- **Open item**：compat 口径的噪声底**未测**（需一次重配 `NINFER_SM120_NATIVE=OFF`）。
- 文档陈旧：`AGENTS.md:34,37` 仍写 sm_86 / RTX 3090 / CUDA 12.8，与本机 sm_120a / 5070 Ti / CUDA 13.3
  矛盾（同文件「Windows build environment」节已自洽），归 WP9 统一订正。
- 结论已回写计划书 **附录 D-1**。

### 2026-10-07-6 — 建立本进度日志

- 依据用户指示：建立唯一进度落盘点（本文件），并把「信息必须落盘」写成规则记入计划书 §0.5。
- 规则要点：每个 WP 结束（含中止）必须 ① 更新 §0 快照、② 追加一条 §3 记录、
  ③ 影响验收口径的结论回写计划书。失败与未决结论**不得**省略。

### 2026-10-07-7 — 信息落盘制度写入 AGENTS.md（仓库级）

- 依据用户指示，把「信息落盘」从计划书级提升为**仓库级制度**，写入 `AGENTS.md`：
  - 新增 `## Durable progress record` 一节（5 条）：单一记录单一权威 / 失败与未决同样入账 /
    每个工作包边界更新（快照 + 日期条目 + 回写计划）/ 实测优先且被推翻的结论标注而非删除 /
    **日志未更新 = 该工作包未完成**。
  - `Reference navigation` 表中 KVarN 行更新为
    「`kvarn-port-into-precision-tail-plan.md`（active plan; §0.5 record protocol）、
    `kvarn-port-progress.md`（active progress record）」+ 原 archived records 保留。
- 位置：`AGENTS.md`「## Reporting and completion」与「## Reference navigation」之间。

### 2026-10-07-8 — WP0.5-A：MTP parity 测试移植完成；**实跑发现 A3 失败（非 kvarn 相关）**

- **移植**：FORK `tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（569 行）→ TAIL 同路径
  （557 行），并在 TAIL `tests/models/qwen3_5/tests.cmake:188-191` 注册
  `ninfer_qwen3_5_mtp_greedy_parity_real_test`（`LIBRARIES ninfer_engine`、`SKIP_RETURN_CODE 77`）。
  与 FORK 的**逐行 diff 尚未复核**（子代理按其自述做了 §4 所列 API 适配：
  删 kvarn profile、`mtp_draft_policy`→`mtp_policy`、删 `enable_nvfp4_scale_compression`/`compressed_scales`）。
- **构建：成功**。`ninfer_tests` 目标 193 项，`[191/193]` 编过该 TU、`[192/193]` 链接
  `tests\ninfer_tests.exe`（1,331,801,600 B），无错误无告警。
- **实跑：失败（真发现）**。命令：
  `NINFER_TEST_ARTIFACT=<本机模型> ./build-port/tests/ninfer_tests.exe ninfer_qwen3_5_mtp_greedy_parity_real_test`
  输出（`/tmp/run_wp05a.log`）：
  - `bf16` `spec=mtp` **k=0** repeat 0/1 → `matched 512 tokens` ✓
  - `bf16` **k=3** repeat 0/1 → `matched 512 tokens` ✓
  - `bf16` **k=1** → **`mismatch at token 91: expected=2466 actual=2640`** ✗（测试在首个失败处中止）
- **解读**：发生在 **纯 bf16**（与 kvarn 无关）⇒ 这是 **TAIL 固有的 MTP-on/MTP-off greedy 不一致**，
  被新移植的仪器**首次**暴露。k=3 过、k=1 不过，形态可疑（非"越大越差"单调）。
  ⇒ **A3 的仪器已就位，但 A3 本身目前不成立**。属计划 §7 WP0.5-A 的门禁发现，**须回写计划**。
- **待做**：`--no-cuda-graph` 复跑以分离"CUDA Graph 复用缺陷"与"数值/调度缺陷"（进行中）。
- **已做（诊断，决定性）**：`--kv-dtype bf16 --no-cuda-graph` 复跑 ⇒ **完全复现**同一失败：
  `bf16 k=1 ... mismatch at token 91: expected=2466 actual=2640`，且 `k=0`/`k=3` 仍全等。
  ⇒ **排除 CUDA Graph 复用**：这是**确定性**的 MTP 草稿/验证在 **k=1** 上的数值或调度分叉
  （同 token、同数值，与 graph 无关）。根因仍未定位（k=1 特异的路径值得优先查）。
- **未做**：与 FORK 的 diff 复核；`ctest -R` 复跑；根因定位。
- 说明：子代理已被主动停止（用户即将重启），未留下写了一半的文件——落盘文件已核对：
  MTP 测试文件 557 行、花括号平衡（131/131）、`main` 正常收尾；`tests.cmake` 注册完整。

### 2026-10-07-9 — A3 差分诊断（进行中）：上游 vs TAIL 的代码溯源

**动机（用户指示）**：在上游父仓库 `ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco`
（= 成品 `ninfer-package` 的源码）上测同一问题，判断 k=1 分叉是否为 precision-tail 移植引入。

**代码溯源（已做，一手实测）**

1. 上游确认是 precision-tail 的**父仓库**：commit `b06908ba`；`kv-tail-tokens` 全库 **0 命中**；
   无 MTP parity 测试、**无 build 树、无 `.deps`**（无法直接构建；`dist/` 只有 README）。
2. **MTP/投机实现 20 个文件两仓逐字节相同**（`execution/mtp.*`、`load/mtp.cpp`、
   `program/speculative/*`、`ops/{kernel,launcher,wrapper}/*mtp*`、`*speculative_round*`、
   `include/ninfer/ops/{mtp_pack,mtp_round,speculative_round}.h`、`product/speculative_options.h`）
   ⇒ **MTP 代码本身未被 precision-tail 改动**（即使有 bug 也是继承来的）。
3. 但 **MTP 窄宽度验证所走的 attention 路径被改过**。`.../dense/causal_cache/` 目录实测：
   - TAIL 独有文件：`small_t_tail.cuh`、`small_t_tail_shadow.cuh`（precision-tail 新增）
   - 上游独有：无
   - 共同但**内容不同**：`prompt.cu`、`small_t.cu`、`small_t.cuh`、**`small_t_bf16.cuh`**、
     `small_t_i8.cuh`、`small_t_i8_launch.cuh`
4. **`small_t_bf16.cuh`（正是本次失败的 bf16 路径）被实质修改**：13 删 / 33 增，5 个 hunk：
   - 函数签名新增形参 `std::int32_t tail_tokens`
   - `active_split_count` / `window` 的算法**整体替换**为
     `causal_small_t_tail_partition<...>(window, tail_tokens, split_count, TokenTile)`
   - 新增 `append_start/append_end`：**最后一个 split 的 cache 写入范围改为 `[split_start, window)`**
     （上游是 `[split_start, split_end)`）
   - **空 split 的提前返回被移到 cache 写入与 `__syncthreads()` 之后**（上游在之前）
   - 注释自称"with no tail (tail_tokens == 0) this is … exactly as before"——
     **该"零变更"断言在 append 范围与提前返回位置上并不显然成立**
5. `small_t.cu` 亦改（11 删）：新增 tail 启动路径，并把 `wave_splits` / `tail_tokens`
   穿透进启动宏与 partial 核调用。
6. `small_t.cuh` 是**纯新增**（48 增 0 删，新增 `CausalSmallTTailPartition` +
   `causal_small_t_tail_partition`）；`wave_splits` 上游本就有（6 处），非本仓新增。
   ⇒ 共享 reducer / `causal_small_t_active_splits` 未改。
7. `causal_softmax_attention.cpp`、`launch.h`、`geometry.cuh`、`prompt_common.cuh`、
   `paged_kv_address.cuh` 两仓**相同**。

**当前假设（待实测裁决）**：k=1（宽 2）的 bf16 MTP 验证走 SmallT，而 precision-tail 重构了
`small_t_bf16.cuh` 的 split/append 结构；作者认为 `tail_tokens=0` 时等价，但**实测在 tail=0
（本测试未设尾部）下 k=1 与 k=0 仍分叉** ⇒ 该"零变更"断言疑似被违反。
**若成立**，这是 precision-tail **移植引入**的回归，且直接冲击其自身"tail=0 逐位不变"的承诺（A1 同族要求）。

**实测（进行中）**：两包各自的 `ninfer-serve.exe`（上游 `ninfer-package\engine`，2026-10-02；
TAIL `ninfer-precision-tail-package\engine`，2026-10-06）跑**同一模型**
（`ninfer-precision-tail-package\model\...IQ3_XXS-vision-bf16-mtp.ninfer`；上游包 model 目录为空），
对每台引擎做 **MTP off vs `--spec mtp --draft-tokens 1`** 的 `--greedy` 同请求对照。
结果见下条。

**注**：本条的代码溯源是**静态证据**，不能单独定论（共同但不同的文件里也可能有与本路径无关的改动）；
决定性的是上面的活体差分。

### 2026-10-07-10 — A3 差分实测：**上游同样失败 ⇒ 缺陷先于 precision-tail 存在**（推翻 07-09 的假设）

**方法**（子代理执行，`/tmp/mtpdiff/` 留有全部原始 JSON 与日志）：两包各自的 `ninfer-serve.exe`
跑**同一模型**、`--greedy`、`port 8111`、`max-context 2048`、`default-max-tokens 512`、`prefill-chunk 1024`；
每台引擎各跑 **MTP off** 与 **MTP on（`--spec mtp --draft-tokens 1`）**，请求体完全相同
（model id = `Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp`，prompt = 测试同款，`temperature 0`）。

**结果（决定性）**

| 对照 | 结果 |
|---|---|
| TAIL：MTP off vs on | **分叉**，首个不同字符 **index 538**（off 走 curses 方案 / on 走 tkinter 方案），1996 vs 2007 字符 |
| 上游：MTP off vs on | **同样分叉，同 index 538，同文本** |
| `up_off` vs `tail_off` | **逐字节相同** |
| `up_on` vs `tail_on` | **逐字节相同** |
| `--max-context 768`（测试同档） | 四个输出与 2048 档**逐字节相同** |
| `--draft-tokens 3` | **两仓都分叉**（char 1491） |
| MTP-on 复跑 | 逐字节相同 ⇒ **确定性** |
| 投机计数 | 两仓完全一致（draft_n=280 / accepted=231；k=3: 555/326），`ngram_drafted_tokens=0` |

**裁决：上游（`ninfer-package` @ `b06908ba`）同样存在该分叉，且与 TAIL 输出逐字节相同
⇒ `--kv-tail-tokens` / `--kv-tail-type` 只是新增开关（默认关），precision-tail 移植
没有引入这个差异。这是继承自上游的既存行为/缺陷。**

**⇒ 明确推翻 §3-2026-10-07-9 的静态假设**（"`small_t_bf16.cuh` 的重构违反了 tail=0 等价不变量"）：
上游用的是**未改动**的 `small_t_bf16.cuh`，却产生**逐字节相同**的 MTP-on 输出
⇒ 该重构在本场景下**行为等价**（作者的"tail=0 exactly as before"断言在此成立）。
**保留原条目不删**，按落盘规则以本条为准。

**性质仍未定（重要）**：两仓"draft/accepted 计数相同但提交 token 不同"，更支持
"验证比较在近似并列处 argmax 翻转（数值/路由差异）"，而非"验证逻辑写错"。但
**"这是 bug" 还是 "parity 从来就不是该实现承诺的契约" 仍未判定** —— 这现在是**上游问题**，
与 kvarn/precision-tail 正交。

**保留**：无 token id 暴露（`--request-log-jsonl` 只记计数），故**首个不同 token 的精确下标未测到**；
char 538 ≈ token 138，与测试的 token 91 不同 —— 两者 prompt/采样基准不同，**只有差分结论是承重的**。

### 2026-10-07-11 — A3 上游调研（GitHub + 三仓代码/文档）：**不是上游 bug，而是 Fork B 私有合同**

> **完整报告已落盘**：`docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md`
> （体例随 `PORT-E8-INVESTIGATION.md`；含逐条引文、`文件:行` 证据、复现命令、处置选项 O1/O2/O3、边界）。
> 本节保留要点，细节以报告为准。

**方法**：代理 `127.0.0.1:7897` 联网查 `Neroued/ninfer` 的 issue/PR/release/discussion（枚举了
**全部 370 个 issue+PR**，非抽样）；本地同时核对上游克隆 `D:\ninfer\ninfer`（`master` = `68c54356`
@2026-10-05，`origin/dev` = `070fa61a` @2026-10-06，已 `git fetch`）与三仓文档/代码。

**① 上游是否记录了这个问题：部分记录（同族机制），未记录本症状**
- **issue #265**（**open**，2026-09-16 由 `cometkim` 提，**0 评论、timeline 空 ⇒ 维护者从未回复、无修复引用**）：
  "The GDN input projection record pass (the one that runs at **verify widths >= 2**) … computes each
  column differently from how the width-1 decode pass computes it"；实测 layer-0 GDN 输出
  **0.72% 相对分歧** ⇒ 最终隐态 **~3.5%** ⇒ **翻转约 5% 的 argmax 决策**。与我们的"同计数不同 token"同族。
- **#80**（closed 2026-08-27，**由维护者 Neroued 关闭**）原文：**"Not in scope. Greedy does not mean
  batch invariant. And batch invariant limits optimizations."**
- **PR #220**（closed unmerged 2026-09-10，Neroued）："the Op contract explicitly allows private
  convolution intermediates to remain unrounded. Exact equivalence is required only for ReplaySSM
  record/fold versus the corresponding full computation."
- **#349**（closed 2026-10-01，`not_planned`，提交者自行关闭）：accept/reject 判据在 `proposal_q`
  miss 路径上的过接受问题；修复**只存在于某第三方 fork** 的 `350136a8`，**未进 dev/master**。
- **反向先例（保持公允）**：**#105** 确实是"改了 greedy 输出"被真实测试抓到并由维护者修复
  （`fd48e2fa`，prefill 分块数值），⇒ 上游**会**修真实回归，只是不认跨宽度逐位等价。
- **未查到的（0 命中，属实质负证据）**：无任何标题/正文含 `parity` 的 issue；无中文相关词（投机/不一致/分叉）；
  无 `token 91`/`2466`/`2640`/`5070 Ti` 命中；**仓库无 release、无 tag** ⇒ 不存在"某版已修复"。

**② 是否被修复：未修复**（#265 仍 open；`gdn_input_record_schedule`/`decode_order` 全库 0 命中；
`compare 68c54356...070fa61a`：dev 领先 23 个提交，全是 xgrammar/GBNF/线性形状工作，**无一触及 GDN 输入投影**）。

**③ 但 #265 的机制不是本机的机制（一手代码核实，重要）**
- 本机模型是 **IQ3_XXS GGUF 权重** ⇒ `gdn_input_proj` 只在 `qtype==NVFP4` 时才走 nvfp4 宽度表
  （`src/ops/wrapper/gdn_input_proj.cpp:603`）；IQ3_XXS 走 `gguf_project`
  （`gdn_input_proj.cpp:1336-1347` snapshot / `:1439-1443` record，**同一个** `gguf_gdn_project`），
  其唯一宽度键是 `t <= kMaxVectorColumns(=8)`（`src/ops/linear/gguf/gguf_linear.cpp:183`、
  `ggml_bridge.h:49`）⇒ **宽度 1/2/4 落同一个 MMV kernel**（`ggml_vec_iq3_xxs.cu`），**根本不查宽度表**。
  另：上游与 FORK B **完全没有 GGUF/IQ3 代码**（`grep -rl IQ3_XXS` 两仓 0 命中）⇒ 线性层数值是 TAIL 特有面。
- **本机真实机制**：bf16 causal-cache **SmallT attention 的 CTA 内归约形状随 verify 宽度改变**——
  `src/ops/softmax_attention/dense/causal_cache/small_t.cu:383-395` 实测 `case 1: DISPATCH(1,2)` /
  `case 2: DISPATCH(2,4)` / `case 4: DISPATCH(4,4)`（第二参 = `WarpsPerCta`，`Br=Wc*16`，
  `small_t_bf16.cuh:29-31`）；且 decode 用 cached/unmasked 入口（`execution/text.cpp:571`），
  verify 用 batched/masked 入口（`text.cpp:965`）。bf16 的 **split 数只由 window 决定、与宽度无关**
  （`small_t.cu:67-85`；宽度相关的 `narrow=tokens<=5` 仅 `batch_size>1` 生效，`:274`）
  ⇒ 跨 split 归约树相同，分歧在 **CTA 内** ⇒ near-tie argmax 翻转。
- **k=3 通过是运气**：宽度 4 用 `<_,4,4>`，与宽度 1 的 `<_,1,2>` **不同**，不存在"宽度 4 恰好等价"的性质；
  #265 也从未主张宽度 4 与宽度 1 轨迹一致。**没有任何上游 issue/PR/文档解释 k=1 失败而 k=3 通过**。
- **没有可拨的开关**：`NINFER_LINEAR_ROUTES`/`NINFER_GDN_TWO_STAGE`/`NINFER_SMALLT_PV_F16`/
  `NINFER_PROMPT_*`/`ProposalHead`/`mtp_attention_window` 均不能把宽度 1 算术变成所有宽度的规范算术。

**④ 上游合同 + TAIL 自身合同：都明确不保证 A3（决定性）**
- 上游/TAIL/FORK B 三仓**逐字相同**的 `docs/maintainer/qwen3_5-model.md:328-330`：
  "**This does not impose token or logits equality between different quantization, prefill or kernel paths.**"
- `op-development.md:397`："**Re-running a shorter projection can choose different arithmetic and is
  not an equivalent reference**"；`:218-223` 合同"must not freeze bitwise equality unless an exact
  semantic format requires it"。`replayssm-gdn.md:457-460` 把 exactness 限定在"同一物理 verify block"，
  并说 bitwise clone 验证时"最终文本或 BF16 output parity 都不够"。`dflash.md`：不同 block 宽度
  "their logits need not match one another"。`engine-architecture.md:125-126`：投机路径数值按 **Op oracle**
  验证，而非按等值验证。**上游 master/dev 均无任何真实模型 MTP greedy parity 测试**（只有
  `test_mtp_pack/test_mtp_round/test_speculative_round` 三个 op 测试），且 `tests/README.md:288-289`
  自陈 fixture "does not define bit parity across arbitrary floating-point routes"。
- **TAIL 本仓早已实测并写下相反判据**（`docs/performance.md:29-45`，2026-09-09，23 构型 × 3 重复，
  `scripts/sweeps/dflash2-draft-tokens-realtext.ps1` 的 `content_sha256`）：
  "**Speculative decoding is not bit-identical to non-speculative decoding here, it is not required to
  be**"；"every configuration reproduced its own output exactly, **23 of 23**"；
  "**the width-1 greedy path produced a hash matched by no speculative configuration**"；
  "Verification evaluates k+1 columns in one pass … reductions run in a different order and **a near-tie
  argmax can flip** … **what is guaranteed is per-configuration determinism, not cross-configuration
  equality**"。并指出 bit-identity 需要每轮都用宽度-1 kernel 算被接受列 ⇒ **正是投机要省掉的开销**，
  且 perplexity 12 位有效数字不变（无质量代价）。
- `docs/archive/TODO.md:4462-4464`（同仓既有条目）：**"A test asserting bit-identity to greedy would
  fail permanently by design, which is worse than no test."** 推荐保证项改为
  ①self-determinism ②quality parity ③documented divergence。
  ⇒ **A3 与本仓既有书面裁决直接冲突**；先前 §3-2026-10-07-8 把它当"新发现的 TAIL 固有缺陷"是**误判**，
  本仓文档早已预见该形态。

**⑤ A3 的真实出处：FORK B 的私有合同，且以其自有 kernel 改动为条件**
- FORK B（`D:\ninfer\ninfer-rtx5090-mobile`，Mirko Covizzi）引入的使能改动：
  `7d566547`(2026-08-23) `fix(mtp): make greedy verification width-invariant`（31 文件 +342/−117，
  **首次加入 parity 测试**，把 bf16 宽度 switch 钉成 `kTokenTile=1; kWarpsPerCta=2`）；
  `1388b7c2`(08-29)；`114b0fcb`(09-05) `fix(kvarn): preserve greedy parity across speculative decode`；
  `d476fafa`(09-19) 加入 `small_t.cu:230-233` 强制 `tokens=1; batch_size=1` 与
  `softmax_attention.h:129-131` "**canonical per-query arithmetic profile**"；`dff96dca`(09-22)；
  `16e12737`(10-04，仅 NVFP4 scale-compression 用例)。
- **TAIL 只含上游的 `a7818988`（Neroued 本人 `perf/qualify variable-width causal cache attention`）**，
  它正是宽度特化 switch 的源头；`7d566547/d476fafa/dff96dca/114b0fcb/1388b7c2` **在 TAIL 中不是对象**
  （不在提交图内）。⇒ **移植的测试带来的是 Fork B 的合同，TAIL 从未承诺它**。
- **代价**：若要满足 Fork B 合同，需把 canonical profile 移植进 TAIL——5 个核心文件约 +380/−723
  （`small_t.cu` 155/264、`small_t.cuh` 15/97、`small_t_bf16.cuh` 105/110、`causal_softmax_attention.cpp`
  95/230、`launch.h` 7/22）+ `1388b7c2`/`114b0fcb` 的 linear_add/gdn/kvarn 片段约 +1200/−570；
  且 TAIL 自己的 `small_t_tail.cuh`/`small_t_tail_shadow.cuh`/`small_t_bf16.cuh` 改动是**宽度特化**的，
  **必须重算而非合并**。FORK B **从未基准测试** canonical vs 宽度特化的开销（其 `docs/performance.md:387`
  只是"preserves canonical-column BF16 attention"即拒绝上游 schedule）；这与 #80 维护者
  "batch invariant limits optimizations" 的立场相悖 ⇒ **性能风险未量化，属实质反对证据**。
- **FORK B 合同的内在矛盾**：其 `docs/maintainer/qwen3_5-model.md:269-272` 声称
  "the fork requires exact committed-token parity between ordinary greedy decode and MTP draft
  windows 1..15"，**同一文件 `:358` 却保留了上游的**"This does not impose token or logits equality…"；
  其 `paged-kv-cache.md:188` 自陈"reproduced cross-backend divergence at **output index 357**"。
  且该保证从未在 IQ3_XXS GGUF 上验证（FORK B 无 GGUF 代码，fixture 是 NVFP4/QUASAR 产物）。

**裁决（本轮结论）**
1. **A3 不是上游缺陷，也不是 precision-tail 移植回归**；上游对"跨宽度逐位等价"**明示拒绝且不修**（#80/#220 + 文档）。
   先前 §3-2026-10-07-9 的"small_t 重构破坏 tail=0 等价"假设已被 §3-2026-10-07-10 实测推翻，本轮进一步确认
   **连"这是缺陷"的定性也不成立**。
2. **#265 是真实、开放、未修的同类缺陷**（GDN 输入投影按宽度换 schedule），但**不在本机 IQ3_XXS 路径上**；
   若将来改用 NVFP4 产物（kvarn WP 系列可能涉及），#265 会**直接命中** ⇒ 需重新评估。
3. **WP0.5-A 的验收方式必须改**（计划书 §1 A3 / §7 待用户裁决后再回写）：建议判据替换为
   **①同构型自确定性（已实测成立：k=0 重复全等；上游 MTP-on 重复逐字节相同）②perplexity 质量一致
   ③分叉已文档化**，并保留**④"kvarn 不得使 MTP 一致性劣于同配置基线"**作为 kvarn 专属门禁。
   原 `test_engine_mtp_greedy_parity_real.cpp` 应改造为**自确定性 + 质量**断言，
   或降级为**记录首个分叉下标与 perplexity 差**的诊断工具；**不可**作为通过/失败门禁保留绝对逐位断言。
4. **不阻塞 kvarn 移植**：kvarn 的 decode/speculative 路线依赖的是"同配置可复现 + 质量不劣化"，
   而非"跨配置逐位相等"（后者上游与本仓均已书面否认）。

**本条未做（如实记录）**：未在本机跑 perplexity 三方对照以量化 k=1 分叉的质量代价（引用
`docs/performance.md` 的"12 位有效数字不变"是**本仓既有实测**，非本轮）；未复现 #265 的 nvfp4 路径；
未向 #265 提交上游 issue（本轮为调研，**未做任何网络写操作**）。
**联网方式**：`export HTTPS_PROXY=http://127.0.0.1:7897` + `gh` v2.97.0 / `curl --proxy`；
本地 `git fetch --all --prune` 只更新远端 ref，**未动工作树与 HEAD**。

### 2026-10-07-12 — A3 定案：报告复核 + 计划调整（**回写计划书 v4**）

**触发**：用户指示审阅 `docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md`，确认三仓之间该问题的
本质，并据此调整工作计划。

**主代理逐条独立复核（非转述）——报告的 C1–C9 全部通过**

| 论点 | 复核手段 | 结果 |
|---|---|---|
| C1 上游无对应 issue | `search/issues` 标题 `parity` / `MTP greedy` | 各 **0** ✓ |
| C2 #265 同族、open、无人回应 | `gh api .../265` + 正文 | open / 0 评论 / `cometkim` / 2026-09-16；引文逐字命中 ✓ |
| C3 未修复 | `releases` / `tags` | 各 **0** ✓ |
| C4 #265 不命中本机 | `gdn_input_proj.cpp:603` 门在 `NVFP4`；GGUF 宽度键 `kMaxVectorColumns=8`；`IQ3_XXS` 命中 TAIL 8 / 上游 0 / Fork B 0 | ✓ |
| C5 本机机制 | `small_t.cu:383-395`（`case1→(1,2)`/`case2→(2,4)`/`case4→(4,4)`）+ `kBlock=32*WarpsPerCta` | ✓（**但为静态结论，见下**） |
| C6 上游不认为是缺陷 | `#80` `closed_by:Neroued`、评论文本 | 逐字命中 "Not in scope. Greedy does not mean batch invariant." ✓ |
| C7 A3=Fork B 私有合同 | 6 SHA 在 Fork B=`commit`/TAIL=`fatal`；`a7818988` 在 TAIL=`commit`；Fork B `small_t.cu:280` 单参宏 + "canonical" 注释；测试 `:233-234` 逐字 | ✓ |
| C8 TAIL 早已书面否决 | `docs/performance.md:29-45`、`docs/archive/TODO.md:4462-4464` | 逐字核过 ✓ |
| C9 不阻塞 kvarn | 判断性结论，与已核实事实一致 | ✓ |

**复核发现的边界/瑕疵（如实记录）**
1. C5 的**机制判定是静态代码结论，未经运行时确认**（报告 §10.2 自陈）；决定性实验（把 `case 2` 改成
   `<2,2>` 后看 k=1 是否转全等）**未做**（属代码改动，需授权）⇒ C5 应读作"代码结构上唯一合理的解释"，
   **非已证事实**。
2. 报告 §5.3 引用的 `docs/maintainer/paged-kv-cache.md:188` 实为 **Fork B** 的文件（TAIL 无该行）；
   已在报告加 "(Fork B)" 标注。

**裁决：采纳 O1；拒绝 O2；O3 次优。**（理由与证据见计划书 **附录 D-6**，本节不重复。）

**本轮已落盘的文档调整（唯一产物）**
- `kvarn-port-into-precision-tail-plan.md`：**v3 → v4**；新增「v4 相对 v3 的变更摘要」；§1 **A3** 改为
  相对判据（原"绝对逐位"标**已废弃**）；§7 **WP0.5-A** 改为"仪器交付 + 改造为诊断"、WP3/WP9 验收同步；
  §9 风险表 A3 行标**已定案**；附录新增 **D-6**（复核裁决全文）。
- `docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md`：抬头"已回写"、§9 标"O1 已采纳"、
  §5.3 加"(Fork B)"标注。
- 本文件：§0 快照（WP0.5-A 行、未决项 5、工作树清单）+ 本条。

**对 kvarn 移植的影响**：**不阻塞**。下一步 **WP2（枚举 + 页几何）与 A3 无关，可立即开工**。

**未做（如实记录）**：本轮为**审阅 + 文档回写**，**未跑 GPU**，**未做任何网络写操作**（`gh` 仅只读查询）；
C5 的决定性运行时实验**未做**；MTP 测试改造为诊断仪器（去掉逐位断言）**尚未动代码**，归入 WP3 前后。

### 2026-10-07-13 — **WP2 完成**：页几何 + 存储枚举 + parser/名字/指纹；**261 测试全量回归 + 基线对照**
（**回写计划书 §1 A1 / §4.4 / §7 WP2 / §9 / §10 D1·D2 / 附录 D-7**）

**实现（16 文件，逐点如下；详见 §0「工作树改动清单（WP2）」）**

1. `include/ninfer/types.h`：`KvCacheStorage` **末尾追加** `KvarnGroup128`（遵守 `:67` 不重排约定）；
   新增 `enum class KvarnBits : uint8_t { Bits4=4, Bits5=5, Bits6=6 }`；`EngineOptions` 增
   `KvarnBits kvarn_bits = KvarnBits::Bits4`（D1 决议：**单枚举 + profile 字段**）。
2. 三处 `--kv-dtype` parser（serve / cli / perplexity）接受 `kvarn`（**裸别名 ≡ k4v4**，D2 决议）、
   `kvarn:k4v4|k5v5|k6v6`；写 `KvarnBits`。`ServeOptions`/`Options` 各增 `kvarn_bits`；两处
   `engine_options.kvarn_bits = ...` 接线。
3. 6 处名字 switch（3 生产 + 3 基准）增 `KvarnGroup128` case（`"kvarn"`）。
4. `model_instance.cpp` `hybrid_cache_fingerprint` 追加 `;kvbn=<bits>`（对照 `;kvtt=`）。
5. `paged_kv_cache.h` 增 `kKvarnPageTokens=128`、`kv_page_tokens(storage)`、`kv_page_shift(storage)`；
   `paged_kv_cache.cpp` 的 `validate_geometry` **放宽为 64 或 128**，两处 plane 形状与形状校验改用
   `spec.geometry.page_tokens`（原来硬写 `kPagedKVPageSize`）。**对既有格式 `page_tokens` 仍恒为 64
   ⇒ 行为逐位不变。**

**构建**：`env-port.bat` + `cmake --build build-port --target ninfer_ops -j4`（**0 错 0 新告警**）→
`ninfer_tests ninfer-serve ninfer ninfer-perplexity -j8`（exit 0）。

**功能检查（CLI，无需模型/GPU）**：`ninfer-perplexity bogus.ninfer --kv-dtype kvarn:k7v7` →
`--kv-dtype must be ... kvarn:k4v4, kvarn:k5v5, or kvarn:k6v6`（**拒绝**）；`kvarn:k5v5` 与裸 `kvarn` →
**通过解析**（随后因缺 `--corpus` 报错）⇒ 三档解析路径成立。

**回归①（合成，`ctest -E real -j4`）**：**261 注册测试中的 245 项合成测试，242 通过 / 3 失败**。
三项失败**全部经基线对照证实为先前存在**：

| 失败 | 消息 | 基线（stash WP2 代码重建后同测） |
|---|---|---|
| `ninfer_device_sync_empty_test` | `invalid sync setting did not fail before CUDA initialization` | **同样失败** ✓ 先前存在 |
| `ninfer_gdn_gating_proj_test` | `qwen3_6_27b/35b: GDN control interval missed a route endpoint` | **同样失败** ✓ 先前存在 |
| `ninfer_kvarn_test` | `K~oracle relative_l2=6.09e-4 limit=3.0e-4`（WP1 已知，归 WP5） | **同样失败** ✓ 先前存在 |

**基线方法（可复现）**：`git diff -- <16 文件> > /tmp/wp2-changes.patch`；`git stash push -m wp2-code -- <16 文件>`
（**只 stash 代码文件，保留 WP1 未跟踪文件与文档**）；`cmake --build … --target ninfer_tests -j8`；
`ctest -R <该测试>`；逐项比对后 `git stash pop` 并 `diff -q` 校验恢复与备份**逐字节相同**。
`device_sync_empty` 与 `gdn_gating_proj` 的失败消息**与 WP2 前逐字相同**；二者的失败点
（`src/core/device.cu:31-36` 的 `NINFER_CUDA_SYNC` 解析；`gdn_gating_proj_workspace_capacity_bytes`
的 route 区间）**均不在 WP2 改动面内**。另注：`ctest` 的 `ENVIRONMENT "NINFER_CUDA_SYNC="`
（`tests/cmake/CoreTests.cmake:82-83`）**未把空串交给 `getenv`** ⇒ 该项在本机 **恒红**，属工具/环境行为。

**回归②（真实模型，`ctest -R real -j1`，`NINFER_TEST_ARTIFACT=<本机唯一 27B 产物>`）**：
16 项中 **10 通过 / 6 失败**，**无一项可由 WP2 解释**：

| 失败 | 实测消息 | 性质 |
|---|---|---|
| `prefix_real` | `registered tokenizer/chat template has no prompt golden: model=Qwen3.8-27B-…` | 该模型无已注册 golden（fixture） |
| `hybrid_prefix_real` | `Qwen3.5 config: missing component dflash2` | 产物缺 DFlash2 组件 |
| `dflash2_real` | `Qwen3.5 config: missing component dflash2` | 同上 |
| `dflash_real` | `FATAL: Qwen3.5 config: missing component dflash` | 产物缺 DFlash 组件 |
| `moe_real` | `35B Engine construction has an invalid load summary: target=Qwen3_5ForCausalLM` | 本产物不是 35B MoE |
| `mtp_greedy_parity_real` | `bf16 k=1 sample=0 prompt=68 mismatch at token 91: expected=2466 actual=2640` | **已知 A3**（与 D-4 基线逐字相同） |

> **注（修正上一轮的一次误读）**：首次 `ctest -E real` 曾报 5 项 `Not Run` + 2 项 Python 失败
> （`chat_templates`、`artifact_writer_interop`）。**根因是当次只构建了 `ninfer_tests` 目标**，
> 而 5 个 `STANDALONE` 测试是可执行文件目标（`ninfer_jinja_test.exe` 等，`build-port/tests` 下当时
> 只有 `ninfer_tests.exe`）；补建这 5 个目标后 **7 项全部转通过**。⇒ 该 7 项**不是**回归。

**结论**：**WP2 未引入任何新失败**；「非 kvarn 格式 ctest 全绿」在本机应读作「**除 2 项先前存在的
本机失败 + 1 项已知 kvarn 容差外全绿**」（已回写 §1 A1）。

**未做（如实记录）**：① kvarn **档位尚不可运行**（`plan_cache` 仍对 kvarn 抛，见 §0 未决项 6），本轮
只做枚举/几何/解析/指纹，**未接 route、未注册两处 host switch**（属 WP3）；② `--kv-dtype` 的 `--help`
与 `docs/` 未改（不宣传不可用功能）；③ **真实模型测试未做基线对照**（消息均指向产物缺件/无 golden，
结构性）；④ 未做 `--kv-dtype kvarn:*` 的端到端跑（不可运行）；⑤ 未新增 kvarn parser 单元测试（归 WP3）。

**下一轮（WP3 起手）**：`plan_cache` 的 kvarn 单 U8 plane 分支（`{RecordBytes/Group, kv_heads, 256}`，
绕过 `paged_kv_storage_layout()`）+ 两处 host switch 注册 + `small_t.cu:460-505` body 挂载 + `kvarn`
可跑到 k4v2 的端到端；随后 WP4 位宽参数化。**注意 §4.4 陷阱的实测修正（见计划书 WP2 行）**。

### 2026-10-07-14 — WP3 起手：模型接入（`kvarn:k4v2` **短上下文端到端跑通**；长上下文阻塞已精确定位）

**触发**：用户指示继续推进（独显已空闲可用）。子代理并行侦察 FORK-B 接入面（省上下文），主代理落地 + 验证。

**侦察的关键修正（推翻计划书 §7-WP3 的措辞）**：FORK-B **不在** `causal_softmax_attention` 路由或
`small_t.cu` body 挂载 kvarn——它在**模型执行层** `src/models/qwen3_5/execution/text.cpp` 的**每个注意力
调用点**用 `if (storage == kvarn) ops::kvarn_attention(...) else ops::causal_softmax_attention(...)` 分派；
**`small_t_kvarn` 在 FORK-B 根本不存在**（是计划书的设想）。取证方式：把 FORK-B 作为**本地只读远端**
`forkb`（`D:/ninfer/infer-rtx5090-mobile`；merge-base `f76e19c0`，base→FORK 仅 50 提交）fetch 进本仓做跨仓
diff——**未做任何网络写操作**。

**实现（14 文件；清单见 §0「工作树改动清单（WP3）」）**：状态层 kvarn 单 U8 记录平面 + sink/tail 尾槽 +
`kvarn_layer_view`/`kvarn_batch_layer_view`/`reset_kvarn_tail_row`；CLI 内部档 `kvarn:k4v2`；规划页几何
（`kv_page_tokens` 贯穿 startup）；`text.cpp` **5 处**派发 + MTP provisional；`decode.cpp` group 钳制。

**关键设计决议**

1. **只发布 `kvarn:k4v2`（临时内部档）**：ops 仍是发布版 `kvarn_k4v2_g128` 的固定 K=4/V=2，故
   `KvarnBits` 增 `Bits2`；`kvarn`/`kvarn:k4v4|k5v5|k6v6` 由 parser **明确拒绝**，避免“要 k6v6 却实跑
   k4v2”的静默错标。**这临时偏离 WP2/D2 的“裸 `kvarn` ≡ k4v4”**——D2 的目标档要等 K=V 实现（WP4）才成立。
2. **两处 host 硬抛以「显式分支 + 守卫」替代「注册」**：`plan_cache` 对 kvarn 直接绕开
   `d256_kv_cache_profile`/`paged_kv_storage_layout`；`layer_rank`/`layer_view` 对 kvarn 短路/抛。
   理由：给 kvarn 伪造一个 D256 profile 会**静默算出错的平面几何**，比 fail-fast 更糟。
3. **单 rank 约束**：kvarn 尾槽是单块 rank-0 分配，故要求所有 full-attn 层同 rank（多 rank 抛）。

**验证**

- **构建**：`ninfer_tests ninfer ninfer-serve ninfer-perplexity` **全绿**（0 error 0 新告警；复跑
  `ninja: no work to do`）。改 `include/ninfer/types.h` 触发全量重建（~240 步，CUDA 为主，约 20 分钟）。
  中途发现并修正一处作用域错误（`page_tokens` 在 workspace 规划函数内未声明）。
- **CLI 门禁（逐字实测）**：`kvarn` → “KVarN K=V profiles are not implemented yet; use --kv-dtype
  kvarn:k4v2”；`kvarn:k4v2` → **通过解析**；`kvarn:k7v7` → 列出合法值；`kvarn:k6v6` → 拒绝。
- **定向回归（非 kvarn 逐位不变）**：`ninfer_qwen3_5*` **18 项全通过**（`*_real` 无 artifact 跳过）；
  `cli_options`/`serve_options`/`kv_cache`/`kv_cache_append`/`paged_kv_window`/`state_image`/
  `state_image_layout`/`speculative_round` 均通过。**未跑全量合成 245 项**（单项可达 ~220s，全套预计数小时；
  WP2 已立基线；本轮全部改动经 `kv_page_tokens==64` 分支或 `kvarn==` 守卫，非 kvarn 路径逐位不变）。
- **端到端（真实 27B 产物，本机单卡）**：
  - 短（28 tok 打分）：`kvarn:k4v2` **PPL 11.1801 / 384.8 tok/s** vs `rk4v4` **PPL 11.2210 / 164.4 tok/s**
    ⇒ **kvarn 路径确实被执行**（若落到共享 attention，`validate_cache` 会对 kvarn 抛），数值合理（28 tok
    落在噪声内，不构成质量结论）。
  - 长（4900 tok）：`rk4v4` 成功（PPL 1.0375 / 1170 tok/s）；`kvarn:k4v2` **失败**：
    `scoring long.txt window 0 failed: text KV address space has no free active entry` ⇒ **页几何核算不一致**
    （未决项 8），**非** kernel/数值错误。

**未做（如实记录）**：① **未回写计划书**（计划书 §7-WP3 的「`small_t.cu:460-505` body 挂载」措辞与实测不符，
须改）；② state_image/续列尾未实现（未决项 9）；③ 地址空间页几何未改（未决项 8）；④ kvarn 与
`--mtp-attention-window` 的规划期拒绝未加（未决项 10）；⑤ `--help`/`docs/` 不宣传 kvarn（保持 WP2 决定）；
⑥ kvarn parser 单测未加；⑦ **未跑 MTP 路径**（perplexity 只走 prefill ⇒ `target_verify_batch_impl`/
`decode_mtp_batch`/`mtp.cpp` 的 provisional 分支**未被激励**）；⑧ 未做长文的质量对照矩阵。

### 2026-10-07-15 — WP0.5-B 起手：**首个 KVarN 准入 KLD 数据点（正向）**；但准入门被 **WP3 页几何**前置

**触发**：用户指示据记忆文档评估「是否还有必要继续推进」，独显空闲可用。

**仪器**：`ninfer-perplexity --score-width 8 --score-topk 100 --kld-base`（decode-width，`--score-width<=8`
会合并精确 KV 尾部，即计划 A4 要求的仪器），本机唯一 27B 产物，base 一律 `--kv-dtype bf16 --kv-tail-tokens 0`。

**关键障碍（实测）**：`--kv-dtype kvarn:k4v2` 在 **`--corpus … --quick`（ctx 8192 / disjoint，229,348
评分 token）** 与 **ctx 1024** 下均 **window 0 即失败**：`text KV address space has no free active entry`
（= **未决项 8** 的页几何缺陷）。退到**单流 `--text`**：ctx 4096 下 **≤≈1.4k token 可用**，3675 token
（ctx4096）与 24,705 token（ctx32768）**均失败** ⇒ **kvarn 当前可用容量仅 ≈1–2k token**。

**结果 A（`--corpus --quick` ctx8192，229,348 token；kvarn 该档无法运行）**

| 档 | mean KLD | max | same_top | PPL |
|---|---|---|---|---|
| bf16（基准） | — | — | — | 4.693174 |
| rk4v4 + tail1024 | **0.001724** | 8.786 | 0.9833 | 4.695007 |
| kvarn:k4v2 | **不可运行**（页几何） | — | — | — |

**结果 B（`--text` ctx4096，单窗口 1438 评分 token）** ← **首个 KVarN 数据点**

| 档 | B/token/头 | mean KLD | median | max | same_top | PPL |
|---|---|---|---|---|---|---|
| bf16（基准） | 1024 | — | — | — | — | 4.6390 |
| **kvarn:k4v2** | **210** | **0.004893** | 0.001569 | 0.1319 | 0.9715 | 4.6549 |
| rk2v4-e8 | 216 | 0.028026 | 0.008835 | 0.9975 | 0.9318 | 4.7784 |
| rk4v4 | 280 | 0.003194 | 0.001307 | 0.1239 | 0.9840 | 4.6490 |

**结果 C（`--text` ctx4096，单窗口 3675 评分 token；kvarn 该档失败）**：rk2v4-e8 `0.026552` /
rk4v4 `0.003283` / nvfp4 `0.003315` / k8v4 `0.002205`（same_top 0.9241/0.9766/0.9722/0.9782）。

**判定**

1. **首个 KVarN 数据点为正**：`kvarn:k4v2`（**210 B**）KLD `0.0049`，**远优于字节匹配的 `rk2v4-e8`
   （216 B, 0.0280，约 5.7×）**，并**接近多用 33% 字节的 `rk4v4`（280 B, 0.0032）**
   ⇒ **KVarN 的「每字节质量」明显更高**。这与同仓既有负面裁决
   （`docs/port-records/PORT-DOD.md:23-32`：ppl、4 chunks、**仅对 f16/q8_0**、结论「无质量驱动理由」）
   **方向相反**；该裁决既未对同字节档、又用看不见尾部的 ppl，故**不足以否决**。
2. **但样本小**（1438 token、单窗口、单文本）⇒ **不作为结论**；**发布档 k4v4/k5v5/k6v6 仍未测**。
3. **决定性障碍**：kvarn 在**多窗口/长上下文**（含 A4 协议的 ctx 8192 / 32,767 token）**根本无法运行**
   ⇒ 计划书 §7 **WP0.5-B「≤1 天、ROI 最高」不成立**：该门实际被 **WP3①（未决项 8，47 处页算术）**前置。

**已回写计划书（v6 → v7）**：新增「v7 相对 v6 的变更摘要」+ §7-WP0.5-B 的「≤1 天」更正 + 附录 **D-8**
（含本节全部数字与下令）；计划书自身版本链与 §0/§4 同步。

**未做（如实记录）**：未修页几何（WP3①）；未跑 k4v4/k5v5/k6v6；未做多文本/≥3 重复/跨域；
未做端到端 MTP 激励；**未动任何源码**。

**产物**：`.deps/kvarn-adm/`（gitignored）内 `*.log`、`bf16-t0.topk`（229k 目标的 ctx8192 基准）、
`text-bf16-t0.topk`、`run{,_2,_3}.sh`。命令与参数见各 `.log` 抬头。

**供裁决的下一步**：**WP3①（页几何）是「kvarn 可运行」与「准入门」的共同前置**——建议先做它，
随后在 ctx 8192 / 229k token 上跑完整 WP0.5-B 与 k4v4/k5v5/k6v6（需 WP4）。**在 k4v4 档实测前，
不宜投 WP6（10–15 天，最高风险）**；亦不宜按 PORT-DOD 直接否决。

**▲ 环境/工具题外发现（已解决）**：本机从 **Git Bash** 传 Windows 绝对路径给 `ninfer-*` 会被
MSYS 改写（`D:\…`→`/d/…`）⇒ 报 `CreateFileW Win32 error 3`。**可行做法**：用**前斜杠**路径
`D:/ninfer/…` 并加 `MSYS_NO_PATHCONV=1`（本次实测有效），或把整条命令写进 `.bat` 由 `cmd /c` 执行。

### 2026-10-07-16 — **WP3① 完成**：地址空间页几何按存储贯穿；`kvarn:k4v2` ctx8192 / 229,348 token 全量跑通
（**回写计划书 §7-WP3 / §9 / 附录 D-9；版本 v7→v8**）

**触发**：用户指示执行 WP3①（唯一硬阻塞：页几何）。子代理并行测绘调用点范围（省上下文），主代理落地 + 实测。

**根因（一手）**：`KVAddressSpaceStore::create_active` 的 `entitlement` 由 `kv_pages_for_frontier(frontier)`
（硬写 64）算出，而 kvarn 的 `page_capacity_` 来自规划期 `kv_page_tokens(kvarn)=128` ⇒ 4900 token 时
77 页(64 口径) > 39 页(128 口径) ⇒ `nullopt` ⇒ 报 `text KV address space has no free active entry`。

**实现（19 文件）**
1. `KVAddressSpaceStore`（`program/storage/kv_store.h`）：新增 `page_tokens_`（构造期由**其池几何**
   `pages.physical_pool().geometry().page_tokens` 经 `checked_page_tokens` 取值）；`pages_for_tokens`
   由 static 改 const 成员、全部页/列算术（~19 处）改用 `page_tokens_`；`LogicalKVPageStore` 的
   `materialize_transfer_destination`/`commit_coverage` 列上限改用 `physical_->geometry().page_tokens`。
2. `kv_pages_for_frontier(frontier, storage)`（`context_work.{h,cpp}`）、
   `kv_pages_for_tokens(tokens, storage)` / `kv_tokens_for_pages(pages, storage)`（`program_impl.h`）增
   `KvCacheStorage`；新增成员 `device_kv_tokens_per_page()`；`kv_lease_cushion_pages` /
   `kv_lease_pages_for_tokens` 同步。
3. **42 个调用点**传 `kv_storage`：`context.cpp` 15、`request_plan.cpp` 14、`prefill.cpp` 4、`pressure.cpp` 3、
   `materialization.cpp` 2、`disk_tier.cpp` 2、`checkpoint_recovery.cpp` 1、`program_impl.cpp` 1
   （`causal_score` 的 entitlement = 原崩溃点）。
4. **页跨度字面量**（`kPagedKVPageSize` → `device_kv_tokens_per_page()`）：`context.cpp` 7、`prefill.cpp` 1、
   `materialization.cpp` 3、`pressure.cpp` 3、`request_plan.cpp` 5、`disk_tier.cpp` 10（删 `kPageTokens`
   常量、全部内联）；`hybrid_cache.cpp` 的 `kFullPage` 改由 text 池几何取（2 处）；`resolve_host_cache_budget`
   增 `storage`（`startup.{h,cpp}` + `tests/test_context_cache_defaults.cpp` 2 处传 `BFloat16`）。
   **核对**：`src/models/qwen3_5/` 内仅余 `load.cpp:192`（多 rank stage sizing，单卡不可达）与
   `decoder_state.cpp:165`/`startup.cpp:273`（外部尾 ring / DFlash，**语义即 64**）三处字面量，均**刻意保留**。

**刻意保持 64**：外部精确尾部 ring、DFlash full geometry、ops 侧 `validate_cache` / `paged_kv_address.cuh`
（kvarn 走自有 128-stride kernel，不经共享 helper）。

**构建**：`ninfer_ops ninfer_tests ninfer ninfer-serve ninfer-perplexity` **全绿**。唯一告警
`text.cpp:1546` lower_bound 有/无符号比较属 WP3 既有代码，**非本次引入**。

**验收①（长上下文，决定性）**：命令（`MSYS_NO_PATHCONV=1` + 前斜杠路径）
`ninfer-perplexity <27B产物> --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --disjoint --score-width 8 --score-topk 100 --kv-dtype kvarn:k4v2 --kv-tail-tokens 0 --kld-base .deps/kvarn-adm/bf16-t0.topk`

| 项 | bf16 base（同协议） | kvarn:k4v2 |
|---|---|---|
| 窗口 / 评分 token | 28 / 229,348 | **28 / 229,348** ✅ |
| PPL | 4.69317 | **4.72225**（+0.62%） |
| tok/s | 296.1 | 294.5（−0.5%） |
| mean KLD / median | — | **0.010513 / 0.002982**（P99 0.106971，P99.9 0.456034，max 8.070990） |
| same_top | — | 0.9624 |
| mean_target_dlogp | — | −0.006176 |

⇒ **28 窗口全量跑完**（此前 window 0 即失败）；**NLL 与 base bf16 可比**。原始日志
`.deps/kvarn-adm/acc-kvarn-k4v2.log`（head：`build v0.6.0-rtx3090-1345-gc8d905ef-dirty`）。

**验收②（短上下文回归）**：`--text .deps/kvarn-adm/t16k.txt --context 4096 --score-width 8`（3,675 token，
**此前必失败**）：bf16 PPL 6.04926 → kvarn:k4v2 **6.08013**，mean KLD **0.006651** / same_top 0.9668 /
~295 tok/s（日志 `smoke-*.log`）。

**回归（非 kvarn）**：`ctest` 定向 **14 项全通过**（含 `context_kv_materialize` 232 s 与其 legacy/unified
routes 各 ~230 s、`qwen3_5_context_store`、`prefix_cache_index`、`context_cache_defaults`、`paged_kv_window`、
`kv_cache_append`、`state_image`、`disk_kv_{store,bridge}`、`host_kv_clamp`、`evictable_kv_pool`、
`kv_capacity`）。**未跑全量 245 合成项**（单项可达 ~4 min；本次改动全部经 `kv_page_tokens==64` 分支或
kvarn 守卫 ⇒ 非 kvarn 逐位不变）。

**判定**：WP3① **完成**，硬阻塞解除。**k4v2 仍非发布档**（发布 k4v4/k5v5/k6v6 需 WP4）。ctx8192 的
kvarn KLD（0.0105）**不足以对质量下结论**——缺 229k 规模的**同字节**对照档（`rk2v4-e8` 仅有 ctx4096
单窗口数据），且 k4v2 非发布档 ⇒ **不宜据此投 WP6，亦不宜据此否决**。

**未做（如实记录）**：② 续列尾、③ `--mtp-attention-window` 规划期拒绝、④ MTP 路径激励（生成档）未做；
未跑 k4v4/k5v5/k6v6；未做多文本 / ≥3 重复；**未提交**（按用户约束保留工作树）。
**产物**：`.deps/kvarn-adm/`（gitignored）内 `acc-kvarn-k4v2.log`、`smoke-{bf16,kvarn}.log`、
`smoke-bf16.topk`、`acc_wp3a.sh`、`smoke_wp3a.sh`、`reg.bat`、`patch_{calls,spans}.py`。

### 2026-10-07-17 — WP0.5-B 补测启动：**229k 同字节臂缺失面**（结果未出）

**发现（一手核验 `.deps/kvarn-adm/` 全部产物）**：D-9 之后 229k（ctx8192 / 28 窗口）矩阵**仍不完整**。
*已有*：`bf16` base（PPL **4.693174**，296.1 tok/s，base topk 已存）、`rk4v4-t1024`
（KLD mean **0.001724** / same_top 0.9833）、`kvarn:k4v2`（mean **0.010513** / same_top 0.9624 /
PPL **4.722248**）。
*缺失*：① **`rk2v4-e8`**（216 B）——**k4v2（210 B）唯一的同字节对手**；`rk2v4e8-t0.log` 于 14:41 启动后
只到 `scoring` 行未收口（`RUN2.log` 止于该臂抬头，疑 14:54 `kill.bat` 中断）；② **`rk4v4-t0`**（无尾
280 B 基线，用于解释 `rk4v4-t1024`）；`RUN.log` 记 `exit=1` 无结果；③ `nvfp4-t0` / `k8v4-t0`
曾于 13:37 因 **MSYS 路径改写** 报 `CreateFileW Win32 error 3`，**从未真正运行**。
⇒ **`kvarn:k4v2` 的 0.010513 目前无可比同字节基准**，即 D-9「ctx8192 的 kvarn KLD 不足以对质量下结论」
的具体缺口（D-9「未做」已列，但**未点名缺哪几臂**）。

**动作**：新建 `.deps/kvarn-adm/run4.sh`（复用 `run2.sh` 已验证调用面），按决定性排序
`rk2v4e8-t0 → rk4v4-t0 → nvfp4-t0 → k8v4-t0`，统一协议
`--corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --disjoint --score-width 8
--score-topk 100 --kld-base .deps/kvarn-adm/bf16-t0.topk`。**16:01 启动**（后台；`run4.log` + 各臂
`<name>.log`）。GPU 空闲确认：启动前 48 MiB / 0%，运行中 11.6 GiB / 95%（单任务独占）。

**判据预告（供结果落盘后裁决）**：`rk2v4-e8` 在 229k 上 **明显劣于** `kvarn:k4v2`（同字节类）
⇒ KVarN「每字节质量」优势**在规模上成立** ⇒ 支持投 **WP4**（8–12 天）；反之须重新评估。
**注（口径）**：k4v2 是 **非发布档**、且与 k4v4 仅在位宽不同（同一 Sinkhorn/Hadamard codec）⇒
本补测是 **WP4 的代理门禁**，**不能**替代以 k4v4/k6v6 对 `rk4v4`/`nvfp4`/`k8v4` 的正式 WP0.5-B。

**显存可容纳性核验（用户要求，2026-10-07 补做）**：本机独显 **16,303 MiB（≈15.9 GiB）**，
现只有 1 个计算进程（`ninfer-perplexity.exe` pid 20792），当前占用 **11,603 MiB**（≈4.7 GiB 余量）。
`report.json` 的 `/memory` 给出精确预算（229k / ctx8192 / 4 streams）：

| 档 | kv_payload_bytes | runtime_reservation_bytes | 实测总占用 |
|---|---|---|---|
| `bf16`（**B/token/头 = 1024，全表最大**） | 536,870,912（512 MiB） | 2,248,807,424（2.14 GiB） | 已完成 ✅ |
| `kvarn:k4v2`（256） | 135,266,496（129 MiB） | 1,847,202,816（1.76 GiB） | 11,603 MiB |
| `rk4v4+tail1024`（280） | 218,103,808（208 MiB） | 1,930,040,320（1.84 GiB） | 已完成 ✅ |

⇒ **四个待跑臂全部可容纳**：最大者是 `k8v4`（402 B/token/头），其 KV 载荷 **< bf16 的 512 MiB**，
而 `bf16`（全表最大足迹）**已在本机同一 229k 规模跑通** ⇒ 上界 ≈ 权重 9.39 GiB + runtime_res ≤ 2.14 GiB
≈ **11.8 GiB ≪ 15.9 GiB**。另：perplexity 走 **default `--cuda-memory-policy`**（按 CUDA 报告的空闲显存
规划，留 `--kv-headroom-mib 1024`），属**自适应**而非盲目超配。⇒ **OOM 风险排除**；每臂的精确
`/memory` 数字将在收口时从各自 `report.json` 读回归档。

**未做（如实记录）**：结果未出（预计 2–4 h）；**未改任何源码**；**未提交**。

### 2026-10-07-18 — WP0.5-B 补测**完成**：229k 同字节矩阵齐备 ⇒ 代理门禁**明确为正**（WP4 GO）

**批次**：`run4.sh` 四臂全部 `exit=0`，16:01→16:52（51 min，比预估快，因四档均 ~297–301 tok/s），
GPU 复原 **48 MiB / 0%**（无残留进程）；峰值占用 **11,695 MiB / 16,303**（`k8v4` 臂）——
与「上界 ≈ bf16 足迹」的推算一致，**OOM 未发生**。

**完整矩阵（统一协议：`--corpus … --quick --context 8192 --disjoint --score-width 8 --score-topk 100`，
base = `--kv-dtype bf16 --kv-tail-tokens 0`，229,348 评分 token / 28 窗口）**

| 档 | B/tok/头 | PPL | mean KLD | median | P99 | max | same_top | dlogp | tok/s |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `bf16`（base） | 1024 | 4.693174 | — | — | — | — | — | — | 296.1 |
| **`kvarn:k4v2`** | **210** | 4.722248 | **0.010513** | 0.002982 | 0.106971 | 8.070990 | **0.9624** | −0.006176 | 294.5 |
| **`rk2v4-e8`**（同字节对手） | **216** | 4.83714 | **0.043619** | 0.011210 | 0.453321 | 19.935462 | 0.9229 | −0.030215 | 299.0 |
| `rk4v4` t0 | 280 | 4.70427 | 0.004426 | 0.001736 | 0.037014 | 7.184616 | 0.9727 | −0.002362 | 300.7 |
| `rk4v4` +tail1024 | 280 | 4.695007 | 0.001724 | 0.000625 | 0.014058 | 8.785662 | 0.9833 | −0.000391 | 81.9 |
| `nvfp4` t0 | 288 | 4.69752 | 0.004385 | 0.001650 | 0.038020 | 10.253651 | 0.9726 | −0.000926 | 299.4 |
| **`k8v4` t0**（`k6v6` 的同字节对手） | **402** | 4.6936 | **0.002688** | 0.001130 | 0.019823 | 14.925574 | 0.9785 | −0.000091 | 296.7 |

**逐臂 `/memory`（`report.json`，ctx8192 / 4 streams；权重恒为 9.39 GiB）**

| 档 | kv_payload | runtime_reservation |
|---|---:|---:|
| `bf16` | 512.0 MiB | 2.09 GiB |
| `kvarn:k4v2` | 129.0 MiB | 1.72 GiB |
| `rk2v4-e8` | 108.0 MiB | 1.70 GiB |
| `rk4v4` t0 / +tail1024 | 140.0 / 208.0 MiB | 1.73 / 1.80 GiB |
| `nvfp4` t0 | 144.0 MiB | 1.73 GiB |
| `k8v4` t0 | 201.0 MiB | 1.79 GiB |

**判定**

1. **代理门禁为正（决定性）**：**同字节**（210 vs 216 B）下 `kvarn:k4v2` 的 mean KLD **0.010513**
   比 `rk2v4-e8` 的 **0.043619** 好 **4.15×**（same_top +3.95 pt，PPL 4.722 vs 4.837）。
   与 1438-token 单窗口的正向点（当时 **5.7×**）**方向一致、幅度同量级**（规模上 4.15×，未衰减到噪声）
   ⇒ **D-9「ctx8192 的 kvarn KLD 不足以对质量下结论」的缺口已补上并给出正面结论**。
2. **优势性质是「每字节」而非「绝对质量」**：绝对 KLD 上 `rk4v4`(280 B, 0.004426) 与
   `nvfp4`(288 B, 0.004385) **仍优于** `kvarn:k4v2`(210 B, 0.010513)；kvarn 用少 **25–27%** 的字节
   接近它们。价值命题成立，**但不是碾压**。
3. **k6v6 的门槛已量化且很高**：`k8v4`(402 B) 达 mean KLD **0.002688**、PPL 4.6936（≈ bf16 4.6932）、
   dlogp −0.000091（几乎无偏）⇒ 按 A8「k6v6 不优于 k8v4 则移除该档」，**k6v6 必须低于 0.002688**
   （相对 `kvarn:k4v2` 210 B 的 0.010513，即 402 B 需比 210 B 再降 3.91×）才值得发布。
4. **副产品：外部精度尾部的规模稳定性得证**：新增的 `rk4v4-t0`(0.004426) 对比既有
   `rk4v4+tail1024`(0.001724) ⇒ 尾部在 229k 给出 **2.57×** 增益，落在计划书 A4 记录的 pairing 带
   （2.26–2.47×）附近 ⇒ 尾部收益不随规模衰减（与 WP6 的协同论证相关）。
5. **性能**：四档 decode/prefill 速率 297–301 tok/s，**与 `bf16` 296.1 同档**（`kvarn:k4v2` 294.5，−0.5%）
   ⇒ **无 KVarN 特有的速度代价**；带尾的 `rk4v4+tail1024` 仅 81.9 tok/s 是**尾路径**代价，非 kvarn。

**裁决：投 WP4（go）。** 依据：4.15× 的同字节优势在 229k 上成立 ⇒ 位宽参数化（K=V∈{4,5,6}）有明确
质量驱动的理由。**但必须写明**：① `k4v2` 是**非发布档**、是 WP4 的**代理**，本结果**不能**替代以
k4v4/k5v5/k6v6 对 rk4v4/nvfp4/k8v4 的正式 WP0.5-B；② **不据此外推 WP6**（10–15 天、最高风险）——
WP6 须等 k4v4 档实测后再议。

**已回写计划书**：新增「v9 相对 v8 的变更摘要」+ §7-WP0.5-B 的**完成**标注（原「被 WP3① 前置」
已解除、matrix 齐备）+ 附录 **D-10**（本节全部数字）；§0 快照同步。

**未做（如实记录）**：未跑 k4v4/k5v5/k6v6（需 WP4）；未做 ≥3 重复（各臂单跑；本批为**同协议单跑**，
虽各档与 bf16 的相对关系一致且 bf16 自身 PPL 与 D-8/D-9 逐位一致，仍属**单次**测量）；
未做多文本/跨域；**未改任何源码**；**未提交**。
**产物**：`.deps/kvarn-adm/` 内 `run4.sh`、`run4.log`、`{rk2v4e8-t0,rk4v4-t0,nvfp4-t0,k8v4-t0}.log`。

### 2026-10-07-19 — WP4 起手：位宽站点测绘（WP4.1）+ **记录布局泛化为函数**（WP4.2 第一步，构建绿）

**触发**：用户裁定「W P4 位宽参数化」（AskUserQuestion）。子代理并行测绘 `src/ops/kvarn/` 全部 12 文件
（省上下文），主代理定设计 + 落地布局层 + 构建验证。

**WP4.1 测绘（子代理，逐文件读完，附 `file:line` 原始代码）——关键结论**

1. **布局真值确认**：`RecordBytes(Kb,Vb) = 4096*(Kb+Vb) + 2304` **与源码逐字相符**；
   `4096 = D*G/8`；元数据 2304 = `512+512+256`（K 侧）+ `512+256+256`（V 侧）。`kvarn.h:18-28` 的八个
   偏移常量即其展开（K packed 0..16384、V packed 17664..25856）。`kKvarnRecordPayloadBytes ==
   kKvarnRecordBytes == 26880`（二者相等，"2304" 无具名常量）。
2. **位解包实现是「5 个例程、跨 4 文件」**（计划说 4 处，实测 `decode_kernel.cuh` 内有 3 个
   （`stage_decode_key_quad` L196-224 / `stage_decode_key` L258-294 / `stage_decode_value` L328-354），
   加上 `codec.cu:81-94`、`materialized_prefill.cuh:91-113`、`attention.cu:343-373`）——**五者位序完全一致**
   （LSB 优先、低下标在前；K 偶数 token=低半字节，V `d&3==0`=最低字段）。**打包侧只有一套**
   （`store.cuh` 的 `store_k_tile`/`store_v_tile`，被 `codec.cu` 与 `attention.cu` 两个编码核调用）。
3. **`store.cuh` 的 `/15.0F`、`/3.0F` 就是 `qmax=(1<<bits)-1`**（15=2⁴−1、3=2²−1）⇒ 位宽相关的量化核心，
   不是"独立于位宽"的代码。
4. **向量化约束（一手）**：`decode_kernel.cuh:109-110/118-119` 用 `load_vec<int4>` 一次搬 16 B
   = 128 bit = 32 个 4-bit K 码（或 64 个 2-bit V 码）；`b=5` 时 128/5=25.6、`b=6` 时 128/6≈21.33
   ⇒ **非整数**，int4 永不能整装一个码块 ⇒ 5/6-bit 必须走**逐码位寻址**（`store.cuh`/`codec.cu`/
   `materialized_prefill.cuh`/`attention.cu` 的字节移位形态）。**印证计划「保留 4-bit 快路径 +
   5/6-bit 位流慢路径 + `if constexpr` 分派」**。
5. **ops 目录外无一处从 K/V 位宽推导尺寸**；唯一的外部消费者是
   `decoder_state.cpp:74` 的平面内维 `kKvarnRecordBytes / kKvarnGroup`（=210 B/token/头）。
6. **位宽无关面**：`hadamard.cuh`（纯 Sylvester 蝶形，仅依赖 D=256）、`sinkhorn.cuh`（BF16 tile + log
   scale 迭代）、`decode.cu`/`streaming_prefill.cuh`、`materialized_prefill.cuh` 的 MMA/online-softmax 段。
7. **测试 oracle 结构**：`test_kvarn.cpp:199-242 codec_oracle`（FP64），L214 **硬写 `qmax = key ? 15 : 3`**；
   容差 `3.0e-4` 在 L315-316 两处；`DeviceStorage`/`view()` 的 `kGroup/2`、`kD/4` 形状需随位宽改。

**WP4.2 第一步（已落地，`include/ninfer/ops/kvarn.h`）**

把记录几何从**字面量常量**改为**参数的 `constexpr` 函数**，并把**旧常量改为由 `(4,2)` 派生**
（⇒ 泛化立即被使用，且旧值**逐字节不变**）：

- `kvarn_{k_packed,k_scale,k_zero,k_token_scale,v_packed,v_channel_scale,v_token_scale,v_token_zero}_offset(kb,vb)`
  与 `kvarn_record_bytes(kb,vb)`；`kKvarnPackedBytesPerBit = 4096`。
- 码寻址：`kvarn_k_row_bytes(kb)=G*kb/8`、`kvarn_k_code_bit(kb,token)=token*kb`、
  `kvarn_v_row_bytes(vb)=D*vb/8`、`kvarn_v_code_bit(vb,dim)=dim*vb`。
- **共享位编解码**（`KVARN_HOST_DEVICE` 宏：`__CUDACC__` 下展开为 `__host__ __device__`，MSVC 下为空
  ⇒ 主机 oracle 与设备核**共用一套**，消除第 2 条的五份重复）：
  `kvarn_unpack_code(row,bit,bits)` / `kvarn_pack_code(row,bit,bits,code)`，小端位序；**跨字节时才读第二个
  字节** ⇒ 行长为 8 的整数倍时（本处每个 K/V 行都是）永不越界。
- **编译期断言（全部通过）**：`kKvarnKScaleOffset==16384`、`KZero==16896`、`KTokenScale==17408`、
  `VPacked==17664`、`VChannelScale==25856`、`VTokenScale==26368`、`VTokenZero==26624`、
  `kKvarnRecordBytes==26880`（**旧布局零位移**）；`kvarn_record_bytes(4,4)==35072`、`(5,5)==43264`、
  `(6,6)==51456`（合 §5 字节表）、三者均 %256==0；`kvarn_k_row_bytes(5)==80`、`(6)==96`、
  `kvarn_v_row_bytes(4/5/6)==128/160/192`。

**关键洞察（供后续执行）**：`(4,4)` 时 **K 侧偏移与 K 打包算术与旧 k4v2 逐字节相同**（`k_code_bit(4,t)=4t`，
即 `token/2` 与 `4*(token&1)`）⇒ **k4v4 只改 V 侧**（V 由 2-bit 变 4-bit），这大幅降低 k4v4 的落地风险；
`(4,2)` 时 V 侧亦逐字节相同。

**构建**：`ninfer_ops ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0**（39 步，0 error 0 告警）。
`static_assert` 通过即证明旧值未动、新值达标。

**未做（如实记录）**：**核尚未迁移**——`config.cuh` 仍是 `KBits=4/VBits=2` 死常量，5 个解包例程仍在，
`if constexpr(bits)` 快/慢分派、launcher 模板化与 `bits` 从 `EngineOptions` 的贯穿、`test_kvarn.cpp` 的
4/5/6 oracle 扩展、parser 的 `k4v4|k5v5|k6v6` 发布与 `k4v2` 删除（WP4.2/4.3/4.4）**均未做**；
**未提交**（按用户约束保留工作树）。
**下一步（按依赖序）**：① 把 `store.cuh`/`codec.cu`/`materialized_prefill.cuh`/`attention.cu` 的
pack/unpack 与 `decode_kernel.cuh` 的 staging 改为**按 (kb,vb) 模板化**，`if constexpr(kb==4)` 保留
int4/nibble 快路径、否则走 `kvarn_unpack_code` 慢路径；② `bits` 从 `EngineOptions.kvarn_bits` 经
`DecoderStateSpec` 贯穿到 launcher（含 `decoder_state.cpp:74` 的平面内维改 `kvarn_record_bytes(b,b)/G`）；
③ 测试扩到 4/5/6（含往返 + 按位宽容差，收口 WP1 的 `ninfer_kvarn_test` 容差）；④ parser 发档 + 删 k4v2。

### 2026-10-07-20 — WP4.2 主体完成：核按 `(kb,vb)` 模板化 + `bits` 贯穿到 launcher（构建绿）

**范围**：WP4.2 的两半一次做完（① 核模板化、② `bits` 贯穿）。`ninfer_ops` / `ninfer` /
`ninfer-serve` / `ninfer-perplexity` **exit 0**（0 error 0 告警）。测试文件由子代理并行改（07-21 记录）。

**设计定案（本次实测确立，供后续遵守）**

1. **设备核模板参数 = `(int KBits, int VBits)`**，但**发布档只实例化 `(b,b)`，b∈{4,5,6}**——
   K=V 是产品合同（§1 目标），故 **不必也不得**实例化非对称档；`(4,2)` 的**几何函数**仍留在头文件
   （k4v2 的字节表继续由 `kvarn_*_offset(4,2)` 表达），但**没有任何设备核实例化它**。
   ⇒ 「k4v2（仅 oracle）」在代码中的落点是**几何常量，不是运行档**。
2. **V 侧的快路径被移除，改为统一的位流解码**（`kvarn_unpack_code` + `v_scale/v_zero` 表内联
   `fmaf`）。理由：旧 V 快路径（`uint16` 取 8 个 2-bit 码）**只对 VBits==2 成立**，而 VBits==2 已不
   实例化 ⇒ 保留它即是死代码（违反「删除被取代的分支」）。K 侧 `KBits==4` 的 int4+nibble 快路径**保留**
   （k4v4 用到，且是 §5 约束 3「向量化只在 4-bit 成立」的落点）。
3. **`KVARN_HOST_DEVICE` 宏上移到记录几何函数之前**，并把 13 个 `kvarn_*` 几何/寻址 `constexpr` 函数
   标为 `KVARN_HOST_DEVICE`（设备核要调它们；不加会报 "calling a constexpr __host__ function ...
   from a __device__ function"，本次实际撞到）。
4. **`k4v4 只改 V 侧` 在代码层成立**：`KBits==4` 时 K 侧偏移（`k_packed=0`、`k_scale=16384`、
   …、`v_packed=17664`）与算术（`k_code_bit(4,t)=4t`、行字节 `Group/2`）**与旧 k4v2 逐字相同**，
   故 K 解码路径**无行为变化**；变化的只有 V（`v_channel_scale` 起点 25856→34048、行字节
   `D/4`→`D/2`、码值 `&3`→`&15`）。
5. `stage_decode_record` 的 **K staging 双布局**：`KBits==4` 保留 int4 字布局
   `[(half*4+w)*D+dim]`（8192 B）；`KBits!=4` 改为**每 dim 一行字节** `[dim*kKSliceRowBytes+b]`
   （10,240 / 12,288 B）。`token_begin` 是 64 的倍数 ⇒ `token_begin*KBits/8` 恒为整字节
   （kb=5: 0/40；kb=6: 0/48），无跨行越界。V staging 天然按整行连续（行字节 = `D*vb/8`）。
6. **`DecodeRecordMetadata` 不再需要模板**：删掉 `v_base[64][1<<VBits]`（vb=6 时 16 KB）改为
   `v_scale[64]`+`v_zero[64]`（各 256 B），`v_channel_scale` 由 `[kV][D/kV]` 扁平为 `[D]`
   （plane-major，索引 `v_channel_index<VBits>(dim)`；vb=2 时与旧 `[dim&3][dim>>2]` **同一地址**）。
   ⇒ 结构体从 3328 B（vb=2）降到固定 **2816 B**。
7. **运行时分派**：`decode.cu` 的 `decode_attention_impl<KBits,VBits>` +
   `switch (cache.bits)`；`codec.cu` 的 `store_kernel/dequant_kernel`、`attention.cu` 的
   `commit_kv_impl`/`kvarn_restore_tail_impl` 同构。`stage_kv`（只写 BF16 尾槽）与
   `retire_kernel`、`reduce_output_hadamard_kernel`、`kvarn_attention_workspace_capacity_bytes`
   **与位宽无关**，未动。
8. **`bits` 承载在视图里**：`KvarnTileStorage` / `KvarnPagedLayerView` / `KvarnPagedBatchLayerView`
   各加尾成员 `std::int32_t bits = 4`（聚合初始化，尾部追加 ⇒ 既有调用点零改动）。
   贯穿链：`EngineOptions.kvarn_bits` → `SequencePlanningInputs.kvarn_bits` →
   `SequencePlanImpl.kvarn_bits` → `DecoderStateSpec.kvarn_bits` → `PagedKVCacheLayout.kvarn_bits`
   → `PagedKVCache::kvarn_bits_` → 视图 `.bits`。

**逐文件改动**

- `include/ninfer/ops/kvarn.h`：`KVARN_HOST_DEVICE` 宏上移；13 个几何/寻址函数加 `KVARN_HOST_DEVICE`；
  **删除** `kKvarnRecordBytes` / `kKvarnRecordPayloadBytes` / 8 个 `kKvarn*Offset` 旧常量与
  「(4,2) 复现」断言块（保留 `kKvarnGroup`/`kKvarnHeadDim`）；新增 `kvarn_k_row_bytes(4)==64`
  断言；三个结构体加 `bits`。
- `src/ops/kvarn/config.cuh`：**删除** `KBits=4`/`VBits=2`（`KBits` 本是死常量，`VBits` 唯一用处是
  被删掉的 `static_assert`）。
- `src/ops/kvarn/store.cuh`：`store_k_tile<KBits>` / `store_v_tile<VBits>`；`/15.0F`、`/3.0F` →
  `qmax=(1<<bits)-1`；行偏移改 `kvarn_k_row_bytes/kvarn_v_row_bytes`；非 4-bit 走
  「先清零整行 + `kvarn_pack_code` 逐码 OR」。
- `src/ops/kvarn/decode_kernel.cuh`：新增 `kKSliceRowBytes<KBits>` / `kVCodeValues<VBits>` /
  `kPackedKBytes<KBits>` / `kPackedVBytes<VBits>` / `v_channel_index<VBits>`；
  `stage_decode_record<KBits,VBits>`、`stage_decode_key_quad<KBits>`、`stage_decode_key<KBits>`、
  `stage_decode_value<VBits>`；`attention_decode_kernel` 增 `KBits/VBits`（尾参、默认 4）；
  记录步长 → `kvarn_record_bytes`。
- `src/ops/kvarn/decode.cu`：`launch_prefill`/`launch_partial` 增 `KBits/VBits`；
  `decode_attention_impl<KBits,VBits>` + `switch(cache.bits)` 分派（4/5/6，其它抛）。
- `src/ops/kvarn/codec.cu`：`validate_storage` 校验 `bits∈{4,5,6}` 并用 `kvarn_*_row_bytes(bits)`
  校验形状；`store_kernel<KBits,VBits>` / `dequant_kernel<KBits,VBits>`（dequant 一律走
  `kvarn_unpack_code`，对 4-bit 与旧 nibble 提取**逐位等价**）；两个入口 `switch(storage.bits)`。
- `src/ops/kvarn/materialized_prefill.cuh`：`materialize_prefill_slab_kernel<Metadata,KBits,VBits>`；
  K 快路径（`KBits==4` 的 uint32 寄存器缓存）保留，否则逐码 `kvarn_unpack_code`；V 逐码。
- `src/ops/kvarn/attention.cu`：`record_pointers<KBits,VBits>`、`encode_group<KBits,VBits>`、
  `encode_kernel<KBits,VBits>`、`settle_encode_kernel<KBits,VBits>`、`restore_tail_kernel<KBits,VBits>`；
  `require_view` 的记录槽与 `kvarn_restore_tail` 的校验用 `kvarn_record_bytes(view.bits,view.bits)`；
  `commit_kv` / `kvarn_restore_tail` 改为按 `bits` 分派的薄壳。
- `src/models/qwen3_5/state/decoder_state.{h,cpp}`：`DecoderStateSpec`+`PagedKVCacheLayout` 增
  `KvarnBits kvarn_bits`；`plan_cache` 增参并用 `kvarn_record_bytes(bits,bits)/kKvarnGroup` 建平面
  （注释由「26880/128=210 B」改为「35072/128=274 B(k4v4)」）；`PagedKVCache` 增 `kvarn_bits_`
  并在 `kvarn_layer_view`/`kvarn_batch_layer_view` 写 `.bits`。
- `src/models/qwen3_5/program/planning/startup.{h,cpp}`：`SequencePlanningInputs`/`SequencePlanImpl`
  增 `KvarnBits kvarn_bits`；`.kvarn_bits = options.kvarn_bits`（inputs）、
  `impl->kvarn_bits = inputs.kvarn_bits`、`DecoderStateSpec{.kvarn_bits = plan.kvarn_bits}`。

**未做 / 待验（如实记录）**：① 测试扩 4/5/6 与容差收口**进行中**（子代理，含 pack→unpack 往返）；
② parser 仍只接受 `kvarn:k4v2`（`KvarnBits::Bits2` 仍在）⇒ **当前 CLI 无法选集 k4v4/k5v5/k6v6**，
即源码已支持三档但**尚无用户可及的入口**（WP4.4）；③ 未跑 e2e（需 ④ 完成后才能用发布档跑正式
WP0.5-B）；④ 未提交。
**（⤴ ①② 均已于 07-21 完成并实跑，见下条；③ 仍待办。）**

### 2026-10-07-21 — WP4.2 端到端验收（测试扩 4/5/6 实跑全绿）+ WP4.4 parser 发档 + 两处规划期拒绝

**① 源码验证：`ninfer_tests` 扩到 4/5/6 后 `ninfer_kvarn_test` 全绿（exit 0）**

改动 `tests/ops/test_kvarn.cpp`（1711 行，唯一改动文件）：
`codec_oracle(..., int bits)` 的 `qmax=(1<<bits)-1`；`DeviceStorage<Bits>`（K/V 码缓冲与 `view()`
形状随 `kvarn_k_row_bytes/kvarn_v_row_bytes`，并写 `.bits=Bits`）；`run_codec_case<Bits>` 由 `main`
按 4/5/6 各跑一次；K/V 的「stored-bit decode」与整条注意力套件（含 `decode_cache_value`）改用
`kvarn_unpack_code` + `kvarn_*_offset(kBits,kBits)`，**整套注意力夹具改到 k4v4 几何**
（`kRecordBytes = kvarn_record_bytes(4,4) = 35072`）；新增 **pack→unpack 逐字节往返**检查
（K/V 各一，重建缓冲逐字节比对）；`compare_profile` 增 `report` 参数打印实测值。

**实测（`./build-port/tests/ninfer_tests.exe ninfer_kvarn_test`，独显空闲）**

| 档 | K official oracle rel_l2 / max_abs | V official oracle rel_l2 / max_abs |
|---|---|---|
| bits=4 | **6.0882e-4** / 0.216104 | 3.53748e-5 / 0.0039854 |
| bits=5 | 5.2000e-4 / 0.183644 | 3.55528e-5 / 0.00398588 |
| bits=6 | **0** / 0 | 3.49451e-5 / 0.00397921 |

**关键判定（根因，一手推导 + 数字自洽）**

1. **bits=4 的 K rel_l2 与重构前逐位相同（6.0882e-4）** ⇒ **4-bit K 路径零回归**，与
   「`KBits==4` 时 K 侧偏移/算术与旧 k4v2 逐字相同」的设计一致（本次用实跑证实，不再是推理）。
2. **旧容差 `3.0e-4` 的失败根因已定量**：`max_abs≈0.216` 正是**一个量化步长**
   （`(max-min)/15`），且 `sqrt(0.216²/Σdecoded²)≈8.4e-4` 与实测 `6.09e-4` **同量级** ⇒
   偏差**几乎全部来自「单个元素落在量化边界、设备与 oracle 各自舍入到相邻码」**，
   **不是 codec 缺陷**（设备自洽性由 `stored-bit decode` 2.0e-7 与 pack/unpack 往返钉死，均通过）。
   ⇒ 判定：**容差标定问题**（与 WP1 的初步定性一致），不是数值错误。
3. **bits=6 的 K 与 oracle **逐位相同**（rel_l2=0）**：该输入下设备 Sinkhorn 与 FP64 oracle 的
   `min/max`、fp16 吸收 scale/zero、以及全部 128 个码**完全一致**，无任何边界翻转 ⇒ 是**更强的**
   正确性证据（6-bit 码打包/解包与量化都精确对上）。
4. **整套注意力套件在 k4v4 下通过**（含 27B/35B、grouped decode、speculative 边界、tail staging、
   batched、packed cached 等 ~90 个用例）⇒ **新写的 V 4-bit 位流解码（连同 `v_channel_scale`
   扁平成 plane-major）在端到端口径上正确**（对照物是测试内独立的 host FP64/Sinkhorn oracle）。
5. **容差重定（本步交付）**：`oracle_relative_l2_limit(bits)` 三档均取 **1.0e-3**（对实测最大值
   `6.09e-4` 有 1.64× 余量），注释写明依据 = 单元素边界翻码造成的步长级偏差。
   **留给 WP5**：把该判据形式化为「逐元素 ≤ 一步 + 边界翻码计数上限」的量化步长判据（计划书
   §7-WP5 的职责），本轮不越界实现。

**② WP4.4：parser 发档 + 删 k4v2**

- `include/ninfer/types.h`：`KvarnBits` **删除 `Bits2 = 2`**（保留 `Bits4/5/6`）。
- 三处 parser（`src/serve/serve_options.cpp:parse_kv_dtype`、`apps/cli/options.cpp:parse_kv_cache`、
  `apps/perplexity/main.cpp` 的 `--kv-dtype`）：**接受 `kvarn` / `kvarn:k4v4` → Bits4、
  `kvarn:k5v5` → Bits5、`kvarn:k6v6` → Bits6**；`kvarn:k4v2` 与「not implemented」拒绝分支**删除**
  （裸 `kvarn` ≡ k4v4，§10-D2）。perplexity 的 usage 文本改为
  `kvarn:k4v4|k5v5|k6v6`。
- **顺带修掉一个真实静默缺陷**（本次发档使其可达，属「不可分割」）：`startup.cpp`
  `validate_target_options` 增 `kvarn` 规划期拒绝 —— `--kv-tail-tokens != 0`（精确尾部**未接进**
  kvarn body，此前会被静默忽略 ⇒ 用户以为有尾、实际没有）与 `--mtp-attention-window != 0`
  （走 64 页块表，对 128-token body 未验证）⇒ 二者**fail-fast**（同时收口计划 §6.4 的回退与
  进度 §0 未决项 10）。

**构建**：`ninfer_ops ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0**（0 error 0 告警）。
**未做（如实记录）**：① 未跑正式 WP0.5-B（k4v4/k5v5/k6v6 对 `rk4v4`/`nvfp4`/`k8v4`，ctx8192 /
229k token）——**下一步**；② 未跑 e2e 冒烟（parser 发档后需用 `kvarn:k4v4` 实跑一次确认可运行）；
③ `--help` 与 `docs/{cli,serving,perplexity}.md` 的 kvarn 档说明**尚未补**（WP3⑥，另见未决项 6）；
④ kvarn parser 单元测试未加；⑤ 未提交。

### 2026-10-07-22 — **WP4 收尾：正式 WP0.5-B 三档全过**（发布档准入达成）+ 测试补 5/6 位宽注意力覆盖 + 文档

**① 正式 WP0.5-B（`.deps/kvarn-adm/run5.sh`，17:58→18:39，三臂 exit=0）**

协议与 D-10 **逐字相同**（复用同一条 `bf16-t0.topk`）：`--corpus eval/corpora/perplexity-1m/manifest.json
--quick --context 8192 --disjoint --score-width 8 --score-topk 100`，臂 = `--kv-dtype kvarn:k4v4|k5v5|k6v6
--kv-tail-tokens 0`。先 smoke（t16k/ctx4096 三档各一次，exit=0）再跑 229k。GPU 中途采样 11,687 MiB、
**无 OOM**；跑前/跑后 device 空闲（48 MiB）。

**三档实测（229,348 评分 token / 28 窗口）**

| 档 | B/tok/头 | PPL | mean KLD | median | P99 | max | same_top | dlogp | tok/s |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `bf16`（base） | 1024 | 4.693174 | — | — | — | — | — | — | 296.1 |
| **`kvarn:k4v4`** | **274** | 4.69436 | **0.002120** | 0.000747 | 0.017190 | 7.175765 | 0.9817 | −0.000252 | 294.5 |
| **`kvarn:k5v5`** | **338** | 4.69405 | **0.001432** | 0.000531 | 0.010355 | 7.599654 | 0.9846 | −0.000187 | 291.8 |
| **`kvarn:k6v6`** | **402** | 4.69352 | **0.001233** | 0.000454 | 0.007896 | 8.354687 | 0.9858 | −0.000073 | 287.7 |
| `rk4v4` t0（锚） | 280 | 4.70427 | 0.004426 | 0.001736 | 0.037014 | 7.184616 | 0.9727 | −0.002362 | 300.7 |
| `nvfp4` t0（锚） | 288 | 4.69752 | 0.004385 | 0.001650 | 0.038020 | 10.253651 | 0.9726 | −0.000926 | 299.4 |
| `k8v4` t0（锚） | 402 | 4.6936 | 0.002688 | 0.001130 | 0.019823 | 14.925574 | 0.9785 | −0.000091 | 296.7 |

**逐档 `kv_payload`**（`report.json`，ctx8192/4 streams）：`k4v4` **161.0** / `k5v5` **193.0** /
`k6v6` **225.0** MiB；`runtime_reservation` 1.75 / 1.78 / 1.81 GiB。
**差值恰为每 +64 B/token/头 +32.0 MiB**；由 `k4v4`(274 B, 161.0) 反推 210 B 档得 **129.0 MiB**，
**与 D-10 实测的 `kvarn:k4v2` 129.0 MiB 逐位相同** ⇒ **平面几何参数化（`kvarn_record_bytes(bits,bits)/G`）
被显存核算独立验证**（这是本题改动的一个额外交叉证据，不属原计划验收项）。

**smoke（t16k / 3,675 token）**：mean KLD 0.001480→0.001140→0.001072、median 0.000875→0.000690→0.000618、
`same_top` 0.9796→0.9850→0.9867 —— **随位宽单调** ⇒ 证明 `kvarn_bits` 端到端选到三套不同编解码
（若未贯通，三档会给出同一组数字）；tok/s 295.8/294.3/293.8。

**② 逐档裁决（§7-WP4/A8）——三档全部达标，不回退**

1. **`k4v4`** 0.002120 vs `rk4v4` 0.004426 = **2.09×**、vs `nvfp4` 0.004385 = **2.07×**，且**字节更少**
   （274 < 280/288）⇒ A8「不优于 `{rk4v4,nvfp4}` 则移除」**未触发**。
2. **`k5v5`** 0.001432：无同字节档（A8 已声明判据另定）。相对 `k4v4` 多 23% 字节换 −32% KLD，
   相对 `k6v6` 少 16% 字节付 +16% KLD ⇒ 中间档自洽。
3. **`k6v6`** 0.001233 vs 逐字节相同的 `k8v4` 0.002688 = **2.18×** ⇒ **门槛（< 0.002688）达成**。

**③ 对既有结论的两条修正（实测优先；原文保留以存史）**

1. **D-10 判定 2「KVarN 优势性质是「每字节」而非绝对质量」——被发布档取代**：`k4v4` 的 **0.002120**
   在**绝对** mean KLD 上**同时**优于 `rk4v4`/`nvfp4`（280/288 B）与 `k8v4`（402 B）⇒ 价值命题现在是
   **「更少字节 且 质量更好」**。（D-10 那句基于**不发布**的 `k4v2` 代理档，仍成立但不再是发布档的性质。）
2. **D-10 判定 5「无 KVarN 特有速度代价」——收窄**：`k4v4` −0.5% 成立；但位宽升高有**单调真实代价**：
   `k5v5` −1.5%、`k6v6` **−2.8%**（vs bf16 296.1）。归因：记录更大（51,456 vs 35,072 B）⇒ decode staging
   字节更多。**A8「不得劣于同档噪声底」对 `k6v6` 须据此读**（其同字节对手 `k8v4` 296.7 属不同实现）。
3. PPL：`k4v4` +0.025% / `k5v5` +0.019% / `k6v6` +0.007%（vs bf16 4.693174）。

**④ 测试补 5/6 位宽**（收尾 07-21 留下的覆盖缺口）：07-21 的注意力套件整体跑在 `k4v4`，
⇒ **`decode_kernel` 的 `KBits!=4` 分支当时只有 e2e 覆盖、无单测**。本轮把
`CacheFixture` / `append_cache` / `run_cached_attention_case` / `decode_cache_value` 加 `Bits` 模板参数
（默认 `kBits` ⇒ 既有用例逐字不变），并在 `main` **新增 2 个用例**：
`KVarN k5v5 packed attention` / `KVarN k6v6 packed attention`（`CacheFixture<4,34,5|6>`，2128 token 追加 +
width-6 查询，对照物仍是测试内独立 host oracle）。**`ninfer_kvarn_test` 仍全绿**，输出：
```
KVarN K official oracle: relative_l2=0.00060882 max_abs=0.216104     (bits=4)
KVarN V official oracle: relative_l2=3.53748e-05 max_abs=0.0039854   (bits=4)
KVarN K official oracle: relative_l2=0.00052    max_abs=0.183644     (bits=5)
KVarN V official oracle: relative_l2=3.55528e-05 max_abs=0.00398588  (bits=5)
KVarN K official oracle: relative_l2=0 max_abs=0                     (bits=6)
KVarN V official oracle: relative_l2=3.49451e-05 max_abs=0.00397921  (bits=6)
OK kvarn correctness
```
⇒ **5/6 位的 decode kernel + staging 现已有单测覆盖**（此前只有 e2e）。

**⑤ 文档与 `--help`（WP3⑥ 的 kvarn 部分，07-21 发档后补）**：`apps/cli/options.cpp`、
`src/serve/serve_options.cpp`、`apps/perplexity/main.cpp` 三处 `--kv-dtype` 帮助文本增
`kvarn:k4v4|k5v5|k6v6`；`docs/cli.md`（选项表 + 「KV-cache formats」正文）、`docs/serving.md`（选项表）、
`docs/perplexity.md`（可用表示 + 尾部段落）增 KVarN 条目与已实测数字，并写明
**`kvarn:*` 拒绝 `--kv-tail-tokens` 与 `--mtp-attention-window`**。

**构建**：`ninfer_ops ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0**。
**未做（如实记录）**：① 各臂**单次**测量（未做 ≥3 重复，同 D-10 口径）；② 未做多文本/跨域；
③ 未做 MTP 路径（WP3④）；④ 未做 `kvarn + tail`（已 fail-fast）；⑤ **三档在报告目录里同名 `kvarn/`、
仅时间戳区分** ⇒ **未决项 7 仍开放**（本次实测使其可见）；⑥ kvarn parser 单元测试未加；⑦ **未提交**。
**产物**：`.deps/kvarn-adm/` 内 `run5.sh`、`run5.log`、`smoke-{k4v4,k5v5,k6v6}.log`、
`{k4v4-t0,k5v5-t0,k6v6-t0}.log`、`relbf16-base.log`、`relbf16.topk`。
**计划书已回写**：版本 **v9→v10** + 「v10 相对 v9 的变更摘要」（6 行）+ §1 A2/A8 + §5「WP4 执行定案」
+ §7-WP0.5-B（标完成）/WP4（标完成）+ §10-D2/D4/D5 + 附录 **D-11**。

---

### 2026-10-07-23 — KVarN「本质」核对：报告描述 vs 本项目实现（三方审计，无代码改动）

**背景**：一份先行调研报告（面向 beellama）描述了 KVarN 的机制。核对问题：**本项目引入的 KVarN 是否符合该描述？**
**方法**：三个子代理分别审计 `D:\ninfer\beellama.cpp`（描述来源）、`D:\ninfer\KVarN`（华为原版）、本项目；主代理抽查关键点。

**结论：核心机制逐条符合；且有一条比"符合"更强的溯源证据。** 本项目**不是**照抄 beellama，而是
**华为 KVarN 的 vLLM 原版 dense 预设 `kvarn_k4v2_g128`（D256）的移植** —— `src/ops/kvarn/config.cuh:3-4`
自述并给出上游 commit `7586257f…` 与 `vllm/model_executor/layers/quantization/kvarn/config.py`。
描述讲的是**同一算法在 beellama 的 llama.cpp 实现**，本项目是**同算法的华为/vLLM 原版**，故机制逐条对得上。

| 描述项 | 本项目 | 判定 | 证据 |
|---|---|---|---|
| 免标定/方差归一化/华为系 | 头注释直接点名华为 commit + `config.py` | ✅（溯源更强） | `config.cuh:3-4` |
| per-head 自逆 WHT | 沿 head_dim 的蝶形，`*0.0625F`=1/√256 | ✅（措辞见下） | `hadamard.cuh:30,57`、`codec.cu:105-115` |
| Sinkhorn 双轴 + 取最优 imbalance | 先列后行；`if (current<=best)` 记 `best_*` | ✅ | `sinkhorn.cuh:139-163` |
| 迭代默认 16 | 本项目 **8**（= 华为 dense `config.py` 默认 8；beellama=16） | ⚠️ 数值异、与华为一致 | `config.cuh:12` |
| per-row min/max **非对称**仿射 | `scale=(hi-lo)/qmax`、`zp=lo`（存 `scale=row*rtn`/`zero=row*min`） | ✅ | `store.cuh:67-94,125-152` |
| 布局 payload+F16 scale/zp+第二轴；无 outlier/无嵌套 | K: `scale[256]+zero[256]`+`token_scale[128]`；V: `channel_scale[256]`+`token_scale[128]+token_zero[128]`，全 FP16 | ✅ | `kvarn.h:44-98` |
| group 固定 128 | `kKvarnGroup=128` | ✅ | `kvarn.h:13` |
| 位宽 {2..8}、K/V 独立、36 组合 | 只发 **{4,5,6} 且强制 K=V**（`KvarnBits{Bits4,Bits5,Bits6}`）；`<KBits,VBits>` 模板使非对称**可行但未实例化/未发布**；fork 原有 `k4v2` 已被 WP4 删 | ❌ 产物档位不符 | `types.h:84-88`、`decode.cu:229-241` |
| 旋转域 + Q 旋转 + 输出反旋转；**被门控** | Q 旋转/输出反旋转均 ✓，但**无条件**（华为 dense 亦无条件；`use_kvarn_q_rot` 门控是 **beellama 独有**） | ✅ 机制 / ⚠️ 门控不符 | `decode.cu:55-57,83,156-157` |
| 不可关 sink+精确 suffix+stage | **1 个无损 128-token sink 页 + 3 个非量化 tail 槽**；运行期不可关 | ✅（分解不同） | `kvarn.h:15-16`、`attention.cu:110,234,305`、`startup.cpp:1025-1034` |
| dim64 矩形 / 128×128 切片 | 单一**扁平 D256×G128 记录**；无 dim64、无切片 | ❌ 简化/特化 | `kvarn.h:12-13,106-108` |

**三处需要澄清的实质差异（均为"产品参数"而非算法）**
1. **WHT 的轴**：描述说"对整个 128-token tile 做 WHT"——严格说 WHT 沿 **head_dim（逐 head 的 256 维）** 做，
   128-token 是**量化 group**（Sinkhorn/仿射量化沿 tile 的行×列）。本项目与此**规范语义一致**（逐向量、D=256）。
2. **外壳被大幅裁剪**：规范支持 head_dim∈{64,128,256,512}、K/V 各 6 档（36 组合）、含 MLA；本项目 **dense-only、
   D=256 only、K=V∈{4,5,6}**。有意的工程裁剪（目标模型固定 D=256），非算法走样。
3. **门槛 8 vs 16**：本项目 8 次 = 华为 dense 默认；beellama 16 次。属"取最优 imbalance"迭代上界差异。

**对描述本身的两处小错（供修正报告，非本项目问题）**
- 描述 #2 称 256/512 的"完整逻辑 head Hadamard"在 `src/llama-kvarn.cpp:128` 附近：`:128` 实为注释；
  实现在 `ggml/src/ggml-cpu/ops.cpp:11781-11815`，注释在 `src/llama-kv-cache-kvarn.cpp:130`。
- 描述 #8 的"2×128 F16 stage"不精确：beellama 实际是 **2 tail group + 1 sink group = 3 个 F16 group**
  （第二个 tail group 仅"回滚保险"，`src/llama-kv-cache-kvarn.h:58-82`）。

**取用**：本核对为**只读审计**，无代码/构建改动；产出的"事实索引"已回写本文件 §2（4 行）。**不影响任何验收口径。**
<!-- vend @@S3BODY@@ -->

## §B 逐字归档：§0 快照的「最后更新」散文段

<!-- 源文件第 16–34 行 -->

<!-- vbegin @@B@@ -->
> 每次推进后更新本节。最后更新：**2026-10-08（记录 08-05：P2b 完成）** ——
> **P2b：MTP parity 测试改造为诊断仪器**（去逐位门禁；**加自确定性门禁** + 首分叉报告；`kKProfiles` 并入
> `rk4v4`/`kvarn:k4v4|k5v5|k6v6` 并贯通 `options.kvarn_bits`；加 `--quick` 且 `tests.cmake` 注册 `TEST_ARGS --quick`）。
> 实测（sample 0、512 token）：**自确定性全档成立**；`bf16@k1` 分叉 413/512、`rk4v4@k3` 408/512、
> **`kvarn:k4v4`@k1/k3 与 greedy 逐字节相同**、@k15 418/512（bf16 同宽 412/512，同首分叉 token 91）
> ⇒ **kvarn 自确定性成立且分叉不劣于基线**。`ctest --quick` **Passed 130.68 s**。
> 前一条 2026-10-08（记录 08-04：GPU 收尾四件 + P2 前置） ——
> **①P3a 9 臂收尾**（`kvarn:k4v4|k5v5|k6v6`×3 全 `exit=0`；**质量指标逐位重复**（极差 0）、吞吐 1.3% 内散布 ⇒ D-11 单次口径被 3× 重复**逐位证实**）；
> **②P1 e2e 验收 PASS**（kvarn r2 `cached_tokens=851 (99.4%)` 且 message 逐字节同 r1；bf16 对照 851；TTFT 721→37.9 ms 旁证）；
> **③WP5 判据余量实测**（`flips` 最大 **1/65**、`over_step=0`、`wide_flips=0` ⇒ 65× 余量，**判据收口**）；
> **④续列尾 device 段** 首跑 FAIL(2)→定位为**测试自身 3B/12B pattern bug**→修正后 `OK`；
> **P2 前置：kvarn MTP provisional 路径首次激励通过**（`mtp accepted 84/113`、加速比 1.50× ≈ bf16 1.52×）。
> 前一条 2026-10-08（记录 08-03：**纯 CPU 三件** —— (B) **WP5 容差形式化**
> （`test_kvarn.cpp` 删拟合阈值 `oracle_relative_l2_limit=1.0e-3`，改**量化步长判据**：逐元素 ≤ 1 步 +
> 边界翻码计数上限 `1.0e-3` + `|Δcode|>1` 零容忍，并补 4/5/6 位**穷举逐码往返**）；
> (A) **P1 续列尾专项单测**：新增 `test_kvarn_continuation_image.cpp`（ctest **261→262**）；
> (C) **P1 移植只读审计 = 无 HIGH/MED**（3 项 LOW 全为 FORK 继承））。
> 再前 08-02：**WP3② 续列尾实现落地** + P3a `run6.sh` 开跑；08-01：**全量 ctest 复核**；
> 07-23：**KVarN 本质三方核对**；07-22：**WP4 收尾 —— 正式 WP0.5-B 三档全过、不回退**。
<!-- vend @@B@@ -->

## §C 逐字归档：§0 WP 状态表

<!-- 源文件第 36–47 行 -->

<!-- vbegin @@C@@ -->
| 项 | 状态 |
|---|---|
| WP0 基线与环境 | **部分完成**：环境/构建已核实；整树全量构建**不通过**（先存缺陷，与本工作无关）；模型冒烟 + KLD 基线**未做** |
| WP0.5-A MTP 一致性仪器 | **定案 + 仪器改造完成（08-05）**：实跑在纯 `bf16` k=1 分叉（k=0/k=3 全等）。经上游调研（07-11）+ 审阅复核（07-12）**定案**：非本移植引入、非上游缺陷，而是 **Fork B 私有合同** ⇒ **A3 判据按 O1 替换**。**仪器改造已于 08-05 落地**（去逐位门禁 + 自确定性门禁 + 首分叉报告 + 并入 kvarn/rk4v4 档 + `--quick`；见 §3-08-05、附录 D-13） |
| WP0.5-B 准入实验（KLD 对照） | **完成（07-18）**：229k 同字节矩阵齐备（`run4.sh` 四臂全部 exit=0）。**代理门禁为正**：同字节 `kvarn:k4v2`(210 B) mean KLD **0.010513** vs `rk2v4-e8`(216 B) **0.043619** ⇒ **4.15×**（规模上与 1.4k 单窗口的 5.7× 同向同量级）。**裁决 WP4 go**。边界：绝对 KLD 上 `rk4v4`/`nvfp4`(280/288 B, 0.0044) 仍优；`k6v6` 门槛已量化为 < `k8v4`(402 B) 的 **0.002688**；正式 WP0.5-B 仍需 k4v4/k5v5/k6v6（WP4） |
| WP0.5-C 构型口径声明 | **完成**（见计划书附录 D-1，取 native 口径） |
| WP1 kvarn ops 移植 | **完成**：ops 零告警编译 ✓；测试已移植并注册 ✓；**容差项已由 WP5 收口（08-04 余量实测 65×）**；kvarn bench **未做**（需重配，待决） |
| WP2 页几何 + 存储枚举 | **完成**：`KvarnGroup128` 枚举 + `KvarnBits` profile + 三个 CLI parser + 6 处名字 switch + 指纹 `;kvbn=` + `kv_page_tokens/kv_page_shift` + 校验放宽到 64\|128。**回归：合成 242/245 通过、3 项失败全部经基线（stash 重建）证实为先前存在**；真实模型 10/16 通过，6 项失败全部为产物缺组件/无 golden 或已知 A3（见 §3-2026-10-07-13） |
| WP3 模型接入 | **完成（08-05）**：状态层、CLI 档 `kvarn:k4v4|k5v5|k6v6`、规划页几何、`text.cpp` 5 处派发 + MTP provisional + `decode.cpp` group 钳制**已落地**；**① 地址空间页几何已完成**（⇒ ctx8192/229k token 全量跑通）；**② 续列尾已完成并验收**（08-02 实现；08-03 专项单测；**08-04 host+device 段全绿**、**08-04 Engine 级 e2e 前缀复用 PASS**：`cached_tokens=851` 且 message 逐字节同）；**③ `--mtp-attention-window` 规划期拒绝（07-21）**；**④ MTP 激励（08-04，`mtp accepted 84/113`、加速比 1.50× ≈ bf16 1.52×）+ 仪器改造（08-05：parity 测试→诊断仪器、并入 kvarn 档；token 级实测 kvarn 自确定性成立、分叉不劣于基线）**。**08-03 独立审计**：无 HIGH/MED（3 项 LOW 为 FORK 继承）。 |
| WP4 位宽参数化（K=V∈{4,5,6}） | **完成（07-22）**：4.1 测绘；4.2 核按 `(KBits,VBits)` 模板化（发布档只实例化 `(4,4)/(5,5)/(6,6)`；`KBits==4` 保留 int4/nibble 快路径，V 侧统一走 `kvarn_unpack_code`；`qmax=(1<<bits)-1`）+ `bits` 经 `EngineOptions.kvarn_bits → SequencePlan → DecoderStateSpec → PagedKVCache → 视图` 贯穿；4.3 测试扩 4/5/6 **实跑全绿**（+pack/unpack 往返；**补 5/6 位宽注意力单测**；容差按位宽重定为 1.0e-3，**根因定量为单元素边界翻码**）；4.4 parser 发档 + 删 `Bits2`/`k4v2`，新增 kvarn×tail / kvarn×mtp-window 规划期拒绝；**4.5 正式 WP0.5-B 三档全过、不回退**（`k4v4` 274 B **0.002120** = 对 `rk4v4`/`nvfp4` **2.09×/2.07×**；`k5v5` 338 B **0.001432**；`k6v6` 402 B **0.001233** = 对同字节 `k8v4` **2.18×**、门槛 <0.002688 达成）；`--help`/`docs` 补档。五目标构建绿。**残余**：**各臂单次测量已由 08-04 的 P3a 3× 重复收口**（质量逐位重复、极差 0）、三档报告目录同名 `kvarn/`（未决项 7）、**MTP 激励已于 08-04 完成**（kvarn 可用）、kvarn+tail 激励（已 fail-fast） |
| WP5 容差形式化 | **完成（08-04）**：量化步长判据（逐元素 ≤ 1 理论步长 + 边界翻码计数上限 `1.0e-3` + `\|Δcode\|>1` 零容忍）+ 4/5/6 位穷举逐码往返。**08-04 实测余量**：`flips` 最大 **1/65**（65× 余量）、`over_step=0`、`wide_flips=0` ⇒ 判据成立、`exit=0`、无需放宽。口径已回写 `docs/maintainer/op-development.md §6.3` |
| WP6–WP9 | 未开始 |
<!-- vend @@C@@ -->

## §D 逐字归档：§0 工作树改动清单（WP1–WP5 / P1，逐文件）

<!-- 源文件第 49–131 行 -->

<!-- vbegin @@D@@ -->
**工作树改动清单（截至本快照）**

新增（WP1）：
- `src/ops/kvarn/`：`attention.cu`、`codec.cu`、`config.cuh`、`decode.cu`、`decode.cuh`、
  `decode_kernel.cuh`、`hadamard.cuh`、`materialized_prefill.cuh`、`sinkhorn.cuh`、`sources.cmake`、
  `store.cuh`、`streaming_prefill.cuh`（12 文件，逐字节复制自 FORK）
- `include/ninfer/ops/kvarn.h`、`include/ninfer/ops/kvarn_attention.h`（逐字节复制自 FORK）
- `tests/ops/test_kvarn.cpp`（复制自 FORK，**唯一改动**：`__builtin_popcount`→`std::popcount` 11 处 + `#include <bit>`）
- `tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（WP0.5-A，自 FORK 移植，557 行）
- 本文件 `kvarn-port-progress.md`

修改：
- `src/ops/CMakeLists.txt`：在 `kv_cache/sources.cmake` 之后增 `include("${CMAKE_CURRENT_LIST_DIR}/kvarn/sources.cmake")`
- `tests/ops/tests.cmake`：增 `ninfer_add_op_test(ninfer_kvarn_test ...)`（在 `linear_swiglu/tests.cmake` 之后）
- `tests/models/qwen3_5/tests.cmake`：增 `ninfer_qwen3_5_mtp_greedy_parity_real_test` 注册（:188-191）
- `kvarn-port-into-precision-tail-plan.md`：增 §0.5 记录规则 + 附录 D 执行记录；**再增**（07-12）
  版本 v3→v4 + 「v4 相对 v3 的变更摘要」+ §1 A3 判据替换 + §7 WP0.5-A 改造 + WP3/WP9 验收同步 +
  §9 A3 行定案 + 附录 D-6
- `docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md`：审阅复核标注（§9 标 O1 已采纳、
  §5.3 加 "(Fork B)" 标注、抬头"已回写"）
- `AGENTS.md`：增 `## Durable progress record` 一节（信息落盘制度）+ 更新 Reference navigation
  中 KVarN 行（纳入计划书 §0.5 与本进度记录）

修改（**WP2**，2026-10-07，16 文件）：

- `include/ninfer/types.h`：`KvCacheStorage` **末尾追加** `KvarnGroup128`；新增 `enum class KvarnBits`
  `{Bits4=4,Bits5=5,Bits6=6}`；`EngineOptions` 增 `KvarnBits kvarn_bits = Bits4`
- `src/core/paged_kv_cache.h`：新增 `kKvarnPageTokens=128`、`kv_page_tokens(storage)`、`kv_page_shift(storage)`
- `src/core/paged_kv_cache.cpp`：`validate_geometry` 放宽为 **64 或 128**；两处 plane 形状与形状校验
  改用 `spec.geometry.page_tokens`（原来硬写 `kPagedKVPageSize`）
- `src/serve/serve_options.{h,cpp}`、`apps/cli/options.{h,cpp}`、`apps/perplexity/main.cpp`：三处
  `--kv-dtype` parser 接受 `kvarn` / `kvarn:k4v4` / `kvarn:k5v5` / `kvarn:k6v6`，并写出 `KvarnBits`
- `src/serve/operational_log.cpp`、`src/serve/request_log.cpp`、`apps/cli/main.cpp`：三处生产 KV 名字
  switch 增 `KvarnGroup128 → "kvarn"`
- `bench/inference/ninfer_bench_support.cpp`、`bench/ops/{causal_softmax_attention_bench.cu,kv_cache_append_bench.cu}`：
  三处基准名字 switch 增同 case（bench 未构建，仅保证不落 `unknown`）
- `src/runtime/engine/model_instance.cpp`：`hybrid_cache_fingerprint` 追加 `;kvbn=<bits>`
- `src/serve/generation_service.cpp`、`apps/cli/main.cpp`：`engine_options.kvarn_bits = ...`

修改（**WP3**，2026-10-07，14 文件）：

- `include/ninfer/types.h`：`KvarnBits` 增 **`Bits2 = 2`**（临时的 k4v2 非对称编码，唯一已实现档；K=V 档到位后删除）
- `src/serve/serve_options.cpp`、`apps/cli/options.cpp`、`apps/perplexity/main.cpp`：三处 parser 改为
  **只接受 `kvarn:k4v2`**；`kvarn`/`kvarn:k4v4|k5v5|k6v6` **明确拒绝**（“not implemented yet”）；perplexity 的
  `kv_name` 增 kvarn case（WP2 漏的第 4 处名字 switch，原先抛 `unknown KV dtype`）
- `src/models/qwen3_5/state/decoder_state.{h,cpp}`：`PagedKVCacheLayout` 增 3 个 `TensorRegion`（kvarn 尾槽）
  与 `payload_bytes()` 求和；`plan_cache` kvarn 分支（单 U8 平面 `{U8, RecordBytes/Group=210, kv_heads, 256}`、
  `page_tokens=128`、绕过 `d256_kv_cache_profile`/`paged_kv_storage_layout`、单 rank 约束）+ 尾槽张量
  `{BF16,{D=256,G=128,rows*Hkv*3,layers}}`/`{I32,{3,rows,layers}}`；ctor 绑定 + `0xff` 初始化；
  `storage()`/`kv_heads()` 访问器；`layer_rank`/`layer_view` 对 kvarn 守卫/短路；
  新增 `kvarn_layer_view`/`kvarn_batch_layer_view`/`reset_kvarn_tail_row`
- `src/models/qwen3_5/program/planning/startup.cpp`：`page_count`/`maximum_main_page_groups` 增 `page_tokens`
  参数；`logical_pages`/`mtp_extra_pages`/`kv_capacity`/容量曲线/校验全部改用 `kv_page_tokens(plan.kv_storage)`；
  新增 `attention_workspace` 选择器（kvarn → `kvarn_attention_workspace_capacity_bytes`）；
  `paged_kv_window` 引用改 `page_tokens`
- `src/models/qwen3_5/execution/text.{h,cpp}`：`kvarn_provisional_` 字段 + `set_kvarn_provisional`；
  `mtp_forward_tail` 两处、`mtp_prefill_chunk`（final 块 K/V 提升出 bulk scope + `kvarn_kv_append`/final 注意力）、
  `attn_mix` 两处 —— 共 **5 处** kvarn 派发（门控由 `ops::sigmoid_mul` 在 Op 外施加）；
  `target_verify_batch_impl` 的 `ScopedValue<bool>` 与 `prefill_chunk` AR 循环的 `set_kvarn_provisional(true)`
- `src/models/qwen3_5/program/decode.cpp`：`decode_mtp_batch` 增 kvarn **group 边界钳制**（`kKvarnGroup`）
- `src/models/qwen3_5/program/speculative/mtp.cpp`：两处 `set_kvarn_provisional(true)`（decode bridge AR / decode draft 相位）

修改（**WP4**，2026-10-07，1 文件）：

- `include/ninfer/ops/kvarn.h`：记录几何由字面量常量改为 **`constexpr` 函数** `kvarn_*_offset(kb,vb)` /
  `kvarn_record_bytes(kb,vb)`（+ `kKvarnPackedBytesPerBit=4096`）；码寻址 `kvarn_k_row_bytes/k_code_bit/
  v_row_bytes/v_code_bit`；**共享位编解码** `kvarn_unpack_code`/`kvarn_pack_code`（`KVARN_HOST_DEVICE` 宏）；
  旧常量 `kKvarn*Offset`/`kKvarnRecordBytes`/`kKvarnRecordPayloadBytes` **改为由 `(4,2)` 派生**（值不变）；
  新增编译期断言（旧偏移全等 + `(4,4)/(5,5)/(6,6)`=35072/43264/51456 + 行字节数）。
  **注意：核尚未迁移**（`config.cuh` 仍 `KBits=4/VBits=2`，5 个解包例程仍在）。

修改（**WP5 / P1 单测**，2026-10-08，4 文件）：

- `tests/ops/test_kvarn.cpp`（WP5）：`codec_oracle` 返回 `CodecOracle{decoded,step,code}`；**删**
  `oracle_relative_l2_limit`；**增** `check_step_criterion`（点值 `≤ q*(1+5e-2)` + `flips ≤ 1.0e-3*total`
  + `|Δcode|>1` 零容忍，`q=max(1<<bits)-1` 步长）；`run_codec_case` 收集设备码并改用新判据；
  **增** `run_codec_bit_order_case`（4/5/6 位穷举逐码 pack→unpack + 行外哨兵）并在 `main` 注册。
- `tests/models/qwen3_5/test_kvarn_continuation_image.cpp`（**新增**，P1 续列尾专项单测）：
  host 段（几何 vs 分页尾、transfer work、capture→activate 逐字节/逐 token 往返、非法 spec 拒绝）
  + device 段（真实 `StateImageDevicePool` 的 kvarn 尾视图 / `copy_slot` / `zero_slot` / D2D 往返 /
  MTP 不别名 / 越界拒绝）。
- `tests/models/qwen3_5/tests.cmake`：注册 `ninfer_qwen3_5_kvarn_continuation_image_test`（**ctest 261→262**）。
- `docs/maintainer/op-development.md` §6.3：写入「量化 codec 的步长判据」条款（点值 + 总量上限）。
<!-- vend @@D@@ -->

## §E 逐字归档：§0 未决项 1（已关闭 —— 08-04 余量实测收口）

<!-- 源文件第 135–142 行 -->

<!-- vbegin @@E@@ -->
1. ~~`ninfer_kvarn_test` 的 4-bit K codec oracle 容差~~ → **已收口（07-21，WP4.3）**：`ninfer_kvarn_test`
   全绿（三档）。实测 bits=4 的 K `relative_l2=6.0882e-4`、`max_abs=0.216`（**与重构前逐位相同**
   ⇒ 4-bit K 零回归）；bits=6 的 K **rel_l2=0**。**根因已定量**：`max_abs` 恰为一个量化步长，且
   `sqrt(0.216²/Σdecoded²)≈8.4e-4 ≈ 实测 6.09e-4` ⇒ 偏差来自**单个元素落在量化边界、设备与 oracle
   各自舍入到相邻码**，不是 codec 缺陷（自洽性由 stored-bit 2.0e-7 + pack/unpack 往返钉死）。
   **07-21 的 `oracle_relative_l2_limit(bits)=1.0e-3` 已于 08-03 删除并形式化**为量化步长判据
   （逐元素 ≤ 1 步 + 边界翻码计数上限；见 §3-2026-10-08-3）。**残留**：该判据的**数值余量未实测**
   （GPU 被 P3a 独占）⇒ 首跑须确认 `kMaxFlipFraction=1.0e-3`（65/65536）与 `kStepSlack=5e-2` 成立。
<!-- vend @@E@@ -->

## §F 逐字归档：§0 未决项 5（A3 定案依据，已裁决采纳 O1）

<!-- 源文件第 146–167 行 -->

<!-- vbegin @@F@@ -->
5. ~~**A3 判据不成立，需裁决替换**~~ → **已裁决（2026-10-07-12）：采纳报告 O1**。以下为定案依据（保留）。
   上游 GitHub **issue #265**（open，2026-09-16，
   维护者从未回复）**记录了同族缺陷但未修复**；不过 #265 的机制（nvfp4 GDN 按宽度选 GEMM schedule 表）
   **与本机无关**——本机 IQ3_XXS 权重走 `gguf_project`，宽度 1/2/4 落**同一个 MMV kernel**
   （`gdn_input_proj.cpp:603` 的 nvfp4 路由要求 `QType::NVFP4`；`linear/gguf/gguf_linear.cpp:183`
   唯一宽度键是 `t<=8`）。本机真实机制是 **bf16 SmallT attention 按宽度改变 CTA 内归约形状**
   （`small_t.cu:383-395`：宽度 1 用 `WarpsPerCta=2`，宽度 2–8 用 `4`），且 decode/verify 走不同入口
   （`text.cpp:571` cached/unmasked vs `text.cpp:965` batched masked）。bf16 的 split 数只由 window
   决定、与宽度无关（`small_t.cu:67-85`），故差异在 CTA 内 ⇒ **near-tie argmax 翻转**；
   **k=3 通过是运气**（宽度 4 用 `<_,4,4>`，与宽度 1 的 `<_,1,2>` 不同）。详见 §3-2026-10-07-11。
   **上游合同明确不保证跨路径逐位一致**（#80 维护者原文 "Not in scope. Greedy does not mean batch
   invariant."；`qwen3_5-model.md:328-330`、`op-development.md:397`），**且 TAIL 本仓文档自己也这样写**
   （`docs/performance.md:29-45` 实测 23 构型：width-1 greedy 哈希**无任何投机构型匹配**；
   `docs/archive/TODO.md:4462-4464`：「断言与 greedy 逐位相同的测试**会永久按设计失败**，比没有测试更糟」）。
   **A3 的真实来源是 FORK B（rtx5090-mobile）的私有合同**，且依赖其自有 kernel 改动
   （`7d566547`/`d476fafa` 引入 "canonical per-query arithmetic profile"，强制 `tokens=1; batch_size=1`），
   **TAIL 只含上游的 `a7818988`**（宽度特化调度的源头）。⇒ **建议把 A3 从"绝对 parity"改为**
   ①同构型自确定性（已成立）②perplexity 质量一致 ③分叉已文档化 ④"kvarn 不得使一致性劣于同配置基线"。
   **已裁决**：A3 按 **O1** 替换（①同配置自确定性 ②质量不劣化 ③首分叉文档化 ④ kvarn 专属"不劣于同配置基线"）；
   移植的 parity 测试**改造为诊断仪器**，不再作通过/失败门禁。
   **已回写计划书 §1 A3 / §7 WP0.5-A·WP3·WP9 / §9（计划 v4 附录 D-6）**。O2（移植 canonical 算术面）**拒绝**
   （性能代价未量化、与 #80 "batch invariant limits optimizations" 立场冲突）。
<!-- vend @@F@@ -->

## §G 逐字归档：§0 未决项 8（WP3 长上下文阻塞，已解决）

<!-- 源文件第 177–182 行 -->

<!-- vbegin @@G@@ -->
8. ~~**WP3 长上下文阻塞（07-14 实测定位）**~~ → **已解决（07-16，WP3①；计划附录 D-9）**：根因是
   `kv_pages_for_frontier`/`kv_pages_for_tokens` 与 `KVAddressSpaceStore` 内部**硬写 64**，与 kvarn 的
   128-token 池几何不一致（entitlement 77 页 > `page_capacity_` 39 页 ⇒ `create_active` 返回 `nullopt`）。
   **修法**：三族 helper 增 `KvCacheStorage` 参数（42 调用点传 `kv_storage`）、`KVAddressSpaceStore` 由
   构造期池几何取 `page_tokens_`、`LogicalKVPageStore` 列上限取池几何、页跨度字面量同步（非 kvarn 仍 64）。
   **验收达成**：`kvarn:k4v2` 在 ctx8192 / 229,348 token / 28 窗口全量跑通（PPL 4.72225 vs bf16 4.69317）。
<!-- vend @@G@@ -->

## §H 逐字归档：§0 未决项 9（WP3 续列尾「未实现」—— 已被 08-02/08-04 取代）

<!-- 源文件第 183–187 行 -->

<!-- vbegin @@H@@ -->
9. **WP3 未做（续列尾）**：`state_image` 的 kvarn 续列镜像（`KvarnContinuationStateSpec` /
   `kvarn_text_tail`/`kvarn_mtp_tail`）与 `program_impl` 的
   `restore/capture/activate_sequence_kvarn_tail` + 三处调用点（`prefill.cpp`、`transactions/capture.cpp`、
   `transactions/commit.cpp`）**未实现**（本轮只声明过又撤回，避免留下未定义声明）。⇒ 目前 kvarn 的
   **前缀复用/续列**不成立（单请求 fresh 路径可用）。
<!-- vend @@H@@ -->

## §I 逐字归档：§0 未决项 10（mtp-attention-window / kv-tail-tokens 规划期拒绝，已实现）

<!-- 源文件第 188–192 行 -->

<!-- vbegin @@I@@ -->
10. ~~**kvarn 与 `--mtp-attention-window` 不兼容**~~ → **已加规划期拒绝（07-21）**：该 MTP 窗口变换走
   64 页块表（`ops::paged_kv_window_rows`），对 128-token body 语义未验证。`validate_target_options`
   现对 `kvarn` 拒绝 `--mtp-attention-window != 0`；**同一条守卫同时拒绝 `--kv-tail-tokens != 0`**
   （精确尾部未接进 kvarn body，此前会被**静默忽略** ⇒ 用户以为有尾而实际没有，属真实静默缺陷，
   07-21 发档使其可达故一并 fail-fast）。**残余**：kvarn 的真实 MTP 路径激励与 A3 相对判据仍属 WP3④。
<!-- vend @@I@@ -->

## §J 逐字归档：§4.1 任务清单（P0–P5 原始定义）与 §4.4 历史 WP 状态

<!-- 源文件第 1574–1607 行（§4.1），随后第 1615–1648 行（§4.4）；两段之间在原文件中隔着 4.2/4.3 两小节 -->

<!-- vbegin @@J@@ -->
### 4.1 任务清单（P = 优先级；每项"目标 / 内容 / 验收 / 风险 / 估时"）

**P1 — WP3② 续列尾（前缀复用 / 检查点）**〔**功能缺口，最大价值**〕
- 目标：让 kvarn 支持跨请求**前缀复用**与 `state_image` **检查点往返**（当前仅 fresh 路径可用，未决项 9）。
- 内容：`KvarnContinuationStateSpec` + `kvarn_text_tail`/`kvarn_mtp_tail`；`program_impl` 的
  `restore/capture/activate_sequence_kvarn_tail`；调用点 `prefill.cpp`、`transactions/{capture,commit}.cpp`。
  顺带回写计划书 §7-WP3 的「`small_t.cu:460-505` 挂载」措辞为「模型执行层 `text.cpp` 分派」实测。
- 验收：同一 prompt 二次请求（或检查点存/取）后 **PPL 与 fresh 路径不劣化**（ΔPPL ≈ 0，口径同 §3-07-22）。
- 风险：**尾槽/页几何在续列镜像里的偏移一致性**（易静默错位）⇒ 必须加"**续列往返逐 token 对拍**"单测。
- 估时：3–5 天。

**P2 — WP3④ MTP 路径激励 + A3 相对判据**〔判据补全〕**✅ 完成（08-04 激励 / 08-05 仪器）**
- 目标：证明 kvarn 在 MTP 投机构型下一致性**不劣于**同配置基线（A3 按 O1）。
- 内容：~~用 `ninfer`/`ninfer-serve` 跑 MTP 生成~~（08-04 `.deps/kvarn-adm/p2_mtp_gen.sh`：kvarn 接受 84/113、
  加速比 1.50× ≈ bf16 1.52×）；~~把逐位断言改为诊断仪器~~（08-05 完成：去逐位门禁 + 自确定性门禁 + 首分叉报告 +
  并入 `rk4v4`/`kvarn:k4v4|k5v5|k6v6`）。
- 验收（达成）：三构型 MTP **同量级**（加速比/接受率）；仪器下 **kvarn 自确定性成立**（全档 `self_det=ok`）、
  **分叉不劣于基线**（`kvarn:k4v4`@k1/k3 identical；@k15 418/512 vs bf16 412/512，首分叉同为 token 91）。
- 风险（未触发）：kvarn MTP provisional 路径首次激励**未见规划/几何缺陷**。
- 残留：仅 `sample 0`、宽度 {0,1,3,15}、`k5v5/k6v6` 只测 k=1；测试名仍含 `parity`（未改名，记为未决项）。

**P3 — WP4 补强（可与 P1/P2 并行）**〔加固既有结论〕
- **P3a**：229k 三档 **≥3 次重复**测量，给出 mean KLD / tok/s 的**均值±散布**（当前单次，D-10 同口径）。
- **P3b**：修未决项 7 —— 三档报告目录同名 `kvarn/`：把 `KvarnBits` 并入 `MemorySummary`/报告名/日志名
  （现仅指纹 `;kvbn=` 区分）。
- **P3c**：kvarn parser 单元测试（`ninfer_cli_options_test`/`ninfer_serve_options_test` 增 kvarn 用例）。
- 验收：重复测量给出稳定区间；报告目录可按档区分；parser 单测通过。

**P4 — WP5 容差形式化 + bench 归属**〔口径收口〕
- **P4a**：把 `ninfer_kvarn_test` 的 oracle 容差从 `1.0e-3` 形式化为**量化步长判据**（逐元素 ≤ 一步 +
  边界翻码计数上限），并把口径写进 `docs/`。
- **P4b**：kvarn bench 是否纳入 —— 需 `-DNINFER_BUILD_BENCHMARKS=ON` 重配（与"不重配"冲突）⇒ **需显式决定**（未决项 3）。

**P5 — WP6 评估**〔最高风险，门禁后启动〕：10–15 天，计划书 §7-WP6。

**（下接 §4.4「历史 WP 状态」，源文件第 1615-1648 行；原文件中两段之间的 4.2/4.3 两小节仍留在日志正文）**

### 4.4 历史 WP 状态（存档）

1. ~~**WP0.5-A**（进行中）~~ → **定案**（§3-2026-10-07-12）：仪器已交付并**改造为诊断仪器**；A3 判据按
   O1 替换。**剩余动作**：把 `test_engine_mtp_greedy_parity_real.cpp` 的逐位断言改为"自确定性 +
   报首分叉下标/分叉率/KLD 差"（属仪器改造，与 WP3 一并做）。已确认的 API 差异（供改造时参考）：
   FORK 用 `KvCacheStorage::KvarnK4V2Group128`（TAIL 无）、`speculative.mtp_draft_policy`
   （TAIL 名为 `speculative.mtp_policy`）、`enable_nvfp4_scale_compression`+`compressed_scales`（TAIL 均无）。
2. ~~**WP0.5-B**~~ → **完成（§3-2026-10-07-18 代理档；§3-2026-10-07-22 正式档）**：229k 同字节矩阵齐备，
   `kvarn:k4v2`(210 B) vs `rk2v4-e8`(216 B) = **4.15×**（mean KLD 0.010513 vs 0.043619）⇒ **代理门禁为正、
   裁决投 WP4**。**正式版已完成**：发布档 `k4v4`/`k5v5`/`k6v6` 对 `rk4v4`/`nvfp4`/`k8v4` **三档全过、不回退**
   （见 §3-07-22 与计划书附录 D-11）。
3. ~~**WP1 补齐**：待 WP5 定容差~~ → **容差项已收口（07-21）**：`ninfer_kvarn_test` 三档全绿，
   根因定量为单元素边界翻码（见未决项 1）。**bench 归属仍待决**（需重配，见未决项 3）。
   **WP5 剩余**：把容差形式化为量化步长判据（逐元素 ≤ 一步）并把该口径写进 `docs/`。
4. ~~**WP2**~~ → **完成**（§3-2026-10-07-13）：枚举 + parser + 名字 + 指纹 + 页几何（64|128）已落地并
   经 261 测试回归 + 基线对照。**遗留（刻意，见 §0 未决项 6/7）**：kvarn 档尚不可运行（两处 host
   switch / `plan_cache` kvarn 分支 / route 挂载属 WP3）；`--help` 与 `docs/` 未改；kvarn parser 单测未加。
5. **WP3（进行中，2026-10-07-14 起手；① 于 07-16 完成）**：状态层平面/尾槽/视图、规划页几何、
   `text.cpp` 5 处派发、MTP provisional、decode group 钳制**已落地**（§0 清单）；
   **① 地址空间页几何已完成**（§3-2026-10-07-16）⇒ **KVarN 在 ctx8192 / 229,348 token 全量跑通**
   （当时内部档 `kvarn:k4v2` PPL 4.72225；正式档 `k4v4` 于 07-22 复核为 4.69436）。**剩余（按依赖顺序）**：
   ② **续列尾**（未决项 9）：`state_image` kvarn 镜像 + `program_impl` 的
   `restore/capture/activate_sequence_kvarn_tail` + `prefill.cpp`/`transactions/{capture,commit}.cpp` 调用点；
   验收 = 前缀复用/检查点往返后 PPL 不劣化。**（kvarn 前缀复用/续列目前不成立，单请求 fresh 路径可用。）**
   ③ ~~**`--mtp-attention-window` 的 kvarn 规划期拒绝**（未决项 10）~~ → **已完成（07-21）**：
   `validate_target_options` 对 kvarn 同时拒绝 `--mtp-attention-window != 0` 与 `--kv-tail-tokens != 0`。
   ④ **MTP 路径激励与 A3 相对判据**：用 MTP 生成（`ninfer`/`ninfer-serve`）跑 kvarn 与 `bf16`/`rk4v4`
   基线并排，检查 MTP 一致性**不劣于**基线；**顺带**把 `test_engine_mtp_greedy_parity_real.cpp` 改造为
   诊断仪器（去逐位断言）。
   ⑤ **回写计划书**：§7-WP3 的「`small_t.cu:460-505` body 挂载/`small_t_kvarn`」措辞改为「模型执行层
   `text.cpp` 分派」实测；§1-A3 相对判据已就位；§9/§10 未决项同步（D2 的裸 `kvarn` ≡ k4v4 已在 v10 标注
   “WP4 生效”）。
   ⑥ ~~`--help`/`docs/` 的 kvarn 档说明~~ → **已完成（07-22，WP4⑤）**。**残留**：kvarn parser 单元测试
   （`ninfer_cli_options_test`/`ninfer_serve_options_test` 增 kvarn 用例）**仍未加**（未决项 7）。
<!-- vend @@J@@ -->


## §K 逐字归档：正文中被订原句（订正前原文）

精简时正文有 5 处**表述订正**（非历史条目）。为满足「无信息丢失」，订正前的原句逐字保留如下：

| 位置 | 订正原因 | 原文（逐字） |
|---|---|---|
| 计划书原第 800 行（附录 D 标题） | 标题只覆盖 WP0/WP0.5-C/WP1，已不覆盖 D-10…D-13 | 「## 附录 D：执行记录（WP0 / WP0.5-C / WP1，2026-10-07）」 |
| 进度日志原第 6 行（配套权威） | 原写「方案 v3」，计划书现已 v13（陈旧指针） | 「**配套权威**：`kvarn-port-into-precision-tail-plan.md`（方案 v3，active authority）。」 |
| 进度日志 §5 原第 1669 行 | 「工作树未提交」已不成立（2026-10-08 已提交） | 「2. 工作树 **未提交**（用户约束：不要提交）。清单见 §0「工作树改动清单」——本轮新增/改：」 |
| 进度日志 §5 原第 1670 行 | 同上（该三项改动现已提交） | 「   `tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（诊断仪器改造）、」 |
| 进度日志 §5 原第 1671 行 | 同上 | 「   `tests/models/qwen3_5/tests.cmake`（`TEST_ARGS --quick`）、`test_kvarn_continuation_image.cpp`（marker 尺寸 bug 修复）。」 |

订正前原文的 fenced 逐字节副本（供机器比对）：

```text
## 附录 D：执行记录（WP0 / WP0.5-C / WP1，2026-10-07）
> **配套权威**：`kvarn-port-into-precision-tail-plan.md`（方案 v3，active authority）。
2. 工作树 **未提交**（用户约束：不要提交）。清单见 §0「工作树改动清单」——本轮新增/改：
   `tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（诊断仪器改造）、
   `tests/models/qwen3_5/tests.cmake`（`TEST_ARGS --quick`）、`test_kvarn_continuation_image.cpp`（marker 尺寸 bug 修复）。
```

---

## §L 逐字归档：§3 记录 2026-10-08-1 … -13 与 -17（已关闭的 P 包 / WP 链）

> **搬运来源**：`kvarn-port-progress.md` §3（2026-10-08 条目 **-1 … -13** 与 **-17**）。**逐字副本，未改写一字。**
> 这些条目对应 **P0 / P1 / P2 / P3b / P3c / P3d / §6.3 探针 / 尾环 dtype 分析**，均**已关闭**；
> 其结论已由进度 **§0 快照 / §4.1 / §5** 与计划书附录 **D-13 … D-22** 承接，故不必留在正文。
> 正文 §3 保留 **WP6 链的 -14 / -15 / -16 / -18 / -19**（未完成的 WP6.4–WP6.7 的直接前驱）。

### 2026-10-08-1 — 全量 `ctest`（261 项，**带 artifact**）复核：245 通过 / 7 跳过 / 9 失败；**9 项全部非 kvarn 引入**（含 1 项**从未取过基线**的新观测）

**触发**：用户指示检查上一窗口已跑完的全量 ctest 成果。

**结果**（`/tmp/ct_full2.log`、`build-port/Testing/Temporary/LastTest.log`，总时长 1990.61 s）：注册 **261**，
**245 通过 / 7 跳过 / 9 失败**（`97% tests passed`）。**该次为并发运行**（首个结果前有 **23** 个 `Start`
⇒ 显存密集的 real 测试互相争抢）。**`ninfer_kvarn_test` 已通过**（07-21 按位宽重定容差后收口；WP1 遗留项关闭）。

**9 项失败逐项对照基线（`LastTestsFailed.log` 全 9 项）**

| # | 测试 | 实测消息 | 判定 | 依据 |
|---|---|---|---|---|
| 20 | `ninfer_device_sync_empty_test` | `invalid sync setting did not fail before CUDA initialization` | **先存**（改动面外） | 与 D-7 逐字相同；`tests/cmake/CoreTests.cmake:82-83` 的 `ENVIRONMENT` 未把空串交给 `getenv` |
| 84 | `ninfer_qwen3_5_prefix_real_test` | `registered tokenizer/chat template has no prompt golden` | **产物结构性** | 该产物无 prompt golden（D-7 同） |
| 85 | `ninfer_qwen3_5_hybrid_prefix_real_test` | 全量：`... only 0 bytes are available for runtime capacity`；**隔离复跑**：`Qwen3.5 config: missing component dflash2` | **产物结构性**（全量那条是**并发显存争抢**的次生现象） | 隔离复跑消息与 D-7 逐字相同 ⇒ 全量差异来自 `-j` 并行 |
| 89 | `ninfer_qwen3_5_vision_workspace_test` | `Vision workspace exceeded the single-item bound: at=1205905409 above=1205905409` | **先存 / artifact 门控 / 首次观测**（见下） | 代号见下 |
| 90 | `ninfer_qwen3_5_dflash2_real_test` | `Qwen3.5 config: missing component dflash2` | **产物结构性** | 产物缺 DFlash2 组件（D-7 同） |
| 91 | `ninfer_qwen3_5_moe_real_test` | `35B Engine construction has an invalid load summary: target=Qwen3_5ForCausalLM` | **产物结构性** | 本产物非 35B MoE（D-7 同） |
| 93 | `ninfer_qwen3_5_dflash_real_test` | `FATAL: Qwen3.5 config: missing component dflash` | **产物结构性** | 产物缺 DFlash 组件（D-7 同） |
| 101 | `ninfer_qwen3_5_mtp_greedy_parity_real_test` | `bf16 k=1 sample=0 row=0 prompt=68 mismatch at token 91: expected=2466 actual=2640` | **已知 A3** | 与 D-4/D-7 逐字相同 |
| 119 | `ninfer_gdn_gating_proj_test` | `qwen3_6_27b/35b_a3b: GDN control interval missed a route endpoint` | **先存**（改动面外） | 与 D-7 逐字相同 |

**`ninfer_qwen3_5_vision_workspace_test`（#89，唯一新观测）判定为先前存在，四条依据：**
1. **改动面外**：测试文件 `tests/models/qwen3_5/test_vision_workspace.cpp` 与其数值来源
   `ops::causal_softmax_attention_workspace_capacity_bytes`、`src/ops/softmax_attention/dense/causal_cache/*`
   **均不在本次工作树改动清单内**（`git status` 无此二处）。
2. **我们改动的规划代码对该测试逐位等价**：`build_workspace_plan`/`page_count`/`maximum_main_page_groups`/
   `resolve_host_cache_budget` 全部以 `kv_page_tokens(storage)` 参数化，而本测试用
   `KvCacheStorage::Fp8E4M3Row256` ⇒ `kv_page_tokens` 恒 **64**（非 kvarn 逐位不变）；新增的
   `attention_workspace` lambda 对非 kvarn **原样转发**同一函数同一实参。
3. **失败的是主机标定上限，不是功能契约**：失败断言是 `at_item_limit > kWorkspaceCeiling(827 MiB)`
   （`test_vision_workspace.cpp:60-64`）；真正的单件上界契约（`above_item_limit == at_item_limit`）**通过**（两者均
   `1,205,905,409`）。
4. **隔离复跑（独显空闲、单测）**：**同样失败且数值逐位相同**（`at=1205905409 above=1205905409`，exit=1）
   ⇒ **确定性、非并发争抢**。
   （残留：**未做**一次「复原到 HEAD 的基线重建」以取得旧数值——但由 1+2 可知基线数值必同为 `1205905409`，
   即基线同样超限；该测试此前在所有基线运行中均为 **Skipped**（未设 `NINFER_TEST_ARTIFACT`），从未取过基线。）

**结论**：**本次全量 ctest 未发现任何由 kvarn 移植引入的失败**；A1 的「全绿」口径须补入 #89 这一
**artifact 门控的先存失败**（已回写计划书 §1-A1）。**未修 #89**（先存、且与 kvarn 交付面无关；属文档/测试校准范畴）。

**未做（如实记录）**：未做「复原 HEAD 的基线重建」（见上残留，代价 ~20–40 min，判为不必要）；
未串行重跑全量（仅隔离复跑 #89 与 #85 两项有歧义者）；**未改任何源码**；**未提交**。
**产物**：`/tmp/ct_full2.log`、`/tmp/vision_isolated.log`、`/tmp/hybrid_isolated.log`。

### 2026-10-08-2 — WP3② 续列尾**实现落地**（构建绿；e2e 验收待 GPU 空出）+ P3a 并行开跑

**触发**：用户裁定下一步 = **P1（WP3② 续列尾）+ 并行 GPU 补强**（见 08-01 的 AskUserQuestion 结果）。

**P3a（并行，GPU，后台）**：新建 `.deps/kvarn-adm/run6.sh`（协议与 run5.sh 逐字相同：`--corpus … --quick --context 8192 --disjoint --score-width 8 --score-topk 100`，共享 `bf16-t0.topk`），
`kvarn:k4v4|k5v5|k6v6` 各 **3 次**（共 9 臂）⇒ 产出 mean KLD / tok/s 的均值±散布（收口 D-11 的**单次测量**口径）。
00:36 启动（`run6.log`），GPU 11.6 GiB / 94%，预计 **~2 h**。

**WP3② 续列尾实现（6 文件，以 FORK `ninfer-rtx5090-mobile` 为参考，逐点核对后移植）**

| 文件 | 改动 |
|---|---|
| `state/state_image.h` | `+KvarnContinuationStateSpec`/`KvarnContinuationImageLayout`/`KvarnContinuationStateLayout`；`StateImageSpec.kvarn`；host layout `kvarn`+`kvarn_layout`；device layout `kvarn`；`StateImageDevicePool` 增 `has_kvarn`/`kvarn_text_layers`/`kvarn_mtp_layers`/`kvarn_text_tail`/`kvarn_mtp_tail` + 4 个成员 |
| `state/state_image.cpp` | `checked_i32`；`same_kvarn_spec`/`same_kvarn_layout`；`plan_kvarn_continuation_image`；`same_host_layout`/`plan_host_state_image`/`state_image_transfer_work` 并入 kvarn；设备池在 **rank 0** 加 `{image_bytes, slot_count}` U8 张量；ctor 绑定/校验并回填 `device_spec.kvarn`；`kvarn_tail_view` + 两个访问器；`zero_slot`/`zero_all`/`copy_slot`/`copy_to_host`/`copy_from_host` 与 `for_each_host_component` 的 **Rest** 分支均并入 kvarn |
| `program/program_impl.h` | 声明 `restore/capture/activate_sequence_kvarn_tail`（紧邻 `trim_sequence_kv`） |
| `program/storage/context.cpp` | `#include ninfer/ops/kvarn_attention.h`；`trim_sequence_kv` 尾部调 `restore_sequence_kvarn_tail`；三个函数实现（**`KvarnGroup128`** 取代 FORK 的 `KvarnK4V2Group128`；`compute_streams[0]` 取代 FORK 的 `device.stream`；用 `addresses.execution_row()` + `execution.kvarn_layer_view()`；restore 按 16 层一批） |
| `program/prefill.cpp` | 复用路径 `bind_sequence_kv` 后加 `activate_sequence_kvarn_tail(sequence)`（对应 FORK prefill.cpp:636） |
| `program/transactions/{capture,commit}.cpp` | `state_store->freeze(...)` **前**加 `capture_sequence_kvarn_tail(...)`（对应 FORK :684 / :635） |
| `program/planning/startup.cpp` | `KvarnGroup128` 时填 `state_image_spec.kvarn = {full_attention_layers, mtp?1:0, kv_heads, head_dim}`（对应 FORK :155-161） |

**适配点（TAIL vs FORK 的结构差异）**：① TAIL 的 `StateImageDevicePool` 是**多 rank 分片**（`shards`/`RankStreams`/`StateImagePart`/分段拷贝），FORK 是单 rank ⇒ kvarn 镜像绑 **rank 0**（`backings.front()`），并在 `for_each_host_component` 的 `Rest` 分支以 `rank=0` 暴露，**分段拷贝自动覆盖**；② 流参数：`compute_streams[0]`；③ 枚举名 `KvarnGroup128`；④ `checked_i32` 在 TAIL context.cpp **不存在** ⇒ 新增文件内 `kvarn_frontier_i32`；⑤ TAIL 的 host-layout 一致性检查在 ctor 中**先于** DFlash 段 ⇒ kvarn 绑定插在检查之前。

**构建**：`ninfer_ops ninfer_tests ninfer`（42 步）+ `ninfer-serve` **exit 0、0 error**。
**合成回归**（低显存，与 P3a 并存）：`ninfer_qwen3_5_state_image_test` / `ninfer_qwen3_5_state_image_layout_test` / `ninfer_state_store_test` **均 exit 0** ⇒ 未破坏既有 state-image 管道。

**未做 / 待做（如实记录）**：① **e2e 验收未做**（前缀复用 / 检查点往返）——需 GPU 加载 27B，而 P3a 正独占 GPU（~11.6 GiB）⇒ 待 P3a 完成（预计 02:36）；② `ninfer-perplexity` 未重链（其 exe 被 P3a 持有；P3a 用的是**改动前**的二进制，但其测量路径属 fresh 单请求、不经本改动的 capture/activate ⇒ 结论不受影响）；③ 「续列往返逐 token 对拍」单测**未写**；④ **未提交**。
**产物**：`.deps/kvarn-adm/run6.sh`、`run6.log`、`<spec>-t0-r<rep>.log`（进行中）。

### 2026-10-08-3 — WP5 容差形式化（构建绿；GPU 侧未跑）+ P1 续列尾专项单测（host 段实跑绿 / device 段待 GPU）+ P1 移植独立审计（无 HIGH/MED）

**触发**：用户裁定本轮做**纯 CPU**工作（GPU 被 P3a 独占）：(A) P1 续列尾专项单测、(B) WP5 容差形式化、(C) P1 移植只读审计。

**(B) WP5 容差形式化 —— `tests/ops/test_kvarn.cpp`**

- `codec_oracle` 返回值由 `std::vector<double>` 改为 `CodecOracle{decoded, step, code}`：`step` 是**逐元素理论量化步长**
  `q = (max-min)/qmax * row_scale * column_scale`（`qmax=(1<<bits)-1`，即相邻码的值距），`code` 是 oracle 在均衡域选定的整数码。
- **删除**拟合阈值 `oracle_relative_l2_limit(bits)=1.0e-3`，改为 `check_step_criterion(...)`：①**点值判据** `|actual-expected| <= q*(1+kStepSlack)`，`kStepSlack=5e-2`（吸收设备与 oracle 各自 Sinkhorn 的 scale 差异）；②**总量判据** `flips <= kMaxFlipFraction*total`，`kMaxFlipFraction=1.0e-3`（65536 元素 ⇒ 上限 65）；③`wide_flips`（`|Δcode|>1`）**零容忍**。口径与 `docs/maintainer/op-development.md §6.3`「pointwise bound + finite gross pointwise-error cap」一致，**已回写该文档**。
- 新增 `run_codec_bit_order_case()`：4/5/6 位**穷举逐码** pack→unpack 往返（每个可表示码 × 每个行内字段偏移，约 4.3 万组合），并置**行外哨兵 `0xff`** 证明码读写不越出所属行；另校验 pack 不污染未触碰字节。
- **实测**：`ninfer_tests` 目标构建 **exit 0 / 0 error / 0 warning**。**未跑**（`ninfer_kvarn_test` 是 CUDA 测试，GPU 被 P3a 独占）⇒ **判据的数值余量未实测**：`kMaxFlipFraction` 与 `kStepSlack` 由**分析**给出（预估设备/oracle 的 Sinkhorn 相对差 ~2e-6 ⇒ bits=4 约 1 次翻码、bits=5 约 2、bits=6 约 0–4；与 07-21 实测 bits=4 K `rel_l2=6.09e-4`（单元素翻码）、bits=6 `rel_l2=0` 自洽）。**必须在 GPU 空出后首跑确认**；若超限即为**真实发现**（记录并定量定标），**不得事后放宽**。

**(A) P1 续列尾专项单测 —— 新文件 `tests/models/qwen3_5/test_kvarn_continuation_image.cpp`**

- 注册 `ninfer_qwen3_5_kvarn_continuation_image_test`（`tests/models/qwen3_5/tests.cmake`）⇒ **ctest 261 → 262**。
- **host 段（无设备，本轮实跑）**：kvarn 续列镜像几何 vs **分页 KV 尾几何**对拍（`tail_layer_bytes == BF16[D,G,Hkv*3]` 字节数；`marker_layer_bytes == I32[3]`；`Hkv*3*D*2*G` 覆盖一整层尾；**16 层 K+V == 24.0 MiB**，与附录 A 口径一致）；六个组件 256B 对齐、互不重叠、落在镜像内；MTP=0 变体无 MTP 偏移；`state_image_transfer_work` 载荷含镜像且**镜像恰为 1 次拷贝操作**；无 kvarn 的 spec 真报 `!kvarn`；非法 spec（head_dim≠256 / text_layers=0 / kv_heads=0）被拒；**capture→activate 往返逐字节 + 逐 token 对拍**（源按分页尾几何、目的偏移取镜像布局 ⇒ **非自证**）。
- **device 段（需 GPU，待跑）**：真实 `StateImageDevicePool` + kvarn —— `zero_all/zero_slot`（K/V 清 0、markers 复位 `0xff`）、`copy_slot` D2D 携带整层尾、D2D 拷回分页形缓冲后**逐字节对拍**、MTP 池与 text 池**不互为别名**、越界层/槽抛 `out_of_range`、尾视图几何 == 分页尾几何。
- **实测**：`ninfer_tests` 构建 exit 0；**host 段实跑**（`CUDA_VISIBLE_DEVICES=99` 强制无设备 ⇒ 只走 host 段、**零显存占用**）**exit 0**。
  首跑曾**暴露我方一处错误假设**：误以为 host 镜像偏移 == device 区域偏移；实为**两套独立 `LayoutBuilder`**（ctor 以「由 device 组件重建 host 布局并比对」`state_image.cpp:459` 兜底）⇒ 断言已按实测修正为「host 镜像 == **单槽**字节数」。**device 段未跑**。
- **未做（如实记录）**：**未**做「复用路径 vs 一次性 prefill 的 **Engine 级 27B** 逐 token 对拍」（需 artifact + 显存，即 `.deps/kvarn-adm/p1_prefix_reuse.sh` 的缓存命中/PPL 口径，仍待 GPU）。

**(C) P1 移植独立审计（只读子代理，vs 上游 FORK）**：结论 **无 HIGH/MED 缺陷**。全部 4 个调用点齐备（trim `context.cpp:1714` / prefill `prefill.cpp:661` / capture `transactions/capture.cpp:726` / commit `transactions/commit.cpp:660`，相对位置与 FORK 相同）；`plan_kvarn_continuation_image`（`state_image.cpp:92-119`）与 `kvarn_tail_view`（`:501-522`）与 FORK 逐点相同；与 TAIL `decoder_state` 的 kvarn 板（`{256,128,rows*Hkv*3,layers}` / `{3,rows,layers}`）一致；**分片架构由规划期守卫兜底**——KVarN 要求所有 attention 层同 rank（`decoder_state.cpp:53-61`）⇒ 绑 rank 0 安全；`zero_slot/zero_all/copy_slot/copy_to_host(_segments)/copy_from_host(_segments)` 的 Rest 分支**无重复计数/漏项**；非 kvarn 逐位不变（三处早退 + `startup.cpp:241` 门控）。

**三项 LOW（均为 FORK 继承，非本次引入）**：① `reset_kvarn_tail_row`：TAIL **零调用者**（`decoder_state.cpp:360`），FORK 有 **2 处且都在同一测试** `tests/models/qwen3_5/test_prefill_precision_real.cpp:236,284` —— **该测试未被移植到 TAIL**（TAIL `tests/models/qwen3_5/` 无此文件）⇒ 既存在**陈旧 marker** 隐患（行重用若不由 store op 重写 markers），又说明**移植漏掉了一个激励它的 FORK 测试**。**⚠ 本条由 codebase-memory 图谱（`trace_path` inbound）首发现、再以 `grep` 复核修正**：原审计写的「两树均无调用者」**不准确**（FORK 有测试调用者）。② `capture/activate_sequence_kvarn_tail` 在 `!has_kvarn()` 时抛 `logic_error`，而其调用者位于 `catch(...) { return false; }` 内（`capture.cpp:726` / `commit.cpp:660`）⇒ 规划不一致会**降级为静默 capture/commit 失败**；**⚠ 2026-10-08-9 订正（读源码实测）**：本句**只对 commit 成立** —— `commit.cpp:660` 确在 `publish_active_continuation(...) noexcept` 的 `catch (...) { return false; }`（`commit.cpp:674`）内；`capture.cpp:726` 的调用者（`capture.cpp:1302-1308`）是 `catch (...) { abort_active_capture(...); throw; }`（**abort + 重抛，非静默**）；`prefill.cpp:661`（activate）在 `start_sequence`（`prefill.cpp:205`）内且**该函数体无 try/catch**（**直接上抛**）。详见 §3-2026-10-08-9。~~降级为静默 capture/commit 失败~~③ 文档面：`state_image.h:158-170,198-200` 的 `StateImagePart`/`StateImageDevicePool` 注释未提 KVarN。**处置**：①②属既有风险（**未修**，记录在案）；③属文档（本轮未改）。

**本轮全为纯 CPU**：仅**按目标构建** `ninfer_tests`（未重配 build-port、未全树构建）；host 段测试以 `CUDA_VISIBLE_DEVICES=99` 运行（零显存占用）；P3a 未受影响（01:47 仍 11,640 MiB；**run6.sh 每 rep 一个新进程**，故 PID 会变：01:32 为 18880 / 01:47 为 19104）。
**P3a 当时的进度（01:47）**：**6/9** —— k4v4 r1–r3 与 k5v5 r1–r3 均 `exit=0`（k4v4 mean KLD **0.002120**、k5v5 **0.001432**，与 D-11 逐位相同；tok/s 287.5–292.8），`k5v5-t0-r3` 进行中（01:42:54 起）；剩 k6v6 r1–r3 ⇒ 预计 **~02:35** 结束。
**收尾未做（待 GPU 空出，本轮被 P3a 阻塞）**：① 解析 run6.log 9 臂（均值±极差）；② `p1_prefix_reuse.sh` 的 **Engine 级 P1 e2e 验收**（kvarn r2 `cached_tokens>0` 且 message 逐字节相同 + bf16 对照）；③ `ninfer_kvarn_test`（**WP5 判据的数值余量首跑确认**）；④ 新 `ninfer_qwen3_5_kvarn_continuation_image_test` 的 **device 段**。已挂后台等待进程，P3a 退出即续做。
**产物**：`tests/models/qwen3_5/test_kvarn_continuation_image.cpp`（新）、`tests/ops/test_kvarn.cpp`（改）、`tests/models/qwen3_5/tests.cmake`、`docs/maintainer/op-development.md`、`/tmp/tests_wp5.log`、`/tmp/tests_p1c.log`。**未提交**。

### 2026-10-08-4 — GPU 收尾四件（①P3a 9 臂 / ②P1 e2e **通过** / ③WP5 余量**实测** / ④续列尾 device 段**首跑 FAIL→定位为测试自身 bug→修正 PASS**）+ P2 前置：kvarn MTP provisional 路径**首次激励**

**触发**：用户指示 GPU 收尾 + P1/P2 推进。P3a（run6.sh）已 `### P3a repeats DONE Thu Oct 8 02:36:04 2026`，显存回落 0 MiB。

**① P3a 收尾：9 臂（`kvarn:k4v4|k5v5|k6v6` × r1–r3）全部 `exit=0`；质量指标逐位重复，仅吞吐有散布**

协议与 D-10/D-11 逐字相同（复用 `bf16-t0.topk`，229,348 评分 token / 28 窗口）。

| 档 | mean KLD（r1/r2/r3） | median | P99 | max | same_top | dlogp | PPL | tok/s（r1/r2/r3） |
|---|---|---|---|---|---|---|---|---|
| `k4v4` | 0.002120 / 0.002120 / 0.002120 | 0.000747 | 0.017190 | 7.175765 | 0.9817 | −0.000252 | 4.69436 ×3 | 289.1 / 291.9 / 292.8 |
| `k5v5` | 0.001432 / 0.001432 / 0.001432 | 0.000531 | 0.010355 | 7.599654 | 0.9846 | −0.000187 | 4.69405 ×3 | 290.6 / 287.5 / 289.5 |
| `k6v6` | 0.001233 / 0.001233 / 0.001233 | 0.000454 | 0.007896 | 8.354687 | 0.9858 | −0.000073 | 4.69352 ×3 | 289.3 / 289.9 / 289.5 |

**判定（收口 D-11 的单次测量口径）**
1. **质量指标（mean/median/P99/P99.9/max/same_top/dlogp/PPL）在三重复间逐位相同** ⇒ KLD 测量是**完全确定性**的，重复测量的**极差为 0**（远强于"±散布"的预期）。三档数值与 D-11 单次测量**逐位相同**（0.002120 / 0.001432 / 0.001233）⇒ **D-11 的单次测量被 3 次重复完全证实**，无需修正。
2. **唯一有散布的是吞吐**（唯一非确定性来自 wall-clock/调度）：`k4v4` 291.27±3.7、`k5v5` 289.20±3.1、`k6v6` 289.57±0.6 tok/s（极差 1.3–1.3%）。三档均值落在 bf16 296.1 的 −1.6%…−2.3% 内，与 D-11/D-12 的"位宽升高有单调真实代价"一致但不构成新结论。
3. ⇒ **D-11 的"A8 三档全过、不回退"结论在 3× 重复下稳固**；计划书 §1-A2/A8 与附录 D-11 无需改判，只补一句"重复测量极差为 0（质量）/ ≤1.3%（吞吐）"。

**② P1 e2e 验收（WP3② 的验收标准）—— `RESULT: PASS`**

命令：`bash .deps/kvarn-adm/p1_prefix_reuse.sh`（`ninfer-serve`，`--kv-dtype kvarn:k4v4` vs bf16 对照，`--max-context 4096`、`--greedy`，同 prompt ×2）。
**先修掉 harness 自身一个 bug**：`run_case` 的 `local tag=$1 port=$2 kv=$3 url=… log="…$tag…"` 在同一条 `local` 语句里引用**尚未赋值**的 `$tag` ⇒ 在 `set -u` 下报 `tag: unbound variable`（该脚本此前从未跑过，故首次暴露）。

| 判据 | 实测 |
|---|---|
| kvarn r2 `usage.prompt_tokens_details.cached_tokens > 0` | **851**（99.4%，`turn closure`）✓ |
| kvarn r2 `choices[0].message` 与 r1 **逐字节相同** | **YES** ✓ |
| bf16 对照证明 harness 确触发复用 | r2 cached **851** 且 r1==r2 ✓ |
| 旁证（独立于计数器） | kvarn req#1 TTFT 721 ms → req#2 **37.9 ms**（19×）；serve log 记 `capacity \| KV 4,096 tokens, **kvarn**, explicit` ⇒ kvarn 存储确在生效 |

⇒ **P1（续列尾前缀复用）Engine 级验收达成**。连同 08-03 的专用单测（host 段 + 本轮 device 段，见④），WP3② 的**实现 + 验收闭环**完成。日志：`.deps/kvarn-adm/p1-{kvarn,bf16}-serve.log`、`{kvarn,bf16}-r{1,2}.json`。

**③ WP5 判据数值余量实测（首跑）—— 余量充足，判据成立**

`./build-port/tests/ninfer_tests.exe ninfer_kvarn_test`（GPU，非 `CUDA_VISIBLE_DEVICES=99`）：

```
KVarN K official oracle: step_ratio_max=1        flips=1/65 over_step=0 wide_flips=0 of 65536   (bits=4)
KVarN V official oracle: step_ratio_max=0.0049119 flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=4)
KVarN K official oracle: step_ratio_max=1        flips=1/65 over_step=0 wide_flips=0 of 65536   (bits=5)
KVarN V official oracle: step_ratio_max=0.0101499 flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=5)
KVarN K official oracle: step_ratio_max=0        flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=6)
KVarN V official oracle: step_ratio_max=0.0206313 flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=6)
OK kvarn correctness
```

- **无一项超限**：`flips` 最大 **1**（上限 65 ⇒ **65× 余量**）、`over_step=0`、`wide_flips=0`（零容忍项通过）。判据 exit=0。
- **与 08-03 的分析值吻合**：预测"bits=4 约 1 次翻码、bits=5 约 2、bits=6 约 0–4"；实测 K 侧 bits=4/5 各 **1**、bits=6 **0**；V 侧三档全 **0**。`step_ratio_max=1`（K，bits 4/5）即**恰好一个量化步长**，正是"单元素落在量化边界、设备与 oracle 舍入到相邻码"的形态，与 07-21 的 `rel_l2=6.09e-4`（bits4 K）根因一致。
- ⇒ **§1-A2 / §7-WP5 的判据（逐元素 ≤ 1 步 + 边界翻码上限 1e-3 + `|Δcode|>1` 零容忍）经实测成立**，无需放宽；`kMaxFlipFraction=1.0e-3`、`kStepSlack=5e-2` 保留。WP5 **收口**。

**④ 续列尾 device 段 —— 首跑 FAIL(2) → 根因 = **测试自身 bug** → 修正后 PASS**

`./build-port/tests/ninfer_tests.exe ninfer_qwen3_5_kvarn_continuation_image_test` 首跑：
```
FAIL: the captured KVarN tail survives the slot copy
FAIL: the activated KVarN markers match the captured ones
FAIL kvarn continuation image   (exit=1)
```
**定位（决定性）**：两处失败**都只涉及 `marker_pattern`**；同一断言里的 K/V（`restored_k/restored_v`，尺寸 `tail`）**通过**。根因是我在 08-03 写该测试时把 `marker_pattern` 写成 **3 字节** `{0x11,0x22,0x33}`，而 `logical_pages` 是 `I32{ops::kKvarnTailSlots=3}` = **12 字节** ⇒ ① `copy_to_tensor` 从 3 字节 vector 读 12 字节（**越界读 / UB**）；② `read_tensor(...) == marker_pattern` 是"12 字节 vs 3 字节"的 vector 比较，**恒 false**。测试**自身**在第 341 行就断言 `logical_pages.bytes() == marker`(=12) 且该断言通过 ⇒ 12 是正确尺寸，**3 字节才是 bug**。
**修法**：`marker_pattern` 改为按 `marker` 尺寸程序化生成（与 `k_pattern`/`v_pattern` 同构）。**判据不变**（仍是"逐字节存活 + activate 后逐字节相同"）。重跑 **`OK kvarn continuation image`（exit=0）**。
⇒ **实现无缺陷**（K/V/markers 全部逐字节通过 slot copy 与 activate）；这是**测试侧**的尺寸 bug，非移植缺陷。**已如实记录**（未放宽任何阈值）。

**P2 前置（WP3④）：kvarn MTP provisional 路径首次激励 —— 通过**

新建 `.deps/kvarn-adm/p2_mtp_gen.sh`：对 `kv{:kvarn:k4v4|bf16|rk4v4}` × `spec{off, mtp k=1}` 各跑同一 greedy 请求 **2 次**（repeat），比较 message 文本（`.deps/kvarn-adm/p2-*.{json,txt,log}`）。

| 档 | MTP-off decode | MTP-on decode | mtp accepted | self-det(off) | self-det(on) | **MTP-on vs off** |
|---|---|---|---|---|---|---|
| `kvarn:k4v4` | 67.8 tok/s | **101.5 / 101.6** | **84/113 (74.3%)** | none | none | **none** |
| `bf16` | 68.3 | 104.1 / 104.3 | 87/111 (78.4%) | none | none | **none** |
| `rk4v4` | 68.5 | 97.6 | 79/119 (66.4%) | none | none | **none** |

（"none" = 两次/两配置的 message **逐字节相同**；三档 req#2 均 `cached 851`。所有 `exit=0`、`http=200`。）

- **首要目标达成**：`kvarn:k4v4 --spec mtp` **端到端跑通**且日志报 `mtp accepted 84/113` ⇒ **kvarn 的 MTP provisional 路径（WP3 落地但从未激励）首次被真正执行**，无规划/几何缺陷。
- **A3 相对判据（O1）有利**：kvarn 在 MTP-on 下**自确定性成立**（r1==r2 逐字节）；且本样本中 MTP-on vs off **逐字节相同**（无分叉）。
- **同量级**：MTP 加速比 kvarn **1.50×**（101.5/67.8）vs bf16 **1.52×**（104.1/68.3）、rk4v4 **1.42×**；接受率 kvarn 74.3% 介于 bf16 78.4% 与 rk4v4 66.4% 之间。

**边界（如实记录，勿过度解读）**：本轮 P2 前置是 **message 文本级、单 prompt、200 token** 的 e2e 观测；它**不能**替代 **token 级首分叉下标/分叉率** —— 后者需要把 `test_engine_mtp_greedy_parity_real.cpp` 改造为诊断仪器（**P2b，未完成**）。已知 **bf16 在同一 token 级仪器下于 token 91 分叉**（A3，D-4/D-7 逐字相同），而本 e2e 样本 200 token 未复现分叉 ⇒ **分叉是按 prompt/步数触发的 near-tie 翻转，非必然**。

**构建**：`ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0**（10 步；P3a 释放后 `ninfer-perplexity` 重链成功）。
**产物**：`.deps/kvarn-adm/{p1_prefix_reuse.sh(修),p2_mtp_gen.sh(新),p1-*.log/json,p2-*.log/json/txt,run6.log}`（gitignored）。
**未做（如实记录）**：① P2b（仪器改造 + kvarn 档并入 `kKvProfiles` + `kvarn_bits` 贯通到 `EngineOptions`）**未做**；② 未跑 `k5v5/k6v6` 的 MTP；③ 未做 kvarn MTP 的 `k>1`（k=3/15）；④ **未提交**。

### 2026-10-08-5 — P2b 完成：MTP parity 测试改造为**诊断仪器**（去逐位门禁、加自确定性门禁 + 首分叉报告，并入 kvarn/rk4v4 档）；**kvarn 在 token 级仪器下自确定性成立、分叉不劣于基线**

**触发**：承接 08-04 的 P2 前置（激励已通过），本轮做 P2b = WP3④ 的仪器改造（用户裁定 ctest 范围取「**收窄默认档**」）。

**改造内容（`tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`，557 行）**

1. **去逐位门禁**：`verify_result`（跨配置 token 逐位相等，不符即 `throw`）**删除**；换成 `require_output_limit`
   （只保留"达到固定输出长度/`FinishReason::OutputLimit`"这一**结构契约**）+ `divergence(left,right)`
   → `{first, count}`（首分叉下标 + 分叉计数）。
2. **加自确定性门禁（A3/O1 的①，真正的 gate）**：每个宽度 `repeat 0` 记下本配置输出，`repeat 1` 必须**逐字节复现**
   （长度相同且 `divergence.count==0`），否则 `throw "... is not self-deterministic: first_diff=… diverged=…"`。
   （原实现把 `expected` 只在 `depth==0||dflash2` 时写并与 greedy 比 ⇒ 既不是自确定性、又是跨配置门禁。）
3. **跨配置降级为报告**：打印 `self_det=n/a|ok vs_greedy=identical|first_diff=N diverged=C/total`。
   tool-loop 的 adaptive-vs-off 断言同样降级为首分叉报告。
4. **并入档位**：`kKProfiles` **新增 `rk4v4`、`kvarn:k4v4`、`kvarn:k5v5`、`kvarn:k6v6`**（`KvProfile` 增
   `KvarnBits` 字段），`engine_options` 增 `kvarn_bits` 形参并写 `options.kvarn_bits`
   （**此前测试从不设 ⇒ kvarn 会静默用默认 Bits4**）；`--kv-dtype` 允许表与 usage 同步。
5. **`--quick`（用户裁定的 ctest 收窄）**：= 代表性档 `{bf16, rk4v4, kvarn:k4v4}` × `sample 0` × 宽度 `{0,3}`；
   `tests/models/qwen3_5/tests.cmake` 的注册加 `TEST_ARGS --quick`（该宏本就有 `TEST_ARGS` 约定）。
   **全扫**（8 档 × 3 样本 × 7 宽度 × 2 重复 ≈ 336 次生成 ≈ 1.5 h）仍可**无 `--quick`** 手动跑。

**实测（GPU，本机 27B 产物，`sample 0`、`prompt 68`、512 输出 token、C=1、`--greedy`）**

| 档 | k=1 `vs_greedy` | k=3 `vs_greedy` | k=15 `vs_greedy` | `self_det` |
|---|---|---|---|---|
| `bf16` | **first_diff=91, 413/512** | identical | **first_diff=91, 412/512** | ok（全） |
| `rk4v4` | identical | **first_diff=97, 408/512** | 未跑 | ok |
| **`kvarn:k4v4`** | **identical** | **identical** | **first_diff=91, 418/512** | ok（全） |
| `kvarn:k5v5` | **identical** | 未跑 | 未跑 | ok |
| `kvarn:k6v6` | **identical** | 未跑 | 未跑 | ok |

**全部 `exit=0`**（门禁已去除，仅自确定性/结构契约可致失败）。`ctest -R ninfer_qwen3_5_mtp_greedy_parity_real_test`
（`--quick`）**Passed 130.68 s**。

**判定（A3 相对判据 / WP3④）**

1. **自确定性成立（①）**：`self_det=ok` 在**所有**已测（档,宽度）组合上成立（含 kvarn 三档 + rk4v4/bf16，宽度 0/1/3/15）
   ⇒ 这正是 A3-O1 的**唯一绝对判据**，kvarn 满足。
2. **分叉是（档,宽度）特异的 near-tie 翻转，非 kvarn 独有**：`bf16@k1`、`bf16@k15`、`rk4v4@k3`
   各自分叉（首分叉 91/91/97）；**kvarn:k4v4 在 k=1/k=3 与 greedy 逐字节相同**（0 分叉）。
3. **kvarn 不劣于基线（④）**：k=15 处 kvarn `418/512` vs bf16 `412/512`（**首分叉同为 token 91**，差 6 token =
   1.2% 相对）；k=1/k=3 处 kvarn 更好（identical）。⇒ **同量级、不劣化**（**不主张 kvarn 严格更优**；k=15 略高于 bf16）。
4. **与 A3 既有定性一致**：分叉随宽度出现/消失、且只落在部分档位 ⇒ 与 `small_t.cu` 宽度特化 + `docs/performance.md:45`
   「per-configuration determinism, not cross-configuration equality」一致。**未做**决定性实验（把 `case 2` 改 `<2,2>`），
   故仍属**代码结构上最合理的解释**，非已证事实（同 D-6 边界）。
5. **首个 kvarn token 级 MTP 数据**：kvarn 的 MTP provisional 路径在 token 级仪器下**无结构/规划缺陷**（无异常、
   长度契约满足、自确定性成立）。

**边界（如实记录，勿过度解读）**：① 仅 `sample 0`（prompt 68）、单次运行；② 宽度只覆盖 0/1/3/15（`k5v5/k6v6` 只测 k=1）；
③ "identical" 是**该样本**下的结论，**不等于** kvarn 永不与 greedy 分叉（k=15 即分叉）；④ 测试名仍含 `parity`
（语义已变为"自一致性 + 诊断"，**未改名**，避免波及 ctest 名/既有记录引用——记为未决项）。

**变更面（本轮新增/修改）**：`tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（改造）、
`tests/models/qwen3_5/tests.cmake`（`TEST_ARGS --quick`）。
**构建**：`ninfer_tests` **exit 0**（0 error 0 warning）。

### 2026-10-08-6 — P3b 完成：kvarn 档位并入报告目录 / 日志名（`MemorySummary` + 四展示面）；**六展示面 GPU 实测 + 6 项 host 回归全过**；顺带由**图谱**发现一处**潜在（非可达）**身份缺陷；并落盘「图谱使用纪律」（AGENTS.md + skill）

**触发**：用户裁定从待办 1（P3b）开工；中途用户指出「为什么不用 codebase_memory_mcp」⇒ 本轮回溯原因并修正使用纪律（见 (3)）。

**(1) P3b 落地（7 源码文件）**：`include/ninfer/types.h` 的 `MemorySummary` 增 `KvarnBits kvarn_bits`（默认 `Bits4`）；
`program_impl.{h,cpp}` 存成员并写入 `memory_summary()`；四展示面各带 `KvarnBits` 渲染级别 ——
`apps/perplexity/main.cpp:kv_name`、`apps/cli/main.cpp:format_kv_cache`、
`src/serve/{request_log,operational_log}.cpp:kv_cache_name`。
**写法（用户裁定）**：**文本面 `kvarn:k4v4`**（与 `--kv-dtype` 逐字一致）、**报告目录分量 `kvarn-k4v4`**
（Windows 路径禁 `:`；由该站点既有的 `safe_component` 转换）。**非 kvarn 档名字逐字不变**。

**实测（GPU，2026-10-08 04:16–04:17；`ninfer-perplexity --text … --context 512 --stride 256`）**

| # | 展示面 | 实测 |
|---|---|---|
| 1 | 报告目录叶（perplexity 自动命名） | `kvarn-k4v4` / `kvarn-k5v5` / `kvarn-k6v6`（三档**各自独立**，取代原同名 `kvarn/`） |
| 2 | `report.json` `execution.kv_dtype` | `kvarn:k4v4` / `kvarn:k5v5` / `kvarn:k6v6` |
| 3 | perplexity stdout | `kv: kvarn:k4v4|k5v5|k6v6` |
| 4 | `ninfer` 生成摘要 | `kv cache dtype  kvarn:k5v5` |
| 5 | `ninfer-serve` operational log | `capacity | KV 512 tokens, kvarn:k5v5, explicit` |
| 6 | request-log JSONL | `"kv_cache":"kvarn:k5v5"` |

**构建**：`ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0 / 251 步 / 0 error**；
**本轮改动面 0 新告警**（既有 nvcc `#128-D`、MSVC `C4244` 计数不变，均在改动面外）。
**host 回归 6 项全过（0 失败）**：`ninfer_{request_log,load_report,bench_support,cli_options,serve_options,kv_capacity}_test`。
**范围外（如实）**：`bench/inference/ninfer_bench_support.cpp:951 kv_cache_name` **未改** —— bench 的 `parse_kv_cache`
不接受 kvarn（上限 `k8v4`）、`BenchOptions` 无 `kvarn_bits`、`NINFER_BUILD_BENCHMARKS=OFF`（该函数仅在单独的
`ninfer_bench_support_test` 编译，无 kvarn 用例）⇒ bench 无 kvarn 档可标级别（归未决项 3）。
**harness bug（记录，非产品缺陷）**：`--text` 且 `--context` 小于默认 stride 时必须显式 `--stride`，否则 CLI 拒绝
（`context/stride must satisfy context>=2 and 1<=stride<context`）；首跑三档皆 exit=1，加 `--stride 256` 后全绿。

**(2) 图谱发现（潜在、非可达；未改）**：`ProgramImpl::capture_identity_tag()`（`program_impl.h:788-792`）**不含
`kvarn_bits`**，而该 tag 写进**持久化磁盘**缓存身份 `DiskKVIdentity.tag`（`storage/disk_tier.cpp:113`）⇒ 理论上
k4v4/k5v5 的磁盘检查点可同 tag。**实测该隐患被目录名兜住**：`disk_profile_directory`（`disk_tier.cpp:37-42`）
把 bits 相关的 `main_stride`（KV 页 stride = 128×记录字节）编进目录名 ⇒ 三档目录本就分离；且同进程内 bits 恒定。
**未改 tag**（改它会使既有磁盘缓存全部失效，属身份语义变更）⇒ 并入 §5 待裁决项。
**该发现只由 `trace_path` inbound 给出**（我最初的 grep 完全没搜到这条调用面），是本轮「图谱纪律」的直接产出。

**(3) 图谱使用纪律根因与落盘（用户指示）**
- **根因实测确认**：SessionStart hook 报 `no indexed graph project matched this working directory` 的**原因是
  工作区根 `D:\ninfer` 是本仓 `D:\ninfer\ninfer-precision-tail` 的父目录** —— hook 只在「已索引工程的根是 cwd 的
  **祖先**」时匹配。实测三次：`cwd=D:/ninfer` → no match；`cwd=…/infer-precision-tail` 与 `…/src` → 均
  `graph project="D-ninfer-ninfer-precision-tail" is indexed`。`list_projects` 才是权威；`config list` 无 cwd/match 旋钮，
  故无法只靠配置修复（可选：把工作区开在本仓目录，或在 hook 前包一层子目录探测）。
- **AGENTS.md（本仓）**新增 `### Codebase memory (indexed graph)` 小节：list_projects 权威 / 改共享类型先枚舉消費方 /
  否定性結論必須走 trace_path+coverage / 字面量仍用 grep / `metadata_changed` 非判據。
- **skill `~/.qoder/skills/codebase-memory/SKILL.md`**：description 增 (f)「改共享類型的字段/枚舉/顯示名」觸發詞與中文觸發；
  Step 0 增「hook 的 no-match 是 cwd 作用域假象，先 list_projects」；新增 `## Field and type plumbing` 段；
  Freshness 增 `metadata_changed` 非判據；Hard rule 擴展到**自己寫的**否定性句子（不止委派審計）；When NOT to use 明寫
  「別把圖譜用於字面量檢索」。

**未做 / 未測（如實）**：① **P3c（kvarn parser 單測）未做**；② 儀器覆蓋殘留（`k5v5/k6v6` 只測 k=1、`sample 0` 之外未測、
測試名仍含 `parity`）**未動**；③ 計劃書 §7-WP7 的 `MemorySummary` **容量口徑**未動（本次只加**名字欄位**）；
④ 未重跑全量 ctest（只跑受影響的 6 項 + 四目標構建）；⑤ **已提交**（用户于本轮末指示提交）：
`6836b2de` feat（7 源码文件）/ `3e780a41` docs（计划书 v14 + 本日志 + 三份归档）/ `efdb1dce` docs（AGENTS.md）。
**產物**：源碼 7 文件；`.deps/kvarn-adm/{p3b_names.sh,p3b_perp.sh,p3b_text.txt,p3b-*.log}`（gitignored）；
`profiles/perplexity/**/kvarn-kXvX/**`（gitignored）；`AGENTS.md`、`~/.qoder/skills/codebase-memory/SKILL.md`；
計劃書 v14 + 附錄 D-14。

### 2026-10-08-7 — P3c 完成：kvarn parser 单测（`ninfer_cli_options_test` / `ninfer_serve_options_test`）；三档解析 + 未发布拼写拒绝 + Engine 贯通全过；**第三处 parser（perplexity）仍无单测**（结构性不可测，行为覆盖由 P3b e2e 提供）

**触发**：用户指示从待办 1（P3c）开工 —— 它是 P3b 的唯一残余（§0 未决项 7 / §4.1-P3c）。纯 CPU、无源码改动。

**改动（2 测试文件，+63 行；`git diff --stat` = `test_cli_options.cpp` +32、`test_serve_options.cpp` +31）**

- `tests/test_cli_options.cpp`（在既有 `nvfp4`/`k8v4` 用例后）：`--kv-dtype` 的 kvarn 面 —— 裸 `kvarn` 与
  `kvarn:k4v4` ≡ `KvarnGroup128` + `KvarnBits::Bits4`；`kvarn:k5v5|k6v6` → `Bits5|Bits6`；默认 `kvarn_bits == Bits4`；
  拒绝 `kvarn:k4v2`（已删内部档）/ `kvarn:k3v3` / `kvarn:k4v5` / `kvarn:k7v7`；help 含 `kvarn:k4v4|k5v5|k6v6`。
- `tests/test_serve_options.cpp`（同位置）：同上解析面（含默认 `Bits4` 与四个拒绝）+ **`make_engine_options()` 贯通**：
  `kvarn:k5v5` → `EngineOptions.kv_cache == KvarnGroup128` **且** `kvarn_bits == Bits5`（这是 serve 侧唯一能覆盖
  parser→Engine 映射的断言点，`generation_service.cpp:263-264`）+ help 含三档。

**实测**：按目标构建 `ninfer_tests`（8 步；仅两个测试对象重编 + 链接）**`exit=0` / 0 warning**（日志 0 条 `warning`）。
host-only 运行（`CUDA_VISIBLE_DEVICES=99`，零显存）：
`./build-port/tests/ninfer_tests.exe ninfer_cli_options_test` → **exit 0**；
`./build-port/tests/ninfer_tests.exe ninfer_serve_options_test` → **exit 0**。
（`ninfer_tests.exe` 是 dispatch 型 runner：`build-port/tests/ninfer_tests_dispatch.cpp:380-384` 按名字转发、未知名字 `exit 2`
⇒ exit 0 是**真实执行**的结论，不是 no-op。）

**未做 / 未测（如实）**
1. **第三处 parser 未加单测**：`apps/perplexity/main.cpp:197-229` 的 `--kv-dtype` 是 **main.cpp 内联的 `else if` 链**，
   其 `Options` 结构体也是 main.cpp 局部（`:69-89`）；既有 `ninfer_perplexity_evaluation_test` 只编译
   `apps/perplexity/evaluation.cpp`（`tests/cmake/ProductTests.cmake:18-24`）⇒ **不重构出可测函数即无法单测**，
   属 P3c 定义范围之外（**未改**，不新增抽象）。该 parser 的**行为覆盖**由 P3b 的 GPU e2e 提供
   （§3-08-06：`kvarn-k4v4` 报告目录 + `report.json` + stdout 三面实测，其路径经此 parser）。
2. **未跑全量 ctest**：本轮**无源码改动**、只改两个测试文件 ⇒ 只跑受影响的两项（符合 AGENTS.md「按改动面选择检查」）。
3. **未提交**（用户约束）。

**产物**：`tests/test_cli_options.cpp`、`tests/test_serve_options.cpp`；`/tmp/p3c_tests.log`。**未提交**。

### 2026-10-08-8 — 图谱复核（实测，**无源码改动**）：`KvarnBits` 消费方集合与 grep **一致（图谱 0 独有文件）**；`reset_kvarn_tail_row` **callers=0 复现**；并实测两条**工具边界**（field 的 `trace_path` 恒 0；`USAGE`/`WRITES` 按名解析会混同同名域、且无访问点行号）⇒ 已回写 `AGENTS.md` 纪律

**触发**：用户提问「能否用 codebase_memory 替代大量 grep/read 以减少上下文占用」，并要求「试一试，然后继续推进」。

**做法**：① 工作树自 `2026-10-07T19:00Z` 后已变（P3b/P3c 未索引）⇒ 先 `index_repository(mode=full)` 刷新：
**45,923 节点 / 216,539 边**（旧 45,868 / 216,412）；② `search_graph` 定位 `KvarnBits` 图谱面；
③ `trace_path` inbound 复核既有 callable 结论；④ `query_graph`（Cypher `MATCH (a)-[r:USAGE|WRITES]->(b:Field)`）枚举域消费方。

**(1) 确认（正面）**
- **域是图谱一等节点**：`search_graph` 返回 13 个 `Field` 声明 —— `enum KvarnBits` + 12 个 `kvarn_bits`/`kvarn_bits_` 成员
  （`EngineOptions` / `MemorySummary` / `ProgramImpl` / `cli::Options` / `ServeOptions` / perplexity `Options` /
  `DecoderStateSpec` / `PagedKVCacheLayout` / `PagedKVCache` / `SequencePlanningInputs` / `SequencePlanImpl` / `KvProfile`）。
- **`reset_kvarn_tail_row` `callers_total=0`** —— 刷新后的图谱**复现**了 §3-08-03 的记录（TAIL 无调用者）。
- **`activate_sequence_kvarn_tail`**：direct（hop=1）调用者 **1** 个（`prefill.cpp:661`）；另两条出现在的
  `commit.start_request` / `materialization.progress_materialization_transaction` 是 **hop 2/3 的传递祖先**，
  **不是**调用点 ⇒ 与 §3-08-03 的「4 个调用点 = restore 1 + activate 1 + capture 2」一致。

**(2) 工具边界（本轮实测；已回写 `AGENTS.md` 的 codebase-memory 小节）**
- **`trace_path` 对 *field* QN 恒返回 0**：对 `MemorySummary.kvarn_bits`（其 `USAGE`/`WRITES` 边确实存在）做 inbound
  追踪得 `callers_total: 0` ⇒ 原句「否定性结论必须走 `trace_path` inbound」**对域消费方不适用**；域必须走
  `query_graph` 的 `USAGE`/`WRITES`。
- **`USAGE`/`WRITES` 的域归属按名解析 ⇒ 混同同名域**：`tests/test_cli_options.cpp` 读 `cli::Options::kvarn_bits`、
  `operational_log.cpp` 读 `MemorySummary::kvarn_bits`，图谱都回成 `ServeOptions.kvarn_bits`；且边**无访问点行号**
  （查询返回的是**所在符号的起始行**）⇒ 域级否定性结论只能作**指示性**证据，须与 grep 对拍后再落盘。
- **本仓 C++ 域的消费方集合，图谱不比 grep 更完备**：图谱源文件集是 grep 文件集的**子集**（0 个图谱独有文件）；
  唯一候选 `startup.cpp:128 persistent_layout` 经读源码证实**就是** grep 的 `startup.cpp:198`（同一处，只是图谱报
  函数起始行、grep 报访问行）。⇒「图谱找得到 grep 漏掉的读者」在**本例（域）**未被证实（反向：grep 更精确）。
  **与 D-14 不矛盾**：D-14 的发现属 **callable** 层（`capture_identity_tag` 的调用面，`trace_path` 在该层可靠）；
  本例说的是**域**消费方枚举（该层不可靠）。

**(3) 未做（如实）**：未对新索引重跑全量 `check_index_coverage`（仅按需查询）；**未改任何源码**；**未提交**。

**产物**：`AGENTS.md`（纪律订正两处）；本日志 §3-08-08 / §5 图谱段 / §6 索引。**无源码改动**。

### 2026-10-08-9 — 订正（读源码实测）：审计 LOW② 的「capture/activate 抛错被 `catch(...)` 静默化」**只对 commit 侧成立**；并补记 `reset_kvarn_tail_row` 的标记语义与 FORK 测试的真实用途（**无源码改动**）

**触发**：用户要求逐项解释 §5 待裁决项（问题 / 代价 / 后果）；读源码时发现 §3-08-03 的 LOW② 表述不准确，按「实测优先」回写。

**(1) LOW② 订正：三条调用路径中只有 commit 静默**

| 抛错源 | 调用点 | 包裹 | 结果 |
|---|---|---|---|
| `capture_sequence_kvarn_tail`（`context.cpp:1756`） | `capture.cpp:726` | 其唯一调用者 `capture.cpp:1302-1308`：`try { … } catch (...) { abort_active_capture(transaction); transaction.published = true; throw; }` | **abort + 重抛（上抛，非静默）** |
| `capture_sequence_kvarn_tail` | `commit.cpp:660` | `publish_active_continuation(...) noexcept`（`commit.cpp:645-674`）末尾 `catch (...) { return false; }`（:674） | **静默吞掉**：continuation 不发布、无日志 |
| `activate_sequence_kvarn_tail`（`context.cpp:1796`） | `prefill.cpp:661` | 在 `ProgramImpl::start_sequence`（`prefill.cpp:205` 起）内；`380-661` 区间**无 `try`/`catch`** | **直接上抛** |

⇒ **唯一真正的静默路径是 commit**。抛错前置条件是规划不一致（KV=`KvarnGroup128` 而 StateImage 池未带 KVarN），`has_kvarn()` 由 startup 的同一 `KvarnGroup128` 门控置位 ⇒ 当前**不可达**。§3-08-03 该句**已就地标注订正**（未删除）。

**(2) `reset_kvarn_tail_row` 的语义与 FORK 用途（供解释 ①/(b)）**
- **语义**：`PagedKVCache::reset_kvarn_tail_row(table_row, stream)`（`decoder_state.cpp:360-371`）把**某一执行行**在所有层的 `tail_logical_pages` 标记重置为 `0xff`（"不指向任何页"）。标记形状 `I32[kKvarnTailSlots]` 逐 `(layer,row)`；构造期即为全 `0xff`（`decoder_state.cpp:220-222`）。
- **标记运行期只被追加**：KVarN 注意力/staging 在**认领**某页尾槽时写入（`attention.cu:75` 以**非 const** `int32_t*` 传入；`mapped_tail_slot` `attention.cu:91-96` 用它匹配逻辑页）⇒ 热路径不清理。故该函数是**执行行复用**的卫生原语：行易主后若不重置，staging 可能误判"页 P 的尾槽已就位"而**复用上一占用者的尾 K/V**。
- **FORK 的两处调用都在测试里**（`test_prefill_precision_real.cpp:236,284`）：该测试 `execution_tables().acquire(0)` 后**跨多个子用例复用同一行 0**（每子用例 `state.zero_all(...)` + `reset_kvarn_tail_row(0, stream)`），并比较 conv/recurrent 状态与最终 logits（基线 / cold / cached 三档）⇒ **FORK 生产代码同样 0 调用者**。**移植缺的不是"生产行为"，而是那个测试。**
- **未做**：未追查 TAIL 是否存在"回收 KVarN 执行行且其标记随后被读取"的路径 ⇒ 可达性**未验证**。

**(3) 未改源码、未提交。产物**：本日志 §3-08-03 的就地订正 + 本条。

### 2026-10-08-10 — 审计 3 项 LOW **修复落地**（用户批准）：① CausalScore 路由补 `reset_kvarn_tail_row`（**生产行为变更**）· ② commit 发布失败不再静默 · ③ `state_image.h` 注释补 KVarN；新增专项单测（**ctest 262 → 263，实跑 PASS**）；**GPU 侧回归未跑**（用户指示）

**触发**：用户指示「按照你推荐的方法修好这些问题」，并对 FORK 仓（`ninfer-rtx5090-mobile`）给予完全授权。

**调查（2 个只读子代理 + 我的逐点复核）**
- **① 的真实缺口在 CausalScore 而非「函数是死代码」**：执行行 = lane 且**会被回收**（`context.cpp:103-107` release 抬代 → `commit.cpp:85-88,116-120` 准入复用）。**生成路径安全**：每次 start 由 `activate_sequence_kvarn_tail` **整行覆盖**标记（`context.cpp:1823-1825`，源为零清零/捕获的镜像）。但 **CausalScore 不走 `start_sequence`**（`trace_path start_sequence` 实测 direct 调用者仅 `commit.ProgramImpl::start_request`），却 `create_active(entitlement, 0, …)` **硬编码 row 0**（`program_impl.cpp:659`）并断言 `bound_row == 0`（`:663-665`），跨多次打分复用 row 0 而**不恢复任何镜像**；而标记在运行期**只被追加**（`attention.cu:75` 非 const 指针；`mapped_tail_slot` `:91-96` 用它匹配逻辑页）⇒ 上一占用者的标记会被当作本序列的读入。
- **② 的修法**（子代理 B）：`publish_active_continuation`（`commit.cpp:642-692`）为 `noexcept`，两个调用者（`finish` `:583`、`salvage_continuation` `:696`）皆 `noexcept` 且把 `false` 当**正常的最好努力拒绝**（abort→salvage/discard）⇒ `catch(...) { return false; }` 静默且与合法拒绝不可区分。仓内先例：`report_capture_release_drift`（`capture.cpp:601-612`）= **stderr 诊断 + 返回 false**。**(C) 去 `noexcept` 被否决**（一处规划不一致会升级为引擎级 `warning + recover`，失败所有在途请求，`engine_core.h:2453-2472,2193-2199`）。

**改动（3 源码 + 1 新测试 + 1 cmake）**
1. **①** `program_impl.cpp`（`causal_score`）：`bound_row` 断言之后、`ensure_mapped_to_tokens` 之前加
   `decoder->text_kv.reset_kvarn_tail_row(0, compute_streams[0]);`。该函数自述「No-op on a non-KVarN cache」（`decoder_state.h:133-134`）⇒ 无需显式门控。
2. **②** `commit.cpp`：`publish_active_continuation` 的 `catch(...)` 前插入 `catch (const std::logic_error& error) { std::fprintf(stderr, "active continuation publish refused: %s\n", error.what()); return false; }`；补 `#include <cstdio>`。（保留 `catch(...)`：`CUDA_CHECK` 抛 `CudaError : std::runtime_error`。）
3. **③** `state_image.h`：`StateImagePart` 与 `StateImageDevicePool` 的文档注释补上「KVarN body 的 full-attention sink/tail（含 MTP 池）」这一占用者（原先只列 GDN/hidden 与 DFlash）。
4. 新测试 `tests/models/qwen3_5/test_kvarn_tail_row_reset.cpp`（284 行，设备侧）+ `tests/models/qwen3_5/tests.cmake` 注册。

**实测**
- 构建：`ninfer_tests ninfer` **exit 0 / 234 步**（`state_image.h` 触发宽重编）；`ninfer-serve ninfer-perplexity` **exit 0 / 7 步**。
- **新测试 `ninfer_qwen3_5_kvarn_tail_row_reset_test` 首跑 PASS**（`OK kvarn tail row reset`，exit 0）：`DecoderStateSpec` → `plan_decoder_state` → 真 `PagedKVCache`；经**公开**的 `kvarn_batch_layer_view(layer).tail_logical_pages` 播撒标记；断言「重置行的每层标记为 `0xff`、其它行逐字节不变」「越界行 `-1`/`rows` 为空操作」「非 KVarN 缓存调用不抛且不动一字节」。测试自身的几何断言（`I32[kKvarnTailSlots, rows]`、行距 12 B）经我核对 `tensor.cpp:set_contiguous_strides`（**`nb` 以字节计**）与 `decoder_state.cpp:352-353` 成立。
- host（`CUDA_VISIBLE_DEVICES=99`）**4 项全过**：`ninfer_qwen3_5_exact_tail_capacity_test`、`ninfer_qwen3_5_state_image_layout_test`、`ninfer_cli_options_test`、`ninfer_serve_options_test`。
- `ctest` 注册数 **262 → 263**（`ctest -N` 实测）。

**未跑 / 未验证（如实）**
1. **① 的生产行为变更没有执行证据**（用户指示「先不跑需要 GPU 的测试」）。原计划的决定性对照 —— **改动后跑一个 `kvarn:k4v4` KLD 臂，与 P3a 记录的 mean KLD `0.002120` 对比** —— **已启动即被叫停**（无输出）。两种可能都**未测**：若数值仍为 `0.002120`，说明该路由上标记本已被清（本改动为无害 no-op）；若不同，则 **D-11 / P3a 的 KLD 数值需重测**（那将是本移植的一个真实质量缺陷被修）。现有证据仅为：逻辑论证（row 0 硬编码 + 生成路径之外无覆盖 + 标记运行期只追加）+ 上面那个**测原语而非测路由**的单测。
2. **未移植 FORK 的 `test_prefill_precision_real.cpp`**（用户批准的是「移植它」）。**偏离理由**：调查显示该测试练的是**已受保护的 prefill 路径**（生成路径每 start 整行覆盖），而真实缺口在**打分路由**；且 TAIL 已有 prefill/前缀复用覆盖（`test_engine_prefix_real`、P1 e2e）。故改为「生产修复 + 定点单测」。**如需仍要 FORK 那支测试（现已获授权），可再补。**
3. 其余 GPU 回归（`ninfer_kvarn_test`、续列尾 device 段、score real 等）**未跑**。
4. 顺带：`.qoder/`（本会话隔离 worktree 的目录）在仓内且未被 ignore，**未提交**。

**产物**：`src/models/qwen3_5/program/program_impl.cpp`、`src/models/qwen3_5/program/transactions/commit.cpp`、`src/models/qwen3_5/state/state_image.h`、`tests/models/qwen3_5/test_kvarn_tail_row_reset.cpp`（新）、`tests/models/qwen3_5/tests.cmake`、`/tmp/fix_build{1,2}.log`、`/tmp/host_*.log`。**已提交**：`b30c3a98` fix（本轮 3 项修复）/ `0754708e` test（P3c）/ `69f84b3e`+`7582d7ff` docs（见 §0 提交清单）。

### 2026-10-08-11 — P3d ① 端到端对照**已跑**（`kvarn:k4v4` KLD **逐位复现 `0.002120`** ⇒ 该协议上属**无害 no-op**，D-11/P3a 数值**无需重测**）；P3d 后 GPU 回归 3 项**全绿**；并定位「子代理无图谱」根因（4 份代理定义的 MCP 工具名用**连字符**、运行时归一为**下划线**）——**修复已落盘，但本会话仍无效（定义在会话启动时装载一次，须重启 Qoder）**

**触发**：用户指示从 §5 待办 1 起步（最优先、原需 GPU）。跑前 `nvidia-smi` 确认 **0 MiB / 无计算进程**。

**(1) P3d ① 的端到端对照 —— 已完成（决定性结果：无变化）**

脚本 `.deps/kvarn-adm/verify_p3d_kld.sh`（协议与 `run6.sh`/D-10/D-11 **逐字相同**；日志 `p3d-verify-k4v4-t0.log`、驱动 `/tmp/p3d_verify_driver.log`）：
`ninfer-perplexity <27B 产物> --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --disjoint --score-width 8 --score-topk 100 --kv-dtype kvarn:k4v4 --kv-tail-tokens 0 --kld-base .deps/kvarn-adm/bf16-t0.topk`

执行条件（可复现性）：**13m02.5s**（05:18:09→05:31:18）、**exit=0**、GPU 中途 11,632 MiB（D-11 为 11,687）、229,348 评分 token / 28 窗口 / 4 streams、report 叶 `profiles/perplexity/**/kvarn-k4v4/ninfer-ppl-1m-v1/quick/20261007-211814/report.json`。
**二进制确含本修复**：`ninfer-perplexity.exe` mtime 05:10:29 **晚于** `program_impl.cpp` 05:08:51，且对象已重编；修复点在 `program_impl.cpp:669`。

| 指标 | 本次（修复后） | D-11 / P3a（修复前） | 判定 |
|---|---|---|---|
| mean KLD | **0.002120**（0.00212012） | 0.002120 | **逐位相同** |
| median | 0.000747（0.000747432） | 0.000747 | **逐位相同** |
| P99 | 0.017190 | 0.017190 | **逐位相同** |
| P99.9 | 0.076138 | （D-11 未列） | — |
| max | 7.175765 | 7.175765 | **逐位相同** |
| same_top | 0.9817 | 0.9817 | **逐位相同** |
| mean_target_dlogp | −0.000252 | −0.000252 | **逐位相同** |
| PPL（overall） | 4.694357 | 4.69436 | **逐位相同** |
| tok/s | 293.1 | 294.5（D-11）/ 289.1–292.8（P3a r1–r3） | 吞吐本就非确定量 |

**判定**：① 的**唯一可能后果**（打分路由读入上一占用者的陈旧尾标记）在该协议上**未发生** ⇒ `reset_kvarn_tail_row(0, …)` 在此为**无害 no-op**；**D-11 / P3a 的 KLD 数值无需重测**；§4.1-P3d 的「① 缺执行证据」**关闭**。
**⚠ 必须同时记录的不利面**：本实验**只证明「无害」，未证明「必要」**。机制解释（与实测自洽，非事后编造）：`claim_tail_slot`（`attention.cu:100-118`）先扫「标记 == 本页号」命中即复用，否则只 CAS 仍为 `0xff` 的槽；`tail_slot`（`:84-89`）只给 sink/首/末页分槽。本协议窗口**恒为 `context` 长**（`--disjoint` ⇒ `plan_disjoint_windows`，`apps/perplexity/evaluation.cpp:50-58`，`begin += context`），且**页号每窗口从 0 重启**（`create_active(entitlement, 0, …)`）⇒ 上一窗口的 stale 标记与当前窗口**页号重合**，既不阻塞认领、也不改变所读槽位（`retire` 亦已把动态槽置 −1）。而 `Engine::score_tokens`（`engine.cpp:363`）接受**任意**窗口长度，长度变化时陈旧页号可与当前页号**冲突**（首选/备用槽皆被占 ⇒ `claim_tail_slot` 返回 −1、跳过 staging）⇒ 该冲突的数值后果**未实测**（本协议不构造它）。**结论**：本改动属**防御性正确性修复**，其"必要性"仍无端到端证据（已回写计划书 §9）。

**(2) 子代理「无图谱」的根因、修复与残留（用户授权 A+B）**

**现象**：本会话派出的 `codebase-memory` 子代理自报 "Graph tools aren't in my available function set"。
**根因（实测）**：运行时把 MCP 工具名**归一化为下划线**（`mcp__codebase_memory_mcp__trace_path`），而 `~/.qoder/agents/` 下**4 份**代理定义（`codebase-memory`、`-scout`、`-auditor`、`-port-auditor`）的 `tools:` 与 `mcpServers:` 全用**连字符**（`mcp__codebase-memory-mcp__*` / `codebase-memory-mcp`，即 `settings.json` 中 `mcpServers` 的原始 key）。
- **决定性证据**：主会话对连字符名 `mcp_get` → `Error: MCP tool "mcp__codebase-memory-mcp__list_projects" not found. Did you mean: mcp__codebase_memory_mcp__list_projects?`
- **同文件自相矛盾佐证**：`settings.json` 的 `mcpServers` key 是连字符，而 `permissions.allow` 列的是下划线（主会话因此可调用）。
- **修复**：4 份定义 `tools:` + `mcpServers:` 连字符→下划线（`grep -c "codebase-memory-mcp" *.md` 全为 **0**；原件备份 `/tmp/agents-backup-20261008/`，sha1 已记录）。
- **⚠ 修复后探针仍失败**：改动后 9 秒派出的探针子代理仍报 `NO GRAPH TOOLS`（工具集仅 `Glob,Grep,Read`）。日志给出原因：代理清单元数据 **`[AgentListingDelta] isInitial=true`（全日志仅 1 条，05:17:47）** ⇒ **定义在会话启动时装载一次、无热重载**（worker 参数 `plugins.hotReload=false`）⇒ **须重启 Qoder 才生效**。重启是否**充分**（另一可能：本宿主对子代理一律不授予 MCP）**尚未排除**；重启后一个 30 秒探针即可判定。
- **历史影响（不利发现，重要）**：**上一会话**的两个 `codebase-memory` 子代理实测**只用 `Read`/`Grep`**（27+21 / 24+40 次），**图谱调用为 0** ⇒ 这些定义自创建（今日 01:00 前后）起**从未**给过子代理图谱。这解释了 §3-08-03 的 P1 审计子代理为何给出「两树均无调用者」的**错误否定结论**（随后被主会话的图谱 + grep 纠正）——AGENTS.md/§5 中「结构性/否定性问题**必须**交给 codebase-memory 子代理」这条纪律，在此前几轮**实际未被满足**（真正做图谱核查的是主会话）。

**(3) 图谱刷新与复核（本轮顺带）**

工作树自 08-08 索引后已含 P3d ⇒ `index_repository(mode=full)` 重跑：**45,944 节点 / 216,668 边**（旧 45,923 / 216,539）。
`reset_kvarn_tail_row` 的 `trace_path` inbound 现为 **`callers_total=4`**：`ProgramImpl::causal_score`（hop 1，**非测试 ⇒ 该函数唯一的生产调用点**）+ 新测试的三个 hop（`test_kvarn_tail_row_reset` / `test_non_kvarn_reset_is_a_no_op` / `main`）⇒ 与源码一致（`program_impl.cpp:669`）。
> **刷新前**同一查询返回 `callers_total: 0` —— 因索引时间（04:35）早于 P3d 提交（05:12），属**陈旧**而非事实。再次印证 AGENTS.md「依赖图谱前先看 `indexed_at`」。

**(4) 待裁决 3 项 —— 用户本轮已裁决**

| 项 | 裁决 | 处置 |
|---|---|---|
| (a) 是否移植 FORK 的 `test_prefill_precision_real.cpp` | **不移植** | 真实缺口在打分路由（已修 + 定点单测）；FORK 该测试练的 prefill 路径本已受保护（每 start 整行覆盖标记）。**未移植**（不同于 §3-08-10 的原授权，为本轮用户明示裁决） |
| (b) `capture_identity_tag()` 是否补 `kvarn_bits` | **不补** | 潜在、当前不可达（`disk_profile_directory` 已按 `main_stride` 分离三档）；补则作废既有磁盘缓存而收益为零。**保留代码注释**；函数**未改** |
| (c) kvarn bench 归属 | **暂不纳入** | 维持 `NINFER_BUILD_BENCHMARKS=OFF`、**不重配** `build-port`；bench 不构成交付缺口（性能由 perplexity/KLD + MTP e2e 提供） |

**(5) P3d 后 GPU 回归批 —— 3 项全绿**

`ninfer_tests.exe`（已设 `NINFER_TEST_ARTIFACT`；脚本 `.deps/kvarn-adm/post_p3d_regressions.sh`，日志 `/tmp/post_p3d_regressions_driver.log`）：

| 测试 | 结果 | 要点 |
|---|---|---|
| `ninfer_kvarn_test` | **exit=0** | WP5 判据余量与 §3-08-04 **逐位相同**：K bits4/5 `flips=1/65`、bits6 `flips=0`；V 三档 `flips=0`；`over_step=0`、`wide_flips=0`；另见 `KVarN codec bit-order roundtrip: 4/5/6-bit exhaustive OK` |
| `ninfer_qwen3_5_kvarn_continuation_image_test` | **exit=0** | `OK kvarn continuation image`（续列尾 device 段未受 P3d 影响） |
| `ninfer_qwen3_5_score_real_test` | **exit=0** | `OK causal_score_real`。**⚠ 覆盖局限（如实）**：该测试用 `KvCacheStorage::Fp8E4M3Row256`（`test_engine_score_real.cpp:24`），**非** KVarN ⇒ 它**不覆盖**被 P3d 改动的路由，仅作 harness 健全性检查；**KVarN + 打分路由目前没有直接的门禁测试**（改动的覆盖仍只由 (1) 的实测与 `ninfer_qwen3_5_kvarn_tail_row_reset_test` 的原语级测试提供） |

**未做 / 未测（如实）**：① 未构造「**变长窗口**」的 `Engine::score_tokens` 场景去**证明**该修复必要（本协议不构造冲突；见 (1) 不利面）；② 未跑全量 `ctest`（本轮改动面为 0 源码改动 —— 仅新增 harness 脚本与用户级代理定义；GPU 侧按改动面选了上述 3 项 + (1) 的 KLD 臂）；③ 未做「陈旧标记 ⇒ 输出确实变差」的**直接**演示（需新写路由级测试）；④ 用户级代理定义修复**在本会话无法验证**（须重启）。
**产物**：`.deps/kvarn-adm/{verify_p3d_kld.sh,post_p3d_regressions.sh,p3d-verify-k4v4-t0.log}`（gitignored）、`/tmp/p3d_verify_driver.log`、`/tmp/post_p3d_regressions_driver.log`、`profiles/perplexity/**/kvarn-k4v4/**/20261007-211814/`（gitignored）、`~/.qoder/agents/*.md`（仓外）。**未提交**（本轮无源码改动；`AGENTS.md` 的表格重排为**先前存在**的工作树改动，非本轮产生，未动）。
**图谱刷新**：`index_repository(mode=full)` ⇒ 45,944 节点 / 216,668 边。

**(6) 重启后复核：A+B 生效（已闭环）**

用户重启 Qoder 后，`codebase-memory-scout` 探针**通过**：成功调用 `list_projects`（**6 个图谱工程**：`D-ninfer-KVarN` / `D-ninfer-beellama.cpp` / `D-ninfer-ninfer` / `D-ninfer-ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco` / 本仓 / **`D-ninfer-ninfer-rtx5090-mobile`（FORK）**）与 `trace_path`（`reset_kvarn_tail_row` `callers_total=4`，1 个生产调用者 `causal_score` + 3 个测试 hop）—— 与主会话独立得出的结果**一致**。
⇒ **「修定义须重启」已证实，「重启即充分」已确认**（排除「宿主一律不授予子代理 MCP」）。**后续结构性/否定性问题可正式交 `codebase-memory*` 子代理**（此前几轮的此类结论一律按 grep-only 处理）。

**(7) WP6 门禁评估（用户询问「是否已有足够证据可开始 WP6」）—— 判定：**证据不足，建议先做 §6.3 探针**

- **已满足**：功能门「P1/P2 完成」✅（§7-WP6 原文门禁）；P1–P4 全完成；准入证据齐（WP0.5-B 三档全过）；**安全回退已落地**（`kvarn:* + --kv-tail-tokens>0` 规划期 fail-fast，§6.4）；跨工程图谱已可用（上条）。
- **未满足（真正缺口）**：**§6.3 的 (a)/(b)/(c) 低成本探针从未执行** —— 计划书把它定为 **WP4 的前置**（§6.3 末句「在 WP4 之前做一次…而不是把门禁拖到 WP6 才暴露」；§7-WP4「前置：先做 §6.3 的低成本探针（判 (a)/(b)/(c)）」），但 **WP4 验收只覆盖三档准入，该前置被静默跳过**：进度记录 §3 与归档 §A 均**无**探针执行痕迹（`grep 探针/ROTATED_K_ORIGINAL` 只命中计划书自身的强制语句）。
- 因此**路线仍是假设**：首选的 (b) `ROTATED_K_ORIGINAL_V` 目前**只出现在 `docs/port-records/kvarn-kv-tail-feasibility-report.md:44` 的散文里**（描述**上游 ggml/llama.cpp** 契约），**本仓 C++ 代码中 0 命中**（全树 grep 仅命中该报告及其 worktree 副本）⇒ 「TAIL 的 kvarn body 能否表达 V 不旋的域」**未取证**，而 §6.3 自己写明 (b)「**取证/落地前须确认上游契约提供该域**」。
- **另有已确认的缺口**：① ~~TAIL `kvarn_hadamard`（`src/ops/kvarn/codec.cu:185-189`）**要求两侧都是 BF16**、否则抛 `invalid_argument`，而 `--kv-tail-type` **默认 f16** ⇒ §6.3 的「**新增 f16 旋转入口（独立交付物）**」**尚未存在**~~ → **⚠ 订正（同轮，用户提问触发）**：这是**条件性**缺口、**不是硬缺口**。实测：`--kv-tail-type bf16` **今天已完整支持**（`KvTailType{BFloat16,Float16}` `types.h:93-96`；`startup.cpp:204` 映射 `DType::BF16`；`ops/common/kv_tail_element.cuh:45-68,86-93` 两套特化 + `with_kv_tail_element(DType)` 分派；perplexity/serve 的 usage 已列 `bf16|f16`）。且 **BF16 尾环是"逐位精确"档**：其特化**故意为 no-op**（源行本就是 BF16，比特原样 `int4` 拷贝），F16 档则要 `bf16→float→half` 转换（`kv_tail_element.cuh:46-55,73-75`）。⇒ **若让 `kvarn + --kv-tail-tokens` 要求 `--kv-tail-type bf16`，现成的 BF16 旋转入口即够用，「新增 f16 旋转入口」这一交付物可整条删掉**（是否如此属产品面裁决：`§10-D3` 把默认钉在 f16 仅为对齐 TAIL 现状，非技术硬约束）。
  **该项不改变 WP6 判定**：真正的阻塞不是 dtype 形状 —— ① §6.1 明言「fp8 无尾的真正原因是**实现欠账，不是坐标障碍**」，且 **kvarn 不走 `small_t_*`**（`execution/text.cpp` 分派 `ops::kvarn_attention`）；② §6.2-1 是 **partial 契约**（FORK kvarn 的 partial 是 BF16、TAIL 尾核是 FP32 + 共用 reducer ⇒ 不能共用 buffer）；③ §6.2-2 是 **split 策略在 `QHeads==24` 且 `window>8198` 分叉**（正是 A4/WP8 要测的区间）；④ A6 约束的是**旋转时点/次序**，非存储 dtype。**更深一层**：WHT **自逆正交**（`×1/16=1/√256`）⇒ `<Wq,Wk>=<q,k>`，故「旋尾 K」与「给尾 partial 喂未旋 q」**数学等价**，选哪条是**工程代价**问题 —— 正是 §6.3 探针要判定的。
  ② **无任何测试把 kvarn 与 `--kv-tail-tokens` 组合**（当前被规划期拒绝，A6/A7 的 oracle 与 group(128)/ring(64) 边界测试脚手架**均不存在**）。
- **风险敞口**：WP6 工期 10–15 天且为最高风险，(b) 可能收缩为「一个 launcher 分支」而 (a) 是完整内核移植 ⇒ **在路线未定的情况下开工，等于承担 10–15 天投到错分支的风险** —— 这正是 §6.3 明令要避免的。
- **建议**：**先授权 §6.3 探针**（有界、~0.5–1 天、以读码 + 跨工程图谱为主、**不需 GPU**）：判定 (a)/(b)/(c) 哪条成立、并对 WP6 重新估时；**探针结论落盘后再决定是否启动 WP6**。另需用户做**产品裁决**：WP6 是否值得做（不做则 `kvarn + tail` 永久 fail-fast，A4「尾部在 KVarN body 上有可测增益」不可达）。

---

### 2026-10-08-12 — **§6.3 探针已执行**（用户授权「允许使用探针」）：**(b) 首选路线否证** · **(a) 为忠实路线** · **(c) 弱且语义不符** · **新提有界 (c′)**；并产出用户三项终态目标 → A1–A8 的映射与缺口

**(1) 方法与成本**：**三个并发子代理**（2× `codebase-memory-port-auditor` —— 域契约探针与合并契约探针，覆盖 `D-ninfer-KVarN` / `D-ninfer-beellama.cpp` / FORK）
+ 本仓读码（`src/ops/kvarn/` 全树、`small_t_*`、`kv_cache/append`、`execution/text.cpp`、`execution/vision.cpp`）+ 回读 `docs/port-records/kvarn-kv-tail-feasibility-report.md`。
**未用 GPU**（`nvidia-smi` 全程 0 MiB）。**计划书回写 v18→v19 + 附录 D-19；§6.1/§6.2/§6.3/§7-WP6 已就地订正（原文保留）。**

**(2) 判定一览**

| 路线 | 判定 | 决定性证据 |
|---|---|---|
| **(b) `ROTATED_K_ORIGINAL_V`**（v18 首选） | **否证** | 域枚举确存（`beellama.cpp/ggml/include/ggml.h:461-466`），但 ① 只在 **`n_query_tokens > 16`** 生效（`llama-kvarn.cpp:140-146`、`rotated_query_max_specialized=16`）⇒ **decode / ≤16-token 投机块恒为 `ROTATED`**，而尾部合并正发生在这些步；② V 的**存储**恒为旋转域（`kvarn.cu:3064-3080` 对**反量化后**的 V 再补一次 WHT；上游参考同样旋 V `kvarn_attn.py:1757-1758`）⇒ **不能当 fp8 body 复用尾部接线**；③ **坐标系本身即障碍**（本仓 `tests/ops/softmax_attention/causal_cache.cpp:1613-1623` 自述「旋转域档位的 reduce 核消费**旋转系 partial**，原始 BF16 尾行不表达于该系」）；④ **独立第二障碍**：元素类型（本仓 KVarN `partial_acc` 是 **BF16** `decode.cu:113`，共享 reducer 收 **FP32** `small_t.cuh:262`） |
| **(a) `small_t_k8v4` 形状** | **忠实路线**（仓内模板现成） | `small_t_k8v4.cuh:208,229`（K/V 都旋）`:263`（核内旋 Q）`:639-643`（归并 FP32）`:653`（**归并后只做一次反旋**）；上游同构（`fattn-kvarn-portable.cuh:466-469,784-786`） |
| **(c) 暴露内建精确后缀** | **弱、语义不符** | 3 槽 = sink 组 + **滚动**首/末组（`attention.cu:84-89`，分组粒度 128）；`claim_tail_slot` 双槽皆占时 **返回 −1 ⇒ 该行不 staging**（`:100-114`）；`retire` 只清"整组覆盖"的页（`:291-315`）⇒ **不能承诺「最新 N 精确」**；且该内建区**本来就在跑**，接出来**不产生新质量**（只免除 fail-fast） |
| **(c′) body 自建滚动槽**（本轮新增，**未实测**） | **待产品裁决** | 内建 3 固定槽 → `⌈N/128⌉+1` **滚动槽**，让 body 自己在核内保留最新 N 精确组 ⇒ **同时绕开** partial 类型 / 坐标系 / split 三项冲突；代价 = `--kv-tail-tokens` 出现**两套实现**，显存 ≈ N×D×Hkv×层×2×2B（N=1024 ≈ 64 MiB/序列） |

**(3) 上游合并契约（(a) 的照抄对象）**：统计量 = **FP32 `float2(row max, denom)` + FP32 累加器**（`ggml-cuda/fattn-tail.cuh:339-346,379-387`；`fattn-common.cuh:1551`）；
**tail partial 无 split 轴**（每行仅一对，`fattn-tail.cuh:844-847`），KVarN body 在导出前**先归约自身 splits**（`fattn-kvarn-portable.cuh:784-786`）⇒ **两侧不要求 split 一致**；
**旋转域合并 + 末尾只做一次反旋**（`llama-graph.cpp:3901-3904`）；**tail 行写原始、读时旋转**（`llama-kv-cache-kvarn.cpp:1004-1011` + `llama-graph.cpp:3845,3847`）。
**关键：上游不存在 BF16 部分累加器**（`GGML_ASSERT(dst->type == GGML_TYPE_F32)`，`fattn-common.cuh:1551`、`fattn-kvarn-portable.cuh:834`；BF16 只出现在 tail 环元素类型）⇒ **本仓 BF16 `partial_acc` 是对上游的「偏离」，不是「上游的另一种变体」**。

**(4) §6.2 三条「一手反证」的改判**：条 1「partial 契约冲突」→ **本仓自身偏离，照上游改 FP32 即可**；条 2「split 分叉」→ **可规避（tail 无 split 轴、两段式归并），非阻塞**；条 3「f16 旋转入口」→ **条件性交付物（采 bf16 尾即免）**。
⇒ **技术风险低于 v18 的表述，但实现量不变**（仍是一个 KVarN 专属 tail-partial 路径 + 归并 + 6 处接线 + A6/A7 脚手架 + 容量核算）。**WP6 工期重估不变：10–15 天。**

**(5) 本仓副产物（新事实）**
- KVarN 内建尾环**已经是写时旋转**（`attention.cu:157-194` `rotate_stage_kernel` 写入 WHT 后 BF16）且**已被 body 核消费**（`decode.cu:105-107`）；`rotate_on_stage = width <= kFusedStageMaxWidth(=16)`（`attention.cu:25,590`）—— §6.2 条 4「旋转时点未定」对 KVarN **有答案**。
- **可选优化**：外部尾 partial 可在**原始域**算完后**只对 acc 旋一次**（`W` 线性 ⇒ 与"先并入再反旋"等价），远低于逐行旋 384 行 × D。**须落地时实测。**
- 门禁位置：`startup.cpp:1035-1045`（同时拒 `--kv-tail-tokens` 与 `--mtp-attention-window`）。
- 接线面：`execution/text.cpp` **5 处** `kvarn_attention` + 1 处 `kvarn_kv_append`（`:391,414,567,622,1021,1045`）+ `storage/context.cpp:1745` 的 `kvarn_restore_tail`；**`kvarn_attention_cached` 只有测试在用**（无生产调用点）。
- **无任何测试把 kvarn 与 `--kv-tail-tokens` 组合** —— 已复核 `tests/models/qwen3_5/test_kvarn_tail_row_reset.cpp:74` **显式令 `kv_tail_tokens = 0`**（该测试验的是 **KVarN 自身尾行标记重置**，非精度尾环）⇒ 计划书 D-18 条 4 成立。

**(6) 用户三项终态目标 → 验收口径映射（**含缺口，须用户确认**；全文见计划书 D-19 ⑤）**
1. **prefill / decode / MTP 性能不劣化于上游**：decode ✅ A1（`|Δ|≤0.88%`）；MTP ✅ A3（相对判据）+ A8（接受率）；**prefill ⛔ 现无可用判据** —— A1 已**删除 prefill 腿**（同二进制单请求 prefill 实测 `−29%…+8%`，噪声底不可用）⇒ 若须断言 prefill 不劣化，**需新协议**（重复/批量 prefill）。
2. **视觉不劣化**：**A1–A8 无任何一条覆盖视觉**。事实：视觉塔走**独立算子** `ops::packed_softmax_attention`（`execution/vision.cpp:385`）、**不使用分页 KV**（`vision.cpp` 无 kvarn 引用）⇒ kvarn / 精度尾**结构上无法改变视觉算子**；风险**仅在显存预算**（kvarn 更省字节、尾部净增）⇒ 建议作为 **A5 的推论/显存预算判据**。现有"视觉基线"`docs/performance.md:24-26`（85.7% 接受率、7.00 tok/round）取自 **RTX 3090 / sm_86 / int8 / DFlash2**，**非本机、不可作本机基线**。
3. **「智力不劣化」的准确表述**（用户表示不确定如何表述）⇒ 应拆为两条**可证伪**口径：**(i) 特性关闭时引擎行为逐字节不变**（= A1：输出逐字节相同 + `MemorySummary` 逐字相同 + decode tok/s 在噪声内）；**(ii) 特性开启时**，与 bf16/关闭态的**可测差异只能归因于 KV 量化本身**（KLD / `same_top` / needle 检索 A7 / MTP 接受率 A3），**采样与前端语义零变更**。即「**除量化误差底噪外，引擎可观测行为不变**」，而非不可测的"智力不变"。
4. **`k4v4` 优于 `rk4v4`：已实测成立且超出要求** —— `k4v4` mean KLD **0.002120** vs `rk4v4` **0.004426** = **2.09×**（同协议），且同头/token **字节更少**（274 vs 280 B）。**限定：该测量是 tail=0**，WP6 后须以限定的 tail 配置重述。
5. **⚠ 不利面（须进 WP6 验收设计）**：**A4 的 ≥1.13× 阈值在 KVarN 上可能只在 N>384 时可达** —— KVarN body **本来就有** ~384 token 内建高精度区，而 `rk4v4` **没有任何内建尾** ⇒ A4 已记录的 rk4v4 tail 增益带（**2.26–2.47×**）**不可外推**。**若不显式处理，可能出现"功能做完但判据不达标"。**

**(7) 未决与已裁定**：① **产品裁决 (a) vs (c′)** —— **用户倾向 (a)**（08-12），**待最终确认**；量化已产出（见下 **(8)**，并已回写计划书 D-19 ⑧/⑨）；② A6/A7 脚手架仍不存在；③ 若采 (c′)，`--kv-tail-tokens` 的双实现语义待定义；
④ ~~报告 §4.1「BF16 尾在该卡明显更差、须用 F16 尾」是 beellama 侧、非本机观测，采 bf16 尾前须本机复核~~ → **✅ 用户已裁定（08-12）：接受「若采 bf16 尾则在本机复核」**（不据此非本机观测否定 bf16，亦不据此认可）；
⑤ **✅ 视觉口径已裁定（08-12，用户）**：**视觉只作功能判据、不设视觉性能判据**。依据 = 视觉塔走**独立算子** `ops::packed_softmax_attention`（`vision.cpp:385`）、**不用分页 KV**（`text_kv`/`batch_text_kv_` **0 命中**）⇒ kvarn/尾**结构上不影响视觉算子**，风险只剩**显存预算（归 A5）**与**共享代码面（归 A1）**。
**待补功能门**：`--vision` × `kvarn:k4v4` 端到端可用 + 自确定性（**当前无测试把 vision 与 kvarn 组合**，`test_vision_workspace.cpp:46` 只测 `Fp8E4M3Row256`）。已回写计划书 A1 邻域。
⑥ **prefill 判据**：用户 08-12 要求解释 —— 见本轮答复；**结论仍为"现仪器下不可断言"**（需新协议，详见下条"prefill 口径"）。

**(8) (a) 的量化（用户 08-12 要求「具体量化」，讨论后记录；已回写计划书 D-19 ⑧/⑨）**

**显存（先核验用户前提）**：`max_concurrency` 默认 **1**（`include/ninfer/types.h:459`）、`kv_table_rows = plan.max_concurrency`（`startup.cpp:200`）⇒ 内建环与外部环**都随并发线性放大**。
- **C=1**：(a) **88 MiB/序列**（内建 24 + 外部 64，N=1024）vs (c′) **76.5 MiB**（`M=9` 槽）⇒ **差 12 MiB**。
- **C=8**：(a) **≈716 MiB** vs (c′) **≈612 MiB** ⇒ 差 104 MiB。⇒ **显存不是决定因素，用户判断成立。**

**性能（逐项；前提 = 尾是**分区**不是追加，`body_window = window − tail_keys`，`small_t.cuh:151-152` ⇒ 注意力 FLOPs 不变）**

| 项 | 量 | 相对基准 |
|---|---|---|
| 每 token 精确双写 | 4 KiB/层/token ⇒ **64 KiB/token**（16 层） | decode 每 token 流式读 ~10.3 GiB 权重 ⇒ **~6×10⁻⁶** |
| 最新 N 键读取（N=1024） | bf16 **64 MiB/步** vs 同键 k4v4 **17.9 MiB** ⇒ **+46 MiB/步** | ctx8192 的 KV 读 ≈143.6 MiB ⇒ +32%；**但 KV 仅占 decode 总流量 ~1.4%** ⇒ 端到端 **≈ +0.4%** |
| workspace（acc BF16→FP32） | `attention.cu:567` 的 `D*2+8` → `D*4+8` ⇒ **+0.4 MB** | 每步读写各一次 +0.8 MB ⇒ **≈0.6%**，可忽略 |
| 两段式多一次 per-row 往返 | ≈**25 KB/步** | 可忽略 |

⇒ **(a) 的增量性能代价可忽略**。被感知的是**尾部固有、与路线无关**的代价：记录值 **decode −5.8% / MTP 接受率 −2.1 pt**（`docs/port-records/PORT-MEMORY.md:663-670`，post-fix 配对 A/B，口径 `--spec mtp --draft-tokens 7 --kv-dtype rk4v4-e8 --max-context 4096`，tail 0→1024）。
**两条限定**：① 该 −5.8% 是**带 MTP** 的 decode tok/s（含接受率联动），**不是纯 kernel 时间**；② **尾部在非 MTP decode 上的纯 kernel 代价从未单独测过 ⇒ 一项待补测量**（A8 已按此放宽判据）。(c′) 读同样多 bf16 尾键 ⇒ 这部分两者相同。

**改动难度**：**W1** 新增 KVarN 专用 tail-partial 路径（~300–450 行新码）· **W2 `partial_acc` BF16→FP32 + 两段式归并（改 ~120 行于 `decode_kernel.cuh`（1155 行）内，风险最高；A2/A6 须重跑）** ·
**W2b** body 窗口分区（~40–70 行，**在 op 内部完成 ⇒ 不必改 `text.cpp`**）· **W3** workspace 尺寸（~10）· **W4** 接线 3 入口 + 6 调用点（~80）· **W5** 尾环写入接入（~80–120）· **W6** 规划解除 fail-fast + 容量计费（~60–100）· **W7** A6/A7 脚手架（~300–500 行测试）· **W8** 回归（A2/A3/续列尾/前缀/MTP/ctest 263，跑）。
**合计 ≈900–1,400 行新码（含测试）+ ≈300 行改动、触及 12–15 文件、10–15 天**。**难度来源是 W2**；**利好 = 上游 beellama 正是这么做的（FP32 acc、旋转域合并后一次反旋）⇒ W2 是"照上游对齐"而非发明**；唯一必须钉住的是 **A6 的「tail=0 逐位不变」**。
**vs (c′)**：省 W1/W2（约 −5–8 天），但增「A6/A7 判据重写 + 双实现语义」（约 +2–3 天）⇒ 净差 **~3–5 天 + 永久语义债**。

**倾向**：**用户倾向 (a)**（16 GB 单流下显存非决定因素 —— 量化已证实），**待最终确认后写入 §6.3 / §7-WP6 并据此拆 WP6 步骤**。

### 2026-10-08-13 — **路线裁决 = (a)**（用户确认）+ **WP6 拆为 WP6.0–WP6.7** + **两项前置测量入队**；计划书 → **v20**

**(1) 裁决**：用户确认 **WP6 = 路线 (a)** —— 照 TAIL 已有的 `small_t_k8v4.cuh` 形状，把 KVarN body 接进**外部共享精确尾环**（`--kv-tail-tokens`），KVarN 侧新增"旋转域 tail partial + 两段式归并"。**(c′)（body 自建滚动槽）否决。**
依据 = §3-08-12(8) 的量化：显存**非决定因素**（(a) 88 MiB/序列 vs (c′) 76.5 @C=1 ⇒ **差 12 MiB**；C=8 差 104 MiB）、(a) 的**增量性能代价可忽略**（精确双写 64 KiB/token ≈ `6×10⁻⁶` 于权重流；最新 N 键多读 +46 MiB/步，但 **KV 仅占 decode 总流量 ~1.4%** ⇒ 端到端 **≈ +0.4%**；workspace +0.4 MB）、且**消除**双实现语义与 **BF16-acc 偏离**、判据与其它档位**同构**。

**(2) WP6 分步计划（已写入计划书 §7-WP6；每步给产出 / 验收 / 回退）**
- **WP6.0 前置测量**（GPU，~0.5–1 天）：**0a 本机 prefill 噪声底**（同二进制/同 prompt/单请求，重复 **≥5**，报 pp 的**中位数 + 最差 + 极差**；补 WP0 产出 ② 的欠账）· **0b 现有外部尾的纯 kernel 代价**（**关 MTP**、`--kv-dtype rk4v4`（或 `bf16`）× `--kv-tail-tokens {0,1024}`、decode-only、重复 **≥3**、报 tg 分布 ⇒ 把 A8 的 `−5.8% / −2.1pt`（带 MTP 的配对值）**分解**为「纯 kernel」+「接受率」）。
- → **WP6.1** KVarN tail-partial（W1；**"acc 旋一次"** 的等价实现，FP64 oracle）→ **WP6.2 归并改造（W2，最高风险）**：`partial_acc` **BF16→FP32** + `reduce_output_hadamard_kernel` 改**两段式**；**接受条件 = A6「`tail=0` 逐位不变」** → **WP6.3** 分区与接线（W2b/W3/W4/W5/W6；`body_window = window − N` 在 op 内部完成；解除 `startup.cpp:1035-1045` 的 fail-fast；**`--kv-tail-type` 禁止静默**）→ **WP6.4** A6/A7 脚手架（W7：FP32 oracle × 跨 group(128)/ring(64) 边界 + needle）→ **WP6.5** 容量/显存（WP7 的 kvarn 尾环部分**提前到此**，实测 ±5%）→ **WP6.6** 质量/速度收口（**显式处理 A4 的 N>384 风险：至少测 `N ∈ {384, 1024}`**，或声明阈值随 N 调整）→ **WP6.7** 回归（A2/A3/MTP/续列尾/前缀/ctest 263 + **新增 `--vision` × kvarn 功能门**）。
- **不变量**：**A6 的「`tail=0` 逐位不变」是 WP6.2 的接受条件**；`--kv-tail-type f16` **必须显式拒绝或真支持，不得静默**；任一子步失败即回到 **fail-fast**（§6.4）。
- **待定**（WP6.3 前须有结论）：尾 dtype —— **bf16**（复用现成 BF16 旋转入口，**免**"f16 旋转入口"交付物；用户已接受"若采 bf16 尾则在本机复核"）还是 **f16**（须新增 f16 旋转入口，`kvarn_hadamard` 只收 BF16，`codec.cu:185-189`）。

**(3) 两项测量的状态**：**已入队、尚未执行**（本轮只落计划；用户 08-12 指定）。命令与验收见计划书 §7-WP6.0 —— **"补两项测量"按"入队"解释**（用户措辞的提交门是"做好计划并信息落盘"，未把测量结果列为门）。

**(4) 计划书回写**：v19 → **v20**：版本头新增 v20 块；**§6.3** 加"已裁决 = (a)"行；**§7-WP6 整段改写为分步计划（WP6.0–WP6.7）**；**§7-WP7** 加范围订正注（kvarn 外部尾环归 WP6.5）；**D-19 ⑨** 由"倾向"改为"**裁决 = (a)**"；归档索引 D-19 行补 ⑧/⑨。**A1–A8 判据不变。**

**(5) 交付**：本轮改动（计划书 + 本日志）**已提交并推送 `origin/main`**；`AGENTS.md` 的既有改动（**会话开始前即存在**、28/28 行、含"最终向用户汇报时必须使用中文"规则 + 表格重排）**未纳入**（非本轮产物，避免把他人未提交改动折进本轮提交）。推送一并发布此前 **18 个**未推提交。


### 2026-10-08-17 — **尾环 dtype（bf16 vs f16）量化分析**（用户要求"依据要量化"）：环源是 **BF16** ⇒ f16 是**重编码**（范围内逐位等价）；**唯一取值差异来自 `p_s` 的算子精度**；实测 PPL 相对差 **−8.8e-6**、显存**逐字相同**；**建议维持默认 f16（待裁决）**

**触发**：用户要求分析 bf16/f16 各自的优劣、收益与代价，且依据须量化。

**(1) 决定性代码事实（改变了整个分析方向）**
尾环的**源数据是 BF16**：`src/ops/kv_cache/append/kernel.cuh:104-105` 用 `store_tail_vec8(&tail_k[off], &k[src_off])`，而 `store_tail_vec8` 的签名是 `template <typename Dst> void store_tail_vec8(Dst*, const __nv_bfloat16*)`（`src/ops/common/kv_tail_element.cuh:73`）；`KvTailElement<__half>::from_source` = `__float2half(__bfloat162float(v))`（`:59-61`）。⇒ **f16 是把 bf16 值重新编码，不是更高精度的采样**。

**(2) 静态量化（IEEE + 12M 样本 bit-exact 探针，纯 CPU，脚本 `/tmp/tailtype/bf16_vs_f16.py`）**
bf16 = 8 位指数 / **8 位 significand**，最大 3.39e38；f16 = 5 位指数 / **11 位 significand**，最大 **65504**；**两者都是 2 B/元素**。
⇒ 源只有 8 位 significand ⇒ **f16 的 11 位在 `[2^-14, 65504]` 内可逐位精确表示 bf16 值**，**精度收益为零**；f16 只是多了一个失效模式。
探针（4M/组，`bf16→f16` 往返）：`N(0,1)` **99.99947% 精确**、0 溢出、非精确项绝对误差 ≤ **2.98e-8（2^-25）**；`N(0,5)` **99.9999% 精确**、0 溢出；刻意跨界的对数均匀 `1e-8…1e5` 则 **1.43% → inf**。边界探针：`65504 →（bf16 舍入为 65536）→ inf`；`2^-15`/`2^-24`/`6.10e-5` 精确；`2^-25 → 0`（误差 2.98e-8）。
⇒ **唯一实质风险 = 上溢**（`|v| > 65504`）；下溢绝对误差被 `2^-25` 限死。

**(3) 关键修正：dtype 不只决定环存储**
`small_t_tail.cuh` 里 `qkv_s`（Q/K/V，`from_source`）与 `p_s`（**注意力概率**，`from_float(p00)`，`:317-324`）都是 `Elem[]`。Q/K/V 的**实数取值两臂相同**（故差异必来自另一个消费元素类型的算子）⇒ **唯一取值不同的算子是 `p`**：bf16 8 位 vs f16 11 位 significand。MMA 同为 `m16n8k16…f32`（`mma.cuh:33/42`）⇒ 吞吐与累加精度相同。
**该效应对 KVarN 路线 (a) 不存在**（WP6.1 的 p/acc/m/l 全程 FP32 寄存器）⇒ **对 KVarN，dtype 纯属环编码**。
（另注：仓内 `mma_f16_f16acc` 注释称 GeForce 上 **2×** 速率，但 `KvTailElement<__half>::mma` **不调用它**，且用它要牺牲累加精度 ⇒ **f16 目前拿不到 MMA 吞吐收益**。）

**(4) 实测 A/B（GPU，同协议两臂）**
命令：`ninfer-perplexity <MODEL> --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 2048 --stride 1024 --score-width 8 --kv-dtype rk4v4 --kv-tail-tokens 1024 --kv-tail-type {bf16|f16}`（`--score-width 8` 使尾**被真正读取**；第二臂加 `--kld-base <bf16.topk>`）。

| 指标 | bf16 | f16 |
|---|---|---|
| overall PPL（261,223 token） | **4.892212** | **4.892169**（相对 **−8.8e-6**） |
| mean NLL | 1.587644 | 1.587636 |
| 47 个中途检查点 | — | 低 **30** / 高 **15** / 同 **2**；相对差 max **6.05e-4**、mean **−9.29e-5** |
| KLD（100-topk） | — | median **4.98e-4**、mean **1.89e-3**、P99 **8.87e-3**、P99.9 **4.78e-2**、**max 14.96**；`same_top` **0.9847**、`mean_target_dlogp` **9e-6** |
| 显存（CLI，`rk4v4`+N1024） | `sequence 296.7 MiB`、`payload 138.0 MiB`、`total 10.7 GiB` | **逐字相同** |
| score rate | 153.0 tok/s | 154.3 tok/s（噪声内） |

**读法**：两臂**不逐位相同**（47/47 检查点都不同）但差异量级极小；KLD 的 `max 14.96` 与 `same_top 0.9847` 表明其来自**近并列顶点的排序翻转**（`mean_target_dlogp` 仅 **9e-6** ⇒ 模型实际预测几乎未变）；方向混合（30 低/15 高）⇒ **无证据表明任一 dtype 系统性更优**。与 (3) 的 p 机制在量级上一致。

**(5) 代价侧量化**：显存/带宽 **0**（实测逐字相同；几何 = **64 KiB/token**，N=1024 ⇒ 几何 64 MiB / 实测增量 68 MiB）；MMA 同吞吐；写路径 f16 多 **~69,632 条 cvt/token**（每 8 元素 8×2 次，一次性，相对 27B 前向可忽略）；读路径同为单指令（**未单独测量**）。

**(6) 结论与建议（待用户裁决）**
**建议维持全局默认 f16（现状）**：f16 **无实测劣势**（|相对 PPL 差| ≤8.8e-6），保留现有 TAIL 路径 11 位的 `p`，且是现状默认（改动面为零）；KVarN 侧 f16 与 bf16 在范围内取值相同，唯一差别是**不可达**的 `inf` 风险。若改 bf16，收益 = 消除 `inf` 失效模式 + 去掉写路径 cvt，代价 = TAIL 的 `p` 从 11 位降到 8 位（影响同量级）。**不建议**按存储分设默认。

**(7) 未做 / 未测（如实）**
- **未单独隔离 p 机制**（需把 `p_s` 临时改成 FP32 再重跑 ~1 h）⇒ 机制归因是"排除法 + 量级一致性"，非直接实验。
- **未直接测 `max|K/V|`** ⇒ "无上溢"只由"261k token 下 PPL/KLD 无异常"间接排除（若真有 inf，误差会是灾难级而非 1e-5 级）。
- **未测 KVarN 接线后的 dtype A/B**（WP6.3 之后才能做）；**未测 f16/f16-acc MMA 路线**；**未测读路径 cvt 成本**。
- **未提交**（用户约束）。

**产物**：`/tmp/tailtype/{bf16,f16}.log`、`bf16.topk`、`{bf16,f16}/report.json`、`bf16_vs_f16.py`、`{bf16,f16}.ckpt`；回写计划书 §6.2 条 3 再订正、§7-WP6 待定项、附录 **D-22**。

---
