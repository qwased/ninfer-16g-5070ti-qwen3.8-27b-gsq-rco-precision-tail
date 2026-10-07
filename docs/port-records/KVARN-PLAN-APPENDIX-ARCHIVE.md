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
