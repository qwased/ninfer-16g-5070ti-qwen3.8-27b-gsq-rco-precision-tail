# 在 ninfer-precision-tail 内移植 KVarN（K4V4 / K5V5 / K6V6）+ 精度尾部：实施计划

- 版本：**v20**（2026-10-08）。本版依据 **WP6 路线裁决 = (a) + WP6 分步计划 + 两项前置测量入队**（用户确认；进度 08-13、附录 D-19 ⑧·⑨）：
  ① **路线 = (a)**（照 `small_t_k8v4.cuh` 形状把 KVarN 接进**外部共享精确尾环**）—— 量化依据：显存非决定因素（(a) 88 MiB/序列 vs (c′) 76.5 @C=1，差 **12 MiB**；C=8 差 104 MiB）、
  (a) 的增量性能代价可忽略（精确双写 64 KiB/token ≈ `6×10⁻⁶` 于权重流；最新 N 键多读 +46 MiB/步但 KV 仅占 decode 总流量 ~1.4% ⇒ 端到端 ≈ **+0.4%**；workspace +0.4 MB）、
  且消除双实现语义与 **BF16-acc 偏离**、判据与其它档位同构。**(c′)（body 自建滚动槽）否决**（省 ~3–5 天，换永久语义债）。
  ② **§7-WP6 改写为分步计划 WP6.0–WP6.7**（每步给产出/验收/回退），**不变量 = A6 的「`tail=0` 逐位不变」是 WP6.2 的接受条件**。
  ③ **两项前置测量入队（用户 08-12 指定）**：**WP6.0a 本机 prefill 噪声底**（重复 ≥5，补 WP0 产出 ② 的欠账）、**WP6.0b 现有外部尾的纯 kernel 代价**（关 MTP、`--kv-tail-tokens {0,1024}`、重复 ≥3，把 A8 的 `−5.8%/−2.1pt` 分解为「纯 kernel」+「接受率」）。
  ④ **尚未执行**：两项测量与 WP6.0–WP6.7 全部**未开工**（本版只落计划；WP6 工期仍为 10–15 天区间）。**A1–A8 判据不变**（新增：A1 邻域的**视觉功能门**与 prefill/A8 的测量项）。
- 版本：**v19**（2026-10-08）。本版依据 **§6.3 的 (a)/(b)/(c) 低成本探针已执行**（用户授权；进度 08-12、附录 D-19）——
  探针为读码 + **跨工程图谱 Tier-3**（`D-ninfer-KVarN` / `D-ninfer-beellama.cpp` / FORK），**未用 GPU**：
  **① (b) 首选路线不成立**。`ROTATED_K_ORIGINAL_V` 确存于上游（`beellama.cpp/ggml/include/ggml.h:461-466`：`AUTO/ROTATED/ORIGINAL/ROTATED_K_ORIGINAL_V`），
  但 (i) 该域只在 **`n_query_tokens > 16`** 时自动生效（`src/llama-kvarn.cpp:140-146`），**decode 与 16-token 投机块恒为 `ROTATED`**
  ⇒ 尾部合并真正发生的那些步**全部在旋转域**；(ii) V 的**量化与存储始终是旋转域**，"V original" 只是读出时对**反量化后的 V** 补一次 WHT
  （`ggml/src/ggml-cuda/kvarn.cu:3064-3080`、FORK `kvarn_attn.py:1757-1758`）⇒ **KVarN body 不能当 fp8 body 复用尾部接线**。
  更硬的一条：**坐标系本身就是障碍**（不是 §6.1 原文说的"实现欠账而非坐标障碍"）—— 本仓 `tests/ops/softmax_attention/causal_cache.cpp:1613-1623` 自述
  「旋转域档位（fp8-e4m3/nvfp4-g16/k8v4）的 reduce 核消费**旋转系 partial**，而原始 BF16 尾行不表达于该系」。
  **② (a) 是忠实路线**：照 TAIL 已有的 `small_t_k8v4.cuh` 形状（K/V 都旋、核内旋 Q、归并 **FP32** partial、**归并后只做一次反旋**），仓内模板现成。
  **③ (c) 弱且语义不符**：KVarN 内建 3 槽 = sink 组 + **滚动**首/末组（`attention.cu:84-89,100-114`），`claim_tail_slot` **可返回 −1 丢行**、
  `retire` 只清"整组覆盖"的页（`:291-315`）⇒ 不能承诺「最新 N 精确」；且它**本来就在跑**，接出来不产生新质量。
  **④ 另提出有界第四选项 (c′)**〔本版新增，**未实测**〕：把 KVarN 内建尾环由 3 固定槽泛化为 `⌈N/128⌉+1` 滚动槽，
  让 **body 自己在核内**保留最新 N 个精确组（复用 body 自身的合并 ⇒ **绕开 partial 契约 / 坐标系 / split 三项冲突**）。
  **⑤ 另记**：§6.2 条 4「旋转时点未定」对 KVarN body **已有答案 = 写时旋转**（`attention.cu:157-194` 的 `rotate_stage_kernel` 存的是 WHT 后 BF16；
  `rotate_on_stage = width <= kFusedStageMaxWidth(=16)`，`:25,590`）。
  **WP6 工期重估仍为 10–15 天**，但路线从「(b) 可能零内核改动」改为 **(a) 或 (c′)**。**A1–A8 判据不变。**
- 版本：**v18**（2026-10-08）。本版依据 **WP6 门禁评估**（进度 08-11(6)(7)、附录 D-18）：
  用户询问「是否已有足够证据可开始 WP6」⇒ **判定：不足**。功能门（P1/P2 完成）与安全回退（`kvarn + --kv-tail-tokens` fail-fast）**已满足**，
  但 **§6.3 的 (a)/(b)/(c) 低成本探针从未执行** —— 它被定为 **WP4 的前置**（§6.3 末句 / §7-WP4），却在 WP4 验收（只看三档准入）时被**静默跳过**，
  进度记录与归档**均无执行痕迹**。故 WP6 路线仍属假设：首选的 (b) `ROTATED_K_ORIGINAL_V` **只出现在**
  `docs/port-records/kvarn-kv-tail-feasibility-report.md:44` 的上游 ggml 散文里，**本仓代码 0 命中**；且
  ① `kvarn_hadamard`（`src/ops/kvarn/codec.cu:185-189`）只收 BF16 ⇒ 「f16 旋转入口」**仅在坚持 f16 尾时才需要**（**订正**：`--kv-tail-type bf16` 今天已完整支持、且 BF16 尾环是**逐位精确档**，见 D-18 订正）；
  ② **无任何测试把 kvarn 与 `--kv-tail-tokens` 组合**（A6/A7 的 oracle 与 group/ring 边界脚手架均不存在）。
  ⇒ **建议先授权 §6.3 探针**（有界、~0.5–1 天、读码 + 跨工程图谱、**不需 GPU**）判定路线并重估工期，**再决定是否启动 WP6**（10–15 天、最高风险）。
  另记：**子代理图谱已修复并经验证**（4 份代理定义 MCP 名连字符→下划线，**重启后探针通过**；`list_projects` 可见 6 个图谱工程含 FORK 与上游）⇒ 后续结构性/否定性结论可正式交 `codebase-memory*` 子代理；**此前几轮的此类结论一律按 grep-only 处理**。**A1–A8 判据不变。**
- v17（2026-10-08）。本版依据 **P3d ① 的端到端对照已跑 + 待裁决 3 项裁决 +「子代理无图谱」根因修复**（进度 08-11、附录 D-17）：
  ① 的 KLD 对照实测 `kvarn:k4v4` mean KLD **0.002120**（mean/median/P99/max/same_top/dlogp/PPL **全部与 D-11/P3a 逐位相同**）
  ⇒ 该协议上该修复为**无害 no-op**、**D-11/P3a 数值无需重测**；**但「必要性」仍无证据** —— 本协议窗口恒为 `context` 长
  （`plan_disjoint_windows`、`begin += context`）、页号每窗口从 0 重启 ⇒ 陈旧尾标记与当前页号**重合**；`Engine::score_tokens` 的
  **变长窗口**冲突（首选/备用尾槽皆被占 ⇒ 跳过 staging）**未实测**。⇒ §9 该行与 §7-WP2 邻域改判为「**已验证无害、未证必要**」。
  **待裁决 3 项用户已裁决**：**(a) 不移植** FORK 的 `test_prefill_precision_real.cpp`；**(b) 不补** `capture_identity_tag` 的 `kvarn_bits`
  （保留注释，避免作废既有磁盘缓存）；**(c) kvarn bench 暂不纳入**（维持 `NINFER_BUILD_BENCHMARKS=OFF`、不重配）。
  **P3d 后 GPU 回归 3 项全绿**（`ninfer_kvarn_test` 余量与 08-04 逐位同 / 续列尾 device 段 / `score_real`——后者用 `Fp8E4M3Row256`，
  **非** KVarN，不覆盖本路由）。另记**仓外工具发现**：`~/.qoder/agents/` 的 4 份 `codebase-memory*` 定义把 MCP 工具名写作**连字符**
  而运行时归一为**下划线** ⇒ 子代理**一直拿不到图谱**（上一会话两个子代理图谱调用为 0）；已修并备份，但**须重启 Qoder 生效**
  （重启是否充分未排除）。**A1–A8 判据不变**。图谱刷新至 45,944 节点 / 216,668 边。
- v16（2026-10-08）。本版依据 **P3d：审计 3 项 LOW 修复落地**（进度 08-10、附录 D-16）：
  ① **CausalScore 路由**补 `decoder->text_kv.reset_kvarn_tail_row(0, …)` —— 该路由硬编码 Main row 0、跨多次打分复用
  且**不经** `start_sequence`（故无 KVarN 尾镜像恢复），而尾槽标记在运行期**只被追加** ⇒ 上一占用者的标记会被读入；
  ② `commit.cpp` 的 `publish_active_continuation`（`noexcept`）在 `catch(...)` 前加 `catch (const std::logic_error&)`
  + stderr 诊断，**发布失败不再静默**；③ `state_image.h` 的 `StateImagePart`/`StateImageDevicePool` 注释补 KVarN 占用者。
  新增 `ninfer_qwen3_5_kvarn_tail_row_reset_test`（**实跑 PASS，ctest 262→263**）。
  ~~**⚠ ① 是生产行为变更，其端到端对照（`kvarn:k4v4` KLD 臂 vs P3a 的 `0.002120`）未跑**（用户指示不跑 GPU）⇒ 见 §9 该行。~~
  **→ 已于 v17 关闭**（08-11：逐位复现 `0.002120`，**无害但必要性未证**；附录 D-17）。
- v15（2026-10-08）。本版依据 **P3c：kvarn parser 单测**（进度 08-07、附录 D-15）：
  `ninfer_cli_options_test` / `ninfer_serve_options_test` 增 kvarn 用例（裸 `kvarn` 与 `kvarn:k4v4|k5v5|k6v6` 的
  存储 + `KvarnBits`、默认 `Bits4`、拒绝未发布拼写、help 三档；serve 侧加 `make_engine_options()` 贯通），两项 **exit 0**；
  **第三处 parser（`apps/perplexity/main.cpp`）因内联于 main.cpp 结构性不可单测**（行为覆盖由 P3b 的 e2e 提供）。
  **纯测试改动，A1–A8 判据不变**；关闭 §7-WP2 的最后残项。
- v14（2026-10-08）。本版依据 **P3b：kvarn 档位并入报告目录/日志名**（进度 08-06、附录 D-14）：
  `include/ninfer/types.h` 的 `MemorySummary` 增 `KvarnBits kvarn_bits`；perplexity / cli / serve 四个展示面渲染级别
  （**文本面 `kvarn:k4v4`；报告目录分量 `kvarn-k4v4`**，用户裁定，因 Windows 路径禁止 `:`）；**六展示面 GPU 实测 +
  6 项 host 回归全过**；非 kvarn 名字逐字不变。**仅补强展示面，A1–A8 判据不变。**
- v13（2026-10-08）。本版依据 **P2b 仪器改造 + token 级 kvarn MTP 实测**（进度 08-05、附录 D-13）：
  `test_engine_mtp_greedy_parity_real.cpp` 改造为**诊断仪器**（去跨配置逐位门禁 + 加**自确定性门禁** + 首分叉报告 +
  `kKProfiles` 并入 `rk4v4`/`kvarn:k4v4|k5v5|k6v6` 并贯通 `options.kvarn_bits` + `--quick`）；实测 **kvarn 自确定性全档成立、
  分叉不劣于基线**（`kvarn:k4v4`@k1/k3 与 greedy 逐字节相同；@k15 418/512 vs `bf16` 412/512，首分叉同为 token 91）
  ⇒ **A3-O1 判据成立**；§7-WP3 ⑤ 措辞订正。
  **v13 之前的完整版本头与 v3→v12 十张「变更摘要」表已逐字归档** →
  `docs/port-records/KVARN-PLAN-CHANGELOG-ARCHIVE.md`（本版只留下表索引）。
- 日期：2026-10-08
- 目标仓库：`D:\ninfer\ninfer-precision-tail`（下称 **TAIL**）
- 目标硬件：**仅本机** NVIDIA GeForce RTX 5070 Ti 16 GB / **sm_120a** / Windows
- 目标模型：`D:\ninfer\ninfer-precision-tail-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`（实测 **11,092,477,952 B / 10.33 GiB**，mtime 2026-10-02 05:59）
- 一句话目标：**在 TAIL 内移植 beellama 式 KVarN 4/5/6（K=V 同宽）+ 精度尾部，prefill / decode / MTP 核心体验零负面影响。**

> ## §0.5 记录规则（**必须遵守**）
>
> **信息必须落盘。本进度唯一落盘点：`kvarn-port-progress.md`（同目录）。**
>
> 1. **每一步都落盘**：任何推进——含成功的、失败的、被推翻的、以及尚未定位的——都必须追加到
>    `kvarn-port-progress.md` §3 的日期记录里。**失败与未决结论不得省略、不得只留在会话上下文里。**
> 2. **每个 WP 收尾（含中止）三件事**：① 更新进度日志 §0「当前状态快照」；
>    ② 在 §3 追加一条记录（含实测数字、命令、原始输出片段、判定与依据）；
>    ③ 凡影响**验收口径**（A1–A8）、**未决项（§10 D1–D6）**、**风险登记（§9）**的结论，
>     **回写本计划书**对应小节或附录 D，而不是只写在日志里。
> 3. **实测优先**：日志与本计划冲突时，以日志中的**一手实测**为准，并立即回写订正本计划书。
>    本计划中所有"依据"列若被实测推翻，必须显式标注「已被 WPx 实测推翻」而非删除。
> 4. **命令与口径可复现**：记录必须给到可复制的命令（含 `env-port.bat` 调用方式）、
>    工作负载（模型路径、prompt、档位、并发、重复次数），以及"未做/未测"的显式声明。
> 5. **交付物**：WP 的产出物包括日志记录；日志未更新视为该 WP **未完成**。

---

## 变更摘要索引（v3→v12 已逐字归档）

> 每版「v(n-1) → v(n)」的差异表**逐字**移入 `docs/port-records/KVARN-PLAN-CHANGELOG-ARCHIVE.md`
> （源第 28–132 行；含各版的实测数字与依据）。下表只留版本→一句话，供回查；**v13 相对 v12 的摘要仍在下一节**。

| 版本 | 一句话依据 | 主要回写位置 |
|---|---|---|
| v3 | 16 项修正：难度中高 / 工期 34–52 人日 / §2 证据分级 / §3–§6 数字更正 / A1·A4·A5·A8 口径改写 / 新增 WP0.5-B 准入门 | §1 §2 §4 §5 §6 §7 §10 |
| v4 | **A3 判据按 O1 替换**（绝对逐位 parity → 相对判据），parity 测试降级为诊断仪器（附录 D-6） | §1 A3、§7 WP0.5-A/WP3/WP9、§9 |
| v5 | WP2 实测：D1/D2 定案、A1 的 `ctest` 口径收窄、§4.4 页-shift 陷阱不可达（附录 D-7） | §1 A1、§4.4、§7、§10 |
| v6 | WP3 起手：挂载点改为模型执行层 `execution/text.cpp` 分派、`kvarn:k4v2` 内部档、地址空间页几何列为硬阻塞（进度 07-14） | §3、§7 WP3、§10 |
| v7 | WP0.5-B 起手：准入实验被 WP3① 前置；首个 KVarN 数据点为正（附录 D-8） | §1 A4/A8、§7 WP0.5-B |
| v8 | WP3① 完成：页几何按存储贯穿，`kvarn:k4v2` ctx8192 / 229,348 token 跑通（附录 D-9） | §7 WP3、§9 |
| v9 | 229k 同字节矩阵齐备 ⇒ 代理门禁为正；`k6v6` 门槛量化为 < `k8v4` 的 **0.002688**（附录 D-10） | §1 A8、§7、§9 |
| v10 | WP4 完成 + **发布档三档准入全过**（`k4v4` 0.002120 / `k5v5` 0.001432 / `k6v6` 0.001233）（附录 D-11） | §1 A2/A8、§5、§7、§10 |
| v11 | WP5 容差形式化为**量化步长判据** + P1 续列尾专项单测 + 独立审计（进度 08-03） | §1 A2、§7 WP3/WP5 |
| v12 | GPU 收尾四件 + P2 前置（附录 D-12、进度 08-04） | §1 A2、§7 WP3/WP5 |

