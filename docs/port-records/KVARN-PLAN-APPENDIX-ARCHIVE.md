# KVARN-PLAN-APPENDIX-ARCHIVE — KVarN 计划书附录 D-1…D-9 执行记录归档

- 归档日期：2026-10-08
- 来源：`../../kvarn-port-into-precision-tail-plan.md` 附录 D（版本 v13，归档前的提交状态 `6aea191b`，源文件第 804–1094 行）
- 内容：**逐字搬运**的附录 **D-1 … D-9**（WP0.5-C 构型口径、WP0 基线、WP1 ops/测试、A3 的仪器—差分—定案三件、
  WP2 执行、WP0.5-B 起手、WP3① 页几何）
- 归档原因：这九条是 **WP0–WP3① 的早期执行记录**。它们的**结论已被回写进计划书正文**
  （§1 A1/A3、§7 各 WP 状态与验收、§9 风险行、§10 D1–D6），推进计划不再需要全文；
  计划书正文只留 D-N 索引行，**锚点名 D-1…D-13 仍可解析**。
- **未归档**：D-10（229k 同字节矩阵 + `k6v6` 门槛 0.002688）、D-11（发布档三档准入数字）、
  D-12（GPU 收尾四件 + P2 前置）、D-13（token 级 MTP 分叉表）**仍留在计划书正文**，
  因为 WP6/WP8 的对照阈值与发布档判定要直接引用它们。
- 体例：以下整节是源文件的**字节级副本**，未改写任何数字、结论、命令或措辞（AGENTS.md「measured reality wins」）。
  进度侧的同源记录见 `./KVARN-PROGRESS-ARCHIVE-2026-10.md`。

## D-N 索引（正文引用「附录 D-N」时按本表解析）

| 锚点 | 原小节标题（逐字） | 一句话内容 | 承重状况 |
|---|---|---|---|
| D-1 | WP0.5-C 构型口径声明（D6 决议） | `build-port` 实测 `NINFER_SM120_NATIVE=ON` ⇒ 本机为 **native 口径**，A1/A8 噪声底与常量按此读；compat 口径噪声底**未测** | 结论现行（§7-WP0.5-C / §10-D6 / 进度未决项 2） |
| D-2 | WP0 基线发现 | 整树构建唯一失败目标 `ninfer-multi-gpu-probe.exe`（`LNK2019`，**先于本工作存在**）⇒ 一律按目标构建；本机 1 个 CUDA 设备 | 结论现行（§9 / §1 A1 / 进度 §1、未决项 4） |
| D-3 | WP1 结果（ops 编译通过；测试 30 项中 1 项容差失败） | kvarn ops **零告警编译通过**（一手印证 §4.1 设备侧兼容）；`ninfer_kvarn_test` 30 项中 1 项 K oracle `relative_l2=6.09e-4 / limit=3.0e-4 / max_abs=0.216` 失败 ⇒ 归 WP5 容差标定 | 已被 WP4.3（07-21）+ WP5（08-03/08-04）**收口取代** |
| D-4 | WP0.5-A 结果：仪器已就位，但 **A3 目前不成立**（回写 §1 A3 / §7 WP0.5-A）—— ⤴ 本条结论已于 D-6 定案替换（O1） | 纯 `bf16` k=1 在 **token 91 分叉**（`expected=2466 actual=2640`），k=0/k=3 全等；`--no-cuda-graph` 逐字复现 ⇒ 确定性、非 graph 相关 | **本条结论已被 D-6 替换**（原小节标题即已标注） |
| D-5 | A3 差分裁决：缺陷**先于 precision-tail 存在**（2026-10-07，实测） | 上游包与 TAIL 包同模型对照：两仓 MTP-off/on **同 index 538 分叉**、`up_off/tail_off` 与 `up_on/tail_on` **逐字节相同** ⇒ 非 precision-tail 引入；性质当时仍**未定** | 差分结论现行；性质判定由 D-6 接管 |
| D-6 | A3 定案 + 报告复核裁决 + 计划调整（2026-10-07）（回写 §1 A3 / §7 WP0.5-A·WP3·WP9 / §9；版本 v3→v4） | 逐条复验 `PORT-MTP-PARITY-A3-INVESTIGATION.md` C1–C9 通过 ⇒ **采纳 O1、拒绝 O2、O3 次优**；A3 改为相对判据、parity 测试降级为诊断仪器；记录 2 项复核瑕疵（C5 机制未经运行时确认、§5.3 行号属 Fork B） | **现行**（§1 A3、§9 A3 行、§7 WP0.5-A/WP3/WP9 的判据出处） |
| D-7 | WP2 执行记录（页面几何 + 存储枚举，2026-10-07） | 16 文件产出清单 + `stash` 重建基线法：合成 **242/245 通过**（3 项先存）、真实模型 **10/16**（6 项产物结构性/已知 A3）⇒ A1 的 `ctest` 口径按此收窄 | **现行**（§1 A1 口径注、§9 两行的实测出处） |
| D-8 | WP0.5-B 起手实测（2026-10-07，详见进度记录 07-15） | `kvarn:k4v2` 长上下文**不可运行**（`no free active entry`）⇒ 准入门被 WP3① 前置；首个数据点 1438 tok 单窗口：210 B 档 KLD **0.0049** vs `rk2v4-e8` 0.0280（≈5.7×） | 起手结论已由 **D-9 解除阻塞**、由 **D-10 以 229k 规模取代**（数字保留以存史） |
| D-9 | WP3① 执行记录（地址空间页几何，2026-10-07）（回写 §7-WP3 / §9；版本 v7→v8） | 根因 `entitlement`(64 口径) > `page_capacity_`(128 口径) ⇒ `nullopt`；19 文件按 `kv_page_tokens` 贯穿 ⇒ `kvarn:k4v2` ctx8192 / 229,348 token / 28 窗口跑通，PPL **4.72225** vs `bf16` **4.69317**（+0.62%）、tok/s −0.5%；定向 14 项回归全过 | **现行**（§9「地址空间页几何」行标已消除的实测出处、§7-WP3① 验收） |

---

## 逐字归档：附录 D-1 … D-9

<!-- 以下自「### D-1」起为源文件第 804–1094 行的逐字副本 -->
### D-1 WP0.5-C 构型口径声明（D6 决议）

- `build-port/CMakeCache.txt` 实测：`CMAKE_CUDA_ARCHITECTURES=120a`、`NINFER_SM120_NATIVE=ON`、
  `NINFER_BUILD_APPS=ON`、`BUILD_TESTING=ON`、`NINFER_BUILD_BENCHMARKS=OFF`。由
  `CMakeLists.txt:33-37`，`NOT NINFER_SM120_NATIVE` 为假、arch 非 80/86/89 ⇒ **`NINFER_COMPAT_PATH=OFF`**。
- ⇒ 本构建走 **native（本仓自述 unqualified）路径**，**不是** `AGENTS.md` 所说「120a 上已测的
  `mma.sync` 兼容路径」。**A1/A8 的噪声底、route 表与 split-capacity 常量的判断一律属于 native 口径。**
- 取此口径的理由：`AGENTS.md`「使用既有 `build-port`，**不要重配**」为硬约束，且重配是本机唯一会
  真正破坏构建的操作。
- **Open item**：若需 compat 口径的噪声底，须单独一次重配并重测（未做，记账于此）。
- 文档陈旧：`AGENTS.md:34,37` 仍写 sm_86 / RTX 3090 / CUDA 12.8，与本机 sm_120a / 5070 Ti / CUDA 13.3
  矛盾；`AGENTS.md`「Windows build environment」节已自洽，仅「Product and architecture」节待改，归 WP9。

### D-2 WP0 基线发现

- **整树 `cmake --build build-port -j` 不通过**：唯一失败目标 `ninfer-multi-gpu-probe.exe`
  （`LNK2019` 未解析 `ninfer::core::current_resident_memory`）。根因：`d92f0abb`
  （`feat(cuda): add strict and mixed memory policies…`）令 `src/core/device.cu:68` 的 `cuda_check`
  调用 `current_resident_memory()`，而 `tools/CMakeLists.txt:13-16` 的 probe **刻意只编
  `device.cu` + `multi_gpu_probe.cu`、不链 `resident_memory.cpp`**（其注释仍假设 device.cu 只依赖
  `core/device.h`）。**先于本次工作存在，与 kvarn 无关。** 后续一律按目标构建（`ninfer_ops` /
  `ninfer_tests`）。A1「`ctest` 全绿」不受影响（ctest 不需要该目标）。
- 本机设备：`nvidia-smi -L` 只有 **1 个 CUDA 设备** RTX 5070 Ti（Intel UHD 770 为非 CUDA 设备，
  不参与计算）；空闲基线 ~48 MiB。CUDA 设备号无歧义。

### D-3 WP1 结果（ops 编译通过；测试 30 项中 1 项容差失败）

- 已复制 `src/ops/kvarn/`（12 文件）+ `include/ninfer/ops/{kvarn.h,kvarn_attention.h}` 入 TAIL 同路径；
  `src/ops/CMakeLists.txt` 已增 `include("${CMAKE_CURRENT_LIST_DIR}/kvarn/sources.cmake")`。
- **`ninfer_ops` 零告警编译链接通过**（`attention.cu`/`codec.cu`/`decode.cu`）⇒ **一手印证 §4.1
  「设备侧接口兼容」**：kvarn ops 未改一行即在 TAIL 编过（含 `launch.h`/`small_t.cuh` 的差异面）。
- `tests/ops/test_kvarn.cpp`（1710 行）已移植并注册为 `ninfer_kvarn_test`。**与 FORK 原文的移植改动
  仅一处**：`__builtin_popcount` → `std::popcount`（11 处）+ `#include <bit>`（MSVC 无 GCC 内建）。
- **运行结果（RTX 5070 Ti，`ninfer_tests.exe ninfer_kvarn_test`）**：`main()` 累计约 30 项用例，
  **仅 1 项失败**——
  `KVarN K official oracle: relative_l2=6.09e-4 limit=3.0e-4 max_abs=0.216`。
  紧随其后的 `KVarN K stored-bit decode`（限 **2.0e-7**）**未报错** ⇒ 设备反量化与其自身存储位的
  主机重算一致，**设备 codec 自洽**。⇒ 差距是 **FP32 设备 Sinkhorn 与 FP64 oracle 在 4-bit 桶边界的
  少量码翻转**，性质是**容差标定**（**归 WP5**：现有 `3.0e-4` 在本机 4-bit 不成立），不是接线/移植缺陷。
- **未隔离**：该漂移来自 MSVC libm（oracle 侧）还是 nvcc 13.3 libdevice（device 侧），尚未定位；
  亦未证明 FORK 在 Linux 上该项曾通过。**WP1 验收（`ninfer_kvarn_test` 通过）因此尚未完全达成**，
  待 WP5 给出按位宽的容差后再判。
- 未做（WP1 范围内仍欠）：kvarn bench（`kvarn_attention_bench.cu`/`kvarn_codec_bench.cu`，
  需 `NINFER_BUILD_BENCHMARKS=ON` 重配，与「不重配」冲突，**待显式决定**）。

### D-4 WP0.5-A 结果：仪器已就位，但 **A3 目前不成立**（**回写 §1 A3 / §7 WP0.5-A**）—— **⤴ 本条结论已于 D-6 定案替换（O1）**

- **移植 + 构建：成功**。`test_engine_mtp_greedy_parity_real.cpp` 已入 TAIL（557 行），
  注册于 `tests/models/qwen3_5/tests.cmake:188-191`；`ninfer_tests` 目标 `[192/193]` 链接通过。
- **实跑：失败**。`bf16`、`spec=mtp`、`output=512`：**k=0（×2）与 k=3（×2）均 512 token 全等**，
  但 **k=1 在 token 91 分叉**：`expected=2466 actual=2640`（`expected` = MTP-off 参考）。
- **性质**：纯 `bf16`，**与 kvarn 无关** ⇒ 这是 **TAIL 固有的 MTP-on/MTP-off greedy 不一致**，
  被本次新移植的仪器首次暴露。k=3 通过而 k=1 不通过，**非单调**，形态可疑。
- **影响**：§1 **A3**（"主文本与 MTP 的 greedy 与 MTP-off 一致"）**当前不满足**；
  §7 **WP0.5-A 验收**（`ctest -R ninfer_qwen3_5_mtp_greedy_parity_real_test` 通过）**未达成**。
- **待裁决（新增，须在继续 WP2+ 之前决）**：kvarn 的 decode/speculative 路线依赖 MTP 一致性，
  故这是 **WP3 起的前置阻塞项**。**已排除 CUDA Graph**：`--kv-dtype bf16 --no-cuda-graph` 复跑
  **完全复现**（同 token 91、同 `expected=2466 actual=2640`，k=0/k=3 仍全等）
  ⇒ 是**确定性**的 **k=1 特异** MTP 草稿/验证缺陷（数值或调度），与 graph 复用无关。根因仍待定位。
