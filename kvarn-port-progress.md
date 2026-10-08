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
>
> **第二轮压缩（2026-10-08，见 §3-2026-10-08-20）**：§3 的 **08-1…-13 与 -17** 已逐字移入归档 **§L**，
> 计划书附录 **D-10…D-19** 移入 `KVARN-PLAN-APPENDIX-ARCHIVE.md`。**本节现值表数字未动**；
> 表内指向「附录 D-10/D-11」等的格子现按**归档 + 索引表**解析（检索路径：§3 条目索引表 / 附录 D 的「D-10…D-19 索引」表）。

| 项 | 现值 | 详见 |
|---|---|---|
| WP0 基线与环境 | 环境/构建已核实；整树构建**不通过**（先存 `LNK2019`，与本工作无关）⇒ 按目标构建；模型冒烟 + KLD 基线**未做** | 附录 D-2、§3-2026-10-07-2、归档 §C |
| WP0.5-A MTP 一致性仪器 | **定案 + 仪器改造完成（08-05）**：A3 按 O1 替换（非本移植引入、非上游缺陷，属 Fork B 私有合同）；仪器去逐位门禁 + 自确定性门禁 + 首分叉报告 + 并入 `rk4v4`/`kvarn` 档 + `--quick` | 附录 D-6/D-13、§3-2026-10-08-5、归档 §C |
| WP0.5-B 准入实验 | **完成**（07-18 代理档 / 07-22 正式档）：代理门禁同字节 **4.15×** 为正 ⇒ WP4 GO；发布档三档全过 —— `k4v4`(274 B) **0.002120**、`k5v5`(338 B) **0.001432**、`k6v6`(402 B) **0.001233**，对 `rk4v4`/`nvfp4`/`k8v4` = 2.09×/2.07×/2.18×；`k6v6` 门槛 < **0.002688** | 附录 D-10/D-11（**全文已归档**，见附录归档 + 「D-10…D-19 索引」表）、§3-2026-10-07-18/-22 |
| WP0.5-C 构型口径 | **完成**：本机为 **native 口径**（`NINFER_SM120_NATIVE=ON`）；compat 口径噪声底未测（未决项 2） | 附录 D-1、§3-2026-10-07-5 |
| WP1 kvarn ops 移植 | **完成**：ops 零告警编译；`ninfer_kvarn_test` 容差项由 WP5 收口（08-04 余量 65×）；kvarn bench **未做**（需重配，未决项 3） | 附录 D-3、§3-2026-10-07-3/-4、归档 §C |
| WP2 页几何 + 存储枚举 | **完成**：`KvarnGroup128` + `KvarnBits` + 三处 parser + 6 处名字 switch + 指纹 `;kvbn=` + 校验放宽 64\|128。回归：合成 242/245、真实模型 10/16，**失败均非本移植引入**（`stash` 重建基线对照）。**残余（parser 单测）已由 P3c 关闭（08-07）**：cli/serve 两处 parser 单测落地全过 | 附录 D-7、§3-2026-10-07-13、§3-2026-10-08-7 |
| WP3 模型接入 | **完成（08-05）**：① 地址空间页几何（07-16，ctx8192/229,348 token 跑通）② 续列尾（08-02 实现；08-04 host+device 单测全绿 + Engine 级 e2e `cached_tokens=851`、message 逐字节同）③ `--mtp-attention-window` 规划期拒绝（07-21）④ MTP 激励（08-04 `mtp accepted 84/113`、1.50× ≈ bf16 1.52×）+ 仪器（08-05）⑤ 措辞订正 ⑥ `--help`/docs（07-22）。08-03 独立审计：**无 HIGH/MED**（3 项 LOW 为 FORK 继承） | 附录 D-9/D-12/D-13、§3-2026-10-08-1…-5 |
| WP4 位宽参数化 K=V∈{4,5,6} | **完成（07-22）**：核按 `(KBits,VBits)` 模板化（发布档只实例化 `(b,b)`）+ `bits` 贯穿 + 测试扩 4/5/6 实跑全绿 + parser 发三档并删 `Bits2`/`k4v2` + 规划期拒绝（tail / mtp-window）+ 三档准入全过。三档单次测量的口径已由 08-04 的 P3a **3× 重复**收口（质量指标逐位相同、极差 0） | 附录 D-11/D-12、§3-2026-10-07-19…07-22 |
| WP5 oracle 与容差 | **完成（08-04）**：量化步长判据（点值 ≤ `q*(1+5e-2)` + `flips ≤ 1.0e-3*total` + `\|Δcode\|>1` 零容忍）+ 4/5/6 位穷举逐码往返；**实测余量**：`flips` 最大 **1/65**、`over_step=0`、`wide_flips=0` ⇒ **65× 余量、判据未放宽**。口径已回写 `docs/maintainer/op-development.md §6.3` | §3-2026-10-08-3/-4、§1 A2 |
| WP6–WP9 | **WP6 路线已裁决 = (a)**（08-13，用户确认；**(c′) 否决**）⇒ 计划书 §7-WP6 已拆为 **WP6.0–WP6.7**；**WP6.0 两项前置测量已完成**（08-14：0a 短 prompt 相对极差 **26.5%** / 1073-token **3.1%**；0b 纯 kernel 代价 `rk4v4` **−1.61%** / `bf16` **−1.33%**）；**WP6.1 已落地（08-15）：旋转域 tail-partial + FP64 oracle，6 用例全过、余量 ~2000×（文件见 §3-08-15）**，**但尚未接线（WP6.3）**；**08-16 先存偶发（width=16 非确定性）已定位并修复**（`reduce_output_hadamard_kernel` 缺 2 屏障；软件缺陷；**隔离 150/150 + 全量 6/6 + racecheck 0 hazard**）⇒ **A2/A6 逐位判据恢复可用**；**08-17 尾环 dtype 分析完成**（环源 BF16 ⇒ f16 范围内逐位等价、精度收益为零；唯一差异是 TAIL 的 `p_s`；实测 PPL 相对差 −8.8e-6、显存逐字相同）⇒ **建议维持默认 f16，待用户裁决**；**08-18 WP6.2 落地**（`partial_acc` BF16→FP32；A6/A2/续列尾 e2e 全过；A6 口径澄清 = 同版本内比较）；**08-18 尾 dtype 裁决 = 维持 f16**；**08-19 WP6.3 落地**（op 内 `body_window = window − N` 三分区 + 3 入口带尾视图 + 尾环写入接入 append + 解 `startup.cpp` fail-fast；**e2e 四臂 + MTP + workspace 990.0/990.0 MiB 无溢出 + 续列尾 `cached_tokens=851` 全过**；**并修复 2 个 WP6.1 batch 偏移 + 1 个 WP6.1 查询域缺陷**；**测试端 2 处 oracle 已修并复跑全绿**）。**工期 10–15 天** | 计划书 §6/§6.3、§7-WP6、附录 **D-19 ⑧·⑨ / D-22 / D-23 / D-24**、§3-2026-10-08-12/-13/**-14/-15/-16/-17/-18/-19** |

**最近各轮**（原标「最近五轮」，实际已列到 08-18，标签失效故改）：08-06 P3b 档位并入名字（**六展示面 GPU 实测全过**）+ 图谱使用纪律落盘（AGENTS.md + skill）
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
→ **08-14 WP6.0 两项前置测量已执行**（GPU，`ninfer.exe` 单请求 ×7/×3 重复，**未改任何源码**）：0a = 短 prompt pp 相对极差 **26.5%**、1073-token **3.1%**（确认 A1 删 prefill 腿正确；WP0 产出 ② 补齐）；0b = 外部尾**纯 kernel** decode 代价 `rk4v4` **−1.61%** / `bf16` **−1.33%**（tg 极差 ≤0.15%）⇒ **A8 的 −5.8% 分解为「纯 kernel ≈ −1.5%」+「接受率联动 ≈ −4.3%」**。全文见 §3-08-14。
→ **08-15 WP6.1 落地**：旋转域 tail-partial（`src/ops/kvarn/tail_partial.{cuh,h,cu}`）+ FP64 oracle，**6 用例全过、余量 ~2000×**；推论「本路线不旋尾行 ⇒ f16 尾无需新增 f16 旋转入口」。**并发现先存偶发**（width=16 非确定，已用 HEAD 基线证明非 WP6.1 引入，根因未定位）。全文见 §3-08-15。
→ **08-16 先存偶发已定位并修复**：`racecheck` 定点报出 `reduce_output_hadamard_kernel`（`decode_kernel.cuh:1089/1104/1110/1134`）两组 shared-memory hazard ⇒ 根因 = `stage[0]` 暂存复用缺 2 个 `__syncthreads()`；**判定 = 软件缺陷（非硬件）**；补 2 屏障后 **隔离 150/150 + 全量 6/6 + racecheck 0 hazard**（修前 13/150、20次4败、6 errors）⇒ **A2/A6 逐位判据恢复为单次可判**，且该修复正落在 WP6.2 的改动面上。**同轮按用户授权为 `AGENTS.md` 追加图谱门禁三条 + 「图谱不覆盖 kernel 内部」+ 每 WP 边界 `index_repository`**（预存未提交改动原样保留）。全文见 §3-08-16。
→ **08-17 尾环 dtype 量化分析**（用户要求"依据要量化"）：查清环源是 **BF16**（`kv_cache/append/kernel.cuh:104`）⇒ f16 是重编码，`[2^-14,65504]` 内**逐位等于** bf16（12M 样本探针 ≥99.9995% 精确，误差 ≤2^-25），**精度收益为零、只多 `inf` 失效模式**；dtype 的**唯一取值差异**是现有 TAIL 的 `p_s` 算子精度（**对 KVarN 不存在**）。实测（`rk4v4`+N1024+`--score-width 8`，261,223 token）：PPL bf16 **4.892212** vs f16 **4.892169**（相对 **−8.8e-6**，47/47 检查点不同但方向混合）、显存**逐字相同**。**建议维持默认 f16（待裁决）**。全文见 §3-08-17 与计划书 D-22。
→ **08-18 尾环 dtype 裁决 = 维持默认 f16**（用户确认，零 diff）：`kv_tail_type = Float16` 不动；理由 = f16 无实测劣势（|相对 PPL 差| ≤8.8e-6）、为现有 TAIL 保留 11 位的 `p`、`|v|>65504` 失效模式实测不可达。回写计划书 §7-WP6 待定项、D-22(7) 与未决项 15。
→ **08-18 WP6.2 落地**（`partial_acc` **BF16→FP32**，3 文件 / 源码 5 处 + 删除 BF16 tile 暂存改 `float2` 直存）：**A6/A2/续列尾 e2e 全过** —— `ninfer_kvarn_test` **3 连跑逐位确定**、**codec 余量与 08-04 逐字相同**、6 个 FP64 尾 oracle 用例全过、`limit=0` 路线间逐位比对全过；e2e **RESULT: PASS**（`cached_tokens=851`、message 逐字节同 r1，与 08-04 同值）。**改前/改后量化**：decode 路线用例 vs FP64 oracle 的 `relative_l2` **一致降 6.7–10.9%**（如 random packed 0.00457711→**0.00425809**），而走 prompt 路线的 `tiled`×2 与 `slab-boundary` **逐位不变** ⇒ 改动面恰好限于 decode 路线、且为**数值改善**。**同轮修正一处自引入越界写**（打包分支第二笔漏 `head_valid` 守卫，H24 下 `gid∈{6,7}` 越出 head 区间，14 处失败 → 已修）。**⚠ A6 口径**：本步**按计划要求**使 tail=0 输出**不再与改前二进制逐字节相同**（`BF16→FP32` 的意图）；A6 点名的两个载体都是**同版本内**比较 ⇒ 按"同版本内逐位判据成立"理解，跨版本字节同一性按构造不成立。全文见 §3-08-18 与计划书 D-23。
→ **08-19 WP6.3 落地**（分区与接线，源码 7 文件 + 测试）：op 内 `body_window = window − N` 三分区、`KvarnPagedBatchLayerView.tail`、`stage_exact_tail` 接入 `kvarn_attention`/`kvarn_kv_append` 的 `rotate_kv` **之前**、`startup.cpp` 解 tail fail-fast。**功能验收全过**（e2e 四臂 exit 0 / 尾环 f16≡bf16 逐字节 / tail=0 有别 / workspace **990.0 MiB 逐字节同** / 环增长 **+68.0 MiB = 17×16×256 KiB** 精确 / MTP+尾跑通 / 续列尾 `cached_tokens=851` PASS）。**修 3 个潜伏缺陷**：`partial_acc/m/l` 缺 batch 偏移、查询缺 batch 列偏移（均 WP6.1 遗留）、**查询域缺陷**（尾核须先 `hadamard_warp` 反旋查询；直测 `query_buffer_moved=6.328`、`vs_rotation=0.000e+00`）。**裁决一处 oracle 错**：tail-only 的 op 级输出 = `acc_orig/l`（无净 Hadamard）⇒ `run_exact_tail_merged_case` 的期望须去 W。**测试端 2 处 oracle 已修并复跑全绿**（`ninfer_kvarn_test` exit 0：6 直测 **8.0e-7…1.42e-6**、2 启动 **1.33e-6 / 2.14e-6**、2 op 级合并 **3.161e-3 / 3.229e-3**；同族两测试亦 OK）。全文见 §3-08-19。

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
    **→ 08-14 部分回填**：① 的噪声底已实测（短 prompt 26.5% / 1073-token 3.1%，§3-08-14）。
14. ~~**⚠ `ninfer_kvarn_test` 的 width=16 非确定性**~~ → **✅ 已关闭（2026-10-08-16）：根因 = `reduce_output_hadamard_kernel` 的 `stage[0]` 共享暂存复用缺 2 个 `__syncthreads()`；判定 = 软件缺陷（非硬件）；已修复**。`racecheck` 定点报出 `:1089`读/`:1104`写 与 `:1110`读/`:1134`写 两组 shared-memory hazard；补屏障后 **隔离 150/150、全量 6/6、racecheck 0 hazard**（修前 13/150、20次4败、6 errors）。**A2/A6 的"逐位"判据恢复为单次可判**。原文（含「判定倾向 = 软件竞态」与三条已排查项）见 §3-08-15(4)；全过程见 §3-08-16。
15. ~~**尾环 dtype（bf16 vs f16）待用户裁决（08-17 新开，分析已完成）**~~ → **✅ 已裁决（2026-10-08，用户确认）：维持默认 f16**（`kv_tail_type = Float16` 不动，零 diff）。裁决依据 = 环源是 **BF16** ⇒ f16 是重编码、**范围内逐位等价、精度收益为零**，只多一个 `|v|>65504 → inf` 失效模式；dtype 的**唯一取值差异**来自现有 TAIL 路径 `p_s` 的算子精度（bf16 8 位 vs f16 11 位，**该效应对 KVarN 不存在**）；实测 PPL 相对差 **−8.8e-6**、显存**逐字相同**。原文（含 12M 样本 bit-exact 探针与 A/B 全表）见 §3-08-17 与计划书 **D-22 / §7-WP6 待定项**。**该裁决不改变 WP6.2–WP6.7 的任何交付**（dtype 只是环编码；KVarN 侧两条 dtype 在范围内取值相同）。

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
> 本节正文以下只保留 **2026-10-08-14 / -15 / -16 / -18 / -19**；`2026-10-08-1 … -13` 与 `-17` **亦已逐字归档**（见上表与归档 §L）。**追加新记录请接在最新一条之后，不要回填归档。**

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
| 2026-10-08-1 | 全量 `ctest`（261 项，**带 artifact**）复核：245 通过 / 7 跳过 / 9 失败；**9 项全部非 kvarn 引入**（含 1 项**从未取过基线**的新观测） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-2 | WP3② 续列尾**实现落地**（构建绿；e2e 验收待 GPU 空出）+ P3a 并行开跑 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-3 | WP5 容差形式化（构建绿；GPU 侧未跑）+ P1 续列尾专项单测（host 段实跑绿 / device 段待 GPU）+ P1 移植独立审计（无 HIGH/MED） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-4 | GPU 收尾四件（①P3a 9 臂 / ②P1 e2e **通过** / ③WP5 余量**实测** / ④续列尾 device 段**首跑 FAIL→定位为测试自身 bug→修正 PASS**）+ P2 前置：kvarn MTP provisional 路径**首次激励** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-5 | P2b 完成：MTP parity 测试改造为**诊断仪器**（去逐位门禁、加自确定性门禁 + 首分叉报告，并入 kvarn/rk4v4 档）；**kvarn 在 token 级仪器下自确定性成立、分叉不劣于基线** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-6 | P3b 完成：kvarn 档位并入报告目录 / 日志名（`MemorySummary` + 四展示面）；**六展示面 GPU 实测 + 6 项 host 回归全过**；顺带由**图谱**发现一处**潜在（非可达）**身份缺陷；并落盘「图谱使用纪律」（AGENTS.md + skill） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-7 | P3c 完成：kvarn parser 单测（`ninfer_cli_options_test` / `ninfer_serve_options_test`）；三档解析 + 未发布拼写拒绝 + Engine 贯通全过；**第三处 parser（perplexity）仍无单测**（结构性不可测，行为覆盖由 P3b e2e 提供） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-8 | 图谱复核（实测，**无源码改动**）：`KvarnBits` 消费方集合与 grep **一致（图谱 0 独有文件）**；`reset_kvarn_tail_row` **callers=0 复现**；并实测两条**工具边界**（field 的 `trace_path` 恒 0；`USAGE`/`WRITES` 按名解析会混同同名域、且无访问点行号）⇒ 已回写 `AGENTS.md` 纪律 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-9 | 订正（读源码实测）：审计 LOW② 的「capture/activate 抛错被 `catch(...)` 静默化」**只对 commit 侧成立**；并补记 `reset_kvarn_tail_row` 的标记语义与 FORK 测试的真实用途（**无源码改动**） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-10 | 审计 3 项 LOW **修复落地**（用户批准）：① CausalScore 路由补 `reset_kvarn_tail_row`（**生产行为变更**）· ② commit 发布失败不再静默 · ③ `state_image.h` 注释补 KVarN；新增专项单测（**ctest 262 → 263，实跑 PASS**）；**GPU 侧回归未跑**（用户指示） | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-11 | P3d ① 端到端对照**已跑**（`kvarn:k4v4` KLD **逐位复现 `0.002120`** ⇒ 该协议上属**无害 no-op**，D-11/P3a 数值**无需重测**）；P3d 后 GPU 回归 3 项**全绿**；并定位「子代理无图谱」根因（4 份代理定义的 MCP 工具名用**连字符**、运行时归一为**下划线**）——**修复已落盘，但本会话仍无效（定义在会话启动时装载一次，须重启 Qoder）** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-12 | **§6.3 探针已执行**（用户授权「允许使用探针」）：**(b) 首选路线否证** · **(a) 为忠实路线** · **(c) 弱且语义不符** · **新提有界 (c′)**；并产出用户三项终态目标 → A1–A8 的映射与缺口 | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-13 | **路线裁决 = (a)**（用户确认）+ **WP6 拆为 WP6.0–WP6.7** + **两项前置测量入队**；计划书 → **v20** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |
| 2026-10-08-17 | **尾环 dtype（bf16 vs f16）量化分析**（用户要求"依据要量化"）：环源是 **BF16** ⇒ f16 是**重编码**（范围内逐位等价）；**唯一取值差异来自 `p_s` 的算子精度**；实测 PPL 相对差 **−8.8e-6**、显存**逐字相同**；**建议维持默认 f16（待裁决）** | [归档](docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md) §L |


### 2026-10-08-14 — **WP6.0 两项前置测量已执行**（用户授权的 WP6 起手）：**0a 本机 prefill 噪声底**（短 prompt 相对极差 **26.5%** / 1073-token prompt **3.1%**）+ **0b 外部尾纯 kernel 代价**（MTP-off decode `rk4v4` **−1.61%**、`bf16` **−1.33%**，3 重复极差 ≤0.15%）⇒ **把 A8 记录的 −5.8%（带 MTP）分解为「纯 kernel ≈ −1.5%」+「接受率联动 ≈ −4.3%」**

**触发**：用户授权 WP6 起手，且指定 **"先跑 WP6.0a + 0b 并落盘（判据来源，必须在写代码前跑）"**。跑前 `nvidia-smi` = **0 MiB**、无计算进程。

**(0) 方法与工具（可复现）**：新脚本 `.deps/kvarn-adm/wp60_measure.sh`（gitignored），**唯一二进制** `build-port/apps/ninfer.exe`
（`build v0.6.0-rtx3090-1354-gaa533d02-dirty`，mtime 08 05:09），唯一产物
`D:/ninfer/ninfer-precision-tail-package/model/Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`；`--max-context 4096 --greedy`；
**单请求 = 一个全新进程**（每次重新加载 9.39 GiB 权重 + CUDA graph）；指标取自 CLI 生成摘要的 `prefill speed` / `decode speed`
（`apps/cli/main.cpp:220-223`，= `prompt_tokens/prefill_seconds` 与 `(generated-1)/decode_seconds`）。
两个 prompt：**短** = `"Explain how transformer attention works, step by step, with an example."`（**67 token**）；
**长** = 30× 一句英文观测方法论合成 ASCII 文本（**1073 token**；用 ASCII 是因为 `head -c` 截 UTF-8 源文本会在码点中间断开，
chat template 直接报 `invalid UTF-8 in Jinja string`）。
**已知口径限制（如实）**：`format_pretty_rate` 对 ≥1000 的值降到 `x.yzk`（打印"1.60k"）⇒ **长 prompt 的 pp 有 10 tok/s 显示量化**；
tg 在 68 附近为 `68.3`（0.1 分辨率）。原始 stdout/stderr 全部留档（`wp60/*.out|*.err`）与驱动日志 `wp60/driver.log`。

**(1) 0a 本机 prefill 噪声底 —— 单请求 prefill 不可用（短），1073 token 仍留 ~3% 底**

| prompt | 重复 | pp（tok/s，逐次） | 中位数 | 最差（min） | 极差 | **相对极差** |
|---|---|---|---|---|---|---|
| 短 **67 tok** | 7 | 194.3 / 224.6 / 222.4 / 241.1 / 255.5 / 249.5 / 231.0 | **231.0** | **194.3** | 61.2 | **26.5%** |
| 长 **1073 tok** | 7 | 1600 / 1560 / 1590 / 1550 / 1600 / 1600 / 1600 | **1600** | **1550** | 50 | **3.1%** |

- **短 prompt 结论**：**26.5% 的相对极差**逐字复现了 `docs/port-records/PORT-MEMORY.md §5.17(2)` 的「同一二进制单请求 prefill `−29%…+8%`」⇒
  这正是 **A1 已删除 prefill 腿**的依据，**本机实测确认该删除正确**（噪声底远大于任何要检测的效应量）。
- **长 prompt 结论（新数据）**：prompt 拉长到 1073 token 后极差收窄到 **3.1%**（最差 = 中位数 −3.1%）⇒ 机制与文档一致
  （单请求 prefill 由一次性分配/首次触页/驱动超额提交主导，prompt 长到足以摊薄启动项后噪声下降）。
  **含义**：若将来要为 WP6 建立 prefill 判据，**必须用长 prompt 且容差 ≥3%（中位数比较）**；用短 prompt **不可断言**。
  **限制**：3.1% 中含 10 tok/s 的显示量化（0.6%），故真实散布略小于 3.1%；本项**只测了 2 个 prompt 长度、单 prompt 文本**。

**(2) 0b 现有外部尾的纯 kernel 代价 —— 关 MTP、decode-only、长 prompt、max-new 128、3 重复**

| 档 | tail | pp（tok/s） | **tg（tok/s）** | tg 中位数 | tg 极差 |
|---|---|---|---|---|---|
| `rk4v4` | 0 | 953.3 / 967.7 / 1010.0 | **68.3 / 68.3 / 68.3** | 68.3 | **0.0** |
| `rk4v4` | 1024 | 938.7 / 933.2 / 950.7 | **67.2 / 67.2 / 67.3** | 67.2 | 0.1 |
| `bf16` | 0 | 1550 / 1590 / 1560 | **67.8 / 67.9 / 67.9** | 67.9 | 0.1 |
| `bf16` | 1024 | 1560 / 1560 / 1560 | **67.0 / 67.0 / 67.1** | 67.0 | 0.1 |

- **纯 kernel 代价（tail 0→1024，MTP 关）**：**`rk4v4` = (67.2−68.3)/68.3 = −1.61%**；**`bf16` = (67.0−67.9)/67.9 = −1.33%**。
  3 重复的 tg **极差 ≤0.1 tok/s（≤0.15%）** ⇒ 该量在本机是**可分辨的**（与 PORT-MEMORY「只有 decode 稳定」一致）。
- **A8 的分解**：记录值 **−5.8% decode**（带 MTP，`--spec mtp --draft-tokens 7 --kv-dtype rk4v4-e8`；PORT-MEMORY §5.14）
  ⇒ **≈ −1.5%（纯 kernel，本次实测）+ ≈ −4.3%（接受率联动，记录值另记 −2.1 pt 接受率）**。
  **WP6.0 的目的达成**：尾部的**纯 kernel 代价约 −1.5%**，被感知的大部分是 **MTP 接受率**，与 A8 已放宽的口径一致。
- **尾部确已生效（非 inert 旁证）**：`MemorySummary` 的 `kv cache payload` **70.0 MiB → 138.0 MiB**、`gpu sequence used` **228.7 → 296.7 MiB**
  （tail=1024 恰好 +68 MiB，与 64 MiB 环 + 元数据一致）⇒ 测的是**真在合并**的尾环。
- **pp 面上**：`rk4v4` 的 pp 也随 tail 下降（约 −1.5%），`bf16` 无此变化 ⇒ **pp 差异落在 §(1) 的 prefill 噪声内（3–26%），不作结论**。

**(3) 判定与回写**
1. **WP6.0a 结论**：本机 prefill 噪声底 =**短 prompt 26.5% / 1073-token prompt 3.1%（7 重复）**。⇒ **A1 的"删除 prefill 腿"经本机实测确认**；
   且**给出"若要 prefill 判据，须长 prompt + ≥3% 容差"的量化前提**。**WP0 产出 ②（本机噪声底报告）由此补上**（decode 侧另见 §(2)）。
2. **WP6.0b 结论**：**纯 kernel 代价 ≈ −1.5%**（MTP 关，`rk4v4`/`bf16` 一致量级）⇒ **A8 的 −5.8% 分解为「纯 kernel ≈ −1.5%」+「接受率联动 ≈ −4.3%」**。
   **⇒ WP6.1 起手的成本预期明确**：(a) 路线的增量性能代价应按 **~1.5% 量级** 估计，而非 5.8%。
3. **回写计划书**：§1-A1 邻域（prefill 噪声底实测行）、§1-A8（纯 kernel 分解）、§7-WP6.0 行（标记完成）、附录 D-19 ⑧（补实测）。

**(4) 未做 / 未测（如实）**
- **未做真正的"配对 A/B"**：本轮的 0b 用 `rk4v4`（记录值用的是 `rk4v4-e8`）、关 MTP（记录值带 MTP k=7）、prompt 1073 token（记录值口径见 PORT-MEMORY）
  ⇒ §(2) 的分解是**量级归因**，**不是严格成对相减**；要严格成对，须同脚本重跑 `rk4v4-e8` 带 MTP 的 tail{0,1024}。
- **未测**：`kvarn:*` 的 prefill/decode（WP6 尚未接线，当前被 fail-fast 拒绝）；未测 `--kv-tail-type bf16` 的尾（本轮只默认 f16 环）；
  未测多请求/批量 prefill（D-19 ⑤ 的 ㈡ 项）；未测 N>1024 / N=2048；未测 MTP 开启下的 tail{0,1024}（用于直接对拍 −5.8%）。
- **未改任何源码**；**未提交**。

**产物**：`.deps/kvarn-adm/wp60_measure.sh`（新）、`.deps/kvarn-adm/wp60/{driver.log, 0a-*.err/out, 0b-*.err/out}`（gitignored）。

### 2026-10-08-15 — **WP6.1 落地**（旋转域 KVarN tail-partial + FP64 oracle，**6 用例全过、余量 ~2000×**）；**并新发现一处先存偶发**（`ninfer_kvarn_test` 的 width=16 分支非确定，**已用 HEAD 基线对照证明非 WP6.1 引入**）

**(1) WP6.1 实现（4 新文件 + 2 改动；`ninfer_ops` 与 `ninfer_tests` 均构建 exit 0、0 error）**

| 文件 | 内容 |
|---|---|
| `src/ops/kvarn/tail_partial.cuh`（新，~200 行） | `kvarn_exact_tail_partition<Geometry>`（镜像 `causal_small_t_tail_partition`，但**由 `detail::kvarn_decode_active_splits` 驱动**，因 KVarN 在 `window>8198 && QHeads==24` 有独立 split 规则）+ `kvarn_exact_tail_partial_kernel<Geometry, Elem>` |
| `src/ops/kvarn/tail_partial.h`（新） | 宿主入口声明 `exact_tail_partial(...)` |
| `src/ops/kvarn/tail_partial.cu`（新，~70 行） | 宿主 launcher：按 `query.ne[1]` 选 `CausalD256H24Kv4`/`CausalD256H16Kv2`，按 `tail_k.dtype` 走 `with_kv_tail_element`（**BF16/F16 同一份代码**）；`tail_tokens<=0 \|\| ring_pages<=0` 时**直接 return（不落笔）** |
| `src/ops/kvarn/sources.cmake`（改） | 注册 `tail_partial.cu` |
| `tests/ops/test_kvarn.cpp`（改，+~290 行） | `run_exact_tail_partial_case`（FP64 oracle，独立实现）+ 6 个用例 |

**设计要点（与计划书 §7-WP6.1 / §6.2 条 4 订正一致）**
1. **域**：尾 partial 在**原始域**算分（用**原始 q**；`W` 正交 ⇒ `<Wq,Wk>=<q,k>`，故原始域算分与 body 的旋转域算分在实数上等价），acc 也在原始域累加；
   **`W` 只施加一次**（`detail::hadamard_warp(acc, lane)`，核内 in-register，`acc[r]` 正好是 `d = lane+32r` 布局）⇒ 得到 body 归并所消费的**旋转域** acc。m/l 是标量，不受域影响。
   ⇒ **零逐行旋转**：既不旋尾 K 行、也不旋尾 V 行（对 V 只旋 acc 一次）。
2. **实现形态**：**一 warp 管一行**（`(q_head,token)`），q 与 acc 各 8 值/lane 全 D256 在寄存器内，无 smem 分块、无 MMA staging ⇒ **正确性优先的简版**（性能收口留 WP6.6；见下"未做"）。
3. **分区**：`body_window = window − min(N, window)`；`body_active` 由 body 自己的 split 策略给出并夹到 `[1, total_active−1]`（有尾时 body 少占一个 split，保证最新 N 个 key 不落出 softmax）；尾占据 `[body_active, total_active)`。
4. **空闲尾 split 发中性 partial**（`m=−inf, l=0, acc=0`），因为 reducer 读满 `[body_active,total_active)`。
5. **`N=0` 不触发**：由 launcher 早退保证（测试断言缓冲**逐字节未动**）。

**(2) 实测（GPU，RTX 5070 Ti，`./build-port/tests/ninfer_tests.exe ninfer_kvarn_test`）—— 6 用例全过**

| 用例 | 结果 |
|---|---|
| window 256 / N 128（H24Kv4，body_active=3, tail_active=1） | **OK** `max_rel_vs_fp64=8.649e-07` |
| window 1024 / N 384（body 10 / tail 6） | **OK** `1.306e-06` |
| window 100 / N 100（**整窗在尾**，body_window=0，24 个中性尾 split） | **OK** `6.195e-07` |
| window 1024 / **N 0** | **OK** `untouched=yes` |
| H16/KV2 window 512 / N 256 | **OK** `1.001e-06` |
| **F16 尾环** window 1024 / N 384 | **OK** `1.306e-06` |

判据 = 归并后（对 kernel 写入的尾 split 做 fp64 online-softmax 合并 → 一次 W → 归一）与 fp64 oracle 的**最大相对误差 ≤ 2e-3**（oracle 独立实现：直接 fp64 点积/softmax + 独立 Sylvester 蝶形）。**实测余量 ~2000×**。

**(3) 新结论（对"待定项：尾 dtype"直接有用）：`f16` 尾不需要"新增 f16 旋转入口"**
本路线**从不旋转尾行**（只旋 FP32 acc，无 dtype 参与），故 `kvarn_hadamard` 只收 BF16 这条限制**不再构成约束** ⇒ 计划书 §7-WP6"待定项"里 f16 的**唯一额外交付物消失**；bf16/f16 只是环的存储 dtype（`with_kv_tail_element` 已两条都通）。**仍待用户裁决 dtype**（bf16 免"本机复核"？f16 有 10-bit 尾数、更精确）。
**限制（如实）**：本轮 F16 用例的环值经 `round_to_bf16` 再 `round_to_f16`，而 bf16 值在 f16 中**精确可表示** ⇒ 该用例**只验证了 F16 读路径/分派正确，未验证 f16 与 bf16 的量化差异**。

**(4) ⚠ 新发现（不利面，先存缺陷）：`ninfer_kvarn_test` 有 width=16 的非确定性**
- **现象**：偶发 `run_speculative_boundary_case`/`run_cached_attention_case` 的 **width=16** 分支失败（`limit=0`，即要求逐位精确），**同一 case 不同次给出不同误差**（0.0055 / 0.142 / 0.145 / 0.0656）⇒ 非确定。
- **判别（已做，决定性命中软件侧）**：**用 HEAD 基线对照**（`git stash` 掉 WP6.1 的 2 个改动文件 + 移走 3 个新文件 → 重建 → 跑 12 次）：**基线同样 1/12 失败**（`first=8190 width=16 ... limit=0`）⇒ **该偶发是先前存在、非 WP6.1 引入**（我已恢复改动并复核 6 用例全过）。
- **判定倾向**：**软件问题**，依据三条 —— ① **全部**观测到的失败都落在 **width=16**（`ColumnsPerBlock=8` 的 packed 分支；width=16 走 `decode.cu:160` 的 `PackedQueryChunk=16`，`first=4094/8190 > MtpPackedWindow=1024`，**不是** prefill 路径）；② 同一输入不同输出 ⇒ 竞态/未初始化读的特征，而硬件位翻转通常散见各类；③ 本仓**已有同类先例**：`small_t_k8v4.cuh:531-539` 注释记载 fp8/nvfp4/k8v4 三族因 `cp_wait` 后缺 `__syncthreads()` 而「run-to-run 非确定」。
- **已排查**：`attention_decode_kernel` **不用 cp.async**（无 `cp_async/cp_wait`）；`stage_decode_record`（`decode_kernel.cuh:124-184`）**内部第 183 行有 `__syncthreads()`**；`key_ready` 的取值（`:677,712,969`）在块内**是均匀的**（条件无 thread-dependent 项）⇒ 第 710 行的条件屏障不是分歧屏障。**尚未定位根因**。
- **进行中**：跑**其他 GPU 套件**多次作为对照（`ninfer_softmax_attention_test`、`ninfer_kv_cache_append_test` 单次均已 exit 0）——若它们多次全绿而只有 kvarn 偶发 ⇒ 硬件排除；再考虑 `compute-sanitizer --tool racecheck/memcheck` 定点取证。
- **影响（须记住）**：**A2/A6 的"逐位"判据不能凭单次运行下结论**（WP6.2 的接受条件 A6「`tail=0` 逐位不变」尤其如此）⇒ WP6.2 的比较须**多次重复**或避开 width=16。

**(5) 未做 / 未测（如实）**
- **WP6.2 未开工**（`partial_acc` BF16→FP32 + 两段式归并；**唯一高回归项**，未碰）。
- **未接线**（WP6.3）：新 kernel **没有**接进 `kvarn_attention` 的 3 个入口/6 个调用点，`startup.cpp:1035-1045` 的 fail-fast **仍在**；本 kernel 目前只能由测试直接调用。
- **未做性能**：本实现是 warp-per-row 直读全局（无 smem 分块、无 MMA、无 cp.async）；**未测**其 kernel 成本，**未与 WP6.0b 的 −1.5% 预算对拍**。若超预算，优化路径是 smem staging / MMA（照 `small_t_tail.cuh` 形状）。
- **未测**：`tokens>1`（投机宽度）的尾 partial；多 batch；>4096 window（会触发 `kvarn_decode_active_splits` 的 8198 分支，测试的 host 镜像**只覆盖 small-T 默认档**）；N>window 的夹取。
- **未提交**（用户约束）。

**产物**：`src/ops/kvarn/{tail_partial.cuh,tail_partial.h,tail_partial.cu,sources.cmake}`、`tests/ops/test_kvarn.cpp`；`/tmp/tests_wp61*.log`、`/tmp/kv61*.out`、`/tmp/kvbase_*.out`、`.deps/kvarn-adm/wp60/`。（备份：`/tmp/wp61_hold/`；`git stash` 条目 `wp61-baseline-check` 已 `apply` 回并应清理。）

---

### 2026-10-08-16 — **先存偶发（width=16 非确定性）根因定位并修复**：`reduce_output_hadamard_kernel` 的 `stage[0]` 暂存复用缺 2 个 `__syncthreads()`；**判定 = 软件缺陷，非硬件**；修复后 **隔离 150/150 + 全量 6/6 + racecheck 0 hazard**

**触发**：用户指示「先解决问题再继续推进」（承接 §3-08-15(4) 的未定位偶发）。

**(1) 判定硬件 vs 软件 —— 用三条独立证据收敛到软件**

| 证据 | 内容 |
|---|---|
| **① 定点仪器** | `compute-sanitizer --tool racecheck`（CUDA 13.3 自带，`…/CUDA/v13.3/compute-sanitizer/compute-sanitizer.exe`，**零源码改动、3 min**）直接报出 **6 处 shared-memory hazard**，全部在 `reduce_output_hadamard_kernel<CausalD256H24Kv4,false,true>`：`:1089` **读** vs `:1104` **写**（+2380 hazards）、`:1110` **读** vs `:1134` **写**（+1536）。 |
| **② 修复后 100% 消失** | 只加 2 个 block barrier（无数值语义变化）后：隔离用例 **150/150 全过**（修复前 **13/150**）、全量 `ninfer_kvarn_test` **6/6 全过**（修复前 20 次 **4 败**）、racecheck **0 hazards / 0 errors**。**硬件位翻转不可能被 2 个屏障消除**。 |
| **③ 缺陷是局部的、且同族已规避** | `search_graph name_pattern=".*reduce_output.*"` ⇒ 全仓**恰好 4 个** reduce-output kernel；`stage[0][tid]` 复用模式 grep **只**命中 `src/ops/kvarn/decode_kernel.cuh`。另 3 个（TAIL 族）走 `causal_merge_split_statistics`，该函数在 `small_t.cuh:229-230` **显式注释**「Published scalars are separate from the reduction/weight storage, so later writes cannot race another warp's scalar read」——**同类风险在旧共享代码里已被识别并规避**，KVarN 这个较晚新增的 kernel 漏掉了。 |

**硬件侧无异常**：空闲（0 MiB / 0% util）、45 °C、throttle reason 0x1（idle）、消费级卡无 ECC。竞态在 sanitizer 下错误率升到 **2/2**（0.576 / 0.543），是竞态放大器的典型表现。

**(2) 根因**（`src/ops/kvarn/decode_kernel.cuh`，`__launch_bounds__(D)`，256 线程）
`stage[2][D]` 被当作**三段归约的共享暂存反复复用**，但两次复用前没有屏障：
1. `stage[0][tid] = local_m` → 蝶形归约（含屏障）→ `head_m = stage[0][0]`（**:1089，全线程读**）→ **缺屏障** → `stage[0][tid] = local_l`（**:1104**，l 归约开始复写同一块）。
2. 同理 `head_l = stage[0][0]`（**:1110**）→ **缺屏障** → `stage[0][tid] = value`（**:1134**，Hadamard 暂存复写）。
**为什么偶发**：写 `stage[0][0]` 的是 warp0；`tid ≥ active_splits` 的线程（`:1093` 的 `for(split=tid; split<active_splits; split+=D)`）**跳过 expf 循环**先跑到 `:1104`。窗口只有几十个周期 ⇒ 平时对、偶尔错。`first=8190` 时 `active_splits=41`（`>8198` 档，`div_up(8206,192)` 夹到 `DecodeMidSplits=41`），长循环造成更大的 warp 偏斜，故**只在该边界稳定复现**。

**(3) 复现与用例锁定方法（可复用）**
- 失败用例 = `tests/ops/test_kvarn.cpp:2219` 的 `run_speculative_boundary_case<4,24>(16,16,1,8190)`（`limit=0` 逐位判据）；`first=8190 width=16 valid=16`。
- 全量复现率 **4/20**；临时加 `NINFER_KVARN_ONLY` 只跑该用例（**2.1 s/次** vs 全量 39 s）后得 **13/150 = 8.7%**。**该临时开关已移除并重建，工作树无残留**。
- 判决式仪器用法：`compute-sanitizer --tool racecheck --launch-timeout 0 --print-level warn <exe> ninfer_kvarn_test`。

**(4) 修法**（唯一改动，2 行屏障 + 2 行说明）
`decode_kernel.cuh:1090` 与 `:1111` 各补一个 `__syncthreads()`，注释说明 `stage[0]` 随后被复用。**无数值语义变化**；WP6.1 的 6 个 FP64 oracle 用例修复前后逐位相同。

**(5) 对 WP6 验收的影响（已回写计划书）**
- **A2/A6 的"逐位"判据不再是"不可判"**：先前的警告（未决项 14）基于「同输入不同输出」；现根因已消除，**单次运行即可判** width=16。**但仍保留**「关键判据重复 2 次」的建议（成本极低，且这是本次唯一发现的先存缺陷类别）。
- **该修复正落在 WP6.2 的改动面上**（WP6.2 要改这个 kernel：`partial_acc` BF16→FP32 + 两段式归并）⇒ **不是额外工作**，且 WP6.2 动该函数时**必须保留这两个屏障**。
- **方法记入纪律**：这类偶发优先用 `racecheck` 定点（3 min、零源码改动），**先于**大规模重复统计与对照套件。
- **构建与注册测试（补充证据，均已实跑）**：`ninfer_ops ninfer ninfer-serve ninfer-perplexity` 四目标全部重建通过（头文件改动波及面确认）；`ctest -R kvarn` **3/3 通过**（`ninfer_qwen3_5_kvarn_continuation_image_test` 0.44 s、`ninfer_qwen3_5_kvarn_tail_row_reset_test` 0.28 s、`ninfer_kvarn_test` 40.73 s）。**TAIL 族（`small_t_k8v4/nvfp4/small_t`）不受影响**：本次改动只在 `src/ops/kvarn/decode_kernel.cuh`，该头文件不被 TAIL 三族的 `.cu` 包含 ⇒ 未重跑那几个 attention 套件（单次 ~120 s）。

**(6) 顺带完成：AGENTS.md 图谱门禁（用户授权）**
按 §3 前述讨论，在 `AGENTS.md` 的「Codebase memory (indexed graph)」小节**追加**（预存未提交改动**原样保留**，仅新增一节）：**图谱门禁三条**（① 改共享契约 kernel 前先 `search_graph` 枚举同类实现 + grep 定位启动点；② 否定/穷尽断言必须图查询 + `check_index_coverage` 并写明所用查询；③ 陌生文件先 `get_file_outline`）+ **图谱不覆盖 kernel 内部**（局部 `__shared__` 非节点、`<<<>>>` 非 `CALLS` 边、kernel 入度恒 0）+ **每个 WP 边界跑 `index_repository`**（消除 `metadata_changed`/`parse_partial` 导致的信任折损）。

**(7) 未做 / 未测（如实）**
- **生成路径 e2e 未跑**：本机未跑带 KVarN 尾的完整生成（该 reduce kernel 在真实 decode 中的行为只由 `ninfer_kvarn_test` 的 oracle 覆盖，未由 Engine 级生成复核）。**未跑完整 ctest（259 项）**，只跑了受影响的 `ninfer_kvarn_test` + `ctest -R kvarn` 的 3 项 + 四个目标重建。
- **未做对照套件多轮**：`ninfer_softmax_attention_test` 单次 ~120 s、`ninfer_kv_cache_append_test` 单次已 exit 0 —— **对照实验因耗时被放弃**，改由 racecheck 的定点证据替代（证据强度更高：直接指出读-写对）。
- **未做 ncu 剖析**：新增 2 个 block barrier 的开销**未实测**（判断可忽略：reduce kernel 只有 256 线程、每 `(q_head,token)` 一次，相对 attention 主核极小）。
- **未提交**（用户约束）。

**产物**：`src/ops/kvarn/decode_kernel.cuh`（+2 屏障）、`AGENTS.md`（图谱门禁新节）；racecheck 输出 `…/tasks/b3m9oen0w.output`、复跑 `…/tasks/b6w71ds02.output`（修前）/`b388kfku5.output`（修后）、全量 `b7u8dqkrw.output`。

---


> **压缩说明（2026-10-08，第二轮）**：`2026-10-08-1 … -13` 与 `-17` 已**逐字**移入归档 §L（见上表）。
> 本节正文只保留 **WP6 链的 `-14 / -15 / -16 / -18 / -19`** —— 它们是未完成的 **WP6.4–WP6.7** 的直接前驱。
> **追加新记录请接在最新一条之后，不要回填归档。**
### 2026-10-08-18 — **WP6.2 落地**（`partial_acc` **BF16→FP32** + 归并路径与上游对齐）；**A6 / A2 / 续列尾 e2e 全过**；**并修正一处自引入的越界写**；**尾环 dtype 裁决 = 维持 f16 同步落盘**

**触发**：用户裁决「维持默认 f16，按主线推进 WP6.2」。

**(1) 改动面（3 文件、源码 5 处 + 1 处删除）**
- `src/ops/kvarn/decode_kernel.cuh`（−53/+44）：① `attention_decode_kernel` 的 `partial_acc` 形参 `__nv_bfloat16*` → `float*`；② `write_neutral` 的填充 `__float2bfloat16(0.0f)` → `0.0f`；③ **删除** BF16 tile 暂存 + 协作拷贝出（原 `:991-1032`），改为 PV warp 每 lane 把一对相邻累加列以 `float2` **直接**写回 `partial_acc`；④ `reduce_output_hadamard_kernel` 形参 `const __nv_bfloat16*` → `const float*`；⑤ `numerator += partial_acc[index] * weight`（去掉 `__bfloat162float`）。
- `src/ops/kvarn/decode.cu`（−3/+3）：workspace `acc` 由 `DType::BF16` → `DType::FP32`；两处实参 cast（`:113` 交 decode 核、`:120` 交 reduce 核）。
- `src/ops/kvarn/attention.cu`（−1/+2）：`query_heads == 24 && max_visible_keys > 8198` 的 workspace 覆盖项 `kvarn::D * sizeof(std::uint16_t)` → `sizeof(float)`。
- **未改**：`tail_partial.{cuh,h,cu}`（WP6.1 已按 `float* partial_acc` 写）、`sources.cmake`、**08-16 补的两个 `__syncthreads()`**（本次未触碰 reduce 的归并数学）。

**为什么删暂存而不是加宽它**：暂存区是 `qkv_s[2*Bc*D]`（**16384 元素**，与 `WarpGroups*Br*D` 在 C=8 下**恰好相等**）。改为 FP32 需 **64 KiB**（现 32 KiB）⇒ 静态 smem 会越过 48 KiB 上限（且 decode 核未 opt-in 动态 smem，静态也无法超 48 KiB）；改走动态则需为 24 个模板实例各加一次 `cudaFuncSetAttribute`。直接存储去掉了一次 smem 往返与一次屏障，故更小更省。

**(2) 口径澄清一：本仓的归并**本来就是**"两段式"**
计划书写 WP6.2 = "`partial_acc` BF16→FP32 + **两段式归并**"。读码后：**本仓不需要把 reduce 拆成两核** —— WP6.1 的尾 partial 直接写在**同一条 split 轴**的 `[body_active, total_active)` 上，而 `reduce_output_hadamard_kernel` 本来就对 `[0, active_splits)` 做一遍 online-softmax（`m` → `l` → `numerator/head_l` → **一次反旋**）。上游的 reduce+combine 两核存在，是因为上游两侧各自成对（**无 split 轴**）；本仓的等价物 = "body 的 splits 与 tail 的 splits 落在同一遍归并里"。⇒ **本步的实际交付 = dtype 对齐（消除 BF16-acc 偏离），不是新增第二个归并核。** 计划书原文"两段式"保留、此处订正其在本仓的形态。

**(3) 一处自引入的越界写（如实记录，已修）**
第一版直接存储把打包分支的第二笔（`warp_state.accumulator[n][2..3]` → `column+1`）写在 `if (head_valid)` **之外**；原码此处由 `row1_head = ColumnsPerMma == 2 ? row0 : row1` 守卫，等价于 `gid < row_count`。H24 下 `GroupSize = 6 < Br/2 = 8` ⇒ `gid ∈ {6,7}` 的 lane 会写 `q_head = kv_head*6 + 6/7`，**越出该 KV 头的 head 区间**（kv_head=3 时 `q_head = 24/25 ≥ QHeads = 24`）⇒ 踩踏相邻 (token, split) 行。
**症状**（改前 vs 改后的同一命令对照）：`ninfer_kvarn_test` **14 处失败** —— `limit=0` 的路线间逐位比对 `relative_l2 0.24–0.30`（5 个 `speculative boundary` 用例，各 2 条）、`limit=8e-3` 的 FP64 容差比对 `0.26–0.42`（`random packed / width-8 / width-16 / k5v5 packed / k6v6 packed`）；**WP6.1 的 6 个 FP64 oracle 尾用例仍全过**（它们不经过 body 核）。**修法** = 把第二笔移回 `if (head_valid)` 内（与原码 `row1_head = row0` 等价）。修后全绿。

**(4) 验收（GPU 实测）**
- **`ninfer_kvarn_test` 3 连跑全过、逐位确定**（同输入 3 次输出完全相同）；
- **A2 codec 余量与 08-04 逐字相同**（3 次运行同值）：`K official oracle` `step_ratio_max=1` / `flips=1/65` / `over_step=0` / `wide_flips=0 of 65536`；`V official oracle` `flips=0/65` / `over_step=0`（另两组 `flips=0/65`、`step_ratio_max` 0.0049/0.0101/0.0206 也逐字相同）；
- 6 个 WP6.1 FP64 oracle 尾用例全过（`max_rel_vs_fp64` 6.2e-7…1.3e-6）；`tail_tokens=0 leaves the partial untouched=yes`；
- `limit=0` 的逐位路线间比对（`kvarn_attention` vs `decode_attention`）全过；`limit=8e-3` 的 FP64 容差比对全过（余量见 (5)）；
- `ninfer_qwen3_5_kvarn_continuation_image_test`、`ninfer_qwen3_5_kvarn_tail_row_reset_test` 通过；
- **续列尾 e2e**（`.deps/kvarn-adm/p1_prefix_reuse.sh`，`ninfer-serve` + 固定产物）：**RESULT: PASS** —— `kvarn r2 cached_tokens = 851`、`kvarn messages identical: YES`、bf16 对照 `851 / YES`。**851 与 08-04 记录同值。**

**(5) A6「tail=0 输出逐位不变」的量化（改前/改后对照，临时 `report=true` 取余量后已还原）**
`ninfer_kvarn_test` 的 FP64 容差用例（判据 `8e-3`），同一命令改前 vs 改后：

| 用例 | 改前（BF16 acc） | 改后（FP32 acc） | 变化 |
|---|---|---|---|
| H24/KV4 width-1 | 0.00378797 | **0.00344719** | −9.0% |
| H16/KV2 width-6 | 0.00439883 | **0.0040799** | −7.2% |
| H24/KV4 B=2 | 0.00375669 | **0.00335742** | −10.6% |
| H16/KV2 B=2 | 0.00357079 | **0.00318072** | −10.9% |
| random packed | 0.00457711 | **0.00425809** | −7.0% |
| random width-8 | 0.00456953 | **0.00425609** | −6.9% |
| random width-16 | 0.00454911 | **0.00424225** | −6.7% |
| k5v5 packed | 0.00457569 | **0.00424568** | −7.2% |
| k6v6 packed | 0.00458949 | **0.00427863** | −6.8% |
| H24/KV4 **tiled** | 0.00329304 | **0.00329304** | **逐位相同** |
| H16/KV2 **tiled** | 0.00330136 | **0.00330136** | **逐位相同** |
| **slab-boundary** | 0.00457945 | **0.00457945** | **逐位相同** |

**(5a) 读法**：走 **decode** 路线的用例误差**一致下降 6.7–10.9%**；走 **prompt 路线**（`launch_prefill` → `finalize_prefill_slab_kernel`，用 FP32 `running_acc`、**从不经 `partial_acc`**）的 `tiled`×2 与 `slab-boundary` **逐位不变** ⇒ 这是"改动面恰好被限制在 decode 路线"的**独立交叉验证**，也说明改动在数值上是**改善而非回归**。
**(5b) ⚠ A6 口径（必须显式说明）**：本步**按计划书要求**把 body 的 per-split 累加器由 BF16 舍入改为 FP32 ⇒ **tail=0 的输出相对 WP6.2 之前的二进制不再逐字节相同**（这是 `BF16→FP32` 的**意图**，§6.2 条 1 订正"对齐上游"，不是回归；上表的 0.36–0.46% 相对改善即其度量）。计划书 A6 点名的**两个验证载体都是同版本内比较**（既有 `ninfer_kvarn_test` 的 `limit=0` 比对、续列尾 e2e 的 r1/r2 逐字节比对）⇒ A6 的「逐位不变」只能按"**同版本内的逐位判据仍成立**"理解；**跨版本字节同一性按构造不成立**（已写入计划书 A6 行与 §7-WP6.2）。**未测**：跨版本的具体差值绝对值（无 golden 文件）。

**(6) 容量观察（WP6.5 的输入，未实测定论）**
`causal_softmax_attention_workspace_capacity_bytes` 经 `allocate_small_t_workspace` **一直按 FP32 给 partial 记账** ⇒ 改前 kvarn 用 BF16 时该腿是 **2× 过配**、改后转为**恰好**。故 dtype 变更对 workspace 的净影响可能接近 0；本次改的覆盖项本身**改前精确对应 BF16、改后精确对应 FP32**（`split_rows × (D*4+8)` = `24*16*1*82 × 1032` = **32,495,616 B** = `acc(256*24*16*82*4)` + `m/l` 之和，**逐字节相等**）。计划书 D-19 记的「+0.4 MB」是漏乘 `DecodeLongSplits` 的估值。**待 WP6.5 实测 ±5%**。

**(7) 未做 / 未测（如实）**
- **未测性能**：本次只改归并的存储路径（去掉一次 smem 往返 + 一次屏障，但把 16 B 存改为 8 B 存）。**未测**核时间；**WP6.0b 的 −1.5% 尾预算仍未被 WP6.1/WP6.2 对拍**。
- **`ninfer_softmax_attention_*`（9 项）本轮启动后中止**：本次改动只落在 kvarn 私有文件（`decode_kernel.cuh`/`decode.cu`/`attention.cu` 的 kvarn 函数），与共享 `small_t.cuh` reducer 及 `causal_softmax_attention_workspace_capacity_bytes` **无交集** ⇒ 判定不受影响；**该判定未由实测覆盖**（GPU 让位给点名更靠前的续列尾 e2e）。
- **未跑全量 ctest**（259 项）；**未跑 MTP**；**未跑真实模型质量臂**（A4/A8 属 WP6.6）。
- **未提交**（用户约束）。

**产物**：`src/ops/kvarn/{decode_kernel.cuh,decode.cu,attention.cu}`；e2e 日志与响应 JSON 在 `.deps/kvarn-adm/`（`p1-kvarn-{r1,r2}.json`、`p1-kvarn-serve.log`）。回写计划书 §1-A6、§6.2 条 1/2、§7-WP6.2、附录 **D-23**；dtype 裁决另回写 §7-WP6 待定项与 **D-22(7)**、本记录未决项 15。

---

### 2026-10-08-19 — **WP6.3 分区与接线**（W2b/W3–W6）：op 内 `body_window = window − N` 三分区 + 3 个 op 入口带尾视图 + 尾环写入接入 append + 解 `startup.cpp` fail-fast；**功能验收全过**（e2e 四臂 + MTP + workspace 无溢出 + 续列尾无回归）；**并定位并修复 3 个潜伏缺陷**（2 个 WP6.1 batch 偏移 + 1 个 WP6.1 查询域缺陷）；**测试端仍有 2 处 oracle 待修（非生产缺陷）**

**触发**：用户授权推进 WP6.3（"授权进行下一步工作，可并行的工作用子代理；需要隔离时用 worktree"）。

**(1) 改动面（源码 7 文件 + 测试 1 文件）**
- `include/ninfer/ops/kvarn.h`：`#include "core/paged_kv_cache.h"`；`KvarnPagedBatchLayerView` 新增 `PagedKVExactTailView tail;`（尾环是**外部共享**视图，非 kvarn 私有）。
- `src/models/qwen3_5/state/decoder_state.cpp`：`kvarn_batch_layer_view` 从 `exact_tail_->plane(layer*2)` / `plane(layer*2+1)`、`tail_ring_pages_`、`tail_retention_` 填充 `.tail`（同一来源，TAIL 路由已在用）。
- `src/ops/kvarn/decode_kernel.cuh`：把 `KvarnExactTailPartition` + `kvarn_exact_tail_partition` **上移到本头**（body 核 / 尾 partial / 归并核必须共用同一分区）；`attention_decode_kernel` 与 `reduce_output_hadamard_kernel` 各加 `std::int32_t tail_tokens` 形参；body 核的 window 改由 launch 级最新位置推出（见 (3)）。
- `src/ops/kvarn/decode.cu`：`launch_partial` 推出 `tail_tokens = cache.tail.enabled() && cache.tail.page_count > 0 ? cache.tail.retention : 0`，传给 body 与 reduce 两个启动点，并在两者**之间**启动尾 partial（尾读 append 已写入的环、归并同时读 body 与尾）。
- `src/ops/kvarn/tail_partial.{cuh,h,cu}`：`.cuh` 删去已上移的分区代码 + **加 batch 偏移 + 查询反旋**（见 (4)(5)）；`.h` 改签名新增 `width` / `batch_size` / `column_begin`；`.cu` 新增 `kvarn_exact_tail_stage_kernel` + 公用 `stage_exact_tail`（见 (2)）。
- `src/ops/kvarn/attention.cu`：新增 `require_exact_tail(view, batch_size)`（只校验元素数 / dtype / 非空；**刻意不查** plane 声明的轴序，见 (7)）；`validate_inputs` 与 `kvarn_kv_append` 调用之；`kvarn_attention` / `kvarn_kv_append` 在 `rotate_kv` **之前**调用 `kvarn::stage_exact_tail(...)`（环须存**原始**行）。
- `src/models/qwen3_5/program/planning/startup.cpp`：**删除** `options.kv_tail_tokens != 0` 对 `KvarnGroup128` 的 fail-fast（`--mtp-attention-window` 的拒绝保留）。实测：`KVarN does not support --mtp-attention-window yet` 仍在，tail 不再被拒。
- `tests/ops/test_kvarn.cpp`（约 +330 行）：新增 `rel_l2`、`rotate_query_like_op`、`run_exact_tail_stage_case`（**5 用例**）、`run_exact_tail_partial_launch_case`（**2 用例**）、`run_exact_tail_merged_case`（**2 用例**）。

**(2) 尾环写入（W3）**：`kvarn_exact_tail_stage_kernel<Geometry, Elem>` —— 一线程写一个 (batch, token, head) 行的 8 元素向量，经 `kv_tail_row_in_ring`、`store_tail_vec8`、`paged_kv_element_offset` 落到环；`stage_exact_tail` 用 `with_kv_tail_element(tail_k.dtype, ...)` 派发、按 `kv_heads ∈ {CausalD256H24Kv4::KVHeads, CausalD256H16Kv2::KVHeads}` 分派几何。⇒ `--kv-tail-type` 的两种取值都是**真派发**（bf16/f16），非静默。

**(3) 分区（W2b）**：`body_window = window − min(N, window)`；`total_active = kvarn_decode_active_splits(window, split_count)`；`body_active = min(active_splits(body_window), total_active − 1)`，尾有键时下限 1；`tail_active = total_active − body_active`。核内：`split >= body_active` 直接 return（尾核接管 `[body_active, total_active)`）。**`N = 0` 时分区是恒等**（`body_window == window`、`body_active == total_active`、`tail_active == 0`）⇒ 无尾启动的代码路径**逐位不变**（尾复核的 key 循环不进入）。**关键点**：有尾时 body 必须用 **launch 级最新位置**（非每列组 anchor），body/尾/归并三者才对同一 `window` 达成一致；多余 split 贡献恰为零（body 逐列 `key <= query_position` 掩码 + 空行发布 `m=-inf,l=0`，归并跳过）。

**(4) 修复的 2 个 WP6.1 batch 偏移缺陷（此前无 batch 用例故未暴露）**
- `partial_acc/m/l` **缺 batch 偏移**：batched 启动把序列堆在 split 轴上 ⇒ 补 `batch * D * QHeads * tokens * split_count`（`m/l` 同式去 `D`）；batch 0 加 0。
- 查询**缺 batch 列偏移**：`column_base = column_begin + batch * full_width`（原先只读 `column_begin`，batch ≥ 1 读错行）。

**(5) 修复的 WP6.1 **查询域**缺陷（WP6.3 接线后才可见）**
WP6.1 的尾核按"旋转域"写的：分数旋转不变（`<Wq,Wk> == <q,k>`）⇒ 尾可在**原始行**上算 `q·k`；但 op 在启动尾核**之前**就地把**调用者查询缓冲**旋了（`Tensor rotated_query = query; kvarn_hadamard(query, rotated_query, stream);` **别名**，核同址读写 ⇒ in-place 安全）⇒ 尾核拿到的其实是**旋转后**的 q，与环里存的**原始** k 点积是错的。
**修法**：尾核先 `detail::hadamard_warp(qv, lane)` **反旋查询**（每行一次 warp 内 W）。依据：W 正交自逆且线性。
**直测证据**（`run_exact_tail_merged_case` 临时诊断，随后已还原）：`query_buffer_moved=6.328e+00`（调用后缓冲已不再是原值）且 `vs_rotation=0.000e+00`（恰好等于 `W(q_bf16)`）⇒ op **确实**就地旋转了调用者的查询缓冲，反旋正确。

**(6) WP6.3 级合并用例的诊断裁决：**测试 oracle 错，非代码错****
`run_exact_tail_merged_case` 目前对 **1.41** 的相对误差失败。诊断读数（window 1000/1200 两例一致）：`rel2_rotated≈1.42`、`rel2_original≈3.2e-3`（scalar 3.161e-03 / packed 3.229e-03）。
**推导**：body 的 `partial_acc` = `Σ p·W(V_orig)`；归并在末尾**再乘一次 W**，故公开输出 = **原始域** `acc_orig/l`，**无净 Hadamard**。尾核写的是 `W(Σ p V_orig)`、归并再乘 W ⇒ W²=I ⇒ 尾侧同样落到原始域。⇒ **tail-only 启动的 op 级期望 = `acc_orig/l`，不带 W**；测试里 `expected` 误用了 `host_hadamard_d256` ⇒ 是 **oracle 写错**（`1.42` 正是"多旋一次"的量）。**待改**：`run_exact_tail_merged_case` 改用不旋的期望并删诊断字段。

**(7) 一个**刻意不查**的既有标注异常（如实记录，本 WP 不处置）**：尾池 plane 在 `decoder_state.cpp` 里声明为 **HeadMajor**，而环的扁平寻址（`paged_kv_element_offset`）是 **PageMajor**；所有写/读方都走同一扁平助手 ⇒ **对环无害**，但池级**批量拷贝**会按声明步长算 ⇒ 归为既有风险（跨路由、非 WP6 引入），未决项记录。

**(8) 功能验收（GPU 实测，`.deps/kvarn-adm/wp63_e2e.sh`）**
- **A 臂**：`kvarn:k4v4 + --kv-tail-tokens 1024` **exit 0**（prompt 1073 token、生成 128、decode **62.5 tok/s**）。
- **B/C 臂**：尾环 **f16 与 bf16 产出文本逐字节相同**；**tail=0 与有尾不同**（符合预期）。
- **D 臂（容量）**：workspace 峰值 **990.0 MiB / 990.0 MiB 逐字节相同**（有尾 / 无尾）⇒ **无溢出**；环增长 **+68.0 MiB = 17 页 × 16 层 × 256 KiB**（精确对账）。
- **MTP + 尾 + 1073-token prompt**：跑通（走打包 `ColumnsPerBlock=4` verify 路线**且**带尾）。**⚠ 非可比速度**：有尾 100% acceptance / 250.9 tok/s vs 无尾 41.5% / 103.0 tok/s —— 接受率不同 ⇒ **不能当加速**读。
- **续列尾无回归**：`.deps/kvarn-adm/p1_prefix_reuse.sh` → **RESULT: PASS**，`cached_tokens = 851`（与 08-04/08-18 同值）。

**(9) 测试端 2 处 oracle 已修（**非生产缺陷**）+ 复跑结果**
- ① `run_exact_tail_merged_case`：`expected` **去掉 `host_hadamard_d256`**（tail-only 的 op 级输出 = `acc_orig/l`，见 (6)）并**删除全部临时诊断**，改为 `compare_profile(..., report=true)` 常驻报余量。
- ② `run_exact_tail_partial_case` / `run_exact_tail_partial_launch_case`：新增 `unrotate_query_like_kernel()` —— oracle 用**同一旋转后 bf16 查询**在 double 里反旋（`hadamard_warp` 与 `host_hadamard_d256` 是同一蝶形/同一 `2^-4` 尺度），从而只剩"核 float vs oracle double"这一项残差。
- **复跑 `ninfer_kvarn_test`（GPU，exit 0，"OK kvarn correctness"）**：
  - 6 个直接尾用例 `max_rel_vs_fp64` = **8.0e-7 / 1.03e-6 / 1.28e-6 / 1.28e-6 / 1.42e-6**（vs 2e-3 判据 ⇒ 余量 ~1400–2500×）；
  - 2 个 batched/chunked 启动用例 = **1.33e-6 / 2.14e-6**（修前 5.1e-3 / 6.3e-3 **超限**）；
  - 2 个 op 级合并用例（`report=true`，**首次给出余量**）= **relative_l2 3.161e-3 / 3.229e-3**（vs 8e-3 判据，余量 ~2.5×，与其它 decode 路线 op 级用例的 3.4e-3 同量级；残差来自 op 自身 `hadamard_kernel` 的就地旋转无法在 oracle 里逐位建模）；
  - 5 个尾环 stage 用例全 OK；`tail_tokens=0 leaves the partial untouched=yes`。
- **同轮复跑两个同族测试全过**：`ninfer_qwen3_5_kvarn_continuation_image_test`（OK）、`ninfer_qwen3_5_kvarn_tail_row_reset_test`（OK）。
- **仍未做 / 未测**：**`window > 8198` 的 KVarN split 分支带尾无任何 host oracle 覆盖**（WP6.4 范围）；**未做跨二进制 `tail=0` 逐位比对**（A6/A7 脚手架属 WP6.4）；未跑全量 ctest / `ninfer_softmax_attention_*` / 核级性能；**未提交**（用户约束）。

**(10) 门禁① sweep（补跑，用户 08-19 授权）——共享契约的同类实现枚举 + 启动点定位 + 覆盖度**

> 依据 `AGENTS.md` 图谱门禁①：改读/写共享跨核契约的 kernel 前，先 `search_graph` 枚举同类实现、再 grep 定位启动点。**本次 WP6.3 改的契约面** = `partial_acc/partial_m/partial_l` + split 分区（`decode_kernel.cuh`）。**此前漏跑，现补。**

- **图查询 1**：`search_graph(project=D-ninfer-ninfer-precision-tail, name_pattern=".*reduce_output.*")` ⇒ 全仓**恰好 4 个** reduce-output kernel（另 2 条命中是本日志自身的 markdown 段）：`causal_attention_small_t_reduce_output_kernel`（`small_t.cuh:261`）、`..._k8v4_reduce_output_kernel`（`small_t_k8v4.cuh:591`）、`..._nvfp4_reduce_output_kernel`（`small_t_nvfp4.cuh:609`）、**`kvarn::detail::reduce_output_hadamard_kernel`（`decode_kernel.cuh:1084`，入度 0 / 出度 7）**。⇒ **本 WP 改的契约读取方只有 KVarN 一个**，3 个 small_t 兄弟**未被触及**（也未新增 `tail_tokens` 形参）⇒ 分区契约仍是"只有 KVarN 的 reduce 消费 `[body_active, total_active)` 尾段"。
- **图查询 2**：`search_graph(name_pattern=".*tc_partial.*")` ⇒ causal_cache 的 **partial 写入族** = `causal_attention_small_t_tc_partial_bf16_kernel`（`small_t_bf16.cuh:18`，入 2 / 出 33）+ 量化族启动器 `launch_tc_partial_i8`（`small_t_i8_launch.cuh:32`）/`launch_tc_partial_bf16`（`small_t.cu:205`）。**KVarN 侧写入方 = body `attention_decode_kernel` + 本次新增的 `kvarn_exact_tail_partial_kernel`（`tail_partial.cuh:73`，入 1 / 出 20）**。
- **图查询 3**：`search_graph(name_pattern=".*exact_tail.*")`（23 命中）—— **发现一处命名撞车（值得记）**：`src/ops/linear_pair/q8/q8_pair_plan.cpp` 有**既有的、与精度尾无关的** `launch_exact_tail`（`:507-541`）/ `is_exact_tail_schedule`（`:116-129`），属 q8 线性对特性；另有**既有测试** `tests/models/qwen3_5/test_exact_tail_capacity.cpp`（`test_exact_tail_capacity_model:70-159`，**WP6.5 容量项的现成参照**）。⇒ **本 WP 的精确尾一律以 `kvarn::` 前缀 / `tail_partial.*` 文件区分**，勿与 q8 的 `exact_tail` 混同。另：`kvarn_exact_tail_partition` 图记 **in=2 / out=0** ⇒ 两个调用者（body 核 + 尾核），与"body/尾/归并共用一分区"的设计一致。
- **grep 启动点（穷尽，图不覆盖 `<<<>>>`）**：`attention_decode_kernel<<<` → **`decode.cu:106`（唯一）**；`reduce_output_hadamard_kernel` → **`decode.cu:131`（唯一）**；`kvarn_exact_tail_partial_kernel<<<` → **`tail_partial.cu:93`（唯一）**；`kvarn_exact_tail_stage_kernel<<<` → **`tail_partial.cu:71`（唯一）**；公共入口 `exact_tail_partial(` → **`decode.cu:125`（唯一）**；`stage_exact_tail(` → **`attention.cu:615, 653`**；`require_exact_tail(` → **`attention.cu:490, 651`**。
- **覆盖度 `check_index_coverage`（10 路径）**：全部 `no_recorded_issue` 或 `partial`（`decode_kernel.cuh` / `small_t_k8v4.cuh` / `small_t_nvfp4.cuh` / `small_t_bf16.cuh`）；`decode_kernel.cuh` 的 `parse_partial` 命中 **550–560 经直读确认 = kernel 内 `__shared__` 声明**（**按 `AGENTS.md` 本就不是图谱节点**），**不含结论所依赖的任何行**。全路径 `freshness=metadata_changed` 为**脏树预期**（改进未提交），非判定。
- **结论**：本 WP 的契约改动**面窄且闭合**——KVarN 的 body/尾/归并三者共用一分区，3 个 small_t 兄弟与 q8 的 `exact_tail` 均不相干；三个新公共入口的调用点已穷尽（1 / 2 / 2 处）。
- **新工具边界（08-19 实测，建议入纪律）**：`search_graph` 的 **`file_pattern` 需要精确全路径**，**不是正则** —— `file_pattern="src/ops/kvarn/tail_partial.cuh"` **命中 3 节点**，而 `file_pattern=".*tail_partial.*"` **返回 0**（同理 `.*causal_cache.*` 返回 0，而该目录节点确实存在）。**`name_pattern` / `qn_pattern` 才是正则**。⇒ 按路径过滤**一律写全仓库相对路径**；要按文件名做正则，改用 `name_pattern` 或 `qn_pattern`。本次因此白跑一轮。

**(11) `AGENTS.md` 图谱纪律扩展为「分层变更流程」（用户 08-19 要求「固化」，已落地）**
- 在「Codebase memory」小节的边界索引段之后，新增 **Change flow (checklist)**：把原有三条门禁扩成 **A1–A9** 清单，并**按改动分级**——**Tier A**（共享跨核契约 / 共享类型 / 跨文件行为）走全流程；**Tier B**（单点局部改动、字面量、配置、文档）只走 **A7–A9**。
- 关键点：**A1** 索引新鲜度（仅当 `indexed_at` 早于工作树最新改动才重索引）；**A2** 图枚举同类实现（含 `file_pattern` 需精确全路径这条新边界）；**A3** grep `<<<>>>` 启动点（图不覆盖）；**A4** 覆盖度 + 直读被标记行；**A5** 计划含**逐项可行性/风险/回退**（"准代码"只到**接口与不变量**层，不把实现体写进持久文档）；存疑项**闭环回 A2–A4**；**A6** **状态迁移矩阵**（状态维度 × 观测面，空格 = 未覆盖，必须显式记"未测"）；**A7** 实施；**A8** 按目标构建 + **动态验证**（oracle/门禁测试、`racecheck`、FP64 oracle、跨路线一致性）；**A9** 记录 + **强制 `index_repository`** + `git diff --check`。
- 同时明确一条**能力边界**：**图谱只给"结构与完备性"，不给可行性 / 风险 / 正确性** —— 后者来自读码与领域推理。
- **为什么分级**：本轮我跳过门禁①的**机制原因**正是"对小事也付全套仪式成本"；把 Tier 显式化才是流程能被真正执行的前提。**该文件改动仅此一处，未动其余章节；可回退**（`git checkout -- AGENTS.md` 会连带丢弃本会话之前既有的 `AGENTS.md` 未提交改动，故**不要**这么做，如需回退请只撤本段）。

**(12) `AGENTS.md` 瘦身 + 建归档（用户 08-19 要求；「太长」）**
- **新建** `docs/port-records/AGENTS-ARCHIVE-2026-10.md`（沿用本仓 `KVARN-*-ARCHIVE` 的**逐字搬运 + 正文留指针**约定），三节：**§A 构建环境与工具链**（toolchain 表、"无 VS 2026 ⇒ 旧双工具链陷阱失效"史、`env-port.bat` 逐项、探针为何用 `cmd`）、**§B 图谱实测案例与工具边界**（field 级 `trace_path`/`query_graph` 边界、`KvarnBits` 消费方对拍、reduce-output 唯一性、`exact_tail` 命名撞车、`file_pattern` 非正则）、**§C 产品/上游 provenance**。
- **`AGENTS.md` 改动**：① 顶部定义一次"**archive §X**"指针；② 构建环境节 **toolchain 表 + 陷阱史 + `env-port.bat` 枚举 + 探针段**改为压缩版 + 指针；③ 图谱节把**三条门禁并入 A1–A9**（原两处讲同一件事，去重）、三处实测案例外移为一行指针；④ 产品节的**上游 provenance** 压缩 + 指针。
- **量化**：**388 → 354 行（−34）/ 29,613 → 26,964 B（−9%）**。**⚠ 低于我自估的 −34%（约 −130 行）**——因为**规则本体一律保留**（尤其 A1–A9 流程、`Reporting` 5 条、`Durable progress record` 5 条、"否则会踩的坑"清单），只裁了**历史/依据/证据/重复**。若要更深：唯一剩下的手段是把 **A1–A9 移到独立文档**（再省 ~34 行），**代价是它不再随会话加载、也就更容易被跳过** —— **未采**，故 §6×2 节仍是最大两块（45 + 69 行）。
- **验证**：`git diff --check` 干净；代码围栏成对（2 个 ` ``` `）；未动其余章节。
- **⚠ 裁掉又补回的一条规则（如实记录）**：合并门禁②进"否定/穷尽断言"要点时，**丢掉了原门禁②的 "State the query that was run"**（提交 `58b3baf3` 之后经复查发现）⇒ 已用 `5d51c6f6` 补回（"**State the query that was run** whenever the claim is negative or exhaustive"）。⇒ **瘦身的真实风险就是这一条**：规则与依据混在同一段时，裁依据容易连带裁掉规则；本次靠**逐节对照原文**才发现。**再次证明"不裁规则本体"这条纪律必须配"逐节比对"才能落实。**

**(13) `§5` handoff 与 `§4.1` P5 行刷新（用户 08-19 要求；原停在 08-13）**
- `§5` 的**抬头/任务/先读/当前状态/待办⑤/待办⑦/环境与纪律/建议第一条命令/图谱小节**全部对齐到 08-19：当前状态改为"**工作树干净 + HEAD=`7ea2b0bd` + WP6.3 已通 + 测试余量**"；待办⑤ 改为 **WP6.3 ✅ + ▶ 下一步 = WP6.4**（含三块新码与已登记缺口）；图谱小节改为 **46,009/216,983**、**A1–A9 分层流程**、**`file_pattern` 非正则**、**archive §X**。
- `§4.1` 的 **P5 行**由"WP6.1–WP6.7 未开工"改为"**WP6.0–WP6.3 ✅ 已完成；WP6.4–WP6.7 未开工**"。
- **验证**：`git diff --check` 干净。**只动这两节 + 本 §3 条目，未改其它内容。**

**产物**：`include/ninfer/ops/kvarn.h`、`src/models/qwen3_5/state/decoder_state.cpp`、`src/models/qwen3_5/program/planning/startup.cpp`、`src/ops/kvarn/{decode_kernel.cuh,decode.cu,tail_partial.cuh,tail_partial.h,tail_partial.cu,attention.cu}`、`tests/ops/test_kvarn.cpp`；e2e 脚本与日志在 `.deps/kvarn-adm/`（`wp63_e2e.sh`、`wp63/`）。回写计划书 §7-WP6.3、附录 D-24（待写）。

### 2026-10-08-20 — **归档瘦身（第二轮，用户要求「太长」）**：`§3` 的 08-1…-13、-17 与计划书附录 **D-10…D-19** 逐字外移；**两份文档 2935 → 1854 行（−37%）**，只留关键信息 + 归档索引

**触发**：用户指示「`kvarn-port-into-precision-tail-plan.md` 和 `kvarn-port-progress.md` 已经太长了，与未完成的工作无关的信息整理归档到 `docs/port-records`，记忆和计划文档只留关键信息和归档信息的索引」。

**(1) 判据（与 `AGENTS.md` 归档一致）**：**与未完成工作无关** = ① **已关闭 WP 包**的执行记录；② **被取代**的版本变更摘要；③ 结论**已由正文他处承接**（§0 / §4.1 / §5 / 计划书 §1 / §6 / D-20…D-24）的细节。**规则、判据、开放项、状态一律留正文。**

**(2) 搬出（逐字，脚本切片搬运，未改写一字）**
- `kvarn-port-progress.md` → `KVARN-PROGRESS-ARCHIVE-2026-10.md` **§L**：§3 条目 **2026-10-08-1 … -13**（587 行）+ **-17**（47 行）= **634 行**。这些对应 **P0 / P1 / P2 / P3b / P3c / P3d / §6.3 探针 / 尾环 dtype 分析**，**均已关闭**。
- `kvarn-port-into-precision-tail-plan.md` → `KVARN-PLAN-APPENDIX-ARCHIVE.md`「逐字归档：附录 D-10 … D-19」= **512 行**（WP0.5-B 正式准入 / P2b / P3b / P3c / P3d / WP6 门禁评估 / §6.3 探针）。
- **留下**：§3 的 **08-14 / -15 / -16 / -18 / -19**（WP6 链 = 未完成的 WP6.4–WP6.7 的**直接前驱**）与附录 **D-20 … D-24**。

**(3) 体量（实测）**

| 文件 | 前 | 后 | Δ |
|---|---|---|---|
| `kvarn-port-progress.md` | 1331 行 / 199 KB | **753 行 / 117 KB** | **−578 行（−43%）** |
| `kvarn-port-into-precision-tail-plan.md` | 1604 行 / 203 KB | **1101 行 / 146 KB** | **−503 行（−31%）** |
| 合计 | 2935 行 | **1854 行** | **−1081 行（−37%）** |
| 归档两文件 | 1447 / 325 行 | 2091 / 846 行 | +644 / +521 |

**(4) 无损性核对**：移出 **634 / 512** 行，归档净增 **+644 / +521**（差额 **10 / 9** 行 = 新增帧头：小节标题 + 说明 + 空行）⇒ **内容逐字保留，无丢行、无重复**。append 多出的 **EOF 空行已用 `awk` 去掉**（`git diff --check` 由 2 → 0）。

**(5) 索引与指针（保证可检索）**
- 正文 §3 的**条目索引表**新增 **14 行**（08-1…-13、-17：条目号 + 原标题逐字 + `§L` 链接）——与既有 07-NN 表的约定一致；正文/计划书里的 `§3-08-NN` 引用按该表解析。
- 计划书附录 D 新增「**D-10 … D-19 索引**」表（锚点 + 原标题逐字 + 归档链接）。
- 两处「**已归档信息索引**」小节各加一行（progress §L、plan 的 APPENDIX 行扩到 D-1…D-19）。
- **防丢关键数字**：计划书的归档索引段**特意保留**了 WP6.6/WP7 仍要用的数字 —— D-11 三档准入 `k4v4` **0.002120** / `k5v5` **0.001432** / `k6v6` **0.001233**（对 `rk4v4`/`nvfp4`/`k8v4` = 2.09×/2.07×/2.18×）、`k6v6` 门槛 **0.002688**（D-10）、`kvarn:k4v4` KLD 逐位 **0.002120**（D-17）。

**(6) 未做 / 仍可再瘦（如实）**
- **计划书版本头**（v14…v20 的历史块，约 **60 行**）**未搬**：它们是 changelog，本可入 `KVARN-PLAN-CHANGELOG-ARCHIVE.md`（该文件已收 v3→v12）。**判为下一步可选**，因为它不与未完成工作冲突、且 v20 的路线量化仍被 §6.3/D-19 引用。
- **progress §1（环境与构建速查）/ §2（关键路径与事实索引）未搬**：与 §5「环境与纪律」有重叠，但 §1/§2 是**日常操作要用的速查**，搬走会加长冷启动路径 ⇒ 保留。
- **未跑**：无源码/测试改动，故**无构建、无 GPU 动作**；`git diff --check` 干净。**未提交**（用户未要求）。

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
| **P5 — WP6 旋转域尾部合并** | **路线已裁决 = (a)（08-13）；WP6.0–WP6.3 ✅ 已完成（08-14/-15/-16/-18/-19）；WP6.4–WP6.7 未开工** | 10–15 天、最高风险。计划书 §7-WP6 已拆为 **WP6.0–WP6.7**：**0 前置测量 ✅**（0a prefill 噪声底 短 prompt **26.5%** / 1073-token **3.1%**；0b 纯 kernel 尾代价 `rk4v4` **−1.61%** / `bf16` **−1.33%** ⇒ A8 的 −5.8% = 纯 kernel ≈ −1.5% + 接受率联动 ≈ −4.3%）→ **1** KVarN tail-partial ✅（**acc 旋一次** + FP64 oracle）→ **2 归并改造 ✅**（`partial_acc` BF16→FP32）→ **3 分区与接线 ✅**（三分区 + 尾环写入接入 append + 解 `startup.cpp` fail-fast；**e2e 全过**）→ **4 A6/A7 脚手架**（W7）→ **5** 容量/显存 → **6** 质量/速度收口（**显式处理 A4 的 N>384 风险**）→ **7** 回归（含**新增 `--vision` × kvarn 功能门**）。**开工须用户明确授权** |
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
> 环境纪律、建议的第一条命令。**最后更新 2026-10-08（本轮 08-19；WP6.3 已完成、其提交已落地、工作树干净）。**

**任务**：在 `D:\ninfer\ninfer-precision-tail` 继续 KVarN 移植 —— **WP6.0–WP6.3 已完成**（§3-08-14/-15/-16/-18/-19），
**下一步 = WP6.4（A6/A7 脚手架）**，其后 **WP6.5 容量/显存 → WP6.6 质量/速度收口 → WP6.7 回归**（每步在计划书 §7-WP6 有产出/验收/回退）。
⇒ **唯一主线 = 按 WP6.4 → WP6.7 推进**；**开工须用户明确授权**。
其余：**A1 邻域的视觉功能门**（并入 WP6.7）与**覆盖残留**（低优先，非交付面）。
**仓外项已闭环**：子代理图谱「须重启」已证实、重启充分并已多次实战。

**先读（顺序）**：本文件 `kvarn-port-progress.md`（**§0 快照 / §3-2026-10-08-19（最新）/ §4.1 / §5**）
→ `kvarn-port-into-precision-tail-plan.md`（**版本头 v21**、§1 A1–A8、§6.1/6.2/6.3、**§7-WP6（WP6.0–WP6.7 分步计划）**、§7-WP7、附录 D-11…**D-24**）
→ `AGENTS.md`（**「Codebase memory」小节的 Change flow A1–A9**；其历史与依据见 `docs/port-records/AGENTS-ARCHIVE-2026-10.md`，正文引用为 "archive §X"）。
二者是唯一权威；**冲突时以本文件的实测为准并回写计划书**（计划书 §0.5 规则）。

**当前状态（务必先核验，勿臆断）**
1. **GPU 空闲**（显存 0 MiB）；`ninfer_tests` 等目标**可构建**（08-19 构建 `ninfer_tests` exit 0）。
2. 工作树**干净**：WP6.3 及本轮的文档/纪律改动**已全部提交**，`HEAD = 7ea2b0bd`
   （`f0fc7696` feat / `bac98467` test / `7a934782` docs(kvarn) / `58b3baf3`+`5d51c6f6` docs(agents) / `7ea2b0bd` docs(kvarn)）。
   `AGENTS.md` 的旧未提交改动**已随 `58b3baf3` 入库**。
3. **WP6.3 已通**：`kvarn:k4v4 + --kv-tail-tokens 1024` 端到端可跑；`ninfer_kvarn_test` **exit 0**（直测 6 例 8.0e-7…1.42e-6 / 启动 2 例 1.33e-6、2.14e-6 / op 级合并 2 例 **3.161e-3、3.229e-3**，判据 8e-3）；同族 `..._continuation_image_test`、`..._tail_row_reset_test` OK（§3-08-19）。
4. **P3d 后 GPU 回归 3 项全绿**（§3-08-11）；`ninfer_qwen3_5_score_real_test` 用 `Fp8E4M3Row256`，**非 KVarN** ⇒ **不覆盖** kvarn 路由。
5. **子代理图谱：已闭环并多次实战**（MCP 名连字符→下划线已修；重启后 `list_projects` 正常、`trace_path` 与主会话一致；08-19 门禁① sweep 亦成功）⇒ 结构性/否定性结论**可交 `codebase-memory*` 子代理**。

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
5. **【主线】WP6 — 路线已裁决 = (a)（08-13，用户确认）；WP6.0 已完成（08-14，§3-08-14）。** 按计划书 §7-WP6 的 **WP6.0 → WP6.7** 推进：
   - **WP6.0 ✅ 已完成（08-14）**：**0a 本机 prefill 噪声底** = 短 67-token prompt 相对极差 **26.5%**（中位 231.0 / 最差 194.3）、
     1073-token prompt **3.1%**（中位 1600 / 最差 1550）⇒ 确认 A1 删 prefill 腿正确、并给出"prefill 判据须长 prompt + ≥3% 容差"前提；
     **0b 外部尾纯 kernel 代价**（MTP 关、decode-only、3 重复、tg 极差 ≤0.15%）= `rk4v4` **−1.61%** / `bf16` **−1.33%**
     ⇒ **A8 的 −5.8% 分解为「纯 kernel ≈ −1.5%」+「接受率联动 ≈ −4.3%」**。**未做严格配对 A/B**（用 `rk4v4` 而非 `rk4v4-e8`、关 MTP）。
   - **WP6.1 ✅ 已完成（08-15，§3-08-15）**：`src/ops/kvarn/tail_partial.{cuh,h,cu}` 旋转域 tail-partial（**acc 旋一次**）+ FP64 oracle，6 用例全过、余量 ~2000×。**08-16 另修先存竞态**（`reduce_output_hadamard_kernel` 补 2 个 `__syncthreads()`，§3-08-16）。
   - **WP6.2 ✅ 已完成（08-18，§3-08-18）**：`partial_acc` **BF16→FP32** + 删除 BF16 tile 暂存改 `float2` 直存；`reduce_output_hadamard_kernel` 的 `const float*` 读。**验收全过**：`ninfer_kvarn_test` 3 连跑逐位确定 + **codec 余量与 08-04 逐字相同** + 6 个 FP64 尾 oracle；**续列尾 e2e `RESULT: PASS`**（`cached_tokens=851`、message 逐字节同 r1）。decode 路线 vs FP64 oracle 余量**降 6.7–10.9%**，prompt 路线用例**逐位不变**。**⚠ A6 口径**：tail=0 输出**不再与改前二进制逐字节相同**（= BF16→FP32 的意图；A6 点名载体均为同版本内比较）。
   - **WP6.3 ✅ 已完成（08-19，§3-08-19）**：op 内三分区（`body_window = window − min(N, window)` / `body_active` / `tail_active`）+ 尾视图经 `KvarnPagedBatchLayerView.tail` 携带（**envelope 与调用点均未改**）+ `stage_exact_tail` 接入 append（`rotate_kv` **之前**，环存原始行）+ 解 `startup.cpp` 的 KVarN+tail fail-fast。**验收全过**（e2e 四臂 / 尾环 f16≡bf16 逐字节 / workspace **990.0/990.0 MiB** 无溢出 / 环增长 **+68.0 MiB** / MTP+尾 / 续列尾 `851` PASS）。**并修 3 个潜伏缺陷**（2 个 WP6.1 batch 偏移 + 1 个查询域缺陷）。产出：源码 10 文件 + 测试 +837 行；提交 `f0fc7696` / `bac98467`。
   - **▶ 下一步 = WP6.4 A6/A7 脚手架（W7）**：FP32 oracle 覆盖「KVarN body × 旋进 bf16/f16 尾」的**合并路径**；跨 **group(128)/ring(64)** 边界的 **needle 检索**；边界用例（`N ≤ ring`、`N ≥ width`、`N = 0`、跨 checkpoint 恢复），新测试 **~300–500 行**（照 `tests/ops/softmax_attention/causal_cache.cpp`，先 `get_file_outline`）。**必须在矩阵里补的已登记缺口**：① **`window > 8198` 的 KVarN split 分支带尾**（现**无任何 host oracle 覆盖**）；② **跨二进制 `tail=0` 逐位比对**（A6 字面口径）。**验收**：oracle 精确、needle 精确命中、无丢键/重复计数。
     → **WP6.5** 容量/显存（**既有参照 `tests/models/qwen3_5/test_exact_tail_capacity.cpp`**）→ **WP6.6** 质量/速度收口（**显式处理 A4 的 N>384 风险：至少测 `N ∈ {384,1024}`**）→ **WP6.7** 回归（含 **`--vision` × kvarn 功能门**）。
   - **✅ 尾 dtype 已裁决（08-18）= 维持默认 f16**（零 diff；理由见 §3-08-18 与计划书 D-22(7)）。
6. **已关闭 / 已裁决（勿再问）**：P3d ① 端到端对照（08-11）· 待裁决 3 项（08-11：不移植 FORK 测试 / 不补 tag / bench 暂不纳入）·
   **视觉只作功能判据**（08-12）· **接受"若采 bf16 尾则本机复核"**（08-12）· **WP6 路线 = (a)**（08-13）。
7. **计划书侧**：**v21 已回写**（本轮：**版本头 v21**、**§7-WP6.3 行已完成 + 测试尾留行**、**§7-WP6.4 行已点名"必须补的格子"**、**附录 D-24**、归档索引 D-24 行、§1-A6 口径注）。

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
- **子代理图谱：已闭环**（4 份 `~/.qoder/agents/codebase-memory*.md` 的 MCP 名连字符→下划线已修；重启后探针通过，08-12 起多次实战成功）⇒ 结构性/否定性结论**可交 `codebase-memory*` 子代理**；**不要**交给通用 Explore 类子代理（无 MCP）。
- **改共享契约 / 做否定性断言时按 `AGENTS.md` 的 Change flow（A1–A9）走**（Tier A 全流程 / Tier B 只 A7–A9）；**`search_graph` 的 `file_pattern` 需精确全路径**（`.*x.*` 不命中；`name_pattern`/`qn_pattern` 才是正则）。

**建议第一条命令**
```bash
cd /d/ninfer/ninfer-precision-tail && git status --short && git log --oneline -1 && nvidia-smi --query-gpu=memory.used --format=csv,noheader
```
若 GPU 0 MiB ⇒ 可直接跑。**历史进度（勿重新论证）**：08-12 §6.3 探针（否证 (b)）→ 08-13 裁决 WP6 = (a)、计划书拆出 WP6.0–WP6.7 → 08-14 WP6.0 → 08-15 WP6.1 → 08-16 修先存竞态（`reduce_output_hadamard_kernel` 补 2 个 `__syncthreads()`）→ 08-18 裁决尾 dtype + WP6.2 → **08-19 WP6.3 并全部提交**。**起步 = WP6.4**：
① **先按 `AGENTS.md` 的 Change flow A1–A9**（本任务 = **Tier A**）：A1 索引新鲜度 → A2 `search_graph` 枚举同类实现 → A3 grep 穷尽启动点 → A4 覆盖度 + `get_file_outline` → **A5 改动清单写进计划（逐项 可行/存疑/否决 + 风险 + 回退）** → **A6 状态迁移矩阵** → A7 实施 → **A8 按目标构建 + oracle/`racecheck`/FP64/跨路线一致性** → **A9 §3 + §0 + 回写计划 + `index_repository` + `git diff --check`**。
② **WP6.4 的三块新码**：`window > 8198` 的 split 分支带尾、跨 **group(128)/ring(64)** 边界的 needle、跨二进制 `tail=0` 逐位；用例形状照 `tests/ops/softmax_attention/causal_cache.cpp`（**先 `get_file_outline`**）。
③ **已知坑（省一轮）**：tail-only 的 op 级输出 = **`acc_orig/l`（无净 Hadamard）**；直测/启动用例的 oracle 须**反旋同一旋转后 bf16 查询**（`unrotate_query_like_kernel`）；**不要回退** `reduce_output_hadamard_kernel` 的 2 个 `__syncthreads()`；`exact_tail` 与 `linear_pair/q8` 的 `launch_exact_tail` **同名不同物**。
**WP6.4 及其后仍需用户明确授权后推进。** ✅ **尾环 dtype 已裁决（08-18）= 维持默认 f16**（零 diff；见 §3-08-18 / 计划书 D-22(7)；**不再待决**）。⚠ **A6 口径**：自 WP6.2 起 tail=0 输出**不再与改前二进制逐字节相同**（= `BF16→FP32` 的意图）；A6 点名的载体都是**同版本内**比较 ⇒ 按"同版本内逐位判据成立"理解；**跨二进制逐位比对已登记为 WP6.4 的待补项**。

**codebase-memory 图谱（2026-10-08 起可用，勿再被 SessionStart hook 误导）**：本仓已被索引
（`D-ninfer-ninfer-precision-tail`，**46,009 节点 / 216,983 边**；**08-19 WP6.3 边界重跑 `index_repository`，已含 WP6.1–WP6.3 与本次文档/纪律改动**）。注意：**SessionStart hook 仍报 "no indexed graph project
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
**08-19 更新（已写入 `AGENTS.md`）**：图谱纪律升级为**分层变更流程 A1–A9**（原三条硬门禁**并入其中**；**Tier A** 共享契约/共享类型/跨文件行为走全流程，**Tier B** 单点局部改动只走 A7–A9），并明确**图谱不覆盖 kernel 内部**（局部 `__shared__` 非节点、`<<<>>>` 非 `CALLS` 边、kernel 入度恒 0 ⇒ 同步/暂存复用/无竞态只能来自源码 + `racecheck`）。**`AGENTS.md` 的历史与依据已搬到 `docs/port-records/AGENTS-ARCHIVE-2026-10.md`（正文引用为 "archive §X"）**。**每个 WP 边界跑 `index_repository`**。
**实战印证**：08-16 `search_graph name_pattern=".*reduce_output.*"` 给出全仓**恰好 4 个** reduce-output kernel；**08-19 门禁① sweep** 同法给出 `kvarn_exact_tail_partition` **in=2**（body + 尾两个调用者，与"共用一分区"的设计一致）、`exact_tail` 与 q8 **命名撞车**、**既有 `test_exact_tail_capacity.cpp`**，以及 **`search_graph.file_pattern` 需精确全路径而非正则**（`.*x.*` 返回 0）——这些正是 grep 给不了的穷尽性证据。
---

## 6. 已归档信息索引（2026-10-08）

本日志精简时**只搬运、不改写**：下列内容全部逐字（字节级）移入归档文件，正文各处只留一行索引/指针。
§1 环境与构建速查、§2 关键路径与事实索引、§3 的 2026-10-08-14/-15/-16/-18/-19、§4.0/4.2/4.3、§5 handoff **保留在正文**。

| 归档文件 | 覆盖范围（本文件源行号） | 一句话内容 | 正文索引位置 |
|---|---|---|---|
| `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` §A | §3 条目 **2026-10-07-1 … -23**（260–1312） | WP0 基线、WP1 ops/测试、A3 的四轮追查、WP2/WP3/WP4 全过程与 229k 准入实验的逐日记录（含失败与被推翻的假设） | §3 开头的条目索引表（23 行，条目号 + 原标题逐字） |
| 同上 §B | §0「最后更新」散文段（16–34） | 08-05 / 08-04 / 08-03 三轮的实测摘要 | §0 压缩说明 |
| 同上 §C | §0 WP 状态明细表（36–47） | 每包当时的详表状态与实测数字 | §0 现值表（已压缩为现值 + 指针） |
| 同上 §D | §0 工作树改动清单（49–131） | WP1–WP5/P1 的逐文件改动（函数、守卫点、switch 位置） | §0 的「工作树改动清单」指针 |
| 同上 §E–§I | §0 未决项 1 / 5 / 8 / 9 / 10（135–142 / 146–167 / 177–192） | 已关闭或已被取代的未决项原文（含 A3 定案依据、页几何根因与修法、续列尾「未实现」旧表述） | §0 未决项列表中对应编号的索引行 |
| 同上 §J | §4.1 任务清单（1574–1607）与 §4.4 历史 WP 状态（1615–1648） | P0–P5 的原始「目标/内容/验收/风险/估时」与当时的 WP 状态 | §4.1 现行表 + §4.4 指针 |
| 同上 §K | 正文里被**订正**的 5 行原句（计划书原 800 行；本日志原 6、1669–1671 行） | 订正前的原文（含 fenced 逐字节副本）：附录 D 的旧标题、「配套权威（方案 v3）」、「工作树**未提交**」三行 | §4.3 的订正注 + §5 当前状态第 2 条 |
| 同上 **§L** | §3 条目 **2026-10-08-1 … -13 与 -17** | 已关闭的 P0/P1/P2/P3b/P3c/P3d、§6.3 探针、尾环 dtype 分析（含失败、被推翻的假设与原始命令） | §3 开头的条目索引表（对应行）+ §5 |

**未归档（现行推进计划需要）**：§0 现值表、仍开放的未决项 2 / 4 / 6 / 7 / 12 / 13、§1 全部、§2 全部、
**§3 的 2026-10-08-14/-15/-16/-18/-19（WP6 链）**、§4.0/4.1/4.2/4.3、§5。计划书侧的对应归档见
`docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md`（含 **D-10 … D-19**）与 `docs/port-records/KVARN-PLAN-CHANGELOG-ARCHIVE.md`。
**§0.5 记录规则不变**：WP 边界仍须「快照 + 日期条目 + 回写计划」，检索路径为 §3 索引表 → 归档 §A。