### v13 相对 v12 的变更摘要（依据：P2b 仪器改造 + token 级 kvarn MTP 实测，2026-10-08，进度 08-05，附录 D-13）

| # | v12 的说法 | v13 修正 | 依据 |
|---|---|---|---|
| 1 | §7-WP0.5-A/§9：`test_engine_mtp_greedy_parity_real.cpp` 应「改造为诊断仪器」（**未动代码**） | **已改造**：去跨配置逐位门禁；**加自确定性门禁**（repeat1 必须逐字节复现 repeat0，A3-O1 的①）；跨配置降级为首分叉报告；`kKProfiles` 并入 `rk4v4`/`kvarn:k4v4|k5v5|k6v6` 并贯通 `options.kvarn_bits`（**此前恒为默认 Bits4**）；加 `--quick`（代表性档 × sample0 × 宽度{0,3}），`tests.cmake` 注册 `TEST_ARGS --quick`，全扫仍可手动跑 | 进度 08-05、附录 D-13 |
| 2 | §7-WP3 ④「MTP 路径激励与 A3 相对判据」 | **完成（08-04 激励 / 08-05 仪器）**：token 级实测 **kvarn 自确定性全档成立**；**分叉不劣于基线** —— `kvarn:k4v4`@k1/k3 与 greedy **逐字节相同**，`bf16@k1` 分叉 413/512、`rk4v4@k3` 408/512，@k15 kvarn 418/512 vs bf16 412/512（**同首分叉 token 91**）。⇒ A3-O1 判据成立 | 进度 08-05、附录 D-13 |
| 3 | §2 事实索引 / §7-WP3 的 `small_t.cu:460-505` 措辞 | **订正**：`small_t.cu` 路径是 **bf16（非 kvarn）** 的真实体路由（§2 事实索引保持）；kvarn 走**模型执行层 `text.cpp` 分派**，**不存在 `small_t_kvarn`**（§7-WP3 ⑤） | 跨仓 diff、进度 07-14/08-05 |
| 4 | （未评）分叉的 (档,宽度) 特异性 | **新事实**：分叉随**档与宽度**出现/消失（`bf16@k1` 分叉而 `@k3` 不分叉；`rk4v4` 相反；`kvarn` 在 k=1/3 不分叉、k=15 分叉）⇒ 与「跨配置无逐位保证」一致，**不是**某个档或 kvarn 的独有缺陷 | 附录 D-13 |

---

## 0. 结论摘要

| 项 | 判定 | 依据 |
|---|---|---|
| 路线 | **只有 R2 一条路：在 TAIL 内移植** | §2 —— 原计划推荐的 R1（以 `ninfer-rtx5090-mobile` 为基线）对本目标**不可行** |
| 移植难度 | **中高**：设备侧接口兼容（已证 `wave_splits` 默认参），但**宿主侧注册、页几何、共享面、旋转域为硬改造** | §4.1 —— kvarn 只依赖 5 个 device helper + 2 个 host 入口，但两处 host 硬 throw、44 文件共享面、页 64 进内核寻址、FORK 20 文件接入面 |
| 主要工作量 | **三项**：① 位宽参数化 k4v2→K=V∈{4,5,6}；② 旋转域尾部合并（WP6）；③ 共享面/宿主注册改造 | §4 / §5 / §6 |
| 与尾部协同 | 可组合，但要过**旋转域门禁 WP6**；未过之前 `kvarn + --kv-tail-tokens` 必须 **fail-fast** | §6 |
| prefill/decode/MTP 零负面影响 | 有支撑（纯增量 + 关闭态门禁），但**吞吐/质量必须本机自测**，且**已知外部尾部有代价**（−5.8% decode / −2.1pt 接受率）；**MTP on/off 的"跨配置逐位一致"非本仓承诺**（A3 已改为相对判据，附录 D-6） | §7 A1/A8；§1 A3 |
| 总工期 | **约 34–52 人日**（含 WP0.5 前置门 2 天；GPU 实测另计） | §7 |

**与 v1 的三处根本差异（重写主线，保留）：**

1. **路线反转**：v1 推荐 R1（fork 为新基线）并列为「优点：kvarn 与尾部都在」。**该优点为假**（fork 无 `small_t_tail*`、全库 0 处 `kv-tail-tokens`）；更硬的一条是 **fork 根本没有 GGUF 权重路径**（`src/artifact/formats.cpp:10-20` 只注册 9 种格式到 `fp8_e4m3fn_row_bf16`，代码内 `gguf` 0 命中），**加载不了本目标模型**（其清单 1189 张量中 398 个为 `gguf_blocks_v1`）。⇒ R1 出局。
2. **验收指标换仪器**：A4 不能用 `apps/perplexity` 的 ppl —— 它评 prefill，**结构上看不见尾部**（`docs/port-records/PORT-MEMORY.md:385-393`）。改用 TAIL **已有**的 **decode-width KLD**（`apps/perplexity --score-width ≤8 --kld-base`，真实解析在 `apps/perplexity/main.cpp:175-177`）。
3. **数字更正**：KVarN 自带 sink/tail 显存是 **24.0 MiB/序列（K+V）**，不是 12 MiB；生产乘数是 **16 个 full-attention 层 × 4 KV 头**（不是 «16 层» 或 «H16»）；表须补 MTP 第 17 个池（+6.25%）。见附录 A。

---

## 1. 目标与验收标准

**产品目标**

- `--kv-dtype` 暴露三个 KVarN 档位，**恰好 3 个，K=V**：`kvarn:k4v4 | kvarn:k5v5 | kvarn:k6v6`（默认候选 `kvarn:k4v4`，见 §10-D2）。
- 可与 `--kv-tail-tokens N`（+ `--kv-tail-type`，**默认 f16**）叠加：KVarN 作 body，精度尾部作高精度层。
- **默认 `--kv-dtype` 不变**（当前产品默认，非 bf16 也非 kvarn）；KVarN 全部 opt-in。
- 不发布 k4v2、不做 K≠V 混合档、不做 g64。

**验收标准（逐条给出可证伪口径）**

| # | 标准 | 证据形式 | 阈值（修正后） |
|---|---|---|---|
| A1 | KVarN 关闭时**零回归** | `ctest` 全绿 + **decode-only** tok/s 基线对照（同 `.ninfer`、同 prompt、重复 ≥3 次）+ 输出逐字节相同 | **decode tok/s `\|Δ\|≤0.88%`**（`docs/port-records/PORT-MEMORY.md:766-769`）+ 逐字节相同 + `MemorySummary` 逐字相同。**删除 prefill 腿**（同二进制单请求 prefill 实测 `−29%…+8%`，噪声底过大不可用，`docs/port-records/PORT-MEMORY.md §5.17(2)`） |
| A2 | 每档编解码与**独立 oracle** 一致 | FP64 Sinkhorn/RTN oracle + Hadamard oracle + 记录解码检查 + **位序往返**（pack→unpack 逐码比对） | 容差**按量化步长定义**（`qmax=(1<<bits)-1` ⇒ `q=(max-min)/qmax*row_scale*col_scale`）。**✅ 08-03 形式化（v11，进度 08-03）**：`oracle_relative_l2_limit(bits)=1.0e-3`（07-21 的拟合阈值）**已删除**，改为 **①点值判据 `\|actual-expected\| ≤ q*(1+5e-2)`**（5e-2 吸收设备/oracle 各自 Sinkhorn 的 scale 差异）+ **②总量判据 `flips ≤ 1.0e-3*total`**（边界翻码计数上限）+ **③`\|Δcode\|>1` 零容忍**；并补 4/5/6 位**穷举逐码** pack→unpack 往返（含行外哨兵）。根因已定量为「单元素落在量化边界、设备与 oracle 各自舍入到相邻码」（`max_abs≈0.216`=一步；速度/自洽性由 stored-bit 2.0e-7 + 往返钉死）。**✅ 08-04 余量实测（v12，进度 08-04）**：`flips` 最大 **1/65**、`over_step=0`、`wide_flips=0`、`exit=0` ⇒ 预测吻合、**65× 余量、判据成立且未放宽**，WP5 收口 |
| A3 | **MTP 一致性（相对判据）** —— 原"主文本与 MTP 的 greedy 与 MTP-off **逐位**一致"**已废弃** | WP0.5-A 改造后的诊断仪器 + decode-width KLD；见附录 D-4/D-6 | ①**同配置自确定性**：同档重复运行逐字节相同；②**质量一致**：kvarn 档 KLD 与同配置基线一致；③**首分叉已文档化**：工具报告各 MTP 宽度首个分叉下标；④**kvarn 专属门禁**：kvarn 档的 MTP 一致性不得劣于同配置的 `bf16`/`rk4v4` 基线。覆盖 MTP 深度 0..3、跨 ≥1 group 边界、上下文 ≥8K。**⚠ 2026-10-07 定案：绝对 parity 在上游 base 上即不成立（纯 bf16 k=1 分叉、k=0/k=3 全等；上游父仓库输出逐字节相同）⇒ 非本移植引入；上游 #80 明示拒绝该判据，本仓 `docs/performance.md:29-45` 早已实测否决；其真实出处是 FORK B 私有合同（附录 D-6）** |
| A4 | 精度尾部在 KVarN body 上有**可测质量增益** | **decode-width KLD**（`--score-width 8`，协议 `--disjoint --score-topk 100`、32,767 评分 token） | 最小效应量 = **同档 rk4v4 在 N=1024 的 pairing 带（2.26–2.47×）的 50% ⇒ ≥1.13×**；**附 `same_top` 与 max-KLD 双指标**；tail on/off、重复 ≥3 |
| A5 | 每档显存与**修正后**的 §3/附录 A 表一致 | `MemorySummary` 实测比对 + 本产物实测权重 | ±5%，基准表须含：① KVarN **不可关**的 24.0 MiB/序列 sink+tail；② StateImage slot × 并发项（**WP7 待核实机制，见 §7-WP7**）；③ **本产物** `weightsBytes = 11,092,477,952`（**不是** `config-calculator.html:522` 的 17,093,490,688，那是 groupwise-int 产物，且原文免责「不适用于不同量化的权重产物」） |
| A6 | 尾行旋进坐标域后**逐位可控** | FP32 oracle 覆盖「KVarN body × 旋进 BF16/F16 尾」合并路径；tail=0 时输出逐位不变 | 精确 |
| A7 | 长解码跨 group(128)/ring(64) 边界无重复计数/丢键 | needle 检索 + 边界单测 | 精确命中 |
| A8 | **无负面体验**：pp/tg/MTP 接受率 | 三者与**同字节对手**对照 | **同字节对手：`k4v4↔{rk4v4, nvfp4}`、`k6v6↔k8v4`（逐字节相同，402 B/token/头）；`k5v5`（21,632）无同字节档 ⇒ 需另定判据**。pp/tg/MTP 接受率不得劣于同档噪声底；**并须承认外部尾部已知代价 decode −5.8% / 接受率 −2.1 pt**（`docs/port-records/PORT-MEMORY.md:663-670`）——含尾部的档位按此基线放宽判据 |

> **A1 的 `ctest` 口径（2026-10-07 WP2 修正 + 2026-10-08 全量带 artifact 复核，见进度 §3-08-01、附录 D-7）**：
> 本机**并非**「261 项全绿」。**2026-10-08 全量（带 `NINFER_TEST_ARTIFACT`、并发）261 项 = 245 通过 / 7 跳过 / 9 失败**，
> 9 项失败**逐项判定均非本移植引入**：
> ① **先存（改动面外）**：`ninfer_device_sync_empty_test`（`ENVIRONMENT` 未把空串交给 `getenv`，
> `tests/cmake/CoreTests.cmake:82-83`）、`ninfer_gdn_gating_proj_test`（route 区间未命中端点）；
> ② **产物结构性**：`prefix_real`（无 prompt golden）、`dflash_real`/`dflash2_real`（产物缺组件）、
> `moe_real`（本产物非 35B MoE）；
> ③ **已知 A3**：`mtp_greedy_parity_real`（token 91 `expected=2466 actual=2640`，与 D-4 逐字相同）；
> ④ **先存 + artifact 门控 + 首次观测**：`ninfer_qwen3_5_vision_workspace_test`（主机标定上限 827 MiB 被
> 1,205,905,409 B 超出；馈入代码不在改动面内，隔离复跑逐位复现；此前所有基线运行均为 Skipped ⇒ 从未取基线）；
> ⑤ **并发显存争抢**：`hybrid_prefix_real`（`-j` 并行下 `only 0 bytes are available for runtime capacity`；
> **隔离复跑**回到 D-7 的 `missing component dflash2`）。
> 另：`ninfer_kvarn_test`（WP1 曾容差失败）**现已通过**（07-21 按位宽重定容差）。
> 故 A1 的「`ctest` 全绿」读作「**除上述先存 / 产物结构性 / 已知 A3 / 并发环境项外全绿**」。
> **未修 #89**：先存、与 kvarn 交付面无关，归文档/测试校准范畴（非本计划交付面）。
>
> **A1 的视觉口径（2026-10-08 用户裁定：视觉**只作功能判据**，不设视觉性能判据）**：依据 = 视觉塔走**独立算子**
> `ops::packed_softmax_attention`（`src/models/qwen3_5/execution/vision.cpp:385`），且**完全不使用分页 KV**
> （`vision.cpp` 对 `text_kv`/`batch_text_kv_` **0 命中**）⇒ kvarn / 精度尾**结构上无法改变视觉算子**；风险只剩
> ①**显存预算**（归 A5）与 ②**共享代码面**（归 A1 的 `ctest` 口径）。**待补的功能门**：`--vision` × `kvarn:k4v4`（以及 WP6 落地后的 × tail）
> **端到端可用 + 自确定性** —— **当前无任何测试把 vision 与 kvarn 组合**（`tests/models/qwen3_5/test_vision_workspace.cpp:46` 只测 `Fp8E4M3Row256`；
> `test_vision_cpu_real.cpp` 无 kv-dtype 参数化）。**A1–A8 编号与判据不变。**

---

## 2. 路线：为什么只能是 TAIL（R2）

**排除 R1（以 `ninfer-rtx5090-mobile` 为基线）的证据（按独立性分级）：**

| # | 事实 | 独立性 | 证据 |
|---|---|---|---|
| 1 | **跑不了目标模型**：fork 无任何 `gguf_*` 格式、无 `src/ops/linear/gguf/` | **独立**（本计划实测 fork 注册表） | `FORK src/artifact/formats.cpp:10-20`（只到 `fp8_e4m3fn_row_bf16`）；`gguf` 代码 0 命中；目标清单 398/1189 张量为 `gguf_blocks_v1`，FORMA 注册表实测 11/18 种 `unknown numeric format` ⇒ 清单解析即失败 |
| 2 | **没有精度尾部** | 复述 `outdate/` 旧审阅 | fork 全库无 `small_t_tail*.cuh`；`kv-tail-tokens` 0 命中 |
| 3 | 已是另一条产品线：分叉规模巨大 | 复述 `outdate/` 旧审阅 | TAIL-only ≈1027 / fork-only ≈48（子代理复算 1050/50，共同末点 `f76e19c0` 2026-09-17）；`src/` 交集 862 中仅 278 内容相同 |
| 4 | **fork 是 Linux-only 构建**（v2 遗漏，最硬） | **独立** | `FORK README.md:31` 要求 **64-bit Linux + RTX 5090**；`cmake/Dependencies.cmake:1-5` 无条件 `find_package(PkgConfig REQUIRED)` + `pkg_check_modules(FFMPEG REQUIRED)`；**自有代码与构建系统 `WIN32`/`_WIN32`/`MSVC` 命中 0 处**（182 处命中全在 `third_party/` 的 vendored 库）；无 `.bat`/`.ps1`/`vcpkg.json`、无 Windows 构建树（有 `Dockerfile`）；本机无 pkg-config ⇒ R1 在本机 Windows 上连 configure 都起不来 |

**资产拓扑（修正）：**

```
Neroued/ninfer (upstream, RTX5090)  ──►  MirkoCovizzi/ninfer-rtx5090-mobile  ← kvarn k4v2（无 GGUF、无 tail、Linux-only）
ninfer-3090 (ashalliants/…, master 含 v0.11.0)  ──►  Ryan-gsq/ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco (GSQ，经 iamwavecut/ninfer-all 聚合)
                                     └─►  qwased/…-precision-tail  ← **本目标仓库 TAIL**（有 GGUF/vision/MTP/精度尾部，无 kvarn）
```

**结论：kvarn 的唯一可用参考实现是 fork 的 `src/ops/kvarn/`（k4v2），要把移植进 TAIL。** 参考实现与本目标跨产品线，故 WP1 只搬 ops（自包含），WP3 才做模型接入。

---

## 3. 现状架构与对接点（TAIL 侧，file:line）