- **注意**：该失败**不影响** WP1 的结论（kvarn ops 编译通过、codec 自洽），也不影响 A1/A2 的推进。

### D-5 A3 差分裁决：缺陷**先于 precision-tail 存在**（2026-10-07，实测）

**方法与结果**：两包各自 `ninfer-serve.exe`（上游 `ninfer-package\engine` @ `b06908ba` 2026-10-02；
TAIL `ninfer-precision-tail-package\engine` 2026-10-06）跑**同一模型**，`--greedy`、
`max-context 2048`、`512` 输出，对每台引擎做 **MTP off vs `--spec mtp --draft-tokens 1`** 同请求对照：

| 对照 | 结果 |
|---|---|
| TAIL off vs on | 分叉，首异字符 index **538** |
| **上游 off vs on** | **同样分叉，同 index 538，同文本** |
| `up_off`/`tail_off`、`up_on`/`tail_on` | **逐字节相同** |
| `--max-context 768`、`--draft-tokens 3` | 前者四输出同 2048 档；后者**两仓都**分叉 |
| MTP-on 复跑 / 投机计数 | 复跑逐字节相同；计数两仓一致（280/231） |

**裁决**：**上游同样具备该分叉，且与 TAIL 输出逐字节相同** ⇒ `--kv-tail-tokens`/`--kv-tail-type`
只是新增开关（默认关），**precision-tail 移植没有引入它**。
（静态旁证：MTP/投机实现 **20 文件两仓逐字节相同**；`causal_cache/` 下 TAIL 独有文件仅
`small_t_tail.cuh`、`small_t_tail_shadow.cuh`。）

**性质未定**：两仓"draft/accepted 计数相同但提交 token 不同"更支持"近似并列处 argmax 翻转"，
而非验证逻辑写错；**"是 bug" 还是 "parity 本就不是该实现的承诺" 仍未判定** —— 现属**上游问题**。

**对计划的影响（须裁决）**：§1 **A3** 按"**绝对** parity"当前**不可满足**（基线本身不满足）。
建议改为 **"kvarn 不得使 MTP 一致性劣于同配置基线"**（相对判据），并让移植的 parity 测试
按"已知上游基线分叉"记账而非要求全绿。**这是验收判据变更，需显式批准。**

**保留/未测**：无 token id 暴露，**首个不同 token 的精确下标未测到**（char 538 ≈ token 138 ≠ 测试的 token 91，
基准不同）；**只有差分结论承重**。

### D-6 A3 定案 + 报告复核裁决 + 计划调整（2026-10-07）（**回写 §1 A3 / §7 WP0.5-A·WP3·WP9 / §9；版本 v3→v4**）

**触发**：用户指示审阅 `docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md`，据此调整工作计划。

**主代理复核（逐条独立复验，非转述）**：报告的 C1–C9 全部复核通过。承重项一手核过：

- **GitHub**：`#265` = open / 0 评论 / `cometkim` / 2026-09-16，正文引文逐字命中（"width-keyed table"、
  "0.72%…~3.5%…flips roughly 5%"、"No cross-width numerical consistency is implied"、RTX 5090 + NVFP4）；
  `#80` = `closed_by:Neroued` / `state_reason:completed`，评论文本逐字为 **"Not in scope. Greedy does not
  mean batch invariant. And batch invariant limits optimizations."**；`releases`=0、`tags`=0；
  `parity`/`MTP greedy` 标题搜索各 **0**。
- **代码**：`small_t.cu:383-395` 的 `case1→(1,2)`/`case2→(2,4)`/`case4→(4,4)` 与 `kBlock=32*WarpsPerCta`
  逐行确认；`gdn_input_proj.cpp:603` 门在 `QType::NVFP4`、GGUF 路唯一宽度键 `kMaxVectorColumns=8`、
  `IQ3_XXS` 仅存在于 TAIL（上游/Fork B 各 0）。
- **三仓 provenance**：Fork B 的 `7d566547 d476fafa 114b0fcb dff96dca 1388b7c2 16e12737` 在 Fork B 均为
  `commit`、在 TAIL 均为 `fatal`（不在提交图）；`a7818988` 在 TAIL 为 `commit`。Fork B 的 `small_t.cu:280`
  宏为单参 `(TOKENS)` + "canonical-column" 注释（TAIL 为 `(TOKENS,WARPS)`）；Fork B parity 测试
  `:233-234` 的 oracle 注释逐字命中。
- **合同文本**：涉及句在**上游 :330 / Fork B :357 / TAIL :368 三仓逐字相同**；Fork B `qwen3_5-model.md:270`
  的 "exact committed-token parity" 与其 `:358` 保留的上游"不施加等值"句**同文件自相矛盾**（已核实）。
- **本仓既有实测**：`docs/performance.md:29-45`、`docs/archive/TODO.md:4462-4464` 逐字核过。

**复核发现的边界/瑕疵（如实记录）**：

1. 报告 §4（C5）的**机制判定是静态代码结论，未经运行时确认**——报告 §10.2 自陈，决定性实验
   （把 `case 2` 改成 `<2,2>` 后看 k=1 是否转为全等）**未做**（属代码改动，需授权）。故 C5 应读作
   "代码结构上唯一合理的解释"，**不是已证事实**。
2. 报告 §5.3 引用的 `docs/maintainer/paged-kv-cache.md:188` 实为 **Fork B** 的文件（TAIL 无该行）；
   已在该报告加 "(Fork B)" 标注以免误读。

**裁决：采纳报告的 O1（判据替换），拒绝 O2，O3 次优。**

- **O1（采纳）**：A3 由"绝对逐位 parity"替换为 ①同配置自确定性 ②质量不劣化 ③首分叉已文档化
  ④ kvarn 专属"不劣于同配置基线"；移植的 parity 测试**改造为诊断仪器**（报首分叉下标/分叉率/KLD 差），
  **不再作通过/失败门禁**。
- **O2（拒绝）**：把 Fork B 的 canonical per-query 算术面移植进 TAIL（约 5 核心文件 +380/−723 及
  linear_add/gdn/kvarn 片段）——**性能代价从未被 Fork B 量化**，与 #80 Neroued 的"batch invariant limits
  optimizations"正面冲突，且与 TAIL 自身的宽度特化改动（`small_t_tail.cuh`/`_shadow.cuh`/`small_t_bf16.cuh`）
  **需重算而非合并**。
- **O3（次优）**：直接删除测试——丢弃唯一能逐宽度定位分叉的仪器。

**据此的计划调整（本次已回写本文件）**：

- §1 **A3** 行改写为相对判据（原"绝对逐位一致"标为**已废弃**）。
- §7 **WP0.5-A** 由"移植 parity 门禁"改为"仪器交付 + 改造为诊断"，验收改写。
- §7 **WP3 / WP9** 验收中的 "parity 测试通过 / MTP greedy 一致"改为"不劣于同配置基线"。
- §9 风险表 A3 行标记**已定案**（不再是"待批准"）。
- 版本 v3 → **v4**，增「v4 相对 v3 的变更摘要」。

**对 kvarn 移植的影响**：**不阻塞**（报告 C9）。kvarn 的 decode/speculative 路线需要的是
"同配置可复现 + 质量不劣化"，两者实测成立；"跨配置逐位相等"上游与本仓均已书面否认。

**未做（如实记录）**：本轮为**审阅 + 文档回写**，**未跑 GPU**，**未做任何网络写操作**；
上条 1 的决定性运行时实验**未做**。

### D-7 WP2 执行记录（页面几何 + 存储枚举，2026-10-07）

**产出（16 文件）**：`types.h`（`KvarnGroup128` + `KvarnBits` + `EngineOptions.kvarn_bits`）、
`paged_kv_cache.{h,cpp}`（`kKvarnPageTokens`/`kv_page_tokens`/`kv_page_shift`；校验放宽至 64|128；
plane 形状与校验改用 `spec.geometry.page_tokens`）、三处 `--kv-dtype` parser（含
`ServeOptions`/`Options` 的 `kvarn_bits` 与两处 `engine_options` 接线）、三处生产 + 三处基准名字
switch、`model_instance.cpp` 指纹 `;kvbn=<bits>`。

**构建**：`ninfer_ops` 0 错 0 新告警；`ninfer_tests ninfer-serve ninfer ninfer-perplexity` exit 0。
（沿用既有 `build-port`，未重配。）

**功能检查（CLI，无模型/GPU）**：`--kv-dtype kvarn:k7v7` → 拒绝并列出三档；`kvarn:k5v5` 与裸 `kvarn`
→ 通过解析（随后因缺 `--corpus` 报错）。

**回归①合成（`ctest -E real -j4`，245 项）**：**242 通过 / 3 失败**。三项失败**均经基线对照证实为先存**：
`ninfer_device_sync_empty_test`、`ninfer_gdn_gating_proj_test`、`ninfer_kvarn_test`（WP1 已知容差）。
**基线方法**：16 个 WP2 文件 `git diff` 备份 → `git stash push -- <16 文件>`（保留 WP1 未跟踪文件与文档）
→ 重建 `ninfer_tests` → 逐项复跑 → `git stash pop` 且 `diff -q` 校验恢复与备份**逐字节相同**。
另：首轮 `ctest -E real` 曾报 5 项 `Not Run` + 2 项 Python 失败，根因是**当次只构建了 `ninfer_tests`
目标**，5 个 `STANDALONE` 测试（`ninfer_jinja_test.exe` 等）未构建；补建后**7 项全部转通过**，非回归。

**回归②真实模型（`ctest -R real -j1`，`NINFER_TEST_ARTIFACT=<本机唯一 27B 产物>`，16 项）**：
**10 通过 / 6 失败**，无一项可由 WP2 解释——`prefix_real`（无 prompt golden）、`hybrid_prefix_real`/
`dflash2_real`（缺 dflash2 组件）、`dflash_real`（缺 dflash 组件）、`moe_real`（本产物非 35B MoE）、
`mtp_greedy_parity_real`（**已知 A3**，token 91 `expected=2466 actual=2640`，与 D-4 逐字相同）。

**结论**：**WP2 未引入任何新失败**；A1 的 `ctest` 口径按上表收窄（已回写 §1）。**未做**：kvarn 端到端
（尚不可运行）；`--help`/`docs/` 未改；kvarn parser 单测未加；真实模型测试**未做基线对照**。

---

### D-8 WP0.5-B 起手实测（2026-10-07，详见进度记录 07-15）

**触发**：用户指示据记忆文档评估「是否还有必要继续推进」；独显空闲，遂直接实测（单任务，无并发）。
**仪器**：`ninfer-perplexity --score-width 8 --score-topk 100 --kld-base`（decode-width，合并精确 KV 尾部，
即 §1 A4 要求的仪器），本机唯一 27B 产物，base = `--kv-dtype bf16 --kv-tail-tokens 0`。

**① 关键障碍（支配本节）**：`kvarn:k4v2` 在**多窗口/长上下文**下**无法运行**——
`--corpus … --quick`（ctx8192，229,348 评分 token）与 **ctx1024** 均 `window 0` 即报
`text KV address space has no free active entry`；退到**单流 `--text`**：ctx4096 下**仅 ≤≈1.4k token 可用**，
3675 token（ctx4096）与 24,705 token（ctx32768）**均失败** ⇒ **kvarn 可用容量 ≈1–2k token**
（= §10 未决项 8 / §7-WP3① 的页几何缺陷症状）。
⇒ **§7 WP0.5-B「≤1 天、ROI 最高」不成立**：该门被 **WP3①** 前置。**执行顺序改为 WP3① → 完整 WP0.5-B → WP4/WP6。**
**▲ 07-16 更新**：该阻塞已由 **WP3① 消除**（附录 D-9），② 表的 `kvarn:k4v2` 行在 ctx8192 / 229,348 token
上已可补测（本轮实测：mean KLD **0.010513** / same_top 0.9624，PPL 4.72225）。

**② 已测数据（decode-width KLD，top-K 100）**

