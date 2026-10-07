# KVarN 移植进度日志（kvarn-port-progress）

> **用途**：本文件是 KVarN 移植工作的**唯一进度落盘点**。每推进任何一步——包括成功的、
> 失败的、未决的、以及被推翻的结论——都**必须**追加一条记录，确保信息不随会话上下文丢失。
>
> **配套权威**：`kvarn-port-into-precision-tail-plan.md`（方案 v13，active authority）。
> 本文件记录"实际发生了什么"，计划书记录"打算怎么做"；两者冲突时以本文件的实测为准并回写计划书。
>
> **规则**：见计划书 §0.5「记录规则」。任何 WP 在结束（含中止）时，必须：
> ① 更新本文件 §0 当前状态快照；② 在 §3 追加一条带日期的记录；③ 把影响验收口径的结论回写计划书。

---

## 0. 当前状态快照

> **压缩说明（2026-10-08）**：本节的「最后更新」散文段（源 16–34 行）、WP 状态明细表（36–47 行）、
> 工作树改动清单（49–131 行）与已关闭的未决项 1 / 5 / 8 / 9 / 10 **已逐字归档**至
> `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md`（小节 §B / §C / §D / §E / §F / §G / §H / §I）。
> 本节只留**现值 + 指针**；每个数字的一手出处是计划书附录 D 与本日志 §3。归档是搬运，不是删减。

| 项 | 现值 | 详见 |
|---|---|---|
| WP0 基线与环境 | 环境/构建已核实；整树构建**不通过**（先存 `LNK2019`，与本工作无关）⇒ 按目标构建；模型冒烟 + KLD 基线**未做** | 附录 D-2、§3-2026-10-07-2、归档 §C |
| WP0.5-A MTP 一致性仪器 | **定案 + 仪器改造完成（08-05）**：A3 按 O1 替换（非本移植引入、非上游缺陷，属 Fork B 私有合同）；仪器去逐位门禁 + 自确定性门禁 + 首分叉报告 + 并入 `rk4v4`/`kvarn` 档 + `--quick` | 附录 D-6/D-13、§3-2026-10-08-5、归档 §C |
| WP0.5-B 准入实验 | **完成**（07-18 代理档 / 07-22 正式档）：代理门禁同字节 **4.15×** 为正 ⇒ WP4 GO；发布档三档全过 —— `k4v4`(274 B) **0.002120**、`k5v5`(338 B) **0.001432**、`k6v6`(402 B) **0.001233**，对 `rk4v4`/`nvfp4`/`k8v4` = 2.09×/2.07×/2.18×；`k6v6` 门槛 < **0.002688** | 附录 D-10/D-11（正文未归档）、§3-2026-10-07-18/-22 |
| WP0.5-C 构型口径 | **完成**：本机为 **native 口径**（`NINFER_SM120_NATIVE=ON`）；compat 口径噪声底未测（未决项 2） | 附录 D-1、§3-2026-10-07-5 |
| WP1 kvarn ops 移植 | **完成**：ops 零告警编译；`ninfer_kvarn_test` 容差项由 WP5 收口（08-04 余量 65×）；kvarn bench **未做**（需重配，未决项 3） | 附录 D-3、§3-2026-10-07-3/-4、归档 §C |
| WP2 页几何 + 存储枚举 | **完成**：`KvarnGroup128` + `KvarnBits` + 三处 parser + 6 处名字 switch + 指纹 `;kvbn=` + 校验放宽 64\|128。回归：合成 242/245、真实模型 10/16，**失败均非本移植引入**（`stash` 重建基线对照）。**残余（parser 单测）已由 P3c 关闭（08-07）**：cli/serve 两处 parser 单测落地全过 | 附录 D-7、§3-2026-10-07-13、§3-2026-10-08-7 |
| WP3 模型接入 | **完成（08-05）**：① 地址空间页几何（07-16，ctx8192/229,348 token 跑通）② 续列尾（08-02 实现；08-04 host+device 单测全绿 + Engine 级 e2e `cached_tokens=851`、message 逐字节同）③ `--mtp-attention-window` 规划期拒绝（07-21）④ MTP 激励（08-04 `mtp accepted 84/113`、1.50× ≈ bf16 1.52×）+ 仪器（08-05）⑤ 措辞订正 ⑥ `--help`/docs（07-22）。08-03 独立审计：**无 HIGH/MED**（3 项 LOW 为 FORK 继承） | 附录 D-9/D-12/D-13、§3-2026-10-08-1…-5 |
| WP4 位宽参数化 K=V∈{4,5,6} | **完成（07-22）**：核按 `(KBits,VBits)` 模板化（发布档只实例化 `(b,b)`）+ `bits` 贯穿 + 测试扩 4/5/6 实跑全绿 + parser 发三档并删 `Bits2`/`k4v2` + 规划期拒绝（tail / mtp-window）+ 三档准入全过。三档单次测量的口径已由 08-04 的 P3a **3× 重复**收口（质量指标逐位相同、极差 0） | 附录 D-11/D-12、§3-2026-10-07-19…07-22 |
| WP5 oracle 与容差 | **完成（08-04）**：量化步长判据（点值 ≤ `q*(1+5e-2)` + `flips ≤ 1.0e-3*total` + `\|Δcode\|>1` 零容忍）+ 4/5/6 位穷举逐码往返；**实测余量**：`flips` 最大 **1/65**、`over_step=0`、`wide_flips=0` ⇒ **65× 余量、判据未放宽**。口径已回写 `docs/maintainer/op-development.md §6.3` | §3-2026-10-08-3/-4、§1 A2 |
| WP6–WP9 | **WP6 路线已裁决 = (a)**（08-13，用户确认；**(c′) 否决**）⇒ 计划书 §7-WP6 已拆为 **WP6.0–WP6.7**；**两项前置测量（0a 本机 prefill 噪声底 / 0b 外部尾纯 kernel 代价）已入队、未执行**；**WP6.0–WP6.7 全部未开工**。**工期 10–15 天**。**唯一高回归项 = WP6.2（`partial_acc` BF16→FP32 + 两段式归并），由 A6「`tail=0` 逐位不变」抵住** | 计划书 §6/§6.3、§7-WP6、附录 **D-19 ⑧·⑨**、§3-2026-10-08-12/-13 |