| 关注点 | 位置与事实 |
|---|---|
| 存储枚举 | `include/ninfer/types.h:49-73` `enum class KvCacheStorage`：`BFloat16/Int8Group64/Fp8E4M3Row256/RotatedInt8KeyInt4ValueGroup64/Nvfp4Group16/Fp8KeyNvfp4Value/RotatedLloyd4KeyInt4Value/…E8/…E8Root`。**:67 明确要求新值「追加在末尾以保持既有编号」**（中段插入会静默重键 `;kv=` 指纹与磁盘 KV 目录名） |
| 名字 helper（v2 漏） | **3 个生产穷举 switch**：`src/serve/operational_log.cpp:129 kv_cache_name`、`src/serve/request_log.cpp:134 kv_cache_name`、`apps/cli/main.cpp:83 format_kv_cache`；**4 个基准**：`bench/inference/ninfer_bench_support.cpp:951`、`bench/ops/causal_softmax_attention_bench.cu:663`、`bench/ops/kv_cache_append_bench.cu:531`。均在 switch 后 `return "unknown"` ⇒ 新增枚举值**产生警告/静默落 "unknown"，不保证编译失败**（未开 `-Werror=switch`） |
| CLI 解析（三处） | `src/serve/serve_options.cpp:53-69 parse_kv_dtype`（dispatch 在 **:808-809**）+ `apps/cli/options.cpp:110-125 parse_kv_cache`（dispatch `:387`）+ **`apps/perplexity/main.cpp:194-217`**（第三个 `--kv-dtype` parser，v2 只列两个） |
| 物理布局 | `src/core/paged_kv_storage.h:15-29 PagedKVVectorLayout` / `:32-56 PagedKVStorageLayout` / `:58-121 paged_kv_storage_layout()`（唯一真源）；另 `src/ops/kv_cache/d256_profile.h:86-129 d256_kv_cache_profile()` |
| **宿主硬抛（不可绕）** | `src/models/qwen3_5/state/decoder_state.cpp:31` 无条件调 `d256_kv_cache_profile(storage)`；该函数 `ops/kv_cache/d256_profile.h:128` **throw `unknown KV-cache storage profile`**；`src/core/paged_kv_storage.h:120` 同构 throw `unsupported paged KV-cache storage geometry`；`src/calibration/device_calibration.cu:699` 调 `paged_kv_storage_layout(storage, head_dim)`。**新枚举必须先在这两处 host switch 注册，否则启动即崩** |
| 页大小 | `src/core/paged_kv_cache.h:19 kPagedKVPageSize = 64`；**:80-86 `KVPageGeometry`**；`paged_kv_cache.cpp:40-41 validate_geometry` **硬拒 `page_tokens != 64`**，`:253` 同断。`kPagedKVPageSize/Mask/page_tokens` 在 `src include apps` **166 处命中**；`src/ops/kernel/paged_kv_address.cuh:9-10` 定义 `kPagedKVPageShift=6`、`kPagedKVPageMask=kPagedKVPageSize-1`，`:68` 直接进内核地址算术（`position & kPagedKVPageMask`） |
| body 路由/挂载 | 真路径 **`src/ops/softmax_attention/dense/causal_cache/`**：`causal_softmax_attention.cpp:347 causal_attention_resolve_route`（→ `small_t.cu:460-505 causal_attention_small_t_launch`，**是 if-early-return 链不是 switch**，kvarn 在此挂入）。`width<=8 ? SmallT : ChunkedSmallT` 在 **`causal_softmax_attention.cpp:392`** |
| 尾部合并 | 分区 `small_t.cuh:139-148 CausalSmallTTailPartition{body_window,body_active,tail_active}` + `causal_small_t_tail_partition`；尾 partial 核 `small_t_tail.cuh:56`（**`float* partial_acc`**）；**无独立 merge 核**，由共享 reducer 一次 online-softmax 合并（`small_t.cuh:261-347`，**`const float* partial_acc`**） |
| 尾部 ring 写入 | `src/ops/kv_cache/append/kernel.cuh:73-106 kv_cache_append_tail_bf16_kernel`（cached/prefill）+ `small_t_tail_shadow.cuh:23-69`（fused append） |
| 尾部参与方（v2 漏 Prompt） | **BF16 + INT8 族 + Prompt 路线**：`prompt.cu:13` 就 `#include small_t_tail_shadow.cuh`；`small_t_{fp8,nvfp4,k8v4}.cuh` 都 `#include small_t.cuh`（尾部代码**被编进但不走**） |
| 尾部配置链 | `apps/cli/options.cpp:132-144` / `src/serve/serve_options.cpp:71-79,810-813` / `include/ninfer/types.h:467-469`（`kv_tail_tokens=0`、**`kv_tail_type=Float16`**）→ `DecoderStateSpec` → `state/decoder_state.cpp:118-161 ExactTailCacheLayout`（尾部池 **:126-131 硬写 `.page_tokens = kPagedKVPageSize`**） |
| 模型程序 | `src/models/qwen3_5/`：`execution/text.cpp`（attention 派发）、`program/decode.cpp`、`program/prefill.cpp`、`program/planning/{startup,graph_profiles}.cpp`、`program/speculative/`、`program/storage/`、`state/{decoder_state,state_image}` |
| MTP/vision（v2 引用错） | `src/serve/serve_options.cpp:814+`：`--spec mtp\|dflash\|dflash2`、`--draft-tokens`、`--vision*`（**:292-334 是 help 文本，不是解析**）。尾部**不进 graph key、不建新 route family**（设计意图） |
| 身份指纹 | `src/runtime/engine/model_instance.cpp:183-185`：`;kv=<ordinal>`、`;kvt=`、`;kvtt=f16\|bf16`；磁盘 KV 目录名 `program/storage/disk_tier.cpp:41` = `kv<ordinal>_main…` |

**两个必须先厘清的「tail」歧义（保留）：**

| 含义 | 归属 | 本方案处理 |
|---|---|---|
| KVarN 自带 sink(sink 槽) + 尾槽，**合计 3 槽 = 1 sink + 2 动态尾** | kvarn ops 内部 | 作为 kvarn 记录的一部分移植（`kKvarnSinkPages=1`/`kKvarnTailSlots=3`，`FORK paged-kv-cache.md:100-101`） |
| **外部精度尾部**（最近 N token 的 F16/BF16 ring，`--kv-tail-tokens`） | TAIL 已有 | 本方案 WP6 的协同对象 |
| GDN 的 `tail_tokens`（ragged 尾计数） | 同名不同义 | 不相关，勿混 |

---

## 4. 移植方案

### 4.1 可行性裁决：**port with heavy adaptation**（v2 的「light / 中低」被推翻）

| 检查 | 结论 |
|---|---|
| FORK `src/ops/kvarn/`（**2980 行 / 12 文件**：`attention.cu` 615、`codec.cu` 174、`decode.cu` 222、`decode_kernel.cuh` 1044、`streaming_prefill.cuh` 353、`sinkhorn.cuh` 167、`materialized_prefill.cuh` 157、`store.cuh` 133、`hadamard.cuh` 60、`decode.cuh` 26、`config.cuh` 25、`sources.cmake` 4）+ 公共头 `include/ninfer/ops/{kvarn.h 77, kvarn_attention.h 53}` | 自包含；外部仅依赖 `core/{device,arena,tensor}.h`、`ninfer/ops/softmax_attention.h`、`small_t.cuh`、`prompt_common.cuh`、`launch.h` |
| kvarn 用到的小 `small_t` 符号 | `causal_small_t_active_splits`(3 参调用)、`causal_small_t_tc_swz`、`causal_q_index`、`causal_partial_acc_index`、`causal_partial_stat_index`、`causal_attention_split_capacity`、`causal_softmax_attention_workspace_capacity_bytes` |
| **设备侧签名兼容（已证）** | TAIL 的 `causal_small_t_active_splits` 签名只多**默认参数** `wave_splits=0`（`small_t.cuh:104-105`），kvarn 的 3 参调用**原样编译**；`tc_swz`、三个 index helper、`geometry.cuh`/`prompt_common.cuh`（`diff -q` 逐字节相同，18/74 行）**逐字节相同**。但**函数体新增了 wave_splits 钳制块**（`small_t.cuh:126-130`）⇒ 仅签名兼容，行为需重测 |
| **共享面是硬改造** | `KvCacheStorage` **248 行 / 44 文件**；**7 处穷举 `switch(storage)`**（`core/paged_kv_storage.h:65`、`ops/kv_cache/d256_profile.h:87`、`causal_softmax_attention.cpp:365`、`small_t.cu:28`、`serve/operational_log.cpp:130`、`serve/request_log.cpp:135`、`apps/cli/main.cpp:84`）+ **6 处名字 switch** + **两处 host 硬 throw** ⇒ 新枚举必须逐点注册 |
| **容量/路由函数已被 TAIL 改写** | `causal_attention_split_capacity`：FORK `small_t.cu:219-260` vs TAIL `:258-300`（TAIL 收紧为 `tokens > (q_heads==24 ? 8 : 6)`、删 MTP 归一化、新增 `kCausalSmallTSplitKeyLimit=3968`@`small_t.cuh:27`）⇒ **仅因 kvarn 只以 `tokens=1,BF16,batch=1` 调用才等值**；更宽调用在 TAIL 直接抛。`causal_attention_resolve_route` 亦被重写（FORK 亦**不是**恒返 SmallT：`causal_softmax_attention.cpp:343-352` 在 `width>verify` 返 Prompt、`batch>1` 返 ChunkedSmallT）。`launch.h` **不同**（FORK 127 行 / TAIL 142 行，TAIL `causal_attention_prompt_launch` 增加**必填** `bool fast`@`launch.h:102`） |
| **FORK 接入面是 20 个文件** | `grep -rl KvarnK4V2Group128` = **20 文件**（`execution/text.cpp`、`program/decode.cpp`、`planning/startup.cpp`、`program/storage/context.cpp`、`decoder_state.{h,cpp}`、`serve/*`×3、两个 CLI、`include/ninfer/types.h:34`、**5 个测试文件**、`bench/inference/ninfer_bench_support.cpp`） |
| kvarn 的 reducer | kvarn **自带** `reduce_output_hadamard_kernel`（`decode_kernel.cuh:926`，收 **`const __nv_bfloat16* partial_acc`**），**不依赖** TAIL 被改造过的共享 reducer（收 `const float*`） |

⇒ **裁定：设备侧接口兼容，但宿主注册 + 页几何 + 共享面 + 旋转域是硬改造；难度 = 中高。** 「不需要改任何一行」不成立：新枚举必须先在 `d256_profile.h` 与 `paged_kv_storage.h` 两处 host switch 注册，否则启动即崩。

### 4.2 落地清单

1. 复制 `src/ops/kvarn/` 12 文件 + `include/ninfer/ops/{kvarn.h, kvarn_attention.h}` 到 TAIL 同路径。
2. `src/ops/CMakeLists.txt:23` 附近 `include(kvarn/sources.cmake)`；`tests/ops/tests.cmake` 增 `ninfer_kvarn_test`（**含从 FORK 移植的 1710 行 `test_kvarn.cpp`**）；bench 见 WP0 说明（**加 bench 需重配**，与「不重配」冲突，须显式决定）。
3. 加回存储枚举（见 §4.3）与 CLI 解析（**三处 parser + 6 处名字 switch**）。
4. 页面几何泛化（见 §4.4）。
5. 模型侧接入（见 §4.5）+ **WP1 之外的 20 文件接入面**。

### 4.3 存储枚举设计（**推荐方案**）

- 新增**单个**枚举值 `KvCacheStorage::KvarnGroup128`（**必须追加在末尾**，`types.h:67` 约定），而不是三个。
- 档位用 **profile 字段** `KvarnBits ∈ {4,5,6}` 承载，随 `EngineOptions` 流动（对照 `kv_tail_tokens` 的流动方式：`types.h:467-469` → `DecoderStateSpec` → `decoder_state.cpp`）。
- CLI：`--kv-dtype kvarn:k4v4|k5v5|k6v6` 解析为 `(storage=Kvarn, bits=4|5|6)`；`--kv-dtype kvarn` 等价别名见 §10-D2。
- **必须并入身份指纹与 route/graph key**：`model_instance.cpp:183-185` 追加 `;kvbn=<bits>`（对照 `;kvtt=`）。
- **待登记站点（修正版）**：核心 7 个穷举 switch + 6 个名字 switch（§3）+ `core/paged_kv_storage.h`、`ops/kv_cache/d256_profile.h`、`ops/kv_cache/append/*`、`causal_softmax_attention.cpp`、`models/qwen3_5/{load.cpp,state/decoder_state.*,program/planning/*,program/storage/disk_tier.cpp,program/program_impl.h}`、`calibration/device_calibration.cu`、`serve/{serve_options,operational_log,request_log}.cpp`、三个 CLI parser、`runtime/engine/model_instance.cpp`、`ops/kv_cache/plane_types.h`、`include/ninfer/ops/{kv_cache_append.h,softmax_attention.h}`、`ops/softmax_attention/dense/causal_cache/{launch.h,prompt.cu,prompt_*}`、`dense/context/context_softmax_attention.cpp`、`ops/context_kv_materialize/*`、`models/qwen3_5/load.h`、`state/decoder_state.h`、`serve/serve_options.h`、`apps/cli/options.h`。

> 规模口径：**7 个穷举 switch / 44 文件 248 行引用**（不是「37 个 switch 站点」）。

### 4.4 页面几何泛化（group=128）

- 引入 `kv_page_tokens(storage)`：**默认 64，kvarn 返回 128**。
- `src/core/paged_kv_cache.cpp:39-41, 253`：把校验从「必须 64」放宽为「64 或 128」；`create`/geometry 从 `kv_page_tokens` 取。**默认值仍是 64，非 kvarn 格式行为不变**。
- `state/decoder_state.cpp:31-63 plan_cache`：给 kvarn 建**单个 U8 plane** `{RecordBytes/Group, kv_heads, 256}`，**绕过** `paged_kv_storage_layout()`。
- **容量/MemorySummary 路径**：kvarn 的 per-token/head = `32*(Kb+Vb)+18` B。在 `plan_cache`/`load` 为 kvarn 加专用公式（§7-WP7），并同步 `docs/config-calculator.html:529`。
- **⚠ 两池独立但有共享 helper 陷阱**：尾部 ring 仍按 `kPagedKVPageSize=64`（`decoder_state.cpp:126-131`），body 为 128。两池独立属实，但**共享 body helper 用 `>> kPagedKVPageShift(=6)` 索引页**（`small_t_i8.cuh:234,244`）——128-token body 页经 64-shift helper 会**静默读页 2p（恰差 128 键，无断言）**。**WP2 必须为 kvarn body 提供独立 page-shift，并审计所有经共享 helper 的路径**；从无构建同时跑过 128-body + 64-ring。
- **✅ WP2 实测修正（2026-10-07）**：① `kKvarnPageTokens=128` / `kv_page_tokens(storage)` /
  `kv_page_shift(storage)` 已落地（`src/core/paged_kv_cache.h`）；`validate_geometry` 放宽为 64|128、
  plane 形状与校验改用 `spec.geometry.page_tokens`。② **陷阱边界缩小但仍在**：`src/ops/kvarn/` 内
  **0 处**引用 `paged_kv_address.cuh` / `kPagedKVPageShift|Mask`——kvarn 自己的
  `decode_kernel.cuh` 用**逻辑页号**直接索引 block table（`block_table[first_page + page]`,
  `block_tables[page + logical_pages*row]`），**不经 `>>6`**。故 64-shift 陷阱**在 WP2 不可达**；
  它成为真实风险的条件是 **WP3 把 kvarn body 接进共享 `small_t_*`/`prompt_*` helper**（那些文件里
  `kPagedKVPageShift` 出现 **20+ 处**，见 `small_t_{bf16,fp8,i8}.cuh`、`prompt_*.cuh`）。
  ⇒ **WP3 挂载 kvarn body 时必须为该路径使用 `kv_page_shift(KvarnGroup128)=7`，或在 kvarn 分支
  绕开全部共享 helper。**

### 4.5 KVarN 自身的 sink/tail（与外部精度尾部无关）

- kvarn 记录内自带 sink 槽 + 尾槽，**合计 3 槽 = 1 sink + 2 动态尾**（`kKvarnSinkPages=1`/`kKvarnTailSlots=3`，`kvarn.h:15-16`；`FORK paged-kv-cache.md:100-101`）。
- 张量 `kvarn_tail_k/_v` 形状 `{head_dim, kKvarnGroup, table_rows*kv_heads*kKvarnTailSlots, layers}`，**BF16**。
- 显存：**24.0 MiB/序列（K+V）+ 1.5 MiB（MTP 池）= 25.5 MiB**（`decoder_state.cpp:118-161`）。**StateImage slot 是否复制该尾池 → WP7 待核实**（v2 引用的 `state_image.cpp:120-146` 实为 `continuation_hidden` 与 DFlash-local K/V，**无 kvarn/tail**）。前一版「约 12 MiB」只算了 K 侧，必须改。
- 注入/恢复：`program/storage/context.cpp:1478-1548 restore/capture/activate_sequence_kvarn_tail` + `ops::kvarn_restore_tail`。