| 臂 | 上下文/规模 | B/token/头 | mean KLD | max | same_top | PPL |
|---|---|---|---|---|---|---|
| `bf16`（基准） | corpus ctx8192 / 229,348 tok | 1024 | — | — | — | **4.693174** |
| `rk4v4 + tail1024` | 同上 | 280 | **0.001724** | 8.786 | 0.9833 | 4.695007 |
| `kvarn:k4v2` | 同上 | 210 | **不可运行**（①） | — | — | — |
| `bf16`（基准） | `--text` ctx4096 / 1438 tok | 1024 | — | — | — | 4.6390 |
| **`kvarn:k4v2`** | 同上 | **210** | **0.004893** | 0.1319 | 0.9715 | 4.6549 |
| `rk2v4-e8` | 同上 | 216 | 0.028026 | 0.9975 | 0.9318 | 4.7784 |
| `rk4v4` | 同上 | 280 | 0.003194 | 0.1239 | 0.9840 | 4.6490 |
| `rk2v4-e8` / `rk4v4` / `nvfp4` / `k8v4` | `--text` ctx4096 / 3675 tok | 216/280/288/402 | 0.026552 / 0.003283 / 0.003315 / **0.002205** | — | 0.9241/0.9766/0.9722/0.9782 | — |

**③ 判定**
1. **首个 KVarN 数据点为正**：`kvarn:k4v2`（**210 B**）KLD **0.0049**，**远优于字节匹配的 `rk2v4-e8`
   （216 B，0.0280，≈5.7×）**，并**接近多用 33% 字节的 `rk4v4`（280 B，0.0032）** ⇒ **KVarN 每字节质量明显更高**。
   ⇒ **`PORT-DOD.md:23-32`「无质量驱动理由」不足为据**（ppl、4 chunks、**仅对 f16/q8_0**、未对同字节档），
   与本节数据**方向相反**。
2. **但样本小（1438 token、单窗口、单文本）⇒ 不作结论**；**发布档 k4v4/k5v5/k6v6 仍未测**（需 WP4）。
3. ⇒ **不宜投 WP6（10–15 天，最高风险）**；**亦不宜按 PORT-DOD 直接否决**。

**④ 未做（如实记录）**：未修页几何（WP3①）；未跑 k4v4/k5v5/k6v6；未做多文本 / ≥3 重复 / 跨域；
未做端到端 MTP；未动任何源码（本轮仅文档 + `.deps/kvarn-adm/` 下 gitignored 产物）。

**⑤ 下令（供下一窗口执行）**：**先做 WP3①**（`kv_pages_for_frontier`/`kv_pages_for_tokens` 与
`KVAddressSpaceStore` 内部 18 处按 `kv_page_tokens`/`kv_page_shift` 取页，消 47 处硬写 64）；
验收 = kvarn 在 ctx8192 / 229k token 上跑通，随后补 k4v4 档（WP4）复跑本节 ② 表。

**▲ 环境题外发现（已解决）**：Git Bash 传 Windows 绝对路径给 `ninfer-*` 会被 MSYS 改写
（`D:\…`→`/d/…`）⇒ `CreateFileW Win32 error 3`。可行：**前斜杠路径 `D:/ninfer/…` + `MSYS_NO_PATHCONV=1`**，
或整条命令写进 `.bat` 由 `cmd /c` 执行。

---

### D-9 WP3① 执行记录（地址空间页几何，2026-10-07）（**回写 §7-WP3 / §9；版本 v7→v8**）

**触发**：用户指示执行 WP3①（唯一硬阻塞）。

**根因（复述实测）**：`KVAddressSpaceStore::create_active` 的 `entitlement` 由
`kv_pages_for_frontier(frontier)`（硬写 64）算出，而 kvarn 的 `page_capacity_` 来自规划期
`kv_page_tokens(kvarn)=128` ⇒ 4900 token 时 77 页(64 口径) > 39 页(128 口径) ⇒ 返回 `nullopt`
⇒ 报 `text KV address space has no free active entry`。

**实现（19 文件）**
- 真源为既有 `kv_page_tokens(storage)`/`kv_page_shift(storage)`（`src/core/paged_kv_cache.h`）。
- `KVAddressSpaceStore`（`program/storage/kv_store.h`）：新增成员 `page_tokens_`，构造期由**其池几何**
  `pages.physical_pool().geometry().page_tokens` 取值（`checked_page_tokens`）；`pages_for_tokens` 与
  全部页/列算术（**~19 处**）改用 `page_tokens_`；`LogicalKVPageStore` 的
  `materialize_transfer_destination`/`commit_coverage` 列上限改用 `physical_->geometry().page_tokens`。
- `kv_pages_for_frontier(frontier, storage)`（`context_work.{h,cpp}`）、
  `kv_pages_for_tokens(tokens, storage)` / `kv_tokens_for_pages(pages, storage)`（`program_impl.h`）增
  `KvCacheStorage`；新增成员 `device_kv_tokens_per_page()`。
- **42 个调用点**传入 `kv_storage`（`context.cpp` 15、`request_plan.cpp` 14、`prefill.cpp` 4、
  `pressure.cpp` 3、`materialization.cpp` 2、`disk_tier.cpp` 2、`checkpoint_recovery.cpp` 1、
  `program_impl.cpp` 1）。
- **页跨度字面量**同步（`kPagedKVPageSize` → `device_kv_tokens_per_page()`）：`context.cpp` 7、
  `prefill.cpp` 1、`materialization.cpp` 3、`pressure.cpp` 3、`request_plan.cpp` 5、`disk_tier.cpp` 10；
  `hybrid_cache.cpp` 的 `kFullPage` 改由 text 池几何取；`resolve_host_cache_budget` 增 `storage`
  （`startup.{h,cpp}` + 2 处测试调用）。

**刻意保持 64 的站点（非共享面，逐位不变）**：外部精确尾部 ring（`decoder_state.cpp:165`、
`startup.cpp:153-156` 的 `page_count(...)` 默认参）；DFlash full geometry（`startup.cpp:273`）；ops 侧
`kv_cache_append`/`causal_softmax_attention`/`context_softmax_attention` 的 `validate_cache`
（kvarn 走自有 128-stride kernel，不经这些 helper）；`ops/kernel/paged_kv_address.cuh`。
**未改（如实记录）**：`load.cpp:192`（多 rank stage sizing；单卡不可达，且 kvarn 为单 rank ⇒
multi-stage+kvarn 不可能）；`device_calibration.cu`（按存储校准；kvarn 绕过 D256 profile）。二者仍是
字面量 64，若将来 kvarn 走这些路径需重估。

**构建**：`ninfer_ops ninfer_tests ninfer ninfer-serve ninfer-perplexity` 全部 exit 0；唯一告警
`text.cpp:1546` lower_bound 有/无符号比较属 WP3 既有代码，**非本次引入**。

**验收①（长上下文，决定性）**：命令
`ninfer-perplexity <27B产物> --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --disjoint --score-width 8 --score-topk 100 --kv-dtype kvarn:k4v2 --kv-tail-tokens 0 --kld-base .deps/kvarn-adm/bf16-t0.topk`

| 项 | bf16 base（同协议） | kvarn:k4v2 |
|---|---|---|
| 窗口 / 评分 token | 28 / 229,348 | **28 / 229,348** |
| PPL | 4.69317 | **4.72225**（+0.62%） |
| tok/s | 296.1 | 294.5（−0.5%） |
| mean KLD / median | — | **0.010513 / 0.002982**（P99 0.106971，P99.9 0.456034，max 8.070990） |
| same_top | — | 0.9624 |

⇒ **28 窗口全量跑通**（此前 window 0 即失败）；**NLL 与 base bf16 可比**（+0.6%）。

**验收②（短上下文复测）**：`--text .deps/kvarn-adm/t16k.txt --context 4096 --score-width 8`
（3,675 token，**此前必失败**）：bf16 PPL 6.04926 → kvarn:k4v2 **6.08013**，mean KLD **0.006651** /
same_top 0.9668 / ~295 tok/s。

**回归（非 kvarn）**：`ctest` 定向 **14 项全通过**——`host_kv_clamp`、`evictable_kv_pool`、
`disk_kv_store`、`disk_kv_bridge`、`kv_capacity`、`context_cache_defaults`、`prefix_cache_index`、
`qwen3_5_state_image`、`qwen3_5_context_store`、`paged_kv_window`、`kv_cache_append`、
`context_kv_materialize`（232 s）、`context_kv_materialize_legacy_routes`（230 s）、
`context_kv_materialize_unified_routes`（233 s）。**未跑全量 245 合成项**（单项可达 ~4 min；WP2 已立基线；
本次改动全部经 `kv_page_tokens==64` 分支或 kvarn 守卫 ⇒ 非 kvarn 逐位不变）。

**对计划的影响**：§7-WP3 ① 标记**完成**；§9 该风险行标**已消除**；§7-WP0.5-B 的阻塞解除 ⇒
**完整 WP0.5-B（ctx8192 / 229k token）现可执行**（k4v4/k5v5/k6v6 档仍待 WP4）。版本 v7→**v8**。

**未做（如实记录）**：② 续列尾、③ `--mtp-attention-window` 拒绝、④ MTP 路径激励**未做**；
未跑 k4v4/k5v5/k6v6（需 WP4）；未做多文本 / ≥3 重复；**未提交**（按用户约束保留工作树）。

---

## 逐字归档：附录 D-10 … D-19（2026-10-07/08；已关闭的 WP0.5-B / P2 / P3 包与 §6.3 探针）

> **搬运来源**：`kvarn-port-into-precision-tail-plan.md` 附录 D（**D-10 … D-19**）。**逐字副本，未改写一字。**
> 这些条目对应的 WP 包（WP0.5-B 正式准入 / P2b / P3b / P3c / P3d / WP6 门禁评估 / §6.3 探针）**均已关闭**；
> 其结论已由计划书 §1（A1/A2/A3/A6/A8）、§6.1–6.3、§7 各已完成行、§9 与附录 **D-20 … D-24** 承接，故不必留在正文。
> 正文附录 D 保留 **D-1…D-9 索引**与 **D-20 … D-24**（WP6.0–WP6.3，未完成的 WP6.4–WP6.7 的直接前驱）。

### D-10 WP0.5-B 补测完成（229k 同字节矩阵）（2026-10-07）（**回写 §7-WP0.5-B / §1 A8 / §9；版本 v8→v9**）

**触发**：用户指示「判断下一步」并确认测试须能跑进本机 16 GB 显存；独显空闲。

**① 缺口（一手核验 `.deps/kvarn-adm/` 全部产物）**：D-9 后 229k 矩阵仍不全 —— 已有 `bf16` base、
`rk4v4+tail1024`、`kvarn:k4v2`；**缺失** `rk2v4-e8`（216 B，k4v2 的唯一同字节对手；14:41 启动后未收口）、
`rk4v4-t0`（`exit=1` 无结果）、`nvfp4-t0`/`k8v4-t0`（13:37 因 MSYS 路径改写 `CreateFileW Win32 error 3`
**从未运行**）。⇒ `kvarn:k4v2` 的 0.010513 **无同字节基准可解释**（即 D-9「不足以对质量下结论」的具体缺口）。

**② 显存可容纳性（用户要求，先证后跑）**：本机 16,303 MiB；`report.json` 的 `/memory` 给预算 ——
最大足迹是 `bf16`（kv_payload 512 MiB、runtime_reservation 2.09 GiB），而 **`bf16` 本身已在同一 229k
规模跑通** ⇒ 四臂上界（权重 9.39 GiB + ≤2.09 GiB）≈ 11.8 GiB ≪ 15.9 GiB；且 perplexity 走 **default
`--cuda-memory-policy`**（按 CUDA 报告的空闲显存规划，留 1024 MiB headroom）属**自适应**。
**实测证实**：全程峰值 **11,695 MiB**（`k8v4` 臂），**OOM 未发生**。

**③ 实现与执行**：`run4.sh`（复用 `run2.sh` 已验证调用面），15:01→16:52（**51 min**，四档 297–301 tok/s），
四臂全部 `exit=0`，GPU 复原 48 MiB / 0%。协议统一
`--corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --disjoint --score-width 8
--score-topk 100 --kld-base .deps/kvarn-adm/bf16-t0.topk`。

**④ 完整矩阵（229,348 评分 token / 28 窗口）**

| 档 | B/tok/头 | PPL | mean KLD | median | same_top | dlogp | tok/s |
|---|---:|---:|---:|---:|---:|---:|---:|
| `bf16`（base） | 1024 | 4.693174 | — | — | — | — | 296.1 |
| **`kvarn:k4v2`** | **210** | 4.722248 | **0.010513** | 0.002982 | 0.9624 | −0.006176 | 294.5 |
| **`rk2v4-e8`**（同字节对手） | **216** | 4.83714 | **0.043619** | 0.011210 | 0.9229 | −0.030215 | 299.0 |
| `rk4v4` t0 | 280 | 4.70427 | 0.004426 | 0.001736 | 0.9727 | −0.002362 | 300.7 |
| `rk4v4` +tail1024 | 280 | 4.695007 | 0.001724 | 0.000625 | 0.9833 | −0.000391 | 81.9 |
| `nvfp4` t0 | 288 | 4.69752 | 0.004385 | 0.001650 | 0.9726 | −0.000926 | 299.4 |
| **`k8v4` t0**（`k6v6` 同字节对手） | **402** | 4.6936 | **0.002688** | 0.001130 | 0.9785 | −0.000091 | 296.7 |