**最近五轮**：08-06 P3b 档位并入名字（**六展示面 GPU 实测全过**）+ 图谱使用纪律落盘（AGENTS.md + skill）
→ 08-07 P3c kvarn parser 单测（cli/serve 两处全过；第三处 perplexity parser 结构性不可单测，已在 P3b e2e 覆盖）
→ 08-08/08-09 图谱复核（`KvarnBits` 消费方与 grep 一致、`reset_kvarn_tail_row` callers=0 复现；两条工具边界回写 AGENTS.md）
+ 订正审计 LOW②（**只有 commit 侧静默**，capture 是 abort+重抛、activate 直接上抛）
→ **08-10 审计 3 项 LOW 修复落地**（① CausalScore 路由补 `reset_kvarn_tail_row` **生产变更** · ② commit 发布失败加诊断 · ③ 注释补 KVarN）
+ 新增 `ninfer_qwen3_5_kvarn_tail_row_reset_test`（**实跑 PASS，ctest 262→263**）
→ **08-11 ① 的端到端对照已跑**：`kvarn:k4v4` KLD 臂**逐位复现 `0.002120`** ⇒ 该协议上**无害 no-op**、D-11/P3a **无需重测**（**但必要性无证据**，见 §3-08-11 不利面）
+ P3d 后 GPU 回归 **3 项全绿** + 用户裁决待决 3 项（(a) 不移植 FORK 测试 /(b) 不补 `capture_identity_tag` /(c) bench 暂不纳入）
+ **发现并修复「子代理无图谱」根因**（4 份代理定义的 MCP 工具名连字符 ≠ 运行时下划线；**须重启 Qoder 才生效**，重启是否充分未排除）。全文见 §3。
→ **08-12 §6.3 探针已执行**（用户授权；3 个并发子代理 + 本仓读码，**未用 GPU**）：**(b) 首选路线否证**（域只对 `>16` query token 生效、decode 恒旋转、V 存储恒旋转；**坐标系本身即障碍**）、**(a) 为忠实路线**、**(c) 弱且语义不符**、**新提有界 (c′)**；
**上游合并契约**（FP32 `float2` + 无 split 轴 + 旋转域合并后一次反旋 + 尾行写原始读时旋转）⇒ **§6.2 条 1 改判"本仓自身偏离"、条 2 改判"可规避"**；
并产出**用户三项终态目标 → A1–A8 的映射与缺口**（prefill 无判据、视觉无判据但结构上不受影响、`k4v4` 已实测 2.09× 但为 tail=0、**A4 阈值可能只在 N>384 可达**）。全文见 §3-08-12 与计划书 D-19。
→ **08-13 路线裁决完成**：用户确认 **WP6 = (a)**（接外部共享精确尾环；**(c′) 否决**），依据 = 08-12(8) 的量化（显存差 **12 MiB@C=1 / 104 MiB@C=8**、增量性能代价可忽略、消除双实现语义与 BF16-acc 偏离）；
计划书 **v19→v20**：**§7-WP6 拆为 WP6.0–WP6.7**（含**两项前置测量入队**：0a 本机 prefill 噪声底、0b 外部尾纯 kernel 代价）；**同轮另一项裁定**：视觉**只作功能判据**、接受"若采 bf16 尾则本机复核"。全文见 §3-08-13。

**工作树**：WP1–WP5/P1 的逐文件清单已归档（归档 §D）；这些改动**已于 2026-10-08 提交**
（`beda920a` feat / `fe76ad42` test / `6aea191b` docs）。**P3b + 图谱纪律**亦已于 2026-10-08 提交：
`6836b2de` feat（7 个源码文件：`MemorySummary` + 四展示面）/ `3e780a41` docs（计划书 v14 + 附录 D-14 + 本日志 §3-08-06 +
三份 `docs/port-records/KVARN-*-ARCHIVE*` 归档）/ `efdb1dce` docs（`AGENTS.md` 的 codebase-memory 小节）。
**08-07…08-10 的全部改动亦已于 2026-10-08 提交**（4 个提交，工作树除 harness 的 `.qoder/` 外干净）：
`0754708e` test（P3c：cli/serve 的 kvarn parser 用例）/ `b30c3a98` fix（P3d：CausalScore 尾标记重置 + commit 诊断 + 注释）
/ `69f84b3e` docs（`AGENTS.md` 图谱纪律收窄）/ `7582d7ff` docs（计划书 v15/v16 + 附录 D-15/D-16 + 本日志 08-07…08-10）。
**唯一未入库项**是 `~/.qoder/skills/codebase-memory/SKILL.md`（仓库外，非本仓版本控制范围）与 harness 生成的 `.qoder/`（已加入 `.gitignore`）。

> **工作树改动清单**：WP1 新增 12 个 ops 文件 + 2 个头 + 2 个测试、WP2 的 16 文件、WP3 的 14 文件、
> WP4 的 `kvarn.h` 泛化、WP5/P1 的 4 个测试与 `op-development.md` §6.3 —— **逐字清单（含函数名与守卫点）
> 见 `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` §D**（源 49–131 行）。改动现已全部提交（`beda920a` / `fe76ad42` / `6aea191b`），
> `git show --stat <sha>` 是权威清单。

**未决项汇总**

1. ~~`ninfer_kvarn_test` 的 4-bit K codec oracle 容差~~ → **已关闭**（07-21 收口 → 08-03 形式化为量化步长判据 → **08-04 余量实测** `flips` 最大 1/65、`over_step=0`）。原文（含 `relative_l2=6.0882e-4`、`max_abs=0.216` 与 `sqrt(0.216²/Σdecoded²)≈8.4e-4` 的根因推导）**逐字见归档 §E**。
2. WP0.5-C 的 compat 口径噪声底：未测（需重配 `NINFER_SM120_NATIVE=OFF`，与 AGENTS.md「不要重配」冲突）。
3. ~~kvarn bench 是否纳入~~ → **已裁决（2026-10-08-11）：暂不纳入**（维持 `NINFER_BUILD_BENCHMARKS=OFF`、**不重配** `build-port`；bench 不构成交付缺口，性能由 perplexity/KLD 协议与 MTP e2e 提供）。原文「需 `-DNINFER_BUILD_BENCHMARKS=ON` 重配，与「不重配」冲突，**待显式决定**」保留以存史。
4. 先存构建缺陷 `ninfer-multi-gpu-probe.exe` 是否修复：未修（见 §3-2026-10-07-2）。
5. ~~**A3 判据不成立，需裁决替换**~~ → **已裁决（2026-10-07-12）：采纳报告 O1**（①同配置自确定性 ②质量不劣化 ③首分叉文档化 ④kvarn 专属「不劣于同配置基线」；parity 测试改造为诊断仪器；O2 拒绝）。**定案全文**（#265/#80 原文、`small_t.cu:383-395` 机制、`docs/performance.md:29-45` 23 构型实测、Fork B 私有合同 provenance）**逐字见归档 §F**；裁决与判据现于计划书 §1 A3 / §9 / 附录 D-6。残余实测已由 08-05 关闭（§3-2026-10-08-5）。
6. ~~**WP2 遗留：kvarn 可解析但不可运行**~~ → **已解除（07-21，WP4.4）**：三档 `kvarn`/`k4v4`/`k5v5`/
   `k6v6` 已由 parser 发布且**源码端到端可运行**（`plan_cache` kvarn 单 plane 分支、host switch 注册、
   `text.cpp` 5 处派发均已在 WP3/07-16 落地）。**残余**：`--help` 文本与
   `docs/{cli,serving,perplexity}.md` 的档位说明**仍未补**（禁止宣传不能运行的功能这一前提已消失，
   应补），另 `--kv-tail-tokens`/`--mtp-attention-window` 与 kvarn 的组合已在规划期拒绝，文档需同步。