---

## 5. 位宽参数化：k4v2 → K=V ∈ {4,5,6}

**目标**：把 ops 从「k4v2 硬编码」改为**单一 `bits` 参数的通用位流**（K=V，故一个参数、一套 pack/unpack）。

**记录布局**（保持 v2 的对称式，但**必须显式声明几何来源**，见下方 H7）

```
RecordBytes(Kb,Vb) = 4096·(Kb+Vb) + 2304          // 4096 = D·G/8 = 256·128/8
  KPacked   = 4096·Kb
  KScale   512 | KZero 512 | KTokenScale 256
  VPacked   = 4096·Vb
  VChannelScale 512 | VTokenScale 256 | VTokenZero 256      // 元数据合计 2304 = 18 B/token
```

| 档 | Kb,Vb | RecordBytes | B/token/层/头 | B/token(全模型) | 相对 bf16 |
|---|:--:|---:|---:|---:|---:|
| **k4v4** ✅ | 4,4 | 35,072 | 274 | **17,536** | 26.8% |
| **k5v5** ✅ | 5,5 | 43,264 | 338 | **21,632** | 33.0% |
| **k6v6** ✅ | 6,6 | 51,456 | 402 | **25,728** | 39.3% |
| k4v2（仅 oracle） | 4,2 | 26,880 | 210 | 13,440 | 20.5% |
| rk4v4（现役对照） | — | — | 280 | 17,920 | 27.3% |
| rk8v4（现役对照） | — | — | 408 | 26,112 | 39.8% |
| k8v4（现役对照） | — | — | 402 | 25,728 | 39.3% |

> 全模型 = 每头每层值 × **4 KV 头** × **16 个 full-attention 层**（27B dense：64 层中 16 层 full attention，其余 48 层为 GDN；Q/KV/head_dim = 24/4/256）。**注意勿写成「16 层」或「H16」**——`CausalD256H16Kv2` 的 `H16` 是 35B-A3B 的 16 个 Q 头，与层数无关（`geometry.cuh:15-16`，27B 走 `CausalD256H24Kv4`）。
> **MTP 启用时多一个 KV 池**（layers=1）⇒ 生产乘数 = **16 full-attn 层 + 1 MTP 池**（+6.25%），`config-calculator.html:550` 实测 `mtp3 kvRatio≈1.062988281`。
> **k6v6 与现役 `k8v4` 逐字节相同**（402 B/token/头，布局 `Fp8KeyNvfp4Value {FP8,256,FP16,1}+{U8,128,U8,16}` = 258+144 = 402，`paged_kv_storage.h:111-116`）⇒ WP8 的对照集必须用 k8v4，**不是 rk8v4**。

**✅ WP4 落地与设计定案（2026-10-07，见进度记录 07-19）**

记录几何已从**字面量常量**改为**参数的 `constexpr` 函数**，并把**旧常量改为由 `(4,2)` 派生**
（⇒ 泛化立即被使用，且旧值**逐字节不变**，由编译期断言钉死）。`include/ninfer/ops/kvarn.h` 新增：
`kvarn_{k_packed,k_scale,k_zero,k_token_scale,v_packed,v_channel_scale,v_token_scale,v_token_zero}_offset(kb,vb)`、
`kvarn_record_bytes(kb,vb)`（= `4096*(kb+vb)+2304`，与上表一致）、`kKvarnPackedBytesPerBit = 4096`；
码寻址 `kvarn_k_row_bytes(kb) = G*kb/8`、`kvarn_k_code_bit(kb,t) = t*kb`、
`kvarn_v_row_bytes(vb) = D*vb/8`、`kvarn_v_code_bit(vb,d) = d*vb`；**共享位编解码**
`kvarn_unpack_code(row,bit,bits)` / `kvarn_pack_code(row,bit,bits,code)`（用 `KVARN_HOST_DEVICE` 宏：
`__CUDACC__` 下为 `__host__ __device__`、MSVC 下为空 ⇒ **设备核与主机 oracle 共用一套**）。
**编译期断言（全部通过）**：旧偏移 `16384/16896/17408/17664/25856/26368/26624/26880` 全等；
`(4,4)/(5,5)/(6,6)` = **35072 / 43264 / 51456**（均 `%256==0`）。

**三条实测/设计约束（决定后续实现，务必遵守）**

1. **`(4,4)` 的 K 侧与旧 k4v2 逐字节相同**（`kvarn_k_code_bit(4,t) = 4t` ⇒ 字节 `token/2`、移位
   `4*(token&1)`）⇒ **k4v4 只改 V 侧**（2-bit → 4-bit）。这是 k4v4 落地风险最低的原因。
2. **位序约定（五处解包一致，必须沿用）**：小端、LSB 优先、低下标在前；K「偶数 token = 低半字节」，
   V「`d & 3 == 0` = 最低字段」。
3. **向量化只在 4-bit 成立**：`decode_kernel.cuh:109-110,118-119` 的 `load_vec<int4>`（16 B = 128 bit）
   在 K 4-bit 装 32 码、V 2-bit 装 64 码；**b=5/6 为 25.6 / 21.33 码——非整数** ⇒ 5/6-bit 只能逐码位寻址
   （`kvarn_unpack_code`）。⇒ **保留 `if constexpr(bits==4)` 的 int4/nibble 快路径，5/6 走位流慢路径**。

**解包重复面（WP4 要收敛的对象，实测 5 处而非 4 处）**：`decode_kernel.cuh` 三个
（`stage_decode_key_quad:196-224`、`stage_decode_key:258-294`、`stage_decode_value:328-354`）+
`codec.cu:81-94` + `materialized_prefill.cuh:91-113` + `attention.cu:343-373`（`restore_tail_kernel`）。
**打包只有一套**：`store.cuh` 的 `store_k_tile:51-90` / `store_v_tile:92-131`（被 `codec.cu` 与
`attention.cu` 调用）。`store.cuh:66` 的 `/15.0F` 与 `:111` 的 `/3.0F` 即 `qmax = (1<<bits)-1`
（15=2⁴−1、3=2²−1）。硬编码入口：`config.cuh:10-11` 的 `KBits=4/VBits=2` 与
`decode_kernel.cuh:27` 的 `static_assert(VBits == 2)`。

**改动点（修正规模）**

实际命中 **≈55–60 个位打包字面量、8 个 ops/头文件 + 3 个测试/基准文件、4 套独立位解包**（v2 说 14 处 / 3 套）：

| 文件 | 硬编码点 |
|---|---|
| `config.cuh` | `KBits=4` 是**死常量**（全库无引用）；`VBits=2` 仅被 `decode_kernel.cuh:27` 用到 ⇒ **先把散落字面量收拢** |
| `decode_kernel.cuh` | `:25 kPackedVBytes=…(D/4)`、`:27 static_assert(VBits==2)`、`:109-110 dim*(Group/2)+token_begin/2`、`:119 token_begin*(D/4)`、`:334 record_token*(D/4)`；`:34-40 DecodeRecordMetadata` 的 `kVCodeValues=4` |
| `store.cuh` | `:66 /15.0F`（K）、`:111 /3.0F`（V）→ 统一 `qmax=(1<<bits)-1`；`:117-118 (D/4)` |
| `codec.cu` | **第 2 套**独立位解包 |
| `materialized_prefill.cuh:100` | **第 3 套**（8 token/uint32） |
| **`attention.cu:354-360`** | **第 4 套**（`restore_tail_kernel`：`>>(4*(token&1))&15`、`>>(2*(d&3))&3`；v2 漏） |
| `kvarn.h:27-28` | `kKvarnRecordBytes=26880` + `static_assert(%256==0)` → `constexpr` 函数 |
| `state/decoder_state.cpp:44` | `RecordBytes/Group` 形状 |

**难度真相**：现 kernel 依赖 `load_vec<int4>`（`decode_kernel.cuh:109`）的**元素级向量化**；5/6-bit 必然跨字节 ⇒ **「记录级 256 对齐」不等于「元素级可向量化」**。策略：**保留 4-bit nibble 快路径 + 5/6-bit 位流慢路径 + `if constexpr(bits)` 分派**。参照 beellama 的通用位流：值 `i` 占 `[i·bits, i·bits+bits)`，LSB 优先（`BEE src/llama-kvarn.cpp:715-748`，`bit_offset = i*bits`）。

**⚠ H7：参考实现选定即决定字节表（必须显式声明）**
- beellama 把 D256 拆成 **2 个 128 列切片**（`BEE src/llama-kvarn.cpp:674,694`，`assert(record_dim==64||128)`），每切片付 768 B 元数据 ⇒ **3072 B（24 B/token）而非 2304（18 B/token）** ⇒ 每档变 **280/344/408 B/token**，于是 **k4v4 与 rk4v4 逐字节相同（280）**、**k6v6 与 rk8v4 逐字节相同（408）**——价值命题塌成「同字节档的另一次实现」。
- 上游另一极端：对 D≥256 把每 token 槽**向上取 2 的幂**（`UP config.py:122-125`：274→512 B/token = bf16 的 50%），FORK 有意不继承（`FORK paged-kv-cache.md:95-96`）。
- **⇒ 本方案必须写明：只借 beellama 的 bitstream，不借其 record geometry；保留「K 沿 token 配对、V 沿 dim 配对」的不对称说明。§5 的对称公式 `4096·(Kb+Vb)+2304` 即此选择。**

**上游依据（修正）**：上游 `config.py:21-26` **确有 4 个 preset（含 `kvarn_k4v4_g128`、`kvarn_k4v4_g64`）⇒ k4v4 有上游参考**；`kvarn_store.py:92 pack=8//bits`（`8//5==1` ⇒ 上游**表达不了 5/6-bit 稠密打包**）。故 **仅 K5V5/K6V6 无上游参考，唯一实现参考是 beellama**。

**✅ WP4 执行定案（2026-10-07，07-20/07-21 实测；详见进度记录同两条）**

模板化与分派的**最终形态**（5 条与实现绑定的决定，后续不得偏离）：

1. **设备核模板参数 = `(int KBits, int VBits)`，但发布档只实例化 `(b,b)`，b∈{4,5,6}**。
   K=V 是产品合同（§1），故非对称档**既不发布也不实例化**；`(4,2)` 仅作为 **`kvarn_*_offset(4,2)` 的
   几何表达**留在头文件（k4v2 字节表可复算），**没有任何设备核实例化它**。
2. **V 侧不再有「位宽专用快路径」**：旧的 `uint16 → 8 个 2-bit 码` 只对 VBits==2 成立，而 VBits==2
   不实例化 ⇒ 保留即死代码。改为**统一位流解码**（`kvarn_unpack_code` + `v_scale/v_zero` 表内联
   `fmaf`）；`v_channel_scale` 由 `[kV][D/kV]` **扁平为 `[D]` plane-major**，索引
   `v_channel_index<VBits>(dim)`（vb=2 时与旧 `[dim&3][dim>>2]` **同一地址**）。
3. **K 侧保留 `if constexpr(KBits==4)` 的 int4+nibble 快路径**（k4v4 用到，是 §5「向量化只在 4-bit
   成立」的落点）；`KBits!=4` 的 staging 改为「每 dim 一行字节」并逐码 `kvarn_unpack_code`，
   `token_begin` 是 64 的倍数 ⇒ 字节偏移恒整（kb=5: 0/40；kb=6: 0/48）。
4. **`bits` 承载在视图里**（`KvarnTileStorage`/`KvarnPagedLayerView`/`KvarnPagedBatchLayerView` 尾部
   `std::int32_t bits = 4`），贯穿链
   `EngineOptions.kvarn_bits → SequencePlanningInputs → SequencePlanImpl → DecoderStateSpec →
   PagedKVCacheLayout → PagedKVCache::kvarn_bits_ → 视图`；ops 入口签名**不变**（改动面最小），
   运行时分派为 `switch (bits)`（`decode.cu`/`codec.cu`/`attention.cu` 三处）。
5. **`KVARN_HOST_DEVICE` 必须标在全部 `kvarn_*` 几何/寻址 `constexpr` 函数上**（否则 nvcc 报
   "calling a constexpr `__host__` function from a `__device__` function"）；宏定义须在那些函数**之前**。

**`DecodeRecordMetadata` 的简化（对 A5 显存有利）**：删去 `v_base[64][1<<VBits]`（vb=6 时 16 KB 共享内存）
改为 `v_scale[64]`+`v_zero[64]`，结构体固定 **2816 B**（不再随位宽膨胀）；`metadata_s` 已无模板。

**构建与测试口径（可复现）**：`ninfer_ops ninfer_tests ninfer ninfer-serve ninfer-perplexity` 五目标
exit 0；`ninfer_kvarn_test` 三档全绿（实测数字见附录 **D-11**）。

---

## 6. 精度尾部协同与旋转域门禁（WP6）

### 6.1 事实（已独立复核）

- TAIL 的尾部合并**只对原始坐标 body 生效**（BF16 + INT8 族 + Prompt 路线）；旋转域 `fp8-e4m3 / nvfp4-g16 / k8v4` 的 `small_t_*.cu` **不含任何 tail/shadow 头**（被编进但不走）。
- **KVarN 就在旋转域**（K/V 都做 Hadamard，输出再旋回）。
- ⇒ `kvarn + --kv-tail-tokens` **不会自动生效**，若不处理就是**静默惰性**。**WP6 是独立门禁。**
- **fp8 无尾的真正原因是实现欠账，不是坐标障碍**：`append/kernel.cuh:35` 只旋 K（V 不旋），`small_t_fp8.cu:86` 用**未改动的共享 reducer** ⇒ fp8 只有一个被旋转的 KV 轴。
  **⚠ 订正（v19，D-19 ①）**：本仓源码自述与上句**相反** —— `tests/ops/softmax_attention/causal_cache.cpp:1613-1623`（`tail_merge_wired`）写明
  旋转域档位不接尾的原因是「其 reduce 核消费**旋转系 partial**，而原始 BF16 尾行**不表达于该系**」⇒ **坐标系就是障碍**，
  "实现欠账"只是**未做旋转步骤**的表象。原文保留以存史。**该订正强化了 (b) 的否证**（见 §6.3）。

### 6.2 方案 A 按文义不可实现（三条一手反证）

1. **partial 累加器元素类型不同**：FORK kvarn 走 `decode_kernel.cuh:368 __nv_bfloat16* partial_acc`、`:926` reducer 收 `const __nv_bfloat16*`；TAIL tail 核走 `small_t_tail.cuh:56 float* partial_acc`、共享 reducer `small_t.cuh:262 const float*` ⇒ **不能共用一块 buffer**，「由 kvarn 自己的 reducer 一次合并」不成立。
   **⚠ 补充（v19，D-19 ⑦）：上游根本没有 BF16 部分累加器** —— beellama 的 KVarN 与 tail 共享的统计量是 **FP32 `float2`(row max, denom) + FP32 累加器**，
   且 `GGML_ASSERT(dst->type == GGML_TYPE_F32)`（`beellama.cpp/ggml/src/ggml-cuda/fattn-tail.cuh:339-346,379-387`、`fattn-common.cuh:1551`、
   `fattn-kvarn-portable.cuh:466-469,784-786,816,834`、`fattn-mma-kvarn-decode-combine.cuh:67-68`）；BF16 只出现在 **tail 环元素类型**。
   ⇒ **本仓的 BF16 `partial_acc` 是对上游的偏离**，不是"上游的另一种变体"；修法即**对齐上游改为 FP32**（不是发明新契约）。
2. **split 策略在目标工况分叉**：`decode_kernel.cuh:44-53 kvarn_decode_active_splits` 对 `QHeads==24`（正是本 27B）且 `window>8198` 覆盖 `cap = window<=122880 ? 41 : 82`（`config.cuh:16-18`）；TAIL 走 `small_t.cuh:145-180`。**长上下文下 body 与 tail 各自给出合法但不同的 split 集合**，而这恰是 A4/WP8 要测的区间。
   **⚠ 订正（v19，D-19 ⑦）：该条**不是阻塞**。** 上游的 tail partial **没有 split 轴** —— 每 (query, head) 行只有**一对** `(row max, denom)` 与一个 FP32 累加器
   （`fattn-tail.cuh:339-346,844-847`）；KVarN body 侧在导出前**先把自己的 splits 归约成每行一对**（`fattn-kvarn-portable.cuh:784-786`、combine 核；
   split 数定义 `fattn-kvarn-dispatch.cu:615-618`）⇒ **两侧不要求 split 一致**（子代理结论 (iv)：否）。
   ⇒ §6.3(a) 只需**两段式归并**：body 自身 splits 归约 → 每行一对 → 与 tail 行对在线 softmax 合并。**原文保留以存史。**
3. **默认尾部 dtype 被旋转入口拒绝**：TAIL 默认 `kv_tail_type=Float16`（`types.h:469`），FORK `codec.cu:161-165 kvarn_hadamard` 对 `source.dtype != BF16` **抛 `invalid_argument`** ⇒ 必须**新写 f16 旋转入口**（独立交付物）。
   **⚠ 订正（v18，D-18）**：该结论**只在坚持 f16 尾时成立** —— `--kv-tail-type bf16` 今天已完整支持，且 **BF16 尾环是逐位精确档**（源行比特原样拷贝）⇒ 采 bf16 尾即可复用现成的 BF16 旋转入口，**免去该交付物**。原文保留以存史。