**逐臂 `/memory`（ctx8192 / 4 streams；权重恒 9.39 GiB）**：kv_payload / runtime_reservation =
`bf16` 512.0 MiB / 2.09 GiB；`kvarn` 129.0 / 1.72；`rk2v4-e8` 108.0 / 1.70；`rk4v4`-t0 140.0 / 1.73；
`rk4v4`+tail1024 208.0 / 1.80；`nvfp4` 144.0 / 1.73；`k8v4` 201.0 / 1.79。

**⑤ 判定**
1. **代理门禁为正（决定性）**：同字节下 `kvarn:k4v2` 比 `rk2v4-e8` 好 **4.15×**（same_top +3.95 pt），
   与 1438-token 单窗口的 **5.7×** 同向同量级 ⇒ D-9 的缺口补上并给出正面结论。
2. **优势是「每字节」非绝对质量**：`rk4v4`(0.004426)/`nvfp4`(0.004385) 绝对 KLD 仍更优，kvarn 少用
   **25–27%** 字节接近之。
3. **`k6v6` 门槛量化 = mean KLD < 0.002688**（按 A8「不优于 `k8v4` 则移除该档」）；`k8v4` PPL 4.6936
   ≈ bf16 4.6932 且 dlogp −0.000091 ⇒ 该门槛**很高**。
4. **尾部规模稳定性得证**：`rk4v4-t0`(0.004426) vs `rk4v4+tail1024`(0.001724) ⇒ 229k 上 **2.57×**
   （合 A4 记录的 2.26–2.47× 带）⇒ 与 WP6 的协同论证相关。
5. **无 KVarN 特有速度代价**：四档 297–301 tok/s ≈ `bf16` 296.1（`kvarn:k4v2` 294.5，−0.5%）；
   `rk4v4+tail1024` 的 81.9 tok/s 是**尾路径**代价，非 kvarn。

**裁决：投 WP4（go）；WP6 不启动**（须先有 k4v4/k5v5/k6v6 的正式 WP0.5-B）。

**未做（如实记录）**：未跑 k4v4/k5v5/k6v6（需 WP4）；各臂**单次**测量（未做 ≥3 重复）；未做多文本/跨域；
未做 ≥1 组 MTP；**未改任何源码；未提交**（按用户约束保留工作树）。
**产物**：`.deps/kvarn-adm/` 内 `run4.sh`、`run4.log`、`{rk2v4e8-t0,rk4v4-t0,nvfp4-t0,k8v4-t0}.log`。

---

### D-11 正式 WP0.5-B：发布档 KVarN 三档准入（2026-10-07）（**回写 §1 A8 / §5 / §7-WP4；版本 v9→v10**）

**触发**：WP4.2–4.4 完成（进度记录 07-20/07-21）⇒ 按 §7-WP4 的回退条款「用发布档对
`rk4v4`/`nvfp4`/`k8v4` 跑正式 WP0.5-B」。

**① 命令与协议**（可复现；`.deps/kvarn-adm/run5.sh` + `run5.log`）
```
bash .deps/kvarn-adm/run5.sh          # MSYS_NO_PATHCONV=1
# 协议（与 D-10 逐字相同，复用同一条 bf16 base topk）：
#   --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192
#   --disjoint --score-width 8 --score-topk 100 --kld-base .deps/kvarn-adm/bf16-t0.topk
# 先 smoke（t16k / ctx4096，三档各一次，exit=0），再跑三臂 229k：
#   k4v4-t0 / k5v5-t0 / k6v6-t0 = --kv-dtype kvarn:k4v4|k5v5|k6v6 --kv-tail-tokens 0
```
时间 17:58→18:39（**smoke 1 min + 三臂 13 min 各**）；三臂 **exit=0**；GPU 中途采样 11,687 MiB 且
**无 OOM**；device 在跑前/跑后均为空闲（48 MiB）。

**② 三档实测（229,348 评分 token / 28 窗口）**

| 档 | B/tok/头 | PPL | mean KLD | median | P99 | max | same_top | dlogp | tok/s |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `bf16`（base） | 1024 | 4.693174 | — | — | — | — | — | — | 296.1 |
| **`kvarn:k4v4`** | **274** | 4.69436 | **0.002120** | 0.000747 | 0.017190 | 7.175765 | 0.9817 | −0.000252 | 294.5 |
| **`kvarn:k5v5`** | **338** | 4.69405 | **0.001432** | 0.000531 | 0.010355 | 7.599654 | 0.9846 | −0.000187 | 291.8 |
| **`kvarn:k6v6`** | **402** | 4.69352 | **0.001233** | 0.000454 | 0.007896 | 8.354687 | 0.9858 | −0.000073 | 287.7 |
| `rk4v4` t0（锚，D-10） | 280 | 4.70427 | 0.004426 | 0.001736 | 0.037014 | 7.184616 | 0.9727 | −0.002362 | 300.7 |
| `nvfp4` t0（锚，D-10） | 288 | 4.69752 | 0.004385 | 0.001650 | 0.038020 | 10.253651 | 0.9726 | −0.000926 | 299.4 |
| `k8v4` t0（锚，D-10） | 402 | 4.6936 | 0.002688 | 0.001130 | 0.019823 | 14.925574 | 0.9785 | −0.000091 | 296.7 |

**逐档显存**（`report.json`，ctx8192 / 4 streams；权重恒 9.39 GiB）：`kv_payload` =
`k4v4` **161.0 MiB** / `k5v5` **193.0 MiB** / `k6v6` **225.0 MiB**；`runtime_reservation` =
1.75 / 1.78 / 1.81 GiB。**差值恰为每 +64 B/token/头 加 32.0 MiB**，且由 `k4v4` 反推 210 B 档得
129.0 MiB —— **与 D-10 实测的 `kvarn:k4v2` 129.0 MiB 逐位相符** ⇒ 平面几何参数化（记录字节 →
池字节）**被显存核算独立验证**。

**③ smoke（t16k / ctx4096 / 3,675 评分 token，佐证位宽确实贯通到端到端）**

| 档 | mean KLD | median | same_top | tok/s |
|---|---:|---:|---:|---:|
| `k4v4` | 0.001480 | 0.000875 | 0.9796 | 295.8 |
| `k5v5` | 0.001140 | 0.000690 | 0.9850 | 294.3 |
| `k6v6` | 0.001072 | 0.000618 | 0.9867 | 293.8 |

KLD/median 随位宽**单调下降**、`same_top` **单调上升** ⇒ `EngineOptions.kvarn_bits` 确实选到
**三套不同编解码**（若位宽未贯通，三档会给出同一组数字）。

**④ 逐档裁决（§7-WP4/A8 的判据）**

1. **`k4v4` 保留**：mean KLD **0.002120** vs 同档对手 `rk4v4` **0.004426** ⇒ **2.09×** 更好，
   vs `nvfp4` **0.004385** ⇒ **2.07×**，且**字节更少**（274 vs 280/288）⇒ A8 的「不优于
   `{rk4v4,nvfp4}` 则移除」**未触发**。
2. **`k5v5` 保留**：无同字节档（§1 A8 已声明判据另定）。实测 0.001432 落在 `k4v4`→`k6v6` 的
   单调曲线上：相对 `k4v4` 多 **23%** 字节换 **−32%** KLD，相对 `k6v6` 少 **16%** 字节付 **+16%** KLD
   ⇒ 作为中间档自洽。
3. **`k6v6` 保留**：mean KLD **0.001233** vs 逐字节相同的对手 `k8v4`（402 B）**0.002688** ⇒ **2.18×**，
   **门槛（< 0.002688）达成**。
4. ⇒ **三档全部达标，不回退**（§7-WP4 的回退条款「某档不达标则先只发 k4v4」未触发）。

**⑤ 对既有结论的修正（v9 的两个说法被实测取代）**

1. **D-10 判定 2「优势是「每字节」非绝对质量」——已被发布档取代（保留原文以存史）**：`k4v4`
   的 **0.002120** 在**绝对** mean KLD 上同时优于 `rk4v4`(0.004426)/`nvfp4`(0.004385)（280/288 B）
   与 `k8v4`(0.002688)（402 B）。⇒ KVarN 的价值不再是「用更少字节接近对手」，而是
   **在更少字节下质量也更好**。（D-10 那句是 `k4v2` 代理档的实测结论，代理档不发布。）
2. **D-10 判定 5「无 KVarN 特有速度代价」——收窄**：`k4v4` −0.5% 与 D-10 一致；但位宽升高有
   **真实且单调的代价**：`k5v5` −1.5%、`k6v6` **−2.8%**（vs `bf16` 296.1 tok/s）。归因：记录更大
   ⇒ decode staging 字节更多（`k6v6` 每记录 51,456 B vs `k4v4` 35,072 B）。**A8「不得劣于同档噪声底」
   对 `k6v6` 应据此读**（它没有同字节的**同族**对手，`k8v4` 的 296.7 是不同实现的 402 B 档）。
3. **PPL**：`k4v4` 4.69436（+0.025% vs bf16）、`k5v5` 4.69405（+0.019%）、`k6v6` 4.69352（+0.007%）。

**⑥ 未做 / 残留（如实记录）**：① 各臂**单次**测量（未做 ≥3 重复，同 D-10 口径）；② 未做多文本/跨域；
③ 未做 MTP 路径（WP3④）；④ 未做 `kvarn + tail`（规划期已 fail-fast，§10-D5）；⑤ 三档在报告目录里
**同名 `kvarn/`**、仅时间戳区分 ⇒ **进度 §0 未决项 7 仍开放**（本次实测使其可见）；
**→ ✅ 2026-10-08 由 P3b 关闭**（三档现为 `kvarn-k4v4|k5v5|k6v6`，见附录 D-14；本行原文保留以存史）；
⑥ **未提交**（按用户约束保留工作树）。
**产物**：`.deps/kvarn-adm/` 内 `run5.sh`、`run5.log`、`smoke-{k4v4,k5v5,k6v6}.log`、
`{k4v4-t0,k5v5-t0,k6v6-t0}.log`、`relbf16-base.log`、`relbf16.topk`。

### D-12 GPU 收尾四件 + P2 前置（2026-10-08）（**回写 §1 A2 / §7-WP3 / §7-WP5；版本 v11→v12**）

**① P3a 3× 重复收尾（收口 D-11 单次口径）**：`run6.sh` 九臂（`kvarn:k4v4|k5v5|k6v6` × r1–r3）
**全部 `exit=0`**、GPU 复原 0 MiB。

| 档 | mean KLD（r1/r2/r3） | median | P99 | max | same_top | dlogp | PPL | tok/s（r1/r2/r3） |
|---|---|---|---|---|---|---|---|---|
| `k4v4` | 0.002120 ×3 | 0.000747 | 0.017190 | 7.175765 | 0.9817 | −0.000252 | 4.69436 ×3 | 289.1 / 291.9 / 292.8 |
| `k5v5` | 0.001432 ×3 | 0.000531 | 0.010355 | 7.599654 | 0.9846 | −0.000187 | 4.69405 ×3 | 290.6 / 287.5 / 289.5 |
| `k6v6` | 0.001233 ×3 | 0.000454 | 0.007896 | 8.354687 | 0.9858 | −0.000073 | 4.69352 ×3 | 289.3 / 289.9 / 289.5 |

**质量指标（mean/median/P99/P99.9/max/same_top/dlogp/PPL）在三重复间逐位相同（极差 0）**，
且与 D-11 单次值**逐位相同** ⇒ **KLD 测量完全确定性；D-11 的 A8 三档判定被 3× 重复证实**。
唯一散布在吞吐（均值 291.27±3.7 / 289.20±3.1 / 289.57±0.6 tok/s，极差 ≤1.3%）。

**② P1 e2e 验收 PASS（WP3②）**：`bash .deps/kvarn-adm/p1_prefix_reuse.sh`（先修 harness 的 `set -u`
下 `local tag=$1 … log=…$tag…` 同句引用未赋值变量的 bug）。kvarn r2 `usage.prompt_tokens_details.cached_tokens
= **851**`（99.4%，`turn closure`）、`choices[0].message` 与 r1 **逐字节相同**；bf16 对照 `cached 851` 且 r1==r2；
旁证 TTFT 721→**37.9 ms**、serve log `capacity | KV 4,096 tokens, kvarn, explicit`。