7. **WP2 名称覆盖**：~~三处生产名字 switch 只按 `KvCacheStorage` 取名字 → kvarn 三档在日志里同为
   `"kvarn"`，**级别不出现在名字里**（只出现在指纹 `;kvbn=`）~~ **→ ✅ 已关闭（2026-10-08，P3b，§3-08-06）**：
   `MemorySummary` 现带 `KvarnBits`，四个展示面渲染级别 —— 文本 `kvarn:k4v4`、报告目录分量 `kvarn-k4v4`
   （用户裁定）；**六展示面 GPU 实测 + 6 项 host 回归全过**（附录 D-14）。原文**就地加删除线**（未归档，因其为
   §0 仍开放的未决项原文）。
   **✅ 已关闭（2026-10-08，P3c，§3-08-07）**：`ninfer_cli_options_test` / `ninfer_serve_options_test`
   各增 kvarn 用例 —— 裸 `kvarn` 与 `kvarn:k4v4|k5v5|k6v6` 的存储 + 级别、默认级别 `Bits4`、拒绝未发布拼写
   （`k4v2`/`k3v3`/`k4v5`/`k7v7`）、help 含三档；serve 侧另加 `make_engine_options()` 贯通断言。两项 **exit 0**。
   **第三处 parser（`apps/perplexity/main.cpp`）仍无单测**：其分支内联于 main.cpp、`Options` 亦为 main.cpp 局部，
   既有 `ninfer_perplexity_evaluation_test` 只编译 `evaluation.cpp` ⇒ 不重构即不可单测（该 parser 的行为覆盖由
   P3b 的 GPU e2e 提供）。原文**就地加删除线**（未归档，因其为 §0 仍开放的未决项原文）。
8. ~~**WP3 长上下文阻塞（07-14 实测定位）**~~ → **已解决（07-16，WP3①）**：根因、修法（42 调用点 + 池几何取页）与验收（ctx8192 / 229,348 token，PPL 4.72225 vs bf16 4.69317）**逐字见归档 §G**；现于计划书附录 D-9 与 §9。
9. ~~**WP3 未做（续列尾）**~~ → **已被取代**：原文写 `state_image` kvarn 镜像与 `restore/capture/activate_sequence_kvarn_tail`「**未实现**、前缀复用不成立」，该表述**已被 08-02 的实现与 08-04 的 host+device 单测 + Engine 级 e2e 验收（`cached_tokens=851`、message 逐字节同 r1）取代**。**原文按原样保留、未改写** → 归档 §H。
10. ~~**kvarn 与 `--mtp-attention-window` 不兼容**~~ → **已实现（07-21）**：`validate_target_options` 同时拒绝 `--mtp-attention-window != 0` 与 `--kv-tail-tokens != 0`（后者此前会被**静默忽略**）。原文（含其残余「MTP 激励与 A3 相对判据仍属 WP3④」，该项已于 08-04/08-05 关闭）**逐字见归档 §I**。
11. ~~**子代理无图谱（仓外工具/配置问题）**~~ → **✅ 已闭环（2026-10-08-11 修复，重启后复核通过）**：`~/.qoder/agents/` 下 4 份 `codebase-memory*` 定义的 `tools:`/`mcpServers:` 曾用**连字符**，而运行时工具名归一为**下划线**（`mcp__codebase_memory_mcp__*`）⇒ 子代理只拿到 `Read/Grep/Glob`、**图谱调用恒为 0**（含**上一会话**两个子代理：42/64 次调用全是 Read/Grep）。**修复**：4 份定义改下划线（原件备份 `/tmp/agents-backup-20261008/`，sha1 已记）。**本会话内无效** —— 代理清单元数据 `[AgentListingDelta] isInitial=true` **只在会话启动装载一次**（worker `plugins.hotReload=false`）。**重启后探针通过**：`codebase-memory-scout` 成功调用 `list_projects`（返回 **6 个图谱工程**）与 `trace_path`（`reset_kvarn_tail_row` `callers_total=4`，**1 个生产调用者** `causal_score` + 3 个测试 hop）⇒ **「须重启」已被证实，且「重启充分」已确认**（先前的第二假设「宿主一律不授予子代理 MCP」被排除）。**遗留影响（不可撤销）**：§3-08-03 的 P1 审计子代理「两树均无调用者」是在**无图谱**下作出的 ⇒ 此前所有 `codebase-memory*` 子代理的结构性/否定性结论均须视为 **grep-only**。
    **新可得能力**：`list_projects` 显示 6 个可用图谱工程 —— `D-ninfer-KVarN`（上游华为/vLLM 实现）、`D-ninfer-beellama.cpp`、`D-ninfer-ninfer`（上游）、**`D-ninfer-ninfer-rtx5090-mobile`（FORK）**、`D-ninfer-ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco`、本仓 ⇒ **WP6 的 (a)/(b) 取证可跨工程做**（`codebase-memory-port-auditor` 有 `compare_graphs`）。
    **✅ 已于 08-12 首次实战**：域契约 + 合并契约两个 Tier-2/3 探针成功，直接产出 (b) 的否证与上游合并契约（§3-08-12、计划书 D-19）。
12. **WP6 路线裁决（08-12 新开）**：**(a) 忠实路线** vs **(c′) body 自建滚动槽** —— **(b) 已否证**（§3-08-12、计划书 D-19）。**另**：A6/A7 脚手架**仍不存在**；若采 (c′)，`--kv-tail-tokens` 的**双实现语义**待定义。**探针结论已落盘，但 WP6 开工仍须用户明确授权**（计划书 §7-WP6 门禁）。
13. **用户三项终态目标带来的口径缺口（08-12 新开，须用户确认）**：① **prefill 性能无可用判据**（A1 已删 prefill 腿；若须断言 prefill 不劣化需**新协议**）；② **视觉无判据**（结构上不受 kvarn/尾影响 ⇒ 拟归 A5 的显存预算推论，**非**新视觉判据；现有视觉基线取自 RTX 3090/sm_86/int8，**非本机**）；③ **A4 阈值风险**：KVarN 有 ~384 token 内建高精度区而 `rk4v4` 没有 ⇒ 记录的 rk4v4 增益带 **2.26–2.47×** 不可外推，**A4 的 ≥1.13× 可能只在 N>384 可达**。详见计划书 D-19 ⑤。

---

## 1. 环境与构建速查（本机实测可用）

**主机**：Windows 10.0.26300，Git Bash 为 shell。GPU **只有 1 个 CUDA 设备** =
NVIDIA GeForce RTX 5070 Ti 16 GB（Intel UHD 770 是**非 CUDA** 设备，不参与计算）。
空闲显存基线 ~48 MiB。`nvidia-smi` / CUDA 设备号无歧义。

**工具链**（`.deps/env-port.bat` 全部上 PATH）：MSVC 14.44.35207（VS 2022 BuildTools）、
CUDA 13.3、CMake 3.31.6-msvc6、Ninja（均随 BuildTools）。

**构建树**：`D:\ninfer\ninfer-precision-tail\build-port`（Ninja / Release /
`CMAKE_CUDA_ARCHITECTURES=120a` / `NINFER_SM120_NATIVE=ON` / `NINFER_BUILD_APPS=ON` /
`BUILD_TESTING=ON` / `NINFER_BUILD_BENCHMARKS=OFF`）。**不要重配。**

**可用的构建命令**（Git Bash，逐字照抄；`> /tmp/x.log 2>&1` 必须在**外层 bash**写，
不要在 cmd 串里写 `>nul`——MSYS 会改写并破坏 `&&` 链）：