4. **旋转时点未定**：读时旋转（每步对 tail 行做 Hadamard）≈ 尾部注意力 FLOPs 翻倍；**写时旋转**（append 期一次性，≈32768 元素/token）代价可忽略。两者差一个数量级。
   **⚠ 订正/补充（v19，D-19 ⑤）**：① KVarN **内建**尾环早已是**写时旋转**（`attention.cu:157-194` `rotate_stage_kernel` 写入 WHT 后的 BF16；
   `rotate_on_stage = width <= kFusedStageMaxWidth(=16)`，`:25,590`）。② 对外部精确尾环，(a) 路线**不必**逐行旋 tail 行 ——
   尾部 partial 可在**原始域**算完后**只对 acc 做一次 D 维旋转**再并入共享归并（`W` 线性 ⇒ 数学与"先并入再反旋"等价），
   代价是每 (q_head, token) 一次 D=256 变换，远低于旋 384 行 × D。**代价与可行性须在 WP6 落地时实测确认。**

### 6.3 修正后的方案（替代 v2 方案 A）

**探针前的假设顺序：优先 (b)，回退 (a)。** **⛔ 探针后（v19，D-19）：(b) 已被否证**（见 D-19 ①）⇒ 实际候选为 **(a) 或 (c′)**；
**✅ 已裁决（2026-10-08，用户确认；v20 / D-19 ⑨）：路线 = (a)**（照 TAIL 已有的 `small_t_k8v4.cuh` 形状接进外部共享精确尾环）—— 分步计划见 **§7-WP6（WP6.0–WP6.7）**；**(c′) 否决**（量化依据见 D-19 ⑧）。

- **(a) 照 TAIL `small_t_k8v4.cuh` 的形状做**（仓内已有模板）：K 与 V 都旋（`:208,229`）、核内旋 Q（`:263`）、归并 **FP32** partial（`:639-643`）、**归并后只做一次**反旋（`:653`）。即方案 A 想要的东西，改为沿用 TAIL 的 FP32 partial 契约。
- **(b) `ROTATED_K_ORIGINAL_V`**：若 V 不旋，KVarN 在结构上等价于 TAIL 已有的 fp8 body ⇒ 尾部接线退化为一个 launcher 分支，**可能零内核改动**。取证/落地前须确认上游契约提供该域（`ggml.h` 同时定义 `ORIGINAL` 与 `ROTATED_K_ORIGINAL_V`，见 `docs/port-records/kvarn-kv-tail-feasibility-report.md:44`）。
- **(c) 最低形态**：直接暴露 KVarN 内建精确后缀（`kKvarnSinkPages=1` + `kKvarnTailSlots=3`，≈384 token 已近精确）作为「尾部」，把 `--kv-tail-tokens ≤384` 映射上去，属配置工作。
  **⚠ 订正（v19，D-19 ③）**：「≈384 token 已近精确」**过于乐观**：3 槽 = **sink 组 + 滚动首/末组**（`attention.cu:84-89`），
  `claim_tail_slot` 在首选与备用槽都被占用时**返回 −1 ⇒ 该行不 staging**（`:100-114`），`retire` 只清"被整组覆盖"的页（`:291-315`）
  ⇒ 它**不是**"最新 384 连续精确"，也不能承诺"最新 N 精确"。且该内建区**本来就在运行**，接出来**不产生新质量**（只免除 fail-fast）。
- **(c′) 有界第四选项（v19 新增，D-19 ④，未实测）**：把 KVarN 内建尾环由 3 个固定槽泛化为 `⌈N/128⌉+1` 个**滚动槽**，
  让 **body 自己**在核内保留最新 N 个精确组。优点：复用 body 自身的核内合并 ⇒ **同时绕开** partial 元素类型（BF16 vs FP32）、
  坐标系（旋转域）与 split 策略三项冲突，改动面收敛在 `attention.cu` 槽位簿记 + `decoder_state.cpp`/`state_image.cpp` 分配 + 容量核算。
  代价：`--kv-tail-tokens` 会有**两套实现**（非 KVarN 走外部共享环形池、KVarN 走自建槽环），语义需分别定义；显存 ≈ N×D×Hkv×层×2(K+V)×2B（N=1024 时 ≈64 MiB/序列，与 §2.4 估算一致）。**须与 (a) 并列做一次成本/收益裁决。**
- **明确写时旋转**；**BF16 尾可免去「新增 f16 旋转入口」**（`--kv-tail-type bf16` 已完整支持，且 BF16 尾环是逐位精确档 = 源行比特原样拷贝，见 `ops/common/kv_tail_element.cuh:46-55,73-75`）⇒ 该"独立交付物"**仅在坚持 f16 尾时才需要**（产品面裁决，见 D-18 订正）；若采 f16 尾，则需**新增 f16 旋转入口**。
  **⚠ 订正（v19，D-19 ⑦）**：「写时旋转」**只对 KVarN 的内建 stage 成立**（`attention.cu:157-194`）。**外部精度尾环**上游的做法是**写原始行、读时旋转**
  （写：`llama-kv-cache-kvarn.cpp:1004-1011`；读：`llama-graph.cpp:3845,3847` 的 `ggml_kvarn_wht_aux`；旋转域下 tail K 恒旋、tail V 仅在 `use_kvarn_rotated_domain` 时旋 `:3843-3855`）。
  **上游合并契约（可照抄）**：合并在**旋转域**进行、**末尾只做一次反旋**（`llama-graph.cpp:3901-3904`）；统计量 = FP32 `float2(row max, denom)`、累加器 FP32、**无 split 轴**；
  混合域（仅 prefill，`>16` query token）下 V 在**原始域**相遇（body 的 V 累加前逐 token 反旋，`portable.cuh:330-362,434-443`）。
  **本仓可选等价替代**：尾 partial 在原始域算完后**只对 acc 旋一次**（§6.2 条 4 订正），比逐行旋更省。**两条路线都须在 WP6 落地时实测。**
- **逐位一致这一要求本身可满足**：FORK `hadamard.cuh:9-31` 与 TAIL `hadamard_d256.cuh:46-65` 是**同一 Sylvester 顺序、同一符号约定、同一次 2⁻⁴ 归一**，两侧都未开 `use_fast_math`（TAIL `CMakeLists.txt` 只有 `/Zc:` 系列）。但注意 `hadamard_warp`（`hadamard.cuh:33`）是**死代码**，且 fork 是 256 线程/行、TAIL 是 1 warp/行，寄存器映射 `d = lane + 32r` 需重建。
- **执行顺序**：**在 WP4 之前做一次低成本探针**判定 (a)/(b)/(c) 哪条成立，而不是把门禁拖到 WP6 才暴露。
  - **⛔ 实际状态（2026-10-08，v18 / D-18）：该探针从未执行。** WP4 的验收只覆盖三档准入，本前置被**静默跳过**（进度记录 §3 与归档 §A 均无执行痕迹）。⇒ **WP6 开工前必须先补做本次探针**；(b) 的域契约在本仓**至今 0 命中**（只存在于上游 ggml 的散文描述中）。
  - **✅ 已执行（2026-10-08，v19 / D-19；用户授权、无 GPU、读码 + 跨工程图谱 Tier-3）**：结论 = **(b) 否证 / (a) 忠实 / (c) 弱 / 提出 (c′)**。
    **WP6 工期重估不变（10–15 天）**；**开工前仍须一次产品裁决**：选 **(a)**（忠实、保留外部共享尾环、但需新增 KVarN 专属 tail-partial 核与归并）还是 **(c′)**（有界、body 自建滚动槽、但 `--kv-tail-tokens` 出现两套实现）。**两者都未实测，工期估计均为区间。**

### 6.4 回退（硬约束）

- 若坐标域无法调和：**`kvarn:* + --kv-tail-tokens` 必须 fail-fast 拒绝**，交付缩回「仅 KVarN 三档、无尾部」。
- 禁止任何「分配了 ring 但不读不写」的静默路径进主干。

---

## 7. 实施计划（WP0 · WP0.5 前置门 · WP1–WP9）

> 每个 WP 给「产出 / 验收 / 回退 / 工期」。**WP0.5-B/C 为前置门**（准入实验 + 构型口径）；
> **WP0.5-A 已由"门禁"改为"仪器交付"**（A3 判据替换，附录 D-6），**不再阻塞 WP1–WP9** ——
> **WP1、WP2 已完成**（WP2 见附录 D-7），**WP3 可立即开工**。
>
> **★ WP4 之后的推进顺序（2026-10-07 定；详见进度记录 §4.1）**：**P1 = WP3② 续列尾（前缀复用/检查点，
> 最大功能缺口）→ P2 = WP3④ MTP 路径激励 + A3 相对判据 → P3 = WP4 补强**（≥3 重复测量 / 报告目录按档区分 /
> parser 单测；可与 P1/P2 并行）**→ P4 = WP5 容差形式化与 bench 归属 → P5 = WP6**（最高风险；
> **门禁 = P1/P2 完成**）。**WP3①·②·③·⑥ 与 WP4 已完成**；**WP6 不得跳过 P1 直接启动**。
> **截至 2026-10-08：P1–P4 已全部完成** —— P3a/P3b/P3c 与 P4a 分别见附录 D-12/D-14/D-15/D-12；
> 唯一未决的是 **P4b（kvarn bench 归属，需重配，待用户裁决）**；**仅剩 P5 = WP6（门禁后）**。

### WP0 — 基线与环境（1–1.5 天）
- 复用已配置的 `build-port/`（Ninja Release，`CMAKE_CUDA_ARCHITECTURES=120a`，CUDA 13.3，MSVC v143 14.44.35207，`BUILD_TESTING=ON`）；**不要重配**。
- **⚠ 构型口径**：`build-port/CMakeCache.txt:502 NINFER_SM120_NATIVE:BOOL=ON`，而 TAIL `CMakeLists.txt:11,26-30` 自述「120a is admitted locally as an **unqualified** target」「upstream's native routes … are a separate, unqualified code path, while the compatibility one is the **tested route**」⇒ 当前复用**未鉴定路径**。同时 TAIL `AGENTS.md:34,37` 仍写本仓目标 sm_86/RTX 3090/CUDA 12.8，并注「上游 route table 在 sm_86 重测误差 12–41%」——与 `wave_splits`/split-capacity 常量直接相关。**须在 WP0.5-C 决断。**
- 产出：① 现状冒烟（`ninfer-perplexity --score-width 8 --save-topk` 生成基线 topk）；② 一份「本机噪声底」报告（**decode-only** pp/tg 重复 ≥3 次的分布）；③ **取数前先确认独占**（本仓 AGENTS.md 提示常有人挂服务；审阅期间空闲显存波动 1901 MiB→15948 MiB）。
- **验收**：能对本机模型跑通一次 prefill+decode+MTP 与一次 KLD 基线。
- **回退**：无（前置门）。

### WP0.5-A — MTP 一致性仪器（0.5–1 天，**改造为诊断仪器**）〔仪器交付，非门禁〕
- **已交付**（附录 D-4）：FORK `test_engine_mtp_greedy_parity_real.cpp` 已移植入 TAIL（557 行）并注册
  `ninfer_qwen3_5_mtp_greedy_parity_real_test`（`tests/models/qwen3_5/tests.cmake:188-191`）。移植期唯一改动：
  删 kvarn profile、`mtp_draft_policy`→`mtp_policy`、删 `enable_nvfp4_scale_compression`/`compressed_scales`。
- **实跑发现**：纯 `bf16` 下 k=1 与 k=0 在 token 91 分叉、k=0/k=3 全等；差分实测证明上游父仓库同样分叉且
  输出逐字节相同 ⇒ **非本移植引入**（附录 D-4/D-5）。
- **定案（附录 D-6）**：该测试的"绝对逐位 parity"断言**不可作门禁**——上游 #80 明示拒绝该判据
  （"Greedy does not mean batch invariant"）、本仓 `docs/performance.md:29-45` 早已实测否决，且它是
  **Fork B 的私有合同**（其使能 kernel 提交不在 TAIL 提交图内）。
- **本轮交付物**：把该测试**改造为诊断仪器**——输出各 MTP 宽度的**首个分叉下标、分叉率、同配置 KLD 差**，
  并把 kvarn 档与同配置基线并排对照；**删除逐位全等的通过/失败断言**。
- **验收（A3 新判据）**：① 同配置自确定性成立（重复逐字节相同）；② 工具能报告首分叉下标；
  ③ kvarn 档不劣于同配置基线（WP3/WP9 复用）。**不再要求"逐位全等"通过。**

### WP0.5-B — 准入实验前置〔**✅ 完成（正式版 2026-10-07，附录 D-11）**；代理门禁见附录 D-10〕
- **同模型同口径 decode-width KLD 三方对照**：`rk4v4` / `rk4v4+tail` / `k4v4+tail`。
- **正面引用并反驳 `docs/port-records/PORT-DOD.md:23-32`（commit `8e34ad12`, 2026-10-05）的负面裁决**：该 M0 门禁用 wikitext-00 / ctx 4096 / 4 chunks，得 `f16 5.3580 / q8_0 5.3577 / kvarn4 5.3559 / kvarn4+tail1024 5.3622`，噪声 ±0.136，结论「没有质量驱动的理由引入 KVarN」。要么指出前测方法缺陷（4 chunks、ppl、±0.136 噪声——**正是本计划论证的仪器问题**），要么把 WP8 从「收口实验」前置为「准入实验」。
- 同时引用 `docs/performance.md:754-764` 的 kvarn4 KLD 行与 `.deps/wpc/kld-kvarn4-*.out`。
- **验收**：给出三方 KLD 与裁决，明确是否继续。
- **⚠ 07-15 起手实测（附录 D-8）**：**「≤1 天」的定位不成立** —— kvarn 档在**多窗口/长上下文**
  **无法运行**（受未决项 8 页几何阻塞），该门实际被 **WP3①** 前置。已测得的两臂（base、`rk4v4+tail`）
  与**单窗口 1438 token 的首个 KVarN 数据点**（正）记录于附录 D-8 / 进度记录 07-15。
  **执行顺序据此改为**：**先 WP3① → 再跑完整 WP0.5-B（ctx 8192 / 229k token）→ 才考虑 WP4/WP6**。
- **▲ 07-18 完成（代理门禁，附录 D-10）**：WP3① 已于 07-16 解除阻塞后，本轮补齐 229k 同字节矩阵
  （`rk2v4-e8` / `rk4v4-t0` / `nvfp4-t0` / `k8v4-t0`，全部 exit=0）。**裁决：代理门禁为正 ⇒ 投 WP4**。
  决定性数字：同字节 `kvarn:k4v2`(210 B) mean KLD **0.010513** vs `rk2v4-e8`(216 B) **0.043619**（**4.15×**）。
  **边界（必须随结论一起引用）**：① `k4v2` 是**非发布档**、WP4 的**代理**，本节**正式验收**仍需
  k4v4/k5v5/k6v6 对 `rk4v4`(280 B, **0.004426**)/`nvfp4`(288 B, **0.004385**)/`k8v4`(402 B, **0.002688**)；
  ② 优势是**每字节**而非绝对质量；③ **不据此外推 WP6**。
  ⇒ **下一 WP = WP4**；WP0.5-B 的正式版排在 WP4 出档之后。
- **✅ 07-21/07-22 正式版完成（附录 D-11）**：WP4 出档后跑三臂（`run5.sh`，17:58→18:39，全部 exit=0）。
  **裁决：三档全部达标、不回退** —— `k4v4`(274 B) mean KLD **0.002120** vs `rk4v4` **0.004426**（**2.09×**）
  与 `nvfp4` **0.004385**（**2.07×**，且字节更少）；`k5v5`(338 B) **0.001432**（无同字节档，落单调曲线中段）；
  `k6v6`(402 B) **0.001233** vs 逐字节相同的 `k8v4` **0.002688**（**2.18×**，门槛达成）。
  **勘误**：D-10 的「优势是每字节而非绝对质量」与「无 KVarN 特有速度代价」两条被发布档实测取代/收窄
  —— 见附录 D-11 ⑤ 与「v10 相对 v9 的变更摘要」第 4/5 行（保留原文以存史）。**WP6 仍未启动**。

### WP0.5-C — 构型口径声明（0.5 天，可与 A/B 并行）〔新增前置门〕
- 声明 `build-port` 是 `NINFER_SM120_NATIVE=ON`（TAIL 自述 unqualified）：要么改回 compat 路径重测噪声底，要么在 A1/A8 写明噪声底与 route 常量属于 native 路径；并处理 `AGENTS.md:34` 的 sm_86 文档矛盾。
- **验收**：一页说明，A1/A8 引用它。