**③ WP5 余量实测**：`flips` 最大 **1/65**、`over_step=0`、`wide_flips=0`、`exit=0`（详见 §7-WP5）。

**④ 续列尾 device 段：首跑 FAIL(2) → 根因 = 测试自身尺寸 bug → 修正后绿**：
`test_kvarn_continuation_image.cpp` 的 `marker_pattern` 写成 **3 字节**，而 `logical_pages` 是
`I32{kKvarnTailSlots=3}` = **12 字节** ⇒ 越界读 + 「12B vs 3B」vector 比较恒 false。测试自身第 341 行
已断言 `logical_pages.bytes()==marker`(=12) 且该断言通过 ⇒ 12B 正确、3B 是 bug。改为按 `marker` 尺寸
程序化生成后 `OK`（判据不变）。**实现无缺陷**（K/V/markers 全逐字节通过）。

**P2 前置（WP3④）：kvarn MTP provisional 路径首次激励通过**（`.deps/kvarn-adm/p2_mtp_gen.sh`）
——`kv{:kvarn:k4v4|bf16|rk4v4}` × `spec{off, mtp k=1}`，各 2 次 greedy：

| 档 | MTP-off | MTP-on decode | accepted | self-det(off/on) | MTP-on vs off |
|---|---|---|---|---|---|
| `kvarn:k4v4` | 67.8 | **101.5/101.6** | **84/113 (74.3%)** | none / none | **none** |
| `bf16` | 68.3 | 104.1/104.3 | 87/111 (78.4%) | none / none | **none** |
| `rk4v4` | 68.5 | 97.6 | 79/119 (66.4%) | none / none | **none** |

⇒ kvarn MTP 路径端到端跑通且接受草稿；自确定性成立；本样本 MTP-on vs off 逐字节同；加速比
kvarn **1.50×** ≈ bf16 1.52× ⇒ **同量级**。**边界**：message 文本级、单 prompt、200 token，不能替代
token 级首分叉率（P2b，未做）。

**构建**：`ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0**。
**未做（如实记录）**：P2b 仪器改造（parity 测试 → 诊断仪器 + kvarn 档并入 `kKvProfiles` + `kvarn_bits` 贯通）；
`k5v5/k6v6` 的 MTP；MTP `k>1`；**未提交**。
**产物**：`.deps/kvarn-adm/`（gitignored）内 `p1_prefix_reuse.sh`(修)、`p2_mtp_gen.sh`(新)、`p1-*.{log,json}`、
`p2-*.{log,json,txt}`、`run6.log`。

### D-13 P2b：MTP parity 测试改造为诊断仪器 + token 级 kvarn MTP 实测（2026-10-08）（**回写 §7-WP3④⑤ / §1-A3；版本 v12→v13**）

**改造（`tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`）**
1. **删** `verify_result`（跨配置 token 逐位相等即 `throw`）；换 `require_output_limit`（仅保留
   `FinishReason::OutputLimit` 结构契约）+ `divergence(left,right)→{first,count}`。
2. **加自确定性门禁**（A3-O1 的①，真正 gate）：每宽度 `repeat 0` 记录本配置输出，`repeat 1` 必须**逐字节复现**，
   否则 throw `"... is not self-deterministic: first_diff=… diverged=…"`。（原实现只在 `depth==0||dflash2` 写
   `expected` 并与 greedy 比 ⇒ 既非自确定性、又是跨配置门禁。）
3. **跨配置降级为报告**：`self_det=n/a|ok vs_greedy=identical|first_diff=N diverged=C/total`；tool-loop 的
   adaptive-vs-off 断言同降级。
4. **并入档位**：`kKProfiles` 增 `rk4v4`、`kvarn:k4v4|k5v5|k6v6`（`KvProfile` 增 `KvarnBits` 字段）；
   `engine_options` 增 `kvarn_bits` 形参写 `options.kvarn_bits`（**此前恒为默认 Bits4 ⇒ kvarn 档静默错标**）。
5. **`--quick`**（用户裁定 ctest 收窄）：代表性档 `{bf16, rk4v4, kvarn:k4v4}` × `sample 0` × 宽度 `{0,3}`；
   `tests.cmake` 注册 `TEST_ARGS --quick`（全扫无 `--quick` 仍可手动跑）。

**token 级实测**（27B 产物、`sample 0`、`prompt 68`、512 输出、C=1、`--greedy`；全部 `exit=0`）

| 档 | k=1 `vs_greedy` | k=3 `vs_greedy` | k=15 `vs_greedy` | `self_det` |
|---|---|---|---|---|
| `bf16` | **first_diff=91, 413/512** | identical | **first_diff=91, 412/512** | ok |
| `rk4v4` | identical | **first_diff=97, 408/512** | 未跑 | ok |
| **`kvarn:k4v4`** | **identical** | **identical** | **first_diff=91, 418/512** | ok |
| `kvarn:k5v5` / `k6v6` | **identical** | 未跑 | 未跑 | ok |

`ctest -R ninfer_qwen3_5_mtp_greedy_parity_real_test`（`--quick`）**Passed 130.68 s**（全扫 ≈1.5 h，仅手动）。

**裁决（A3-O1 判据）**：①**自确定性成立**（全档 `self_det=ok`，含宽度 0/1/3/15）；②**分叉是 (档,宽度) 特异的
near-tie 翻转**（bf16@k1 分叉而 @k3 不分叉；rk4v4 反之；kvarn k=1/3 不分叉、k=15 分叉）；③**kvarn 不劣于基线**
—— k=1/k=3 identical（更好），k=15 `418/512` vs bf16 `412/512`（**首分叉同为 token 91**，差 6 token = 1.2% 相对）。
⇒ **不主张 kvarn 严格更优**；`small_t` 宽度特化机制的决定性实验（`case 2 → <2,2>`）**未做**，故机制仍属
「代码结构上最合理的解释」（同 D-6 边界）。

**边界**：仅 `sample 0`、宽度 {0,1,3,15}（`k5v5/k6v6` 只测 k=1）；"identical" 限本样本；测试名仍含 `parity`
（语义已变，**未改名**，记为未决项）。
**产物**：`tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（改造）、
`tests/models/qwen3_5/tests.cmake`（`TEST_ARGS --quick`）。

### D-14 P3b：kvarn 档位并入报告目录 / 日志名 + `MemorySummary`（2026-10-08）（**回写 §7-WP2 / §7-WP7 / 关闭 D-11⑤「三档同名」；版本 v13→v14**）

**目标**（进度 §0 未决项 7 / §4.1-P3b）：三档 kvarn 在报告目录、日志、报告 JSON、CLI 摘要里**同名 `kvarn`**
⇒ 只能靠身份指纹 `;kvbn=` 区分。本次把级别并入名字。

**改动（7 文件）**：`types.h` 的 `MemorySummary` 增 `KvarnBits kvarn_bits`（默认 `Bits4`）；
`program_impl.{h,cpp}` 存成员并写入 `memory_summary()`；四个展示面各带 `KvarnBits` 渲染级别
——`apps/perplexity/main.cpp:kv_name`、`apps/cli/main.cpp:format_kv_cache`、
`serve/{request_log,operational_log}.cpp:kv_cache_name`。

**写法（用户裁定）**：**文本面用冒号 `kvarn:k4v4`**（与 `--kv-dtype` 逐字一致，便于按 flag 值 grep）；
**报告目录分量用连字符 `kvarn-k4v4`**（Windows 路径分量禁止 `:`；由该站点既有的 `safe_component` 转换）。
非 kvarn 档的名字逐字不变。

**实测（27B 产物，GPU；`ninfer-perplexity --text … --context 512 --stride 256`, 04:16–04:17）**
1. 报告目录叶：`kvarn-k4v4` / `kvarn-k5v5` / `kvarn-k6v6`（三档各自独立，替代原同名 `kvarn/`）；
2. `report.json` 的 `execution.kv_dtype`：`kvarn:k4v4` / `kvarn:k5v5` / `kvarn:k6v6`；
3. perplexity stdout：`kv: kvarn:k4v4|k5v5|k6v6`；
4. `ninfer --prompt … --kv-dtype kvarn:k5v5` 摘要：`kv cache dtype  kvarn:k5v5`；
5. `ninfer-serve` operational log：`capacity | KV 512 tokens, kvarn:k5v5, explicit`；
6. request-log JSONL：`"kv_cache":"kvarn:k5v5"`。⇒ **六个展示面全部带级别**。
**host 回归**：`ninfer_{request_log,load_report,bench_support,cli_options,serve_options,kv_capacity}_test`
**全过（0 失败）**；`ninfer_tests ninfer ninfer-serve ninfer-perplexity` 构建 **exit 0 / 0 error**，
本轮改动面 **0 新告警**（既有 nvcc `#128-D` / MSVC `C4244` 计数不变，均在改动面外）。

**范围外（如实记录）**：`bench/inference/ninfer_bench_support.cpp:951 kv_cache_name` **未改** —— bench 的
`parse_kv_cache` 不接受 kvarn（上限 `k8v4`）、`BenchOptions` 无 `kvarn_bits`，且
`NINFER_BUILD_BENCHMARKS=OFF`（该函数仅在单独的 `ninfer_bench_support_test` 中编译，无 kvarn 用例）
⇒ bench 无 kvarn 档可标级别，属未决项 3 的范畴。

**顺带发现（潜在、非可达；未改）**：`ProgramImpl::capture_identity_tag()`（`program_impl.h:788-792`）
**不含 `kvarn_bits`**，而该 tag 写进**持久化磁盘**缓存的身份 `DiskKVIdentity.tag`（`storage/disk_tier.cpp:113`），
理论上 k4v4 与 k5v5 的磁盘检查点可同 tag。**实测该隐患被目录名兜住**：磁盘档目录
`disk_profile_directory`（`disk_tier.cpp:37-42`）把 bits 相关的 `main_stride`（KV 页 stride = 128·记录字节）
编进目录名 ⇒ 三档目录本就不同；且同进程内 bits 恒定。**未改 tag**：改它会让既有磁盘缓存全部失效（身份语义变更），
需显式裁决 ⇒ 并入进度 §5 待裁决项。**该发现由 codebase-memory 图谱（`trace_path` inbound）给出，grep 未覆盖。**

**harness bug（记录，非产品缺陷）**：`--text` 与 `--context` **小于默认 stride** 时必须显式 `--stride`，
否则 CLI 直接拒绝（`context/stride must satisfy context>=2 and 1<=stride<context`）；首跑因此三档皆 exit=1。
**产物**：`.deps/kvarn-adm/{p3b_names.sh,p3b_perp.sh,p3b_text.txt,p3b-*.log}`（gitignored）、
`profiles/perplexity/**/kvarn-kXvX/**`（gitignored；`profiles/` 本就在 .gitignore）。

### D-15 P3c：kvarn parser 单测（2026-10-08）（**回写 §7-WP2；关闭 §7-WP2 的最后残项；版本 v14→v15**）

**目标**（进度 §0 未决项 7 残余 / §4.1-P3c）：WP2 落地的三处 `--kv-dtype` parser 中，cli/serve 两处**无单元测试**。

**改动（2 测试文件，+63 行；`git diff --stat` = `test_cli_options.cpp` +32 / `test_serve_options.cpp` +31，无源码改动）**
- `tests/test_cli_options.cpp`：裸 `kvarn` 与 `kvarn:k4v4` ≡ `KvarnGroup128` + `KvarnBits::Bits4`；`kvarn:k5v5|k6v6`
  → `Bits5|Bits6`；默认 `kvarn_bits == Bits4`；拒绝 `kvarn:k4v2`/`k3v3`/`k4v5`/`k7v7`；help 含 `kvarn:k4v4|k5v5|k6v6`。
- `tests/test_serve_options.cpp`：同上 + **`make_engine_options()` 贯通**（`kvarn:k5v5` → `EngineOptions.kv_cache ==
  KvarnGroup128` 且 `kvarn_bits == Bits5`；映射源 `generation_service.cpp:263-264`）+ help 含三档。

**实测**：按目标构建 `ninfer_tests`（8 步）**`exit=0` / 0 warning**；host-only（`CUDA_VISIBLE_DEVICES=99`）运行
`ninfer_cli_options_test` 与 `ninfer_serve_options_test` 均 **exit 0**（`ninfer_tests.exe` 为 dispatch runner，
`build-port/tests/ninfer_tests_dispatch.cpp:380-384` 按名字转发、未知名字 `exit 2` ⇒ 非 no-op）。