```bash
cmd //c "call D:\ninfer\ninfer-precision-tail\.deps\env-port.bat && cmake --build D:\ninfer\ninfer-precision-tail\build-port --target ninfer_ops -j 4" > /tmp/ops.log 2>&1
cmd //c "call D:\ninfer\ninfer-precision-tail\.deps\env-port.bat && cmake --build D:\ninfer\ninfer-precision-tail\build-port --target ninfer_tests -j 4" > /tmp/tests.log 2>&1
```

**整树构建会失败**（先存缺陷，见 §3-2026-10-07-2）⇒ 一律**按目标**构建
（`ninfer_ops` / `ninfer_tests` / `ninfer` / `ninfer-serve` / `ninfer-perplexity`）。

**读编译错误**：MSVC/nvcc 诊断是 GBK 编码，`grep -a "error C[0-9]\|error LNK\|FAILED:" log` 过滤。

**跑单个测试用例**：
```bash
./build-port/tests/ninfer_tests.exe ninfer_kvarn_test
export NINFER_TEST_ARTIFACT='D:\ninfer\ninfer-precision-tail-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer'
```

**模型产物**（唯一，不要 glob）：
`D:\ninfer\ninfer-precision-tail-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`（11,092,477,952 B）。

**KLD 语料**：`eval/corpora/perplexity-1m/manifest.json`（**已确认存在**）。

**旁枝**：`.worktrees/{m5a,wp1,wp2,wp3}` 是**上一轮 precision-tail 移植**留下的 worktree
（分支 `port/*`），与 kvarn 无关，**不要动**。

---

## 2. 关键路径与事实索引（本工作核实）