### WP1 — kvarn ops 原样移植（2–3 天）
- 复制 `src/ops/kvarn/`（12 文件 2980 行）+ 两个公共头；接 CMake；移植 `tests/ops/test_kvarn.cpp`（**1710 行**）+ kvarn bench（**385 行**）；在**测试层**跑通（不改模型分发）。
- **验收**：`ninfer_kvarn_test` 通过（codec/hadamard/cached attention/prefill slab/batched/cache lifecycle/27B/tail staging/speculative boundary/publication settlement；容差见 WP5）。
- **回退**：若某单测依赖 fork 独有的 `paged_kv_cache` 断言，只在测试内适配，不动 ops。
- **注意**：WP1「照搬」只覆盖 ops + 测试，**不覆盖 20 文件接入面**（那是 WP2/WP3）。

### WP2 — 页面几何 + 存储枚举（2–3 天）〔**已完成 2026-10-07**，见附录 D-7；parser 单测残项由 **P3c 收口（08-07，附录 D-15）**〕
- 加 `KvarnGroup128` 枚举（**追加末尾**）；`kv_page_tokens()`；放宽 `paged_kv_cache` 校验到 64|128；**三个 CLI parser + 6 处名字 switch** + 身份指纹 `;kvbn=<bits>`。
- **为 kvarn body 提供独立 page-shift**（§4.4 陷阱），审计所有经共享 `>>6` helper 的路径。
- **验收（A1 前半）**：**非 kvarn 格式**全量 `ctest` 全绿（注册数 **261**，原写 259），page 仍为 64，输出逐字节不变。
- **回退**：枚举与几何解耦，先只加枚举 + parser，几何单独提交。
- **实际结果**：16 文件落地；`ninfer_ops` 0 错 0 新告警；CLI 功能检查（`kvarn:k7v7` 拒绝 / `kvarn:k5v5`、裸
  `kvarn` 通过解析）成立。**合成 245 项：242 通过 / 3 失败，3 项全部经基线（stash 重建）证实为先前存在**
  （`device_sync_empty`、`gdn_gating_proj` 与 WP2 无关；`kvarn_test` 为 WP1 已知容差）；**真实模型 16 项：
  10 通过 / 6 失败，全部为产物缺件或无 golden 或已知 A3**。⇒ **WP2 未引入任何新失败**。
- **当时未做（三条均已收口）**：kvarn **尚不可运行**（`plan_cache`/两处 host switch/route 挂载）→ **WP3 已收口（07-16）**；
  `--help` 与 `docs/` 未改（不宣传不可用功能）→ **WP3/WP4 已补（07-22）**；kvarn parser 单测未加 →
  **P3c 已补（08-07，附录 D-15）**：`ninfer_cli_options_test`/`ninfer_serve_options_test`（第三处 perplexity parser
  因内联于 `main.cpp` 结构性不可单测，行为覆盖由 P3b 的 e2e 提供）。

### WP3 — 模型接入（3–5 天）〔**✅ 完成 2026-10-08**：① 页几何（D-9）· ② 续列尾并 e2e 验收（D-12）· ③ 规划期拒绝 · ④ MTP 激励 + 诊断仪器（D-12/D-13）· ⑤ 措辞订正（v13）；见进度 07-14/07-16/08-04/08-05〕

> **⚠ 实测修正**：挂载点**不是** `causal_softmax_attention.cpp` 路由、也**不是** `small_t.cu` 的
> `small_t_kvarn` body 分支（两者在 FORK-B 都不存在）。kvarn 在**模型执行层** `execution/text.cpp`
> 的每个注意力调用点以 `if (storage==KvarnGroup128) ops::kvarn_attention(...) else
> ops::causal_softmax_attention(...)` 分派，每个分支的 `kvarn_attention` 自带
> stage+attend+commit。同理，「两处 host switch 注册」改为**显式分支 + 守卫**。

- **已落地（2026-10-07）**：`state/decoder_state.*`（kvarn 单 U8 记录平面 + sink/tail 尾槽 +
  `kvarn_layer_view`/`kvarn_batch_layer_view`/`reset_kvarn_tail_row` + `storage()`）；`include/ninfer/types.h`
  的 `KvarnBits::Bits2`；三个 CLI parser 的 `kvarn:k4v2` 内部档；`planning/startup.cpp` 的页几何
  （`kv_page_tokens` 贯穿 `page_count`/`maximum_main_page_groups`/容量曲线/`kv_capacity`/`attention_workspace`）；
  `execution/text.{h,cpp}` 的 `kvarn_provisional_` + **5 处派发**；`program/decode.cpp` group 钳制；
  `program/speculative/mtp.cpp` 两处 provisional。**构建全绿**；`ninfer_qwen3_5*` 18 项 + 相关 host 测试通过；
  `kvarn:k4v2` **短上下文（28 tok）端到端 PPL 11.18**（对照 rk4v4 11.22）。
- **① 地址空间页几何：已完成（2026-10-07，附录 D-9）**：`kv_pages_for_frontier`/`kv_pages_for_tokens`/
  `kv_tokens_for_pages` 增 `KvCacheStorage` 参数（42 调用点 + 各页跨度字面量改写）；
  `KVAddressSpaceStore` 由**构造期池几何**取 `page_tokens_`、`LogicalKVPageStore` 由其池几何取列上限；
  `resolve_host_cache_budget` 亦增 `storage`。**非 kvarn 路径逐位不变（仍 64）**。验收达成：
  `kvarn:k4v2` 在 ctx8192 / 229,348 token / 28 窗口全量跑通（PPL 4.72225 vs `bf16` 4.69317）。
- **剩余（按依赖顺序）**：② 续列尾 —— **✅ 完成（08-02 实现 / 08-04 验收）**：`state_image` kvarn 镜像 +
  `program_impl` 的 `restore/capture/activate_sequence_kvarn_tail` + `prefill.cpp`/`transactions/{capture,commit}.cpp`
  调用点；**e2e 验收 PASS**（`.deps/kvarn-adm/p1_prefix_reuse.sh`：kvarn r2 `cached_tokens=851 (99.4%)`、
  message 与 r1 **逐字节相同**；bf16 对照亦 851）；device 段单测经**测试自身尺寸 bug**（`marker_pattern` 3B
  vs `logical_pages` 12B）修正后绿（附录 D-12）。③ kvarn 与 `--mtp-attention-window` 的规划期拒绝 ——
  **✅ 完成（07-21）**。④ MTP 路径激励（生成档 `ninfer`/`ninfer-serve`）与 A3 相对判据并排对照 ——
  **✅ 完成（08-04 激励 / 08-05 仪器）**：kvarn MTP provisional 首次激励通过（`mtp accepted 84/113`、
  加速比 1.50× ≈ bf16 1.52×；附录 D-12）；`test_engine_mtp_greedy_parity_real.cpp` **已改造为诊断仪器**
  （去逐位门禁 + 加**自确定性门禁** + 首分叉报告 + 并入 `rk4v4`/`kvarn:k4v4|k5v5|k6v6` 并贯通
  `options.kvarn_bits`；`tests.cmake` 注册 `TEST_ARGS --quick`，全扫仍可手动跑）。token 级实测：
  **kvarn 自确定性全档成立；`kvarn:k4v4`@k1/k3 与 greedy 逐字节相同、@k15 418/512（bf16 同宽 412/512、同首分叉
  token 91）⇒ 分叉不劣于基线**（附录 D-13）。
- **⑤ 措辞订正（v13）**：早期计划写的「`small_t.cu:460-505` body 挂载（新增 `small_t_kvarn` 分支）」**不成立**——
  FORK-B 与 TAIL 均无 `small_t_kvarn`；kvarn 在**模型执行层** `execution/text.cpp` 的每个注意力调用点以
  `if (storage==KvarnGroup128) ops::kvarn_attention(...) else ops::causal_softmax_attention(...)` 分派
  （`small_t.cu` 的路径仍是 **bf16（非 kvarn）** 的真实体路由，见 §2 事实索引）。
- **验收**：`--kv-dtype kvarn:k4v4`（发布档）**能跑**（短 28 tok 与长 ctx8192/229k token 均已达成，见
  附录 D-9/D-11）；**前缀复用有缓存命中且输出逐字节稳定**（08-04 达成）；kvarn 档的 MTP 一致性**不劣于**
  同配置 `bf16`/`rk4v4` 基线（08-04 首测 MTP 加速比与接受率同量级）
  （A3 相对判据，**不要求逐位全等**）。
- **回退**：若某处共享面无法隔离，给 kvarn 独立池几何分支，不改公共路径。

### WP4 — 位宽参数化 K=V ∈ {4,5,6}（8–12 天）〔核心增量〕〔**✅ 完成 2026-10-07**：代码+测试+parser 全落地，正式 WP0.5-B 三档全过，见附录 D-11〕
- 收拢 `KBits` 死常量与全部 `4*item`/`&15`/`Group/2`/`D/4` 字面量（**≈55–60 处**）；`RecordBytes/各 Offset` 变 `bits` 的 `constexpr` 函数；`qmax=(1<<bits)-1`；**4 套位解包统一**（含 `attention.cu:354-360`）。
- 保留 4-bit nibble 快路径 + 5/6-bit 位流慢路径 + `if constexpr(bits)` 分派；**显式声明「借 beellama 位流、不借其 128 列切片」**（否则字节表整体作废，H7）。
- 发布 `kvarn:k4v4|k5v5|k6v6`；默认 `kvarn:k4v4`（§10-D2）。
- **验收（A2）**：三档各自 FP64 oracle 全绿；**位序往返测试**（pack→unpack 逐码比对）；`test_kvarn.cpp` 扩到 4/5/6。
- **回退**：若某档不达标，先只发 `k4v4`，其余挂起。**不回退到 k4v2（不在发布集）。**
- **前置**：先做 §6.3 的低成本探针（判 (a)/(b)/(c)）。
- **▲ 07-19 进度（设计细节见 §5「WP4 落地与设计定案」；执行记录见进度记录 07-19）**
  - **4.1 完成**：位宽站点测绘（子代理，逐文件读完）——解包是 **5 个例程跨 4 文件**（非 4 处）、位序一致；
    打包仅一套；`/15.0F`、`/3.0F` 即 `qmax`；**向量化约束确证**（int4 在 b=5/6 装不下整数个码）。
  - **4.2 首步完成**：`include/ninfer/ops/kvarn.h` 记录几何泛化为 `constexpr` 函数（旧常量由 `(4,2)` 派生
    ⇒ 逐字节不变，编译期断言通过），共享位编解码 `kvarn_unpack_code`/`kvarn_pack_code` 就位；构建绿。
    **核心洞察：k4v4 只改 V 侧**（K 侧与旧 k4v2 逐字节相同）。
  - **剩余（按依赖序）**：① 核模板化 `(kb,vb)` + `if constexpr(bits==4)` 保留 int4/nibble 快路径、5/6 走
    `kvarn_unpack_code`；`config.cuh` 的 `KBits/VBits` 与 `decode_kernel.cuh:27` 的 `static_assert(VBits==2)`
    一并参数化；`store.cuh:66/111` 的 `/15.0F`、`/3.0F` → `qmax=(1<<bits)-1`。
    ② `bits` 从 `EngineOptions.kvarn_bits` 经 `DecoderStateSpec` 贯穿到 launcher；`decoder_state.cpp:74`
    的平面内维 `kKvarnRecordBytes / kKvarnGroup` → `kvarn_record_bytes(bits,bits) / kKvarnGroup`。
    ③ `test_kvarn.cpp` 扩 4/5/6（`codec_oracle:214` 的 `qmax = key ? 15 : 3` → `(1<<bits)-1`；
    `DeviceStorage`/`view()` 形状改 `kvarn_k_row_bytes(bits)`/`kvarn_v_row_bytes(bits)`；加 pack→unpack
    逐码往返；容差按位宽重定，**收口 WP1 的 `ninfer_kvarn_test` 容差项**）。
    ④ parser 发 `kvarn:k4v4|k5v5|k6v6`（裸 `kvarn` ≡ k4v4，§10-D2）+ 删 `KvarnBits::Bits2` 与 `kvarn:k4v2`
    （§10-D4）→ 用发布档对 `rk4v4`/`nvfp4`/`k8v4` 跑正式 WP0.5-B（ctx8192 / 229k token；
    **`k6v6` 门槛 = mean KLD < 0.002688**，见附录 D-10）。
  - **✅ ①②③（代码与测试侧）已于 07-20/07-21 完成**：五目标构建绿；`ninfer_kvarn_test` **三档全绿**
    （bits=4 的 K `rel_l2=6.0882e-4` **与重构前逐位相同 ⇒ 4-bit K 零回归**；bits=6 的 K **rel_l2=0**；
    整套注意力套件改到 **k4v4** 几何并通过）；`bits` 贯穿到 launcher 与视图。
    执行细节与最终形态见 §5「WP4 执行定案」，实测数字见附录 **D-11**。
  - **✅ ④ 的 parser 部分于 07-21 完成**（三档发布、裸 `kvarn` ≡ k4v4、删 `Bits2`/`k4v2`），
    **并顺带补上规划期拒绝**（`kvarn` + `--kv-tail-tokens>0`，以及 `kvarn` + `--mtp-attention-window`）
    —— 前者此前会被**静默忽略**（§10-D5 / 进度 §0 未决项 10）。
  - **⬜ ④ 的实测部分（正式 WP0.5-B）为 WP4 的收尾**：用 `kvarn:k4v4|k5v5|k6v6` 对
    `rk4v4`/`nvfp4`/`k8v4` 在 ctx8192 / 229k token 上跑（协议同附录 D-10）；
    **逐档独立裁决**：`k4v4` 不优于 `{rk4v4,nvfp4}`、或 `k6v6` 不优于 `k8v4`（< 0.002688）则该档**移除**
    （回退：先只发 k4v4）。

### WP5 — oracle 与容差规范（2–3 天）〔**✅ 完成 2026-10-08**：判据形式化 + 穷举位序往返 + **余量实测（08-04，65× 余量）**，见附录 D-12 / 进度 08-04〕
- 容差**按量化步长**定义：`qmax=(1<<bits)-1` ⇒ `q=(max-min)/qmax*row_scale*column_scale`（相邻码的值距）；
  `3.0e-4`（**只在 4-bit 有效**）**已废弃**，`1.0e-3` 拟合阈值（07-21）**已于 08-03 删除**。
- oracle 必须 host 侧 FP64、与 kernel **零共享代码**；packed 输入用**存储的 scale 独立解码**后再比。
- **验收**：每档 oracle + 往返 + 记录解码检查三件齐备。**08-03 落实**：①点值 `|Δ| ≤ q*(1+5e-2)`；
  ②`flips ≤ 1.0e-3*total`（边界翻码计数上限）；③`|Δcode|>1` 零容忍；④4/5/6 位**穷举逐码**
  pack→unpack 往返 + 行外哨兵。口径写入 `docs/maintainer/op-development.md §6.3`。
- **残留**：**已无**。**08-04 首跑实测**（`ninfer_kvarn_test`，GPU）：`flips` 最大 **1/65**（bits=4/5 的 K 各 1、
  bits=6 的 K 为 0；V 三档全 0）、`over_step=0`、`wide_flips=0`、`exit=0` ⇒ **预测（bits=4 约 1 翻码）
  与实测吻合，判据 65× 余量、成立**。`step_ratio_max=1`（K，bits 4/5）即「恰好一个量化步长」，与 07-21 的
  单元素边界翻码根因一致（附录 D-12）。**判据未放宽**。
- **回退**：无。

### WP6 — 旋转域尾部合并（**路线 = (a)**，10–15 天）〔最高风险〕〔**✅ 路线已裁决（2026-10-08；v20 / D-19 ⑧·⑨）**〕
- **路线决定**：走 **(a)** —— 照 TAIL 已有的 `small_t_k8v4.cuh` 形状，把 KVarN body 接进**外部共享精确尾环**（`--kv-tail-tokens`）；KVarN 侧新增"**旋转域 tail partial + 两段式归并**"。
  **理由与量化（D-19 ⑧）**：① **显存非决定因素** —— (a) **88 MiB/序列** vs (c′) 76.5 MiB（C=1，差 **12 MiB**）、C=8 差 104 MiB；
  ② **(a) 的增量性能代价可忽略** —— 精确双写 64 KiB/token ≈ `6×10⁻⁶` 于权重流、最新 N 键多读 +46 MiB/步 但 **KV 仅占 decode 总流量 ~1.4% ⇒ 端到端 ≈ +0.4%**、workspace +0.4 MB（`attention.cu:567`）；
  ③ 消除 `--kv-tail-tokens` 的**双实现语义**与本仓 **BF16-acc 偏离**、判据与其它档位**同构**。**(c′)（body 自建滚动槽）已否决**（省 ~3–5 天，换永久语义债）。
  **历史门禁存档**：v18 判"证据不足"（§6.3 探针被静默跳过）→ v19 探针否证 **(b)**（`ROTATED_K_ORIGINAL_V` 只对 `>16` query token 生效、decode 恒 `ROTATED`、V **存储**恒旋转、**坐标系本身即障碍**；且本仓 BF16-acc 与共享 reducer 的 FP32 不符）。
- **不变量（贯穿全部子步）**：**A6 的「`tail=0` 逐位不变」是 WP6.2 的接受条件**；`--kv-tail-type` **f16 必须显式拒绝或真支持，不得静默**（§6.4）；`kvarn + tail` 未完成前任一子步失败即回到 **fail-fast**（§6.4）。