**未做（如实）**：**第三处 parser `apps/perplexity/main.cpp:197-229` 仍未加单测** —— 其 `--kv-dtype` 是 main.cpp
内联的 `else if` 链、`Options` 亦为 main.cpp 局部（`:69-89`），既有 `ninfer_perplexity_evaluation_test` 只编译
`apps/perplexity/evaluation.cpp`（`tests/cmake/ProductTests.cmake:18-24`）⇒ **不重构出可测函数即不可单测**；
未新增抽象（超出 P3c 范围）。其**行为覆盖**由 P3b 的 GPU e2e 提供（D-14 的三面实测路径经此 parser）。
**纯测试改动，A1–A8 判据不变**；未跑全量 ctest（仅改两个测试文件）。
**产物**：`tests/test_cli_options.cpp`、`tests/test_serve_options.cpp`；`/tmp/p3c_tests.log`。

### D-16 P3d：审计 3 项 LOW 修复（2026-10-08）（**回写 §7-WP2 邻域 / §9 风险登记；版本 v15→v16**）

**目标**（进度 §5 待裁决项 (a)）：审计发现的 3 项 LOW —— `reset_kvarn_tail_row` 缺口 / capture-activate 抛错被静默 / 注释未提 KVarN。

**调查（2 个只读子代理 + 复核）——问题定性被改写**
- **① 的真实缺口在 CausalScore，而非「函数是死代码」**：执行行 = lane 且会被回收（`context.cpp:103-107` + `commit.cpp:85-88,116-120`）。
  **生成路径安全**（每次 start 由 `activate_sequence_kvarn_tail` 整行覆盖标记，`context.cpp:1823-1825`）；但 **CausalScore 不经
  `start_sequence`**，却 `create_active(entitlement, 0, …)` **硬编码 Main row 0**（`program_impl.cpp:659`，`bound_row == 0` 断言 `:663-665`），
  跨多次打分复用 row 0 而不恢复镜像；尾槽标记**运行期只被追加**（`attention.cu:75,91-96`）⇒ 上一占用者的标记会被当作本序列的读入。
- **② 的准确范围**：只有 **commit** 侧静默（`commit.cpp:674` 的 `catch(...) { return false; }`，函数 `noexcept`）；capture 侧是
  `abort + throw`（`capture.cpp:1302-1308`）、activate 侧在 `start_sequence` 内无 try/catch（直接上抛）。修法取仓内先例
  `report_capture_release_drift`（`capture.cpp:601-612`）= stderr 诊断 + 返回 false；否决「去 `noexcept`」（会把规划不一致升级为引擎级失败）。

**改动（3 源码 + 1 新测试 + 1 cmake；均已提交）**
1. `program_impl.cpp`（`causal_score`）：`bound_row` 断言后加 `decoder->text_kv.reset_kvarn_tail_row(0, compute_streams[0]);`
   （该函数对非 KVarN 自为 no-op，`decoder_state.h:133-134`）。
2. `commit.cpp`：`publish_active_continuation` 的 `catch(...)` 前插 `catch (const std::logic_error&)` + stderr 诊断；补 `#include <cstdio>`。
3. `state_image.h`：`StateImagePart` / `StateImageDevicePool` 注释补 KVarN sink/tail（含 MTP 池）。
4. 新测试 `tests/models/qwen3_5/test_kvarn_tail_row_reset.cpp` + `tests/models/qwen3_5/tests.cmake` 注册。

**实测**：构建 `ninfer_tests ninfer` exit 0/234 步、`ninfer-serve ninfer-perplexity` exit 0/7 步；新测试 **PASS**（
`OK kvarn tail row reset`）；host 4 项 exit 0；`ctest` 注册 **262 → 263**。
**未跑（用户指示）**：① 的端到端对照 —— 改动后跑一个 `kvarn:k4v4` KLD 臂并与 **P3a 的 `0.002120`** 比对（该实验已启动即被叫停）。
**未做**：移植 FORK 的 `test_prefill_precision_real.cpp`（改为 TAIL 定点单测，理由：该 FORK 测试练的是**已受保护**的 prefill 路径）。
**产物**：上述 5 文件；`/tmp/fix_build{1,2}.log`、`/tmp/host_*.log`。
---

### D-17 P3d ① 的端到端对照 + 待裁决 3 项裁决 +「子代理无图谱」根因（2026-10-08）（**回写 §9 风险登记 / §7-WP2 邻域；版本 v16→v17**）

**触发**：用户指示从进度 §5 待办 1 起步（最优先、原需 GPU）；GPU 实测空闲（0 MiB）。

**① 命令与协议**（脚本 `.deps/kvarn-adm/verify_p3d_kld.sh`；日志 `p3d-verify-k4v4-t0.log`）
```
ninfer-perplexity <27B 产物> --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 \
  --disjoint --score-width 8 --score-topk 100 --kv-dtype kvarn:k4v4 --kv-tail-tokens 0 \
  --kld-base .deps/kvarn-adm/bf16-t0.topk
# 协议与 run6.sh / D-10 / D-11 逐字相同；13m02.5s（05:18:09→05:31:18）；exit=0；GPU 中途 11,632 MiB
```
**二进制含本修复**：`ninfer-perplexity.exe` mtime 05:10:29 **晚于** `program_impl.cpp` 05:08:51，且对象已重编；修复点在 `program_impl.cpp:669`。

**② 实测（229,348 评分 token / 28 窗口）—— 与 D-11/P3a 全部质量指标逐位相同**

| 指标 | 修复后（08-11） | D-11 / P3a（修复前） |
|---|---|---|
| mean KLD | **0.002120**（0.00212012） | 0.002120 |
| median | 0.000747 | 0.000747 |
| P99 / P99.9 | 0.017190 / 0.076138 | 0.017190 /（未列） |
| max | 7.175765 | 7.175765 |
| same_top | 0.9817 | 0.9817 |
| mean_target_dlogp | −0.000252 | −0.000252 |
| PPL（overall） | 4.694357 | 4.69436 |
| tok/s | 293.1 | 294.5（D-11）/ 289.1–292.8（P3a r1–r3） |

**③ 判定**
1. ① 的唯一可能后果（打分路由读入上一占用者的陈旧尾标记）在该协议上**未发生** ⇒ **无害 no-op**；**D-11 / P3a 数值无需重测**；§4.1-P3d「缺执行证据」关闭。
2. **不利面（必须同报）**：本实验**只证「无害」，未证「必要」**。机制：`claim_tail_slot`（`attention.cu:100-118`）先按「标记 == 本页号」命中即复用、否则只 CAS `0xff` 槽；`tail_slot`（`:84-89`）只给 sink/首/末页分槽。本协议窗口恒 `context` 长（`apps/perplexity/evaluation.cpp:50-58`）且页号每窗口从 0 重启 ⇒ 标记**重合**。而 `Engine::score_tokens`（`engine.cpp:363`）接受**任意**窗口长度 ⇒ 变长窗口下陈旧页号可**冲突**（跳过 staging），其数值后果**未实测**。
3. 本改动按**防御性正确性修复**保留（不清除标记的代价在变长窗口下是静默错答）。

**④ 待裁决 3 项 —— 用户裁决**：(a) **不移植** FORK 的 `test_prefill_precision_real.cpp`（真实缺口在打分路由，已修 + 定点单测；FORK 该测试练的 prefill 路径本已受保护）；(b) **不补** `capture_identity_tag` 的 `kvarn_bits`（潜在、不可达；补则作废既有磁盘缓存）；(c) **kvarn bench 暂不纳入**（维持不重配）。

**⑤ P3d 后 GPU 回归 3 项全绿**：`ninfer_kvarn_test`（WP5 余量与 08-04 逐位同：K bits4/5 `flips=1/65`、bits6 `0`；V 全 `0`；`over_step=0`、`wide_flips=0`；另 `codec bit-order roundtrip: 4/5/6-bit exhaustive OK`）、`ninfer_qwen3_5_kvarn_continuation_image_test`、`ninfer_qwen3_5_score_real_test`（**注**：后者用 `Fp8E4M3Row256`，非 KVarN ⇒ **不覆盖**本改动路由，仅作健全性检查；**KVarN + 打分路由暂无直接门禁测试**）。

**⑥ 仓外工具发现（不属本仓交付面，但影响本计划的验证纪律）**：`~/.qoder/agents/` 下 4 份 `codebase-memory*` 代理定义的 `tools:`/`mcpServers:` 用**连字符**（`mcp__codebase-memory-mcp__*`），而运行时工具名归一为**下划线**（`mcp__codebase_memory_mcp__*`；主会话对连字符名报 `not found, did you mean <下划线>`）⇒ 子代理只拿到 `Read/Grep/Glob`，**图谱调用恒为 0**（**上一会话**两个 `codebase-memory` 子代理实测 42/64 次调用全为 Read/Grep）⇒ §3-08-03 的 P1 审计「两树均无调用者」是**无图谱下**的结论。**已修**（4 份定义改下划线，备份 `/tmp/agents-backup-20261008/`），但 `[AgentListingDelta] isInitial=true` **只在会话启动装载一次** ⇒ **须重启 Qoder**；重启是否**充分**未排除（探针复查：30 秒）。
**图谱刷新**：`index_repository(mode=full)` ⇒ **45,944 节点 / 216,668 边**；`reset_kvarn_tail_row` inbound `callers_total=4`（**1 个生产调用点** `causal_score` + 3 个测试 hop）——刷新前同一查询为 `0`，属索引陈旧（04:35 < P3d 提交 05:12）。

---

### D-18 WP6 门禁评估：**「足够证据可开工」不成立**（2026-10-08）（**回写 §6.3 / §7-WP6；版本 v17→v18**）

**触发**：用户询问「判断一下是否已有足够证据证明可以开始 WP6」。

**① 已满足的条件**
- **功能门「P1/P2 完成」**（§7 原文门禁）✅；P1–P4 全完成（D-12/D-14/D-15 + 本轮 D-17）。
- **准入证据齐**：WP0.5-B 三档全过（D-11），P3a 三重复极差为 0。
- **安全回退已落地**：`kvarn:* + --kv-tail-tokens>0` 规划期 **fail-fast 拒绝**（§6.4 要求；07-21 实现）⇒ **当前交付状态即回退态，无静默惰性风险**。
- **工具链就绪**：子代理图谱已修复并验证（4 份 `~/.qoder/agents/codebase-memory*.md` 的 MCP 名连字符→下划线；**重启后探针通过**）。`list_projects` 可见 **6 个图谱工程**：`D-ninfer-KVarN`（上游华为/vLLM）、`D-ninfer-beellama.cpp`、`D-ninfer-ninfer`（上游）、**`D-ninfer-ninfer-rtx5090-mobile`（FORK）**、`D-ninfer-ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco`、本仓 ⇒ (a)/(b) 取证可**跨工程**做（`codebase-memory-port-auditor` 含 `compare_graphs`）。

**② 未满足的条件（真正缺口）**
1. **§6.3 的 (a)/(b)/(c) 低成本探针从未执行。** 计划书两处把它定为 **WP4 的前置**（§6.3 末句「在 WP4 之前做一次…而不是把门禁拖到 WP6 才暴露」；§7-WP4「前置：先做 §6.3 的低成本探针（判 (a)/(b)/(c)）」），但 WP4 的验收只覆盖三档准入 ⇒ **前置被静默跳过**；进度记录 §3 与 `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` §A **均无执行痕迹**（`grep 探针|ROTATED_K_ORIGINAL` 只命中计划书自身的强制语句）。⇒ **WP6 路线仍是假设**。
2. **(b)（首选路线）的域契约在本仓未取证**：`ROTATED_K_ORIGINAL_V` 全树 grep **只命中** `docs/port-records/kvarn-kv-tail-feasibility-report.md:44`（描述**上游 ggml/llama.cpp** 契约）及其 worktree 副本，**本仓 C++ 代码 0 命中**。而 §6.3 自己写明 (b)「**取证/落地前须确认上游契约提供该域**」。
3. **「f16 旋转入口」：条件性缺口（非硬缺口 —— 同轮订正）**：TAIL `kvarn_hadamard`（`src/ops/kvarn/codec.cu:185-189`）要求源/目标**均为 BF16**，而 `--kv-tail-type` **默认 f16** ⇒ §6.3 列为**独立交付物**的 f16 旋转入口尚未实现。**但 `--kv-tail-type bf16` 今天已完整支持**（`KvTailType{BFloat16,Float16}` `types.h:93-96`；`startup.cpp:204` 建 `DType::BF16` 环；`ops/common/kv_tail_element.cuh:45-68,86-93` 双特化 + `with_kv_tail_element` 分派；usage 已列 `bf16|f16`），且 **BF16 尾环是逐位精确档**（其特化故意 no-op、源行比特原样拷贝 `int4`；F16 档要 `bf16→float→half`）。⇒ **若 `kvarn + --kv-tail-tokens` 要求 bf16 尾，现成的 BF16 旋转入口即够用，本交付物可整条删掉**（属产品面裁决：§10-D3 把默认钉 f16 仅为对齐 TAIL 现状）。**该订正不改变本 D-18 的判定**（真正的阻塞是实现欠账 + partial 契约 + split 分叉，见第 2/4/5 条）。
4. **A6/A7 无脚手架**：**无任何测试把 kvarn 与 `--kv-tail-tokens` 组合**（当前被规划期拒绝）；A6 要求的「KVarN body × 旋进 BF16/F16 尾合并路径的 FP32 oracle」与 A7 的 group(128)/ring(64) 边界用例**均不存在**。
5. **工期与风险敞口**：WP6 = **10–15 天、最高风险**；(b) 若成立可能收缩为「一个 launcher 分支」，(a) 则是完整内核移植 ⇒ **路线未定即开工 = 把 10–15 天押在未知分支上**，正是 §6.3 明令要避免的情形。