| 事实 | 出处 |
|---|---|
| 两个宿主硬抛（新枚举必须先注册，否则启动即崩） | `src/models/qwen3_5/state/decoder_state.cpp:31` 无条件调 `d256_kv_cache_profile(storage)`；该函数 `src/ops/kv_cache/d256_profile.h:86-129` 的 host switch 末尾 `throw std::invalid_argument("unknown KV-cache storage profile")`（:128）；`src/core/paged_kv_storage.h:~120` 同构 throw |
| `KvCacheStorage` 枚举（新值**必须追加末尾**） | `include/ninfer/types.h:49-73`（`:67` 明写"Appended last so the existing values keep their numbering"） |
| 尾部默认 dtype = f16 | `include/ninfer/types.h:~469` |
| MTP 相关选项名 | `include/ninfer/types.h:131-155` `SpeculativeOptions`：`backend`/`draft_tokens`/`proposal_head`/**`mtp_policy`**（不是 FORK 的 `mtp_draft_policy`）/`lookup_ngram`/`ngram_*`/`mtp_attention_window`；`ProposalHead{Full,Optimized}` :111；`SpeculativeBackend{None,Mtp,DFlash,DFlash2}` :116；`MtpDraftPolicy{Fixed,Adaptive}` :126 |
| TAIL **没有** `enable_nvfp4_scale_compression` / `compressed_scales` | 全库 0 命中（FORK 有） |
| kvarn ops 的外部依赖面（很小） | `core/{device,tensor,arena}.h`、`ninfer/ops/{kvarn,kvarn_attention,softmax_attention}.h`、`ops/softmax_attention/dense/causal_cache/{launch.h,small_t.cuh,prompt_common.cuh}`、`ops/kvarn/*` |
| `geometry.cuh` / `prompt_common.cuh` 两仓**逐字节相同** | `diff -q` 实测 |
| `op_tester.h` TAIL 是 FORK 的**超集**（向后兼容） | TAIL 417 行 vs FORK 363 行，多出 A8/A4 arch-skip 与 sync guard |
| kvarn 未使用 `launch.h` 的 `causal_attention_prompt_launch`（该处两仓分歧） | kvarn 只用 `causal_prompt_{q_index,swz,swz_addr}`（来自逐字节相同的 `prompt_common.cuh`） |
| 设备侧 `expf`/`logf`（**非** `__expf`），构建**未开** `--use_fast_math` | `src/ops/kvarn/{sinkhorn.cuh:141-153,store.cuh:62-124}`；cmake 全库无 fast-math |
| **移植来源 = 华为 KVarN（vLLM 实现）的 dense 预设 `kvarn_k4v2_g128`（D256）**，**非**照抄 beellama | `src/ops/kvarn/config.cuh:3-4` 自述并给出上游 commit `7586257f1c632e63187bfacbbe21ccb51540f7b3` 与 `vllm/model_executor/layers/quantization/kvarn/config.py`；上游仓在 `D:\ninfer\KVarN` |
| 算法骨架与 beellama/华为一致（07-23 三方核对）：head_dim WHT（**自逆**，×1/16=1/√256）× Sinkhorn **双轴**方差归一化（**默认 8 次**，取**最优 imbalance**）× per-row **非对称仿射**（scale+zero）；记录 = payload + F16 scale/zp + **第二轴因子**，**无 outlier、无嵌套量化** | 本项目：`hadamard.cuh:30,57`、`sinkhorn.cuh:139-163`、`store.cuh:67-94,_token_scale` 字段 `kvarn.h:44-98`、`config.cuh:12`；beellama：`src/llama-kvarn.cpp:198,824-879,913-930,673-688`；华为：`vllm/.../kvarn/config.py:53-69`、`sinkhorn.py:92-96` |
| 与发布档的**外壳差异**（非算法差异）：本项目 dense-only、D=256 only、**K=V∈{4,5,6}**（无 36 组合、无 MLA、无 dim64 矩形/128×128 切片）；注意力**在旋转域且 Q 旋转无条件**（华为 dense 亦无条件；`use_kvarn_q_rot` 门控是 **beellama 独有**） | 本项目 `types.h:84-88`、`config.cuh:10-11`、`kvarn.h:12-13`、`decode.cu:55-57,83`；beellama `src/llama-graph.cpp:3690` |

---

## 3. 记录（追加式，正序）

> **压缩说明（2026-10-08）**：条目 **2026-10-07-1 … 2026-10-07-23**（源 260–1312 行，WP0–WP4 全过程）
> 已**逐字**移入 `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` §A。下表索引保留**条目号 + 原标题（逐字）**；
> 正文与计划书里的「`§3-2026-10-07-NN`」/「记录 07-NN」引用一律按本表解析。
> 本节正文以下只保留 **2026-10-08-1 … -10**。**追加新记录请接在最新一条之后，不要回填归档。**

| 条目号 | 原标题（逐字） | 全文 |
|---|---|---|
| 2026-10-07-1 | 侦察与路线确认 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-2 | WP0：基线构建发现先存缺陷（与本工作无关） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-3 | WP1：kvarn ops 原样移植，零告警编译通过 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-4 | WP1：测试移植与首跑（30 项中 1 项容差未过） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-5 | WP0.5-C：构型口径声明（D6 决议） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-6 | 建立本进度日志 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-7 | 信息落盘制度写入 AGENTS.md（仓库级） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-8 | WP0.5-A：MTP parity 测试移植完成；**实跑发现 A3 失败（非 kvarn 相关）** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-9 | A3 差分诊断（进行中）：上游 vs TAIL 的代码溯源 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-10 | A3 差分实测：**上游同样失败 ⇒ 缺陷先于 precision-tail 存在**（推翻 07-09 的假设） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-11 | A3 上游调研（GitHub + 三仓代码/文档）：**不是上游 bug，而是 Fork B 私有合同** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-12 | A3 定案：报告复核 + 计划调整（**回写计划书 v4**） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-13 | **WP2 完成**：页几何 + 存储枚举 + parser/名字/指纹；**261 测试全量回归 + 基线对照** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-14 | WP3 起手：模型接入（`kvarn:k4v2` **短上下文端到端跑通**；长上下文阻塞已精确定位） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-15 | WP0.5-B 起手：**首个 KVarN 准入 KLD 数据点（正向）**；但准入门被 **WP3 页几何**前置 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-16 | **WP3① 完成**：地址空间页几何按存储贯穿；`kvarn:k4v2` ctx8192 / 229,348 token 全量跑通 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-17 | WP0.5-B 补测启动：**229k 同字节臂缺失面**（结果未出） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-18 | WP0.5-B 补测**完成**：229k 同字节矩阵齐备 ⇒ 代理门禁**明确为正**（WP4 GO） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-19 | WP4 起手：位宽站点测绘（WP4.1）+ **记录布局泛化为函数**（WP4.2 第一步，构建绿） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-20 | WP4.2 主体完成：核按 `(kb,vb)` 模板化 + `bits` 贯穿到 launcher（构建绿） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-21 | WP4.2 端到端验收（测试扩 4/5/6 实跑全绿）+ WP4.4 parser 发档 + 两处规划期拒绝 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-22 | **WP4 收尾：正式 WP0.5-B 三档全过**（发布档准入达成）+ 测试补 5/6 位宽注意力覆盖 + 文档 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |
| 2026-10-07-23 | KVarN「本质」核对：报告描述 vs 本项目实现（三方审计，无代码改动） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) |


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

---

## 4. 下一步（按计划书 WP 顺序）

> **状态**：WP4 于 07-22 收尾、07-23 完成 KVarN 本质三方核对、08-01 完成全量 ctest 复核（§3-08-01）、
> **08-07 完成 P3c（kvarn parser 单测）**。**计划内的功能项与收尾项均已落地**；全量 ctest（261 项，08-01 那次）
> 已比对完毕（见 §4.0-P0），此后**未再重跑全量**（注册数已由 261 → **263**，见 §3-08-10）。

### 4.0 主线判定与推荐顺序

- **排序依据**：**功能缺口已清零**（续列尾/前缀复用、MTP 路径均已落地并验收）⇒ 余下为判据补全（已完成）>
  覆盖补强（残留）> 评估。
- **推荐顺序：P1 → P2 →（P3 可并行）→ P4 → P5(WP6)** —— **P1–P4 全部完成**，仅剩 **P5 = WP6**（门禁后）。
- **WP6 门禁**：**仅当 P1/P2 完成（✅ 已完成）且用户明确授权后才启动**（10–15 天、最高风险、计划书 §6 / §7-WP6）。
  **→ ✅ §6.3 探针已于 08-12 执行完**（计划书 D-19）：**(b) 否证**、**(a) 忠实**、**(c) 弱**、**新提 (c′)**。
  **→ ✅ 路线已于 08-13 裁决 = (a)**（用户确认；**(c′) 否决**）⇒ **WP6 已在计划书 §7-WP6 拆为 WP6.0–WP6.7**（含**两项前置测量入队**）。
  **剩余门禁**：① **两项前置测量（WP6.0a/0b）须先跑**（GPU，~0.5–1 天）；② A6/A7 脚手架（WP6.4）；③ 尾 dtype 待定（bf16 免 f16 旋转入口）。**工期不变（10–15 天）。**
- **P0（✅ 已完成 08-01）**：全量 `ctest`（带 artifact，261 项）**245 通过 / 7 跳过 / 9 失败**；9 项失败
  **逐项判定非 kvarn 引入**（2 先存 + 4 产物结构性 + 1 已知 A3 + 1 并发争抢 + 1 新观测的先存
  `vision_workspace`）。**`ninfer_kvarn_test` 已通过**。详见 §3-08-01。

### 4.1 任务清单（现行）

> P0–P5 各自的「目标 / 内容 / 验收 / 风险 / 估时」原文（源 1574–1607 行）**逐字见归档 §J**；
> 下表只留**未完成项的现行定义**与**已完成项的一句话结论**。

| 项 | 状态 | 剩余内容 / 已达验收 |
|---|---|---|
| P1 — WP3② 续列尾（前缀复用 / 检查点） | **完成**（08-02 实现 / 08-04 验收） | `p1_prefix_reuse.sh`：kvarn r2 `cached_tokens=851`、message 与 r1 **逐字节相同**（bf16 对照亦 851）；专项单测 host+device 全绿 |
| P2 — WP3④ MTP 激励 + A3 相对判据 | **完成**（08-04 激励 / 08-05 仪器） | 仪器下 **kvarn 自确定性全档成立**、分叉不劣于基线（`kvarn:k4v4`@k1/k3 identical；@k15 418/512 vs bf16 412/512，首分叉同 token 91）。**残留**：仅 `sample 0`、宽度 {0,1,3,15}、`k5v5/k6v6` 只测 k=1；测试名仍含 `parity`（未改名） |
| P3a — 229k 三档 ≥3 次重复 | **完成**（08-04） | 质量指标三重复**逐位相同（极差 0）**、与 D-11 单次值逐位相同；仅吞吐 ≤1.3% 散布 |
| **P3b — 报告名并入 `KvarnBits`** | **完成（08-06）** | `MemorySummary` 带 `KvarnBits`；文本面 `kvarn:k4v4`、报告目录分量 `kvarn-k4v4`；**六展示面 GPU 实测 + 6 项 host 回归全过**（§3-08-06、附录 D-14） |
| **P3c — kvarn parser 单测** | **完成（08-07）** | `ninfer_cli_options_test` / `ninfer_serve_options_test` 增 kvarn 用例（未决项 7）：三档解析（存储 + `KvarnBits`）、默认 `Bits4`、拒绝未发布拼写、help 三档、serve 侧 `make_engine_options()` 贯通；**两项 exit 0**。**第三处 parser（perplexity）结构性不可单测**（内联 `main.cpp`）⇒ 行为覆盖由 P3b e2e 提供 |
| **P3d — 审计 3 项 LOW 修复** | **完成（08-10）；① 的端到端对照已跑（08-11）** | ① CausalScore 路由补 `reset_kvarn_tail_row(0, …)`（**生产行为变更**；row 0 硬编码、生成路径之外无覆盖、标记运行期只追加）· ② `commit.cpp` 发布失败加 stderr 诊断（不再静默）· ③ `state_image.h` 注释补 KVarN；新增 `ninfer_qwen3_5_kvarn_tail_row_reset_test`（**实跑 PASS**，ctest 262→263）。**① 的执行证据（08-11，§3-08-11）**：`kvarn:k4v4` KLD 臂**逐位复现 `0.002120`**（mean/median/P99/max/same_top/dlogp/PPL 全部相同）⇒ 该协议上为**无害 no-op**、D-11/P3a **无需重测**；**但"必要性"仍无证据**（本协议窗口恒 `context` 长、页号每窗口归零 ⇒ 陈旧标记与当前页号重合；**变长窗口**的冲突未实测）。**不移植** FORK 的 `test_prefill_precision_real.cpp`（用户 08-11 裁决）；P3d 后 GPU 回归 3 项**全绿**（`ninfer_kvarn_test` 余量与 08-04 逐位同 / 续列尾 device 段 / `score_real` —— 后者用 `Fp8E4M3Row256`，**不覆盖**本改动路由） |
| P4a — WP5 容差形式化 | **完成**（08-03 形式化 / 08-04 实测） | 量化步长判据 + 4/5/6 位穷举往返；`flips` 1/65、`over_step=0` ⇒ 65× 余量、未放宽；口径写入 `op-development.md §6.3` |
| **P4b — kvarn bench 归属** | **待用户决定** | 需 `-DNINFER_BUILD_BENCHMARKS=ON` 重配，与 AGENTS.md「不要重配」冲突（未决项 3） |
| **P5 — WP6 旋转域尾部合并** | **路线已裁决 = (a)（08-13，用户确认）；未开工** | 10–15 天、最高风险。计划书 §7-WP6 已拆为 **WP6.0–WP6.7**：**0 前置测量**（0a 本机 prefill 噪声底、≥5 重复 · 0b 外部尾纯 kernel 代价、**关 MTP**、≥3 重复）→ **1** KVarN tail-partial（W1，**acc 旋一次** + FP64 oracle）→ **2 归并改造（W2，最高风险：`partial_acc` BF16→FP32 + 两段式；接受条件 = A6「`tail=0` 逐位不变」）** → **3** 分区与接线（W2b/W3–W6；解 `startup.cpp:1035-1045` 的 fail-fast；`--kv-tail-type` 禁止静默）→ **4** A6/A7 脚手架（W7）→ **5** 容量/显存 → **6** 质量/速度收口（**显式处理 A4 的 N>384 风险**）→ **7** 回归（含**新增 `--vision` × kvarn 功能门**）。**开工须用户明确授权** |
| 计划书回写 | **已完成**（v11→v13） | §1 A2、§7-WP3②④⑤、§7-WP5、附录 D-12/D-13 均已回写

### 4.2 已完成项（存档）
WP0.5-A 定案 / WP0.5-B（代理 + 正式）/ WP0.5-C / WP1 / WP2 / **WP3 全部（①·②·③·④·⑥）** / WP4 / WP5 全部。

### 4.3 不要做
不动 `.worktrees/{m5a,wp1,wp2,wp3}`；不重配 `build-port`（一律按目标构建）；**未经明确要求不提交**（用户约束；本轮用户已明确要求提交）。

> **订正（2026-10-08）**：上句的「不提交」是当时的用户约束；用户当日指示「先提交再继续进行」⇒ 08-03/08-04/08-05
> 的改动已提交（`fe76ad42` test / `6aea191b` docs）。**文档归档精简与 P3b 亦已于同日提交**
> （`6836b2de` feat / `3e780a41` docs / `efdb1dce` docs）。

### 4.4 历史 WP 状态（存档）

> 本节原文（源 1615–1648 行：WP0.5-A/B/C 与 WP1–WP4 当时的状态、剩余项，以及 FORK/TAIL 的三处 API 差异
> `KvarnK4V2Group128` / `mtp_draft_policy` vs `mtp_policy` / `enable_nvfp4_scale_compression`）**逐字见归档 §J**。
> 现行状态看 §0 现值表，判据看计划书 §1 与 §7。

---

## 5. 下一窗口起手提示（handoff，供直接粘贴）

> 本条为**新窗口冷启动**用。它自包含：权威文档路径、已定案事实（不要重新论证）、当前状态、待办队列、
> 环境纪律、建议的第一条命令。**最后更新 2026-10-08（本轮 08-13）。**

**任务**：在 `D:\ninfer\ninfer-precision-tail` 继续 KVarN 移植 —— **计划内的 kvarn 功能项与收尾项均已完成**；
**08-12 §6.3 探针已执行**（否证 (b)），**08-13 用户裁决 WP6 = 路线 (a)**，计划书 §7-WP6 已拆为 **WP6.0–WP6.7**（每步给产出/验收/回退）。
⇒ **本轮之后的唯一主线 = 按 WP6.0 → WP6.7 推进**；**开工须用户明确授权**。
其余：**A1 邻域的视觉功能门**与**两项前置测量**已入队；**覆盖残留**（低优先，非交付面）。
**仓外项已闭环**：子代理图谱「须重启」已证实、重启充分，且**已于 08-12 首次实战成功**（两个 Tier-2/3 探针）。

**先读（顺序）**：本文件 `kvarn-port-progress.md`（**§0 快照 / §3-2026-10-08-13（最新）/ §3-08-12 / §4.1 / §5**）
→ `kvarn-port-into-precision-tail-plan.md`（**版本头 v20**、§1 A2/A3/A8 + **A1 邻域的视觉口径**、§6.1/6.2/6.3、**§7-WP6（WP6.0–WP6.7 分步计划）**、§7-WP7、附录 D-11…**D-19**）。
二者是唯一权威；**冲突时以本文件的实测为准并回写计划书**（计划书 §0.5 规则）。

**当前状态（务必先核验，勿臆断）**
1. **GPU 空闲**（显存 0 MiB）；`ninfer_tests/ninfer/ninfer-serve/ninfer-perplexity` 均**最新构建绿**
   （08-10 构建：前者 234 步、后两者 7 步，均 exit 0；08-11 未改源码）。
2. 工作树：**08-07…08-10 的源码/测试/文档改动已全部提交**（`git log -1` = `78f183d2`）。
   **唯一未提交项**是 `AGENTS.md` 的**表格重排**（+ 一行中文汇报要求）—— **非本轮产生**，冷启动时会看到 `M AGENTS.md`，属预期。
3. **P3d ① 的端到端对照已完成（08-11）**：`kvarn:k4v4` KLD 臂 mean **0.002120**、全部质量指标与 D-11/P3a **逐位相同**
   ⇒ **无害 no-op**、**D-11/P3a 无需重测**；**但"必要性"无证据**（本协议窗口恒 `context` 长、页号每窗口归零 ⇒ 标记重合；
   变长窗口冲突未测，见 §3-08-11）。
4. **P3d 后 GPU 回归 3 项全绿**：`ninfer_kvarn_test`（余量与 08-04 逐位同）、`ninfer_qwen3_5_kvarn_continuation_image_test`（device 段）、
   `ninfer_qwen3_5_score_real_test`（**注**：该测试用 `Fp8E4M3Row256`，**非 KVarN** ⇒ **不覆盖**被改动的路由）。
5. **子代理图谱：已闭环**（4 份 `~/.qoder/agents/codebase-memory*.md` 的 MCP 名连字符→下划线；**重启后探针通过**，
   `list_projects` 返回 6 个图谱工程、`trace_path` 与主会话结果一致）⇒ 后续结构性/否定性结论**可正式交 `codebase-memory*` 子代理**；
   **但此前几轮子代理的此类结论一律按 grep-only 看待**（§0 未决项 11）。

**待办队列（按依赖序）**
1. **【已关闭】P3d ① 的端到端对照**（08-11）：KLD 逐位 `0.002120` ⇒ 无害 no-op；P3d 后 GPU 回归 3 项全绿。详见 §3-08-11。
2. **【已裁决，无需再问】待裁决 3 项**（用户 08-11 裁决）：(a) **不移植** FORK 的 `test_prefill_precision_real.cpp`；
   (b) **不补** `capture_identity_tag()` 的 `kvarn_bits`（保留注释）；(c) **kvarn bench 暂不纳入**（维持不重配）。
   **残留（未做）**：① 未构造**变长窗口**的 `Engine::score_tokens` 场景证明该修复**必要**；② KVarN + 打分路由**无直接门禁测试**
   （`score_real` 非 KVarN）——**若要补，这是最有价值的一项**（可用 `KvarnGroup128` 参数化 `test_engine_score_real.cpp` 或新写路由级测试）。
3. **【仓外，已闭环】** 子代理图谱：重启后探针通过，**08-12 已实战**（两个 Tier-2/3 探针成功：域契约 + 合并契约）
   ⇒ 结构性/否定性结论**可正式交 `codebase-memory*` 子代理**（**此前几轮**的此类结论一律按 grep-only 看待，§0 未决项 11）。
4. **覆盖残留（低优先，非交付面）**：仪器中 `k5v5/k6v6` 只测了 k=1；`sample 0` 之外未测；MTP 测试名仍含 `parity`（未改名）；
   **KVarN + 打分路由无直接门禁测试**（`ninfer_qwen3_5_score_real_test` 用 `Fp8E4M3Row256`、非 KVarN）。
5. **【主线】WP6 — 路线已裁决 = (a)（08-13，用户确认）；未开工。** 按计划书 §7-WP6 的 **WP6.0 → WP6.7** 推进：
   - **WP6.0（先做；GPU、~0.5–1 天）**：**0a 本机 prefill 噪声底**（同二进制/同 prompt/单请求、重复 **≥5**、报 pp 中位数+最差+极差）；
     **0b 现有外部尾的纯 kernel 代价**（**关 MTP**、`--kv-dtype rk4v4`（或 `bf16`）× `--kv-tail-tokens {0,1024}`、decode-only、重复 **≥3**、报 tg 分布）
     ⇒ **把 A8 的 `−5.8% / −2.1pt` 分解为「纯 kernel」+「接受率」**；结论回写 A1/A8/D-19。
   - **WP6.1** tail-partial（W1，**acc 旋一次** + FP64 oracle）→ **WP6.2 归并改造（W2，最高风险）**：`partial_acc` **BF16→FP32** + `reduce_output_hadamard_kernel` 两段式，
     **接受条件 = A6「`tail=0` 逐位不变」**（失败则退回"独立 merge 核"）→ **WP6.3** 分区与接线（W2b/W3–W6；解 `startup.cpp:1035-1045` 的 fail-fast；**f16 禁止静默**）
     → **WP6.4** A6/A7 脚手架（W7）→ **WP6.5** 容量/显存 → **WP6.6** 质量/速度收口（**显式处理 A4 的 N>384 风险：至少测 `N ∈ {384,1024}`**）→ **WP6.7** 回归（含 **`--vision` × kvarn 功能门**）。
   - **待定**：尾 dtype —— **bf16** 免"f16 旋转入口"交付物（用户已接受"若采 bf16 则本机复核"）vs **f16**（须新增该入口）。
6. **已关闭 / 已裁决（勿再问）**：P3d ① 端到端对照（08-11）· 待裁决 3 项（08-11：不移植 FORK 测试 / 不补 tag / bench 暂不纳入）·
   **视觉只作功能判据**（08-12）· **接受"若采 bf16 尾则本机复核"**（08-12）· **WP6 路线 = (a)**（08-13）。
7. **计划书侧**：**v20 已回写**（本轮：**版本头 v20**、§6.3 裁决行、**§7-WP6 整段改写为分步计划 WP6.0–WP6.7**、§7-WP7 范围订正注、**D-19 ⑧/⑨**、归档索引 D-19 行）。

**08-03 的两个非显然发现（勿丢）**
- **`reset_kvarn_tail_row` 移植缺口**：TAIL **零调用者**；FORK 有 **2 处**，都在
  `D:\ninfer\ninfer-rtx5090-mobile\tests\models\qwen3_5\test_prefill_precision_real.cpp:236,284`，
  **该测试未随移植进入 TAIL**。⇒ 既属"陈旧 marker"隐患（行重用若不由 store op 重写 markers），
  也是**移植遗漏的测试覆盖**。**⚠ 原审计写"两树均无调用者"不准确**——由 codebase-memory 图谱 + grep 复核修正（§3-08-03）。
  **现状（08-11）**：TAIL 现为 **1 个生产调用点**（`program_impl.cpp:669`，P3d）+ 新测试；图谱 `callers_total=4`（§3-08-11(3)）。
- **host 镜像偏移 ≠ device 区域偏移**：`state_image` 的 host 布局与 device 布局由**两套独立 `LayoutBuilder`** 产生，
  仅**单槽**字节量相同（host 镜像装单槽）；ctor 以「由 device 组件重建 host 布局并比对」兜底
  （`state_image.cpp:459`）。改续列镜像时**勿假设偏移相同**。

**环境与纪律**
- **不要重配 `build-port`**；**按目标构建**（`ninfer_ops ninfer_tests ninfer ninfer-serve ninfer-perplexity`）。
  **禁止全树构建**：整树构建本就有**先存缺陷**（`ninfer-multi-gpu-probe` 的 `LNK2019`，见 §3-2026-10-07-2 / §0 未决项 4）
  ⇒ 一律按目标构建。
  ```bash
  cmd //c "call D:\ninfer\ninfer-precision-tail\.deps\env-port.bat && cmake --build D:\ninfer\ninfer-precision-tail\build-port --target ninfer_tests -j 8" > /tmp/build.log 2>&1
  grep -a "error C[0-9]\|error LNK\|FAILED:" /tmp/build.log   # 诊断是 GBK
  ```
- **不要从 Git Bash 内联 vcvars**；`cmd //c` **不要**与 `MSYS_NO_PATHCONV=1` 同用（后者会让 `//c` 不被 cmd 识别，
  变成交互式 cmd 而静默什么也不做）；给 Windows 原生 exe 传路径时才加 `MSYS_NO_PATHCONV=1`。
- 只跑 host 段/无设备测试时用 `CUDA_VISIBLE_DEVICES=99`（强制 0 设备 ⇒ 不掉显存）。
- 模型（唯一，勿 glob）：`D:/ninfer/ninfer-precision-tail-package/model/Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`。
- **16 GB 显存**：跑前 `nvidia-smi` 确认只有 1 个计算进程；perplexity 走 default `--cuda-memory-policy`，实测峰值 11.7 GiB。
- **未经明确要求不要提交**（用户约束；本轮 2026-10-08 用户已明确要求提交，故工作树现干净）。不要动 `.worktrees/{m5a,wp1,wp2,wp3}`（与 kvarn 无关）与 `.deps/`（gitignore）。
- 每次推进**追加**本文件 §3、WP 边界更新 §0 快照、把影响验收/未决项/风险的结论**回写计划书**。
- 结构性/否定性问题（"是否漏了调用点""是否未被使用"）**必须**走图谱 —— **主会话**的 `trace_path` inbound，或**修好之后的**
  `codebase-memory*` 子代理；**不要**交给通用 Explore 类子代理（无 MCP）。
- **⚠ 子代理图谱**尚未确认可用（§0 未决项 11）：4 份 `~/.qoder/agents/codebase-memory*.md` 的 MCP 工具名曾被写成**连字符**
  （运行时实为**下划线**）⇒ 子代理**只拿到 `Read/Grep/Glob`、图谱调用恒为 0**（含**上一会话**）。定义**已改**（备份 `/tmp/agents-backup-20261008/`），
  但 `[AgentListingDelta] isInitial=true` **只在会话启动装载一次** ⇒ **须重启 Qoder**。**重启确认之前，任何子代理的结构性结论一律按 grep-only 处理。**

**建议第一条命令**
```bash
cd /d/ninfer/ninfer-precision-tail && git status --short && git log --oneline -1 && nvidia-smi --query-gpu=memory.used --format=csv,noheader
```
若 GPU 0 MiB ⇒ 可直接跑。**待办 1–3 均已关闭**（① 的端到端对照已跑；3 项待裁决已由用户定；子代理图谱已闭环并**已于 08-12 实战成功**），
**08-12 已完成 §6.3 探针**，**08-13 已裁决 WP6 = (a) 且计划书已拆出 WP6.0–WP6.7** ⇒ 起步建议：
① **先跑 WP6.0 的两项前置测量**（`WP6.0a` 本机 prefill 噪声底 ≥5 重复；`WP6.0b` 外部尾纯 kernel 代价、**关 MTP**、≥3 重复）——
   **它们是 A1/A8 的判据来源，必须在写代码之前跑**（避免事后重新解释判据）；命令与验收见计划书 §7-WP6.0；
② 再按 **WP6.1 → WP6.2 → …** 推进（**WP6.2 是唯一高回归项，用 A6「`tail=0` 逐位不变」守住**）；
③ 若要**先做低风险增量**：可给「KVarN + 打分路由」补一个直接门禁测试（`score_real` 用的是 `Fp8E4M3Row256`、**非** KVarN；可顺带构造**变长窗口**以证明 P3d 修复的必要性，§3-08-11 不利面）。
**WP6 开工仍需用户明确授权。**

**codebase-memory 图谱（2026-10-08 起可用，勿再被 SessionStart hook 误导）**：本仓已被索引
（`D-ninfer-ninfer-precision-tail`，**45,944 节点 / 216,668 边**；**08-11 重跑 `index_repository(mode=full)`，已含 P3b/P3c/P3d**；
08-08 时为 45,923 / 216,539）。注意：**SessionStart hook 仍报 "no indexed graph project
matched this working directory"（陈旧的误报）**——直接 `list_projects` 即可看到本仓。使用纪律：
依赖某文件前先 `check_index_coverage`（`index_status` 报 `parse_partial 253` / `not_indexed 79` /
`parse_unusable 7`（多为 `third_party/`），CUDA `.cu/.cuh` 可能在 partial 列表内 ⇒ **miss 的行直接读源码**）；
**callable** 的否定性/完备性问题（"谁调用 X""X 是否未被使用"）**必须**走 `trace_path` inbound，
**不要**交给无 MCP 的 Explore 子代理。
**两条本轮实测的工具边界（勿踩，详见 §3-08-08、已回写 `AGENTS.md`）**：
① `trace_path` 对 **field** QN **恒返回 `callers_total: 0`**（即使 USAGE/WRITES 边存在）⇒ 域消费方必须走
`query_graph` 的 `USAGE`/`WRITES`；② 那些 `USAGE`/`WRITES` 边**按名解析**（混同同名域、无访问点行号）
⇒ 域级结论只作**指示性**，须与 grep 对拍。**本轮实测结论：`KvarnBits` 的消费方集合图谱与 grep 一致
（0 个图谱独有文件）**；`activate_sequence_kvarn_tail` 的 direct 调用者 1 个（`prefill.cpp:661`，其余为 hop≥2 的传递祖先，切勿当调用点）。
**08-11 更新**：`reset_kvarn_tail_row` 现为 **`callers_total=4`**（**1 个生产调用点** `ProgramImpl::causal_score` + 3 个测试 hop）；
08-08 记的 `callers_total=0` 是**索引陈旧**（索引 04:35 早于 P3d 提交 05:12），**已失效**。⇒ 引用图谱前**先看 `indexed_at` 与工作树 `HEAD` 的关系**。
---

## 6. 已归档信息索引（2026-10-08）

本日志精简时**只搬运、不改写**：下列内容全部逐字（字节级）移入归档文件，正文各处只留一行索引/指针。
§1 环境与构建速查、§2 关键路径与事实索引、§3 的 2026-10-08-1 … -13、§4.0/4.2/4.3、§5 handoff **保留在正文**。

| 归档文件 | 覆盖范围（本文件源行号） | 一句话内容 | 正文索引位置 |
|---|---|---|---|
| `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` §A | §3 条目 **2026-10-07-1 … -23**（260–1312） | WP0 基线、WP1 ops/测试、A3 的四轮追查、WP2/WP3/WP4 全过程与 229k 准入实验的逐日记录（含失败与被推翻的假设） | §3 开头的条目索引表（23 行，条目号 + 原标题逐字） |
| 同上 §B | §0「最后更新」散文段（16–34） | 08-05 / 08-04 / 08-03 三轮的实测摘要 | §0 压缩说明 |
| 同上 §C | §0 WP 状态明细表（36–47） | 每包当时的详表状态与实测数字 | §0 现值表（已压缩为现值 + 指针） |
| 同上 §D | §0 工作树改动清单（49–131） | WP1–WP5/P1 的逐文件改动（函数、守卫点、switch 位置） | §0 的「工作树改动清单」指针 |
| 同上 §E–§I | §0 未决项 1 / 5 / 8 / 9 / 10（135–142 / 146–167 / 177–192） | 已关闭或已被取代的未决项原文（含 A3 定案依据、页几何根因与修法、续列尾「未实现」旧表述） | §0 未决项列表中对应编号的索引行 |
| 同上 §J | §4.1 任务清单（1574–1607）与 §4.4 历史 WP 状态（1615–1648） | P0–P5 的原始「目标/内容/验收/风险/估时」与当时的 WP 状态 | §4.1 现行表 + §4.4 指针 |
| 同上 §K | 正文里被**订正**的 5 行原句（计划书原 800 行；本日志原 6、1669–1671 行） | 订正前的原文（含 fenced 逐字节副本）：附录 D 的旧标题、「配套权威（方案 v3）」、「工作树**未提交**」三行 | §4.3 的订正注 + §5 当前状态第 2 条 |

**未归档（现行推进计划需要）**：§0 现值表、仍开放的未决项 2 / 4 / 6 / 7 / 12 / 13、§1 全部、§2 全部、
§3 的 2026-10-08-1 … -13、§4.0/4.1/4.2/4.3、§5。计划书侧的对应归档见
`docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md` 与 `docs/port-records/KVARN-PLAN-CHANGELOG-ARCHIVE.md`。
**§0.5 记录规则不变**：WP 边界仍须「快照 + 日期条目 + 回写计划」，检索路径为 §3 索引表 → 归档 §A。