**WP6 分步（产出 / 验收 / 回退）**

| 步 | 内容 | 产出 | 验收 | 回退 |
|---|---|---|---|---|
| **WP6.0** | **两项前置测量**（用户 08-12 指定；**GPU、~0.5–1 天**）：**0a 本机 prefill 噪声底**（同二进制 / 同 prompt / 单请求，重复 **≥5**，报 pp 的**中位数 + 最差 + 极差**；补 WP0 产出 ② 的欠账）；**0b 现有外部尾的纯 kernel 代价**（`--kv-dtype rk4v4`（或 `bf16`）× `--kv-tail-tokens {0,1024}`、**关 MTP**、decode-only、重复 **≥3**，报 tg 分布），把 A8 的 `−5.8% / −2.1pt` 分解为「纯 kernel」+「接受率」 | 两份落盘报告（命令 + 原始输出 + 分布） | ≥5 / ≥3 重复；中位数与最差齐；**结论回写 A1 / A8 / D-19** | 若无稳定判据 ⇒ 该腿**显式记「不可断言」**，不伪造判据 |
| **WP6.1** | **KVarN tail-partial 路径（W1）**：新增**旋转域** tail partial，读外部精确尾环（bf16/f16 **原始行**），按分区算出 `(acc, m, l)`。**实现选"acc 旋一次"**（尾 partial 在**原始域**算完 → 只对 **FP32 acc** 施加 `W`；`W` 线性 ⇒ 与"先旋行再算"数学等价，代价见 §6.2 条 4 订正） | `src/ops/kvarn/` 新文件（**~300–450 行**） | 与 **FP64 oracle** 比 `(acc,m,l)` 落在**量化步长判据**内；`N=0` 时该路径**不被触发** | acc 旋转数值不达标 ⇒ 退回**逐行旋**（读时旋转） |
| **WP6.2** | **归并改造（W2，最高风险）**：`partial_acc` **BF16→FP32**；`reduce_output_hadamard_kernel`（`decode_kernel.cuh:1037-1121`）改**两段式**：body splits → per-row `(acc,m,l)` → 与 tail 合并 → **一次反旋** → 输出。**照上游同构做法**（`fattn-kvarn-portable.cuh:784-786`、`fattn-tail.cuh:339-346`） | 改 **~120 行**（于 1155 行文件内）+ `decode.cu:113,120` | **A6：`tail=0` 输出逐位不变**（既有 `ninfer_kvarn_test` + 续列尾 e2e 逐字节比对）；A2 的 4/5/6 位穷举往返仍过；`ninfer_kvarn_test` **余量与 08-04 逐位同** | `tail=0` 无法逐位保持 ⇒ **拒绝该改造**，改走"**独立 merge 核**"（不动既有 reduce） |
| **WP6.3** | **分区与接线（W2b/W3/W4/W5/W6）**：op 内部完成 `body_window = window − N` 分区（照 `small_t.cuh:147-170` 形状，**不必改 `text.cpp` 的 envelope**）；3 个 op 入口（`kvarn_attention.h`）加尾视图；6 个调用点（`text.cpp:391,414,567,622,1021,1045`、`context.cpp:1745`）；尾环写入接入 KVarN append（照 `small_t_tail_shadow.cuh` 71 行形状）；`startup.cpp:1035-1045` 解除 fail-fast | ~**250–350 行** | `kvarn:k4v4 + --kv-tail-tokens 1024` **端到端可跑**；`--kv-tail-type` 按裁决**显式拒绝或支持**；workspace 重算后不溢出 | 保留 fail-fast（§6.4） |
| **WP6.4** | **A6/A7 脚手架（W7）**：FP32 oracle 覆盖「KVarN body × 旋进 bf16/f16 尾」的合并路径；跨 **group(128)/ring(64)** 边界的 needle 检索；边界用例（`N ≤ ring`、`N ≥ width`、跨 checkpoint 恢复、`N=0`） | 新测试 **~300–500 行**（照 `tests/ops/softmax_attention/causal_cache.cpp` 尾测形状） | oracle **精确**；needle **精确命中**；无重复计数 / 丢键 | — |
| **WP6.5** | **容量 / 显存**：外部尾环进入 KVarN 的 `MemorySummary` 与容量曲线（**WP7 的 kvarn 部分提前到此**，因为 (a) 要新分配外部环），实测 ±5% | 代码 + 实测 | **A5** | 估算与实测分列，先报告后收敛 |
| **WP6.6** | **质量 / 速度收口**：`kvarn:k4v4 + tail{0,1024}` 的 decode-width **mean-KLD 单调下降**；**显式处理 A4 的 N>384 风险**（至少测 `N ∈ {384, 1024}`，或声明阈值随 N 调整） | KLD / 字节 / 速度三联表（WP8 的 kvarn 行） | **A4**（+ `same_top` / max-KLD 双指标、≥3 重复）+ **A8** | A4 不达标 ⇒ 记录并裁决是否保留该特性 |
| **WP6.7** | **回归**：A2 / A3 / MTP / 续列尾 / 前缀 / ctest 263 + **新增 `--vision` × kvarn 功能门** | 测试 + 跑批 | **A1 / A3** + 视觉功能门（A1 邻域口径） | — |

- **⚠ 待定（WP6.3 之前必须有结论）**：尾的 dtype —— 采 **bf16** 则复用现成 BF16 旋转入口（**免**"f16 旋转入口"交付物；用户已接受"若采 bf16 尾则在本机复核"）；若坚持 **f16** 则须**新增 f16 旋转入口**（`kvarn_hadamard` 只收 BF16，`codec.cu:185-189`）。

### WP7 — 容量 / 显存核算落地（1.5–2 天）
> **范围订正（v20）**：**kvarn 的外部精确尾环**（(a) 路线要新分配的那一份）归 **§7-WP6 的 WP6.5**；本包余下项（kvarn 本体 `per-token/head`、表与计算器同步）不变。
- 加 kvarn 的 `per-token/head = 32*(Kb+Vb)+18` 到 `plan_cache`/`load.cpp`/`MemorySummary`；同步 `docs/config-calculator.html:529`；**表须含 16 full-attn 层 + 1 MTP 池**（+6.25%）与「页 64→128 的容量取整」。
- **待核实**：StateImage slot 是否复制 kvarn sink/tail（真源 `decoder_state.cpp:118-161`，**不是** `state_image.cpp`）；若复制，按 slot × 并发计入 A5。
- **验收（A5）**：显存估算与实测 ±5%，基准为**修正后**的表（含本产物实测权重 11,092,477,952 B）。
- **回退**：估算与实测分列，先报告后收敛。

### WP8 — 质量/速度收口实验（2–3 天 + GPU 时间）
- 同模型同口径：`rk4v4`、`rk4v4+tail`、`kvarn:k4v4/k5v5/k6v6`（各 ±tail），**统一用 decode-width KLD**（`--score-width 8 --kld-base`，协议 `--disjoint --score-topk 100`、32,767 评分 token）。
- **验收（A4）**：给出「质量-字节-速度」三联表；逐档独立裁决——**k4v4 不优于 {rk4v4,nvfp4}、k6v6 不优于 k8v4 则移除该档**；k5v5 无同字节档，判据另定；三档全不达标则整个特性降级。
- **回退**：无（决策性实验）。

### WP9 — MTP / prefill / graph 全路径回归（2–3 天）
- 扩展长解码、混合行、前缀恢复、eager/full head、长上下文切换矩阵到每档；MTP 深度 0..3。
- **验收（A3/A8）**：MTP 一致性的**相对判据**成立（kvarn 档不劣于同配置基线，A3）；graph 复用键含 `(profile,bits,N,R,type)` 且无需频繁重捕获；pp/tg/MTP 接受率不劣于同字节档噪声底（含尾部的档位按 −5.8%/−2.1pt 放宽）。
- **回退**：graph 重捕获代价过高则 kvarn 档单独关 graph 复用。

**总计 ≈ 34–52 人日**（WP4 8–12、WP6 10–15、WP0.5 前置 2 天；WP1 因设备侧接口兼容仍低于 fork 首发预估）。

---

## 8. 关键文件与规模

| WP | 主要文件 | 规模 |
|---|---|---|
| WP1 | `src/ops/kvarn/*`（12 文件 2980 行）+ `include/ninfer/ops/{kvarn,kvarn_attention}.h` + `tests/ops/test_kvarn.cpp`（1710 行）+ kvarn bench（385 行） | 照搬 + 测试 |
| WP2 | `include/ninfer/types.h`、`core/paged_kv_{cache,storage}.h/.cpp`、`ops/kv_cache/d256_profile.h`、**三个 CLI parser**、`serve/*`（含 6 处名字 switch） | 中 |
| WP3 | `causal_softmax_attention.cpp`、`small_t.cu`、`execution/text.cpp`、`program/{decode,prefill}.cpp`、`state/decoder_state.*`、`program/storage/context.cpp`、`planning/startup.cpp`、`graph_profiles.cpp` | 中 |
| WP4 | `src/ops/kvarn/{config.cuh,store.cuh,codec.cu,decode_kernel.cuh,attention.cu,streaming_prefill.cuh,materialized_prefill.cuh}`、`kvarn.h`、`state/decoder_state.cpp:44` | 高 |
| WP6 | `src/ops/kvarn/decode_kernel.cuh`（tail-partial 路径）、`small_t_tail.cuh`、`kv_cache/hadamard_d256.cuh` 或 `hadamard_transform.h` | 高 |
| WP7 | `load.cpp`、`program_impl.cpp`、`docs/config-calculator.html` | 小-中 |
| WP8/9 | `apps/perplexity`、`tests/ops/*`、`program/speculative/*`、`graph_profiles` | 小-中 |

---

## 9. 风险登记

| 风险 | 影响 | 缓解 |
|---|---|---|
| 共享面（`paged_kv_cache.cpp` 校验放宽）影响非 kvarn 格式 | 默认路径回归 | WP2 关闭态全量 `ctest` 作为门禁；默认仍 64 |
| **宿主硬抛未注册**（`d256_profile.h:128`、`paged_kv_storage.h:120`） | 启动即崩 | WP3 首步注册两处 host switch；WP0.5-C 明确 |
| **共享 page-shift 陷阱**（128-body 经 `>>6` helper） | 静默读错页 | WP2 为 kvarn body 独立 shift + 审计 |
| 5/6-bit 位流与 decode 加载位序不一致 | 静默数值错误 | WP5 位序往返 + FP64 oracle；对标 beellama `pack/unpack` |
| **旋转域坐标不一致** | 尾部对 KVarN **静默惰性** | WP6 独立门禁 + **fail-fast**；禁止静默 |
| 尾行旋进与 body 的 Hadamard 顺序/符号不一致 | 质量异常难归因 | 逐位对齐 fork `hadamard.cuh`；FP32 oracle 覆盖合并路径 |
| `un-rotation` 被施加多次 | 输出错误 | WP6 明确「一次」；oracle 校验 |
| KVarN 自带 sink/tail（24 MiB/序列）与外部 ring（64 MiB/序列）叠加 | 显存超预算 | WP7 两者分列；A5 用修正后基准 |
| **同仓已有负面裁决**（`docs/port-records/PORT-DOD.md:23-32`） | ROI 单点风险 | **WP0.5-B 前置准入实验**，先证再投 |
| **构型未鉴定**（SM120_NATIVE / sm_86 文档矛盾） | 噪声底与常量前提不成立 | WP0.5-C 决断 |
| 无公布吞吐/质量 | 计划不确定 | **部分不成立**：FORK `docs/performance.md:145,153-154,159` 已公布 K4V2-G128 tok/s（407.6→412.4、238.1→240.1）与「约 0.8% decode 代价」；仍**不引用 beellama ladder 作结论**（其 30.1/36.3/42.6% 含 sink+tail+slice，与纯 codec 26.8/33.0/39.3% 不可直接互比） |
| ~~**A3 判据不可满足**~~ → **已定案**（2026-10-07：上游基线本身不满足 MTP greedy parity；差分证明非本移植引入；该判据实为 Fork B 私有合同） | 原"绝对 parity"门禁恒红，会误判移植回归 | **已按 O1 替换判据**：同配置自确定性 + 质量不劣化 + 首分叉文档化 + kvarn 专属"不劣于基线"门禁（附录 D-6）；parity 测试改造为诊断工具 |
| 发布集不含 k4v2 ⇒ 失去 20.5% 档 | 收益缩水 | 已知取舍（§10-D4） |
| **本机 `ctest` 基线非全绿**（`device_sync_empty`、`gdn_gating_proj` 先前存在；`kvarn_test` 容差） | A1「全绿」口径被误读为回归 | **已实测并回写 §1 A1**；WP2 用 `git stash` 重建基线逐项对照（附录 D-7）。后续 WP 一律先取基线 |
| **真实模型测试在本机产物下大量结构性不可跑**（缺 DFlash/DFlash2、非 35B MoE、无 prompt golden） | A8/§7 的端到端证据受限 | 记录于附录 D-7；需要时另取得对应产物，或显式声明该腿未做 |
| ~~**WP2 的过渡态**：`--kv-dtype kvarn:*` 可被 parser 接受但引擎**抛异常**~~ | 用户误以为可用 | **已由 WP3 收口（07-14）**：parser 只接受内部档 `kvarn:k4v2`，其余 kvarn 档**明确拒绝**；`--help`/docs 仍不宣传 kvarn |
| ~~**地址空间页几何未随存储变化**~~ → **已消除（WP3①，2026-10-07，附录 D-9）** | 长上下文 kvarn **无法运行**（`create_active` 因 entitlement(64 页口径) > page_capacity(128 页口径) 返回 `nullopt`） | **已修**：页大小按 `kv_page_tokens(storage)` 贯穿（42 调用点 + 各页跨度字面量；`KVAddressSpaceStore` 取池几何）；验收达成 = `kvarn:k4v2` 在 ctx8192 / 229,348 token 跑通 |
| **kvarn 续列尾未实现**（state_image 无 kvarn 镜像、无 capture/activate/restore） | 前缀复用/检查点往返时 kvarn 尾槽**丢失/陈旧** ⇒ 静默错答 | **WP3 剩余②**：实现尾槽续列；在实现前**限制**在无续列的 fresh 单请求路径（当前短上下文即此路径） |
| **kvarn 与 `--mtp-attention-window` 组合未验证**（窗口变换走 64 页块表） | 静默错读 | **WP3 剩余③**：规划期拒绝二者同用 |
| ~~**KVarN 尾槽标记在执行行复用时陈旧**~~（`reset_kvarn_tail_row` 曾零调用者） | 复用 row 的打分路由读入**上一占用者**的 sink/tail ⇒ 静默错答 | **已修（P3d，2026-10-08，D-16）**：`ProgramImpl::causal_score` 在绑定 Main row 0 后调用 `reset_kvarn_tail_row(0, …)` + 新单测 `ninfer_qwen3_5_kvarn_tail_row_reset_test`（PASS）。**端到端对照已跑（08-11，D-17）**：`kvarn:k4v4` KLD 臂 mean **0.002120**，与 D-11/P3a **逐位相同**（mean/median/P99/max/same_top/dlogp/PPL）⇒ 该协议上为**无害 no-op**、D-11/P3a 数值**无需重测**。**改判**：本项风险实测为「**已验证无害、必要性未证**」——本协议窗口恒 `context` 长且页号每窗口归零 ⇒ 陈旧标记与当前页号**重合**；`Engine::score_tokens` 的**变长窗口**冲突（`claim_tail_slot` 首选/备用槽皆被占 ⇒ 返回 −1、跳过 staging）**未实测**。本改动仍作**防御性正确性修复**保留 |

---

## 10. 待决项

- **D1 存储建模**：**已决（WP2，2026-10-07）**——采**单枚举 + `KvarnBits` profile 字段**：
  `KvCacheStorage::KvarnGroup128`（追加末尾）+ `enum class KvarnBits{Bits4,Bits5,Bits6}`，随
  `EngineOptions.kvarn_bits` 流动。（推荐理由成立：7 个穷举 switch × 3 的注册成本；且**不重排既有枚举值**。）
- **D2 默认档**：**已决（WP2，2026-10-07）**——① `--kv-dtype` 的**产品默认不变**（仍非 bf16、非 kvarn）；
  ② **裸别名 `kvarn` ≡ `kvarn:k4v4`**（k4v4 为 family-default，三处 parser 一致实现）；
  ③ `kvarn:k5v5|k6v6` 需显式写全。指纹 `;kvbn=<4|5|6>` 恒记级别，故 k4v4 与 k5v5 不会被混同。
  **✅ 07-21 已生效（WP4.4）**：三处 parser 现接受 `kvarn`/`kvarn:k4v4|k5v5|k6v6`（②③ 落地），
  内部档 `kvarn:k4v2` 与 `KvarnBits::Bits2` **已删除**。