**③ 判定与建议**
- **判定：证据不足，不建议现在启动 WP6。**
- **建议下一步（有界的先决任务）**：**先授权 §6.3 探针** —— ~0.5–1 天、以读码 + **跨工程图谱**为主、**不需 GPU**；产出 = (a)/(b)/(c) 哪条成立 + WP6 重新估时 + (b) 的域契约在本仓的可达性结论。**探针结论落盘后再决定是否开工。**
- **同时需要用户的产品裁决**：WP6 是否值得做 —— **不做**则 `kvarn + tail` **永久 fail-fast**，§1-A4「精度尾部在 KVarN body 上有可测质量增益」**不可达**（当前 fail-fast 已是有文档的合法终态，无缺陷）。

### D-19 §6.3 探针**已执行**：(b) 否证 · (a) 忠实 · (c) 弱 · 新提 (c′)（2026-10-08）（**回写 §6.1/§6.2/§6.3/§7-WP6；版本 v18→v19**）

**触发**：用户授权探针（「允许使用探针」），并同时明确三项终态目标（性能不劣化于上游 / `k4v4` 优于 `rk4v4` / 智力不劣化）。

**方法**：本仓读码（`src/ops/kvarn/` 全树 + `small_t_*` + `kv_cache/append`）+ **跨工程图谱 Tier-3**（两个并发 `codebase-memory-port-auditor` 子代理：域契约探针与合并契约探针，
覆盖 `D-ninfer-KVarN` / `D-ninfer-beellama.cpp` / `D-ninfer-ninfer-rtx5090-mobile`）+ 回读 `docs/port-records/kvarn-kv-tail-feasibility-report.md`。**未用 GPU。**

**① (b) `ROTATED_K_ORIGINAL_V` —— 否证**
- 域枚举确存：`beellama.cpp/ggml/include/ggml.h:461-466`（`AUTO=0 / ROTATED=1 / ORIGINAL=2 / ROTATED_K_ORIGINAL_V=3`）。
- **只对 `n_query_tokens > 16` 自动生效**：`src/llama-kvarn.cpp:140-146`（`n_query_tokens <= 16` ⇒ 一律回 `ROTATED`）、
  `ggml/src/ggml-cuda/fattn-kvarn-route-policy.h:8,176-186`（`rotated_query_max_specialized = 16`）
  ⇒ **decode 与 ≤16-token 投机块恒为 `ROTATED`**，而尾部合并**正发生在这些步**。
- **V 的存储始终是旋转域**：`ggml/src/ggml-cuda/kvarn.cu:3064-3080`（`emit_rotated == false` 时对**反量化后的 V** 再补一次 `kvarn_wht_*`）、
  `fattn-kvarn-portable.cuh:330-352`、`fattn-mma-kvarn-impl.cuh:389`；上游参考实现同样旋 V（`KVarN/vllm/v1/attention/backends/kvarn_attn.py:1757-1758`，
  读回同样反旋 `:1479,1495,1950-1951`）⇒ **不存在 K/V 不对称的存储格式**；"V original" 是**读出域**。
  ⇒ **KVarN body 不能被当作 fp8 body**；§6.3(b) 的「尾部接线退化为一个 launcher 分支、可能零内核改动」**不成立**。
- **坐标系本身就是障碍**（否证 §6.1 原判「不是坐标障碍，是实现欠账」）：本仓 `tests/ops/softmax_attention/causal_cache.cpp:1613-1623`（`tail_merge_wired`）
  自述旋转域档位不接尾的原因 = 「其 reduce 核消费**旋转系 partial**，而原始 BF16 尾行**不表达于该系**」。
- **独立第二证据（与旋转域无关，本仓自证）**：即便 V 不旋，**元素类型也不匹配** —— KVarN `partial_acc` 是 `__nv_bfloat16*`
  （`src/ops/kvarn/decode.cu:113`、`decode_kernel.cuh:475,1037`），共享 reducer 收 `const float*`（`small_t.cuh:262`；fp8 侧 `small_t_fp8.cu:60`）
  ⇒ §6.2 条 1 单独成立，与 (b) 的域问题**互相独立**。**两条障碍都指向 (a)。**

**② (a) `small_t_k8v4` 形状 —— 忠实路线，仓内模板现成**
- 模板要素已复核：K/V 都旋（`small_t_k8v4.cuh:208,229`）、核内旋 Q（`:263`）、归并 **FP32** partial（`:639-643`）、**归并后只做一次反旋**（`:653`）。
- **新发现（D-19 ⑤）**：**不必逐行旋 tail 行** —— 尾部 partial 可在**原始域**算完后**只对 acc 做一次 D 维旋转**再并入共享归并
  （`W` 线性 ⇒ 与"先并入再反旋"数学等价；且合并只在 `acc` 上线性，`m/l` 是标量不受域影响）。代价远低于旋 384 行 × D。**须在落地时实测确认。**
- 接线面：`execution/text.cpp` **5 处** `ops::kvarn_attention` + 1 处 `ops::kvarn_kv_append`（`:391,414,567,622,1021,1045`）
  + `program/storage/context.cpp:1745` 的 `kvarn_restore_tail`；另 `kvarn_attention_cached` **只有测试在用**（无生产调用点）。
- KVarN 的归并入口在 `src/ops/kvarn/decode.cu:117-125`（`reduce_output_hadamard_kernel`）与 `decode_kernel.cuh:1037-1121`。

**③ (c) 暴露内建精确后缀 —— 弱，且语义不符**（订正见 §6.3）
- 实体：`kKvarnSinkPages=1`、`kKvarnTailSlots=3`（`include/ninfer/ops/kvarn.h:15-16`）；`tail_slot`（`attention.cu:84-89`）
  ⇒ slot0 = sink 页、slot1 = **首**页、slot2 = **末**页；**分组粒度是 128（`position / kvarn::Group`），不是页 64**
  （`tail_k.ne[1] == kvarn::Group`）⇒ 最多 3 组 = **384 token**。
- 它**已在运行**：`rotate_stage_kernel`（`attention.cu:157-194`）**写时旋转**（存 WHT 后的 BF16），body 核经 `cache.tail_k/tail_v` 直接消费
  （`decode.cu:105-107`）⇒ **接出来不产生新质量**，只免除 fail-fast。
- **不能承诺"最新 N 精确"**：`claim_tail_slot`（`:100-114`）在首选与备用槽都被占时**返回 −1 ⇒ 该行不 staging**；
  `retire_kernel`（`:291-315`）只清"被整组覆盖"的页 ⇒ decode（宽 1）**几乎不 retire**，陈旧页可长期占槽。
- §6.3 原文的「≈384 token 已近精确」**过于乐观**。

**④ (c′) body 自建滚动槽（本版新增，未实测）**：把内建尾环由 3 固定槽泛化为 `⌈N/128⌉+1` 滚动槽，让 **body 自己在核内**保留最新 N 个精确组。
  优点：复用 body 自身的核内合并 ⇒ **同时绕开** partial 元素类型、坐标系、split 三项冲突。代价：`--kv-tail-tokens` 出现**两套实现**；显存 ≈ N×D×Hkv×层×2×2B（N=1024 ≈ 64 MiB/序列，与 §2.4 估算一致）。**与 (a) 并列待裁决。**

**⑤ 用户三项终态目标的验收映射（含缺口）**
1. **prefill / decode / MTP 性能不劣化于上游**：decode ✅ A1（`|Δ|≤0.88%`；**但 A1 已删除 prefill 腿** —— 同二进制单请求 prefill 实测 `−29%…+8%`，噪声底不可用）；
   MTP ✅ A3（相对判据）+ A8（接受率）；**prefill 目前无可用判据** ⇒ 若须断言 prefill 不劣化，需**新协议**（重复/批量 prefill）。
   **为什么删掉 prefill 腿（用户 08-12 要求解释）**：① 出处是 `docs/port-records/PORT-MEMORY.md §5.17(2)` 的实测 —— **同一个二进制、同一 prompt、单请求 prefill 重复跑**的吞吐散布达 **−29%…+8%**，
   远大于任何要检测的效应量（A1 的 decode 门是 `|Δ|≤0.88%`）；② 机制上单请求 prefill 在本机没有"稳态"：16 GB 卡 + 10.3 GiB 产物时，它由**一次性分配/首次触页/驱动是否超额提交**主导，
   没有跨请求/跨批次可平均的重复单元；③ 因此该腿要么被撤，要么必须换仪器 —— **撤掉不等于"prefill 无风险"，只等于"现仪器测不出"**。
   **注意范围**：该删除针对的是 **A1（特性关闭时的零回归）**，所以**关掉 kvarn 的 prefill 也不在断言范围内**，不只是 kvarn 路径。
   **要断言 prefill 不劣化，新协议至少需要**：㈠ 每配置 **≥3（更好 ≥5）次重复**，报**中位数 + 最差值**与分布，而非点估计；㈡ **批量/多请求**（或用链式长 prompt 分块）以便把单请求启动噪声平均掉；
   ㈢ 控制变量（同产物、同 prompt 长度、同并发、冷热一致）+ 用 Nsight Systems 把"计算"与"分配"分开；㈣ 判据用**分布比较**（中位差落在实测跨次散布内）或**前置的 kernel/ttft 级拆分**，而非端到端点值；
   ㈤ **先在落盘报告里给出本机 prefill 噪声底**（该数字目前**不存在**：A1 的 0.88% 是 decode 的；prefill 只有 PORT-MEMORY 的区间引述）。
   **配套缺口**：WP0 的产出 ②「本机噪声底报告（decode-only pp/tg 重复 ≥3 次的分布）」**至今未产出**（§0 现值表记 WP0「模型冒烟 + KLD 基线**未做**」）⇒ 新的 prefill 协议须把这份噪声底一并补上。
   **另注**：即使有了协议，带尾部的档位 prefill 成本**必然略增**（新 token 多一次精确双写 + 尾键多读 ≤N 行）⇒ 该判据应按 A8 的口径**按档位放宽**，而非要求"零变化"。
2. **视觉不劣化**：**A1–A8 无任何一条覆盖视觉**。事实：视觉塔走**独立算子** `ops::packed_softmax_attention`（`execution/vision.cpp:385`），
   **不使用分页 KV**（`vision.cpp` 无 kvarn 引用）⇒ kvarn / 精度尾**结构上无法改变视觉算子**；风险**仅在显存预算**
   （kvarn 更省字节、尾部净增）⇒ 应作为 **A5 的推论/显存预算判据**，而非新视觉性能判据。
   现有"视觉基线" `docs/performance.md:24-26`（85.7% 接受率、7.00 tok/round、与非投机视觉运行逐字节相同）取自 **RTX 3090 / sm_86 / int8 / DFlash2**，**非本机**，**不可作本机基线**。
   另：`ninfer_qwen3_5_vision_workspace_test` 目前**失败**（先存主机标定上限，D-7 ④），属 A1 已披露项。
3. **"智力不劣化"的准确表述**：应拆为两条可证伪口径 —— **(i) 特性关闭时引擎行为逐字节不变**（= A1：输出逐字节相同 + `MemorySummary` 逐字相同 + decode tok/s 在噪声内）；
   **(ii) 特性开启时**，与 bf16/关闭态的**可测差异只能归因于 KV 量化本身**（KLD / `same_top` / needle 检索 A7 / MTP 接受率 A3），**采样与前端语义零变更**。
   即「**除量化误差底噪外，引擎可观测行为不变**」，而非不可测的"智力不变"。
4. **`k4v4` 优于 `rk4v4`：已实测成立且超出要求** —— `k4v4` mean KLD **0.002120** vs `rk4v4` **0.004426** = **2.09×**（同协议、tail=0），
   且**同头/token 字节更少**（274 vs 280 B）。**限定**：该测量是 **tail=0**，WP6 后须以限定的 tail 配置重述。