- **D3 尾部 dtype**：默认 **f16**（对齐 TAIL 现状，`types.h:469`）。
- **D4 是否补 k4v2**：~~仅作 oracle 对照（**不在发布集**，且**不加入 parser**；host oracle 不需要 CLI 档）~~ →
  **07-14 修正**：k4v2 在 WP3–WP4 期间为唯一可运行内部档，故曾加入三处 parser。**✅ 07-21 终结**：
  `kvarn:k4v2` 与 `KvarnBits::Bits2` 已从 parser 与枚举中**删除**（不在发布集）；
  **其记录几何（4,2）仍由 `kvarn_*_offset(4,2)` 表达**（头文件的 `constexpr` 函数），
  但**没有任何设备核实例化它**（发布档只实例化 `(b,b)`）；host oracle 不需要 CLI 档。
- **D5 fail-fast 覆盖范围**：WP6 前，`kvarn:* + --kv-tail-tokens>0` 直接报错。**✅ 07-21 已实现**
  （`validate_target_options`；同时拒绝 `kvarn:* + --mtp-attention-window != 0`，见 §0 未决项 10）。
- **D6 构型**：继续 native（SM120_NATIVE=ON）还是回 compat（WP0.5-C）。

---

## 11. 明确不做

- 不移植 KVarN 的 ggml op 家族 / `KVRN` v16 序列化 / Vulkan-HIP / 多 GPU placement / MLA-DSA。
- 不做 36 组合全菜单；只做 **K4V4 / K5V5 / K6V6（K=V）**；不做 K≠V 混合档。
- 不做 g64 tile。
- 不改非 kvarn 格式的任何行为；不改默认 `--kv-dtype`。
- 不做「静默惰性」：任何未实现组合必须 fail-fast。

---

## 附录 A：字节几何（修正版，供复核）

**公式与闭合**

- `RecordBytes(Kb,Vb) = 4096·(Kb+Vb) + 2304`；`4096 = D·G/8 = 256·128/8`。
- 元数据 2304 = `512+512+256+512+256+256` = **18.0 B/token/head**（与 bits 无关，4/5/6 沿用）；由三处独立证实（FORK `kvarn.h:18-27` 偏移、`decode_kernel.cuh:122-127` 六字段全 `__half`、`UP config.py:87,97` 1280+1024）。
- `B/token/层/头 = RecordBytes/128 = 32·(Kb+Vb)+18`。
- `B/token(全模型) = 16 · 4 · (32·(Kb+Vb)+18)`；bf16 基线 = `16·4·(256·2·2)` = **65,536**（与 `config-calculator.html:529` 实测一致）。
- 对齐：35072 / 43264 / 51456 均为 **256 的整数倍**。

**对照实测值**：`config-calculator.html:529` = `kv: { bf16:65536, int8:33792, fp8:33024, rk8v4:26112, rk4v4:17920, k8v4:25728, nvfp4:18432 }`（:490 是**权重 profile 散文**，:522 `weightsBytes:17093490688` 属 groupwise-int 产物，:550 `mtp3 kvRatio:1.062988281`，:563 为 35B 表）。`rk4v4` 结构复推 `(128+8)+(128+16)=280 ×64 = 17920`（`paged_kv_storage.h:82-92`）。

**尾部显存（两类，分列）**

| 类别 | 大小/序列 | 出处 |
|---|---|---|
| KVarN 自带 sink+尾槽（**共 3 槽 = 1 sink + 2 动态尾**，K+V，16 层） | **24.0 MiB**（+MTP 池 1.5 MiB = 25.5 MiB） | `decoder_state.cpp:118-161`（256·128·2·(4·3)·16·2） |
| 外部精度尾部 ring（N=1024） | **64 MiB + 4 MiB reserve**，×并发 | `docs/port-records/PORT-MEMORY.md:394` |

## 附录 B：关键命令（本机）

```powershell
# 构建（复用 build-port，勿重配）——Git Bash 下不要内联 `>nul`（MSYS 会改写为 /dev/null）
call D:\ninfer\ninfer-precision-tail\.deps\env-port.bat
cmake --build D:\ninfer\ninfer-precision-tail\build-port --target ninfer_ops -j 4   # CUDA 算子变更后
cmake --build D:\ninfer\ninfer-precision-tail\build-port --target ninfer_tests ninfer-serve ninfer-perplexity -j 4

# 跑模型（示例；kvarn 档位在 WP3 之后可用，冒烟期先用 rk4v4）
build-port\apps\ninfer.exe "D:\ninfer\ninfer-precision-tail-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer" `
  --prompt "Summarize prefill vs decode." --max-context 32768 --max-new 8192 `
  --kv-dtype rk4v4 --kv-tail-tokens 1024 --kv-tail-type f16 --spec mtp --draft-tokens 3
#   （WP3 之后）：--kv-dtype kvarn:k4v4

# KLD（尾部敏感：score-width<=8 走 small-T 合并路由；协议对齐原实测带）
build-port\apps\ninfer-perplexity.exe <model> --corpus eval\corpora\perplexity-1m\manifest.json --quick `
  --context 8192 --stride 4096 --score-width 8 --disjoint --score-topk 100 `
  --kv-dtype bf16 --kv-tail-tokens 0 --save-topk .deps\bf16-t0.topk
build-port\apps\ninfer-perplexity.exe <model> --corpus eval\corpora\perplexity-1m\manifest.json --quick `
  --context 8192 --stride 4096 --score-width 8 --disjoint --score-topk 100 `
  --kv-dtype kvarn:k4v4 --kv-tail-tokens 1024 --kld-base .deps\bf16-t0.topk

# 测试（今天只匹配 3 个；移植后追加 ninfer_kvarn_test / MTP parity）
ctest --test-dir build-port -R 'ninfer_(kvarn|softmax_attention|kv_cache|kv_cache_append|kv_capacity)_test' --output-on-failure
```

## 附录 C：证据索引（本机核实，修正版）

| 结论 | 出处 |
|---|---|
| fork 无 GGUF/IQ3 ⇒ 跑不了目标模型 | `FORK src/artifact/formats.cpp:10-20`；`gguf` 代码 0 命中；清单 398/1189 为 `gguf_blocks_v1` |
| **fork 是 Linux-only 构建** | `FORK README.md:31`；`cmake/Dependencies.cmake:1-5`；自有代码+构建系统 `WIN32/MSVC` 命中 **0**（182 处全在 `third_party/`）；无 Windows 脚本/构建树 |
| fork 无 `small_t_tail*`、0 处 `kv-tail-tokens` | `Glob ninfer-rtx5090-mobile/**/small_t_tail*.cuh` = 空；grep 0 |
| TAIL 有 tail（含 Prompt 路线），无 kvarn 实现 | `small_t_tail*.cuh` 存在；`prompt.cu:13` include shadow 头；`grep -ril kvarn src include apps` = 0（**源码内无实现，非无证据**） |
| kvarn ops 自包含、2980 行/12 文件 | `src/ops/kvarn/*` `wc -l` |
| 宿主硬抛 | `d256_profile.h:128`、`paged_kv_storage.h:120`、`decoder_state.cpp:31`、`device_calibration.cu:699` |
| 共享面规模 | `KvCacheStorage` 248 行/44 文件；7 处 `switch(storage)`；6 处名字 switch |
| 页 64 进内核寻址 | `paged_kv_address.cuh:9-10,68`；166 处命中 |
| body 挂载点真路径 | `src/ops/softmax_attention/dense/causal_cache/{causal_softmax_attention.cpp:347,392, small_t.cu:460-505}` |
| FORK 接入面 | `grep -rl KvarnK4V2Group128` = **20 文件** |
| 尾部默认 f16 | `types.h:467-469` |
| ppl 看不见尾部 / KLD 仪器 | `docs/port-records/PORT-MEMORY.md:385-393`；`apps/perplexity/main.cpp:175-177` |
| 同仓既有裁决 | `docs/port-records/PORT-DOD.md:23-32`（commit `8e34ad12`）；`docs/performance.md:754-764`；`.deps/wpc/kld-kvarn4-*.out` |
| 尾部已知代价 | `docs/port-records/PORT-MEMORY.md:663-670`（−5.8%/−2.1pt）；`docs/port-records/PORT-MEMORY.md §5.17(2)`（prefill −29%…+8%、decode ≤0.88%） |
| rk4v4 tail 增益带 | `docs/port-records/PORT-VERIFY-REPORT.en.md:189-194`、`README.md:21`、`.deps/verify-b-summary.txt`（2.26–3.14×，tail0→tailN） |
| 27B 几何 16 full-attn 层 / 4 KV 头 / D256 | `FORK docs/maintainer/qwen3_5-model.md:60-64`；`geometry.cuh:15-16` |
| 构建树与本机 | `build-port/CMakeCache.txt`（Ninja/Release/120a/CUDA 13.3/14.44.35207/**BENCHMARKS=OFF/SM120_NATIVE=ON**）；`nvidia-smi` 5070 Ti / cap 12.0 / 617.14；模型 11,092,477,952 B |

*本计划仅记录移植方案，未对任何仓库做写操作。*

---

## 附录 D：执行记录（D-1…D-16；D-1…D-9 已逐字归档，D-10…D-16 在下方正文）

> 本节由执行期追加，记录实际发生的裁决与实测，作为 A1/A8 引用的一页说明。

### D-1 … D-9 索引（已逐字归档；正文引用「附录 D-N」时按本表解析）

> 全文（含命令、原始数字与措辞）在 `docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md`，
> 该文件内的小节标题即 `### D-N <原标题>`，与本表逐条对应。**本表未改写任何结论。**

| 锚点 | 原标题（逐字） | 一句话 | 承重状况 |
|---|---|---|---|
| D-1 | WP0.5-C 构型口径声明（D6 决议） | 本机为 **native 口径**（`NINFER_SM120_NATIVE=ON`）⇒ A1/A8 噪声底按此读；compat 口径未测 | 现行（§10-D6 / §7-WP0.5-C） |
| D-2 | WP0 基线发现 | 整树构建唯一失败目标 `ninfer-multi-gpu-probe.exe`（**先于本工作存在**）⇒ 一律按目标构建 | 现行（§9 / §1 A1） |
| D-3 | WP1 结果（ops 编译通过；测试 30 项中 1 项容差失败） | ops 零告警编过；`ninfer_kvarn_test` 1 项 K oracle `relative_l2=6.09e-4 / limit=3.0e-4 / max_abs=0.216` 失败 ⇒ 归 WP5 | 已由 WP4.3（07-21）+ WP5（08-03/08-04）收口 |
| D-4 | WP0.5-A 结果：仪器已就位，但 **A3 目前不成立** —— ⤴ 本条结论已于 D-6 定案替换（O1） | 纯 `bf16` k=1 在 **token 91** 分叉（`expected=2466 actual=2640`），k=0/k=3 全等；`--no-cuda-graph` 逐字复现 | **已被 D-6 替换**（原小节标题即已标注） |
| D-5 | A3 差分裁决：缺陷**先于 precision-tail 存在**（2026-10-07，实测） | 上游包与 TAIL 包同模型对照：两仓 MTP-off/on **同 index 538 分叉**、对应输出**逐字节相同** | 差分结论现行；性质判定由 D-6 接管 |
| D-6 | A3 定案 + 报告复核裁决 + 计划调整（2026-10-07）（回写 §1 A3 / §7 WP0.5-A·WP3·WP9 / §9；版本 v3→v4） | C1–C9 逐条复验通过 ⇒ **采纳 O1、拒绝 O2、O3 次优**；并记录 2 项复核瑕疵 | **现行**（§1 A3、§9、§7 判据出处） |
| D-7 | WP2 执行记录（页面几何 + 存储枚举，2026-10-07） | `stash` 重建基线法：合成 **242/245**、真实模型 **10/16**，失败均非 WP2 引入 ⇒ A1 的 `ctest` 口径据此收窄 | **现行**（§1 A1 口径注、§9） |
| D-8 | WP0.5-B 起手实测（2026-10-07，详见进度记录 07-15） | `kvarn:k4v2` 长上下文**不可运行** ⇒ 准入门被 WP3① 前置；1438 tok 单窗口首个数据点 0.0049 vs 0.0280（≈5.7×） | 由 D-9 解除阻塞、由 **D-10 以 229k 规模取代** |
| D-9 | WP3① 执行记录（地址空间页几何，2026-10-07）（回写 §7-WP3 / §9；版本 v7→v8） | `entitlement`(64 口径) > `page_capacity_`(128 口径) ⇒ `nullopt`；19 文件按 `kv_page_tokens` 贯穿 ⇒ ctx8192 / 229,348 token 跑通，PPL **4.72225** vs `bf16` **4.69317** | **现行**（§9 该行标「已消除」的实测出处） |


---

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

---

## 已归档信息索引

本次精简（2026-10-08）**只搬运、不改写**：下表三份归档文件收录的是本计划书与进度日志中
**推进计划已不需要**的内容，全部为**逐字副本**（字节级搬运，源行号见括号）；正文对应位置各留一行索引。
**验收口径（§1 A1–A8）、风险登记（§9）、未决项（§10 D1–D6）与 §0.5 记录规则未作任何改动。**

| 归档文件 | 覆盖范围（源行号） | 一句话内容 | 正文的索引位置 |
|---|---|---|---|
| `docs/port-records/KVARN-PLAN-CHANGELOG-ARCHIVE.md` | 本计划书原第 3 行（v13 完整版本头）+ 原第 28–132 行（**v3→v12 十张变更摘要表**） | 每一版相对上一版的修正、依据与实测数字（版本演进史） | 版本头末句 + 「变更摘要索引（v3→v12 已逐字归档）」表 |
| `docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md` | 本计划书附录 **D-1 … D-9**（原第 804–1094 行） | WP0.5-C 构型口径、WP0 基线、WP1 ops/测试、A3 仪器—差分—定案三件、WP2 执行、WP0.5-B 起手、WP3① 页几何 | 附录 D 内的「D-1 … D-9 索引」表（锚点名逐条保留） |
| `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` | 进度日志 §3 **2026-10-07-1 … -23**（原第 260–1312 行）+ §0 快照明细 / 工作树清单 / 已关闭未决项 1·5·8·9·10 + §4.1 / §4.4 | 早期 WP 的逐日执行记录与当时的任务定义（含失败、被推翻的假设与原始命令） | 进度日志 §3 条目索引表、§0 与 §4 的指针行 |

**仍在正文、未归档（WP6/WP8 会直接引用）**：附录 **D-10**（229k 同字节矩阵；`k6v6` 门槛 < `k8v4` 的
**0.002688**）、**D-11**（发布档三档 `k4v4` 0.002120 / `k5v5` 0.001432 / `k6v6` 0.001233，
对 `rk4v4`/`nvfp4`/`k8v4` 为 2.09×/2.07×/2.18×）、**D-12**（GPU 收尾四件 + P2 前置）、
**D-13**（token 级 MTP 分叉表）、**D-14**（P3b 档位并入名字 + `capture_identity_tag` 发现）、
**D-15**（P3c parser 单测）、**D-16**（P3d 三项 LOW 修复，含 CausalScore 尾标记重置的生产变更）、
**D-17**（P3d ① 的 KLD 端到端对照＝逐位 `0.002120` ⇒ 无害但**必要性未证**；待裁决 3 项裁决；「子代理无图谱」根因与修复）、
**D-18**（**WP6 门禁评估：证据不足** —— §6.3 的 (a)/(b)/(c) 探针从未执行、(b) 的域契约本仓 0 命中、A6/A7 脚手架不存在；**订正**：「f16 旋转入口」**仅在坚持 f16 尾时才需要**，采 `--kv-tail-type bf16` 可免；建议先补探针）、
**D-19**（**§6.3 探针已执行**：**(b) 否证**〔`ROTATED_K_ORIGINAL_V` 只对 `>16` query token 生效、decode 恒旋转、V 存储恒旋转；且坐标系本身即障碍〕、
**(a) 为忠实路线**、**(c) 弱且语义不符**、**新提有界 (c′)**；**上游合并契约** = FP32 `float2(row max,denom)` + 无 split 轴 + 旋转域合并后一次反旋 + 尾行写原始读时旋转 ⇒
**§6.2 条 1 改判「本仓自身偏离」、条 2 改判「可规避」**；并给出**用户三项终态目标 → A1–A8 的映射与缺口**：prefill 腿已删（无判据）、**视觉无判据但结构上不受影响**（走独立算子、不用分页 KV ⇒ 归 A5 显存预算）、
「智力不劣化」应拆为「关闭时逐字节不变 + 开启时差异仅归因量化」、`k4v4` vs `rk4v4` **已实测 2.09×** 但为 tail=0、**A4 的 ≥1.13× 在 KVarN 上可能只在 N>384 可达**；
**⑧/⑨ = 路线裁决「(a)」+ 其量化**（显存差 **12 MiB @C=1 / 104 MiB @C=8**、「增量性能代价可忽略」、改动 **≈900–1,400 行新码 + ≈300 行改、12–15 文件、10–15 天**；**(c′) 否决**）；计划书 v20 另把 **WP6 拆为 WP6.0–WP6.7**、并**入队两项前置测量**）；
以及 §1 A1–A8 现行判据、§7 各 WP 现状与验收、§9 风险登记、§10 D1–D6。