5. **⚠ A4 的效应量风险（不利面，须进 WP6 验收设计）**：KVarN body **本来就有** ~384 token 内建高精度区（③），而 `rk4v4` **没有任何内建尾**
   ⇒ A4 已记录的 rk4v4 tail 增益带（**2.26–2.47×**，`docs/port-records/PORT-VERIFY-REPORT.en.md:189-194`）**不可外推**到 KVarN；
   A4 的 **≥1.13×** 阈值**可能只在 N > 384 时可达**。**若不在验收设计里显式处理，WP6 可能"功能做完但判据不达标"。**

**⑥ 未决**
- **产品裁决：(a) 还是 (c′)**（忠实但重 / 有界但 `--kv-tail-tokens` 双实现）。
- **A6/A7 脚手架仍不存在**（FP32 oracle × 跨 group(128)/ring(64) 边界；且**无任何测试把 kvarn 与 `--kv-tail-tokens` 组合** —— 已复核
  `tests/models/qwen3_5/test_kvarn_tail_row_reset.cpp:74` 显式令 `kv_tail_tokens = 0`；该测试验的是 **KVarN 自身尾行标记重置**，不是精度尾环）。
- 若采 (c′)：`--kv-tail-tokens` 的双实现语义（外部共享环 vs body 自建槽环）需定义。
- 报告 §4.1 记 beellama 侧「BF16 尾同配置明显更差、**该卡必须用 F16 尾**」——**非本机、非本仓**观测；若采 bf16 尾须在本机复核（与 D-18 订正相关）。
  **→ 用户已裁定（2026-10-08）：接受「若采 bf16 尾则在本机复核」** ⇒ **既不据该（非本机）观测否定 bf16 尾，也不据现有证据认可它**；复核编号入 WP6 的验收清单。

**⑦ 上游合并契约（第二个并发子代理，Tier-2/3，`D-ninfer-beellama.cpp`）—— 同时**降低 (a) 的难度**并**否证 §6.2 条 2**（回写 §6.2）**
- **统计量/累加器**：FP32 `float2(row max, denom)` + FP32 累加器；`GGML_ASSERT(KQV->type == GGML_TYPE_F32)`、`GGML_ASSERT(dst->type == GGML_TYPE_F32)`
  （`ggml/src/ggml-cuda/fattn-tail.cuh:339-346,379-387`、`fattn-common.cuh:1551`、`fattn-kvarn-portable.cuh:826-829,834`）。**上游不存在 BF16 部分累加器**
  ⇒ **本仓的 BF16 `partial_acc` 是对上游的偏离**（不是"上游的另一种变体"），修法 = **照上游改 FP32**。
- **KVarN body 导出同构统计量**：`fattn-kvarn-portable.cuh:466-469,784-786`、`fattn-mma-kvarn-decode-combine.cuh:67-68`；
  **附尾时由内核自并**（`portable.cuh:816,370-451`，仅 `split==0`；测试 `tests/test-kvarn.cpp:2318-2319` 记"附体时 body_meta 不产出"）；
  入口 `GGML_CUDA_FATTN_KVARN_ENTRY_COMPACT_TAIL`（`fattn-kvarn-dispatch.cuh:44`、`fattn.cu:858-865`）。
- **域**：**旋转域合并、末尾一次反旋**（`llama-graph.cpp:3901-3904`）；混合域（**仅 prefill，`>16` query token**）下 V 在**原始域**相遇
  （body 的 V 累加前逐 token 反旋，`portable.cuh:330-362,434-443`）。materialize 回退按 `emit_rotated`/`!mixed_domain`（`fattn-tail.cuh:753-762`、`kvarn.cu:3064-3078`）。
- **tail 行的旋转时点**：**写原始、读时旋转**（写 `llama-kv-cache-kvarn.cpp:1004-1011`；读 `llama-graph.cpp:3845,3847` 的 `ggml_kvarn_wht_aux`；
  `kvarn.cu:788` 明写 "Stage and records are rotated-domain for both K and V" —— **内建 stage 是旋转域存储，外部精度尾环是原始行**，两者是不同 buffer）。
- **split**：**tail 无 split 轴**（每行一对统计量，`fattn-tail.cuh:844-847`）；body 侧在导出前已把 splits 归约（`fattn-kvarn-portable.cuh:784-786`；split 数 `fattn-kvarn-dispatch.cu:615-618`）
  ⇒ **两者不要求 split 一致**（子代理结论 (iv)：否）⇒ **§6.2 条 2 的"分叉"可规避（两段式归并），不是阻塞。**
- **子代理限制**：未验证**非 KVarN（稠密 F16）**路下 `self_k_rot/self_v_rot` 是否可能非空 ⇒ 结论 (iii) 限定 KVarN 路；`D-ninfer-beellama.cpp` 工作树 dirty（`metadata_changed`）、未 reindex。

⇒ **净效应**：§6.2 三条"一手反证"现为 —— **条 1 = 本仓自身偏离（照上游改 FP32 即可）**；**条 2 = 可规避（非阻塞）**；**条 3 = 条件性交付物（采 bf16 尾即免）**。**(a) 的技术风险低于 v18 的表述，但实现量不变**（仍是一个 KVarN 专属 tail-partial 路径 + 归并 + 6 处接线 + A6/A7 脚手架 + 容量核算）。

**⑧ (a) 的量化：性能代价与改动难度（2026-10-08，用户要求「具体量化」后记录）**

**前提核验（用户论证：16 GB 只能单流、24 GB+ 并发时更宽松）**：`max_concurrency` 默认 **1**（`include/ninfer/types.h:459`），
`kv_table_rows = plan.max_concurrency`（`startup.cpp:200`）⇒ 内建环与外部环**都随并发线性放大**。
- **C=1**：**(a) 88 MiB/序列**（内建 24 + 外部 64，N=1024）vs **(c′) 76.5 MiB**（`M=9` 槽；两者都含 MTP 池 ×1.0625）⇒ **差 12 MiB**。
- **C=8**：**(a) ≈716 MiB** vs **(c′) ≈612 MiB** ⇒ **差 104 MiB**。
- ⇒ **显存不是决定因素，用户判断成立。** (a) 的代价在别处（下表）。

**性能代价（逐项）** —— 结构性前提：TAIL 的尾是**分区**不是追加（`body_window = window − tail_keys`，`small_t.cuh:151-152`），
body 只覆盖 `[0, body_window)`、尾只覆盖 `[body_window, window)` ⇒ **注意力 FLOPs 不变**，只有最新 N 个 key 的**读取字节**从 4-bit 变 16-bit。

| 项 | 量 | 相对基准 |
|---|---|---|
| 每 token 多一次精确双写 | 2 平面 × 256 × kv_heads 4 × 2 B = 4 KiB/层 → **64 KiB/token**（16 层） | decode 每 token 流式读 ~10.3 GiB 权重 ⇒ **~6×10⁻⁶**，可忽略 |
| 最新 N 键的读取字节 | N=1024：bf16 读 **64 MiB/步** vs 同键 k4v4 读 **17.9 MiB** ⇒ **+46 MiB/步** | window 8192 的 KV 读 ≈ 143.6 MiB ⇒ **+32%**；但 **KV 只占 decode 总流量 ~1.4%** ⇒ 端到端 **≈ +0.4%** |
| `partial_acc` BF16→FP32 的 workspace | `attention.cu:567` 的 `D*2+8` → `D*4+8` 字节/行；decode（splits=82、q_heads=24）⇒ **+0.4 MB** | 每步读写各一次 ⇒ +0.8 MB ⇒ **≈0.6%** 于 143.6 MiB，可忽略 |
| 两段式多一次 per-row 往返 | tokens×q_heads×(D+2)×4 B ≈ **25 KB/步** | 可忽略 |
| f16 尾的旋转入口 | **可免**（用户已接受 bf16 尾 ⇒ 复用现成 BF16 入口） | — |

⇒ **(a) 的"增量"性能代价可忽略**。被感知的是**尾部固有、与路线无关**的代价：记录值 **decode −5.8% / MTP 接受率 −2.1 pt**
（`docs/port-records/PORT-MEMORY.md:663-670`，post-fix 配对 A/B，口径 `--spec mtp --draft-tokens 7 --kv-dtype rk4v4-e8 --max-context 4096`，tail 0→1024）。
**两条限定**：① 该 −5.8% 是**带 MTP 的 decode tok/s**（含接受率联动），**不是纯 kernel 时间**；② **尾部在非 MTP decode 上的纯 kernel 代价从未单独测过**
⇒ 若要断言"性能不劣化"，这是**一项待补的测量**（A8 已按此口径放宽判据）。(c′) 读同样多的 bf16 尾键 ⇒ **这部分代价两者相同**。

**改动难度（逐项）**

| 工作项 | 位置 | 规模 | 风险 |
|---|---|---|---|
| **W1** 新增 KVarN 专用 tail-partial 路径 | 新文件（照 `small_t_tail.cuh` 406 行 / `small_t_k8v4.cuh` 661 行） | **~300–450 行新代码** | 中 |
| **W2** `partial_acc` BF16→FP32 + 两段式归并 | `decode_kernel.cuh` 的 `reduce_output_hadamard_kernel`（`:1037-1121`，~85 行）+ 签名/寻址 `:475,556-577,1029-1030`（~6 处）+ `decode.cu:113,120` | **改 ~120 行**（在 1155 行文件内） | **高**（核心归并；A2/A6 必须重跑） |
| **W2b** body 窗口分区（`body_window = window − N`） | `attention.cu` + `decode.cu` 的 `logical_capacity`/split 计算（照 `small_t.cuh:147-170` 的形状；**分区在 op 内部完成 ⇒ 不必改 `text.cpp`**） | **~40–70 行** | 中（A4/A7 正确性） |
| **W3** workspace 尺寸重算 | `attention.cu:559-568` | ~10 行 | 低 |
| **W4** 接线：3 个 op 入口加尾视图 + 6 个调用点 | `include/ninfer/ops/kvarn_attention.h`；`text.cpp:391,414,567,622,1021,1045`；`context.cpp:1745` | **~80 行** | 中 |
| **W5** 尾环写入接入 KVarN append | 照 `small_t_tail_shadow.cuh`（71 行）的形状 | **~80–120 行** | 中 |
| **W6** 规划解除 fail-fast + 容量计费 | `startup.cpp:1035-1045`、`decoder_state.cpp:160-177`、`MemorySummary`/容量曲线（WP7） | ~60–100 行 | 中 |
| **W7** A6/A7 脚手架（FP32 oracle + group(128)/ring(64) 边界） | `tests/`（照 `tests/ops/softmax_attention/causal_cache.cpp` 4,000+ 行的尾测形状） | **~300–500 行新测试** | 中 |
| **W8** 回归 | A2 位序往返、A3/MTP、续列尾/前缀 e2e、ctest 263 项 | 跑，不改 | **高**（判据须仍过） |

**合计**：新代码 **≈900–1,400 行**（含测试）、改动 **≈300 行**、触及 **12–15 个文件**；**工期 10–15 天**（沿用计划书区间）。
**难度的真正来源是 W2**（改核心归并 + BF16→FP32）。**利好**：上游 beellama 正是这么做的（FP32 acc、旋转域合并后一次反旋）
⇒ W2 是「**照上游对齐**」，不是发明新契约；**唯一必须钉住的是 A6 的「tail=0 逐位不变」**。
**与 (c′) 的对照**：省掉 W1/W2（约 −5–8 天），但新增「A6/A7 判据重写 + 双实现语义」（约 +2–3 天）⇒ 净差 ~3–5 天，代价是**永久语义债**。

**⑨ 路线裁决（2026-10-08）**：**用户确认 = (a)**（照 `small_t_k8v4.cuh` 形状，接进外部共享精确尾环）。理由：① 16 GB 单流下**显存非决定因素**（⑧ 已验证：与 (c′) 仅差 12 MiB @C=1、104 MiB @C=8）；
② (a) 的**增量性能代价可忽略**（⑧ 表），被感知的只有**尾部固有、两条路线共付**的代价；③ (a) **消除** `--kv-tail-tokens` 的双实现语义与本仓 **BF16-acc 偏离**、判据与其它档位**同构**。
**(c′) 否决**（省 ~3–5 天，换永久语义债）。**已写入 §6.3 与 §7-WP6（分步计划 WP6.0–WP6.7）；§7-WP6 另含两项前置测量（WP6.0a/0b）。**
**公开的残余风险**：W2（改核心归并 + BF16→FP32）是本轮**唯一高回归项**，由 A6 的「`tail=0` 逐位不变」抵住；若该不变量无法保持，回退到"独立 merge 核"。
