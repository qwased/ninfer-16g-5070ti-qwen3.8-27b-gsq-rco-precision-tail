# 在 ninfer-precision-tail 内移植 KVarN（K4V4 / K5V5 / K6V6）+ 精度尾部：实施计划

- 版本：**v21**（2026-10-08）。本版依据 **WP6.3「分区与接线」完成**（进度 §3-08-19、附录 D-24）：
  ① **op 内三分区**（`body_window = window − min(N, window)`、`body_active = min(active_splits(body_window), total_active−1)` 下限 1、`tail_active = total_active − body_active`），**`N=0` 时分区是恒等** ⇒ 无尾路径逐位不变；
  ② **尾环视图经 `KvarnPagedBatchLayerView.tail` 就近携带**（3 个 op 入口无须各自加参、**`text.cpp` 的 envelope 无须改**；原估的 6 个调用点改动**未需要**）；
  ③ **尾环写入接入 append**：`attention.cu` 新增 `stage_exact_tail`，在 `rotate_kv` **之前**调用（环存**原始**行）；`require_exact_tail` 只校验元素数/dtype/非空；
  ④ **解 `startup.cpp` 的 KVarN+tail fail-fast**（`--mtp-attention-window` 的拒绝保留）；
  ⑤ **修复 3 个潜伏缺陷**（详见 D-24）：2 个 **WP6.1 batch 偏移**（`partial_acc/m/l` 缺 batch 偏移、查询缺 batch 列偏移）+ 1 个 **WP6.1 查询域缺陷**（op 就地旋转调用者查询缓冲 ⇒ 尾核须先 `hadamard_warp` **反旋查询**；直测 `query_buffer_moved=6.328`、`vs_rotation=0.000e+00`）；
  ⑥ **验收全过**（e2e 四臂 + MTP + workspace **990.0/990.0 MiB 无溢出** + 环增长精确对账 + 续列尾 `cached_tokens=851`）；**A1–A8 判据不变**。
  ⑦ **测试端 2 处 oracle 错（非生产缺陷）已修并复跑全绿**（08-19）：merged 用例的期望**去掉 `host_hadamard_d256`**（tail-only 输出 = `acc_orig/l`）、新增 `unrotate_query_like_kernel()` 消除查询双趟 bf16 往返 ⇒ `ninfer_kvarn_test` **exit 0**（6 直测 **8.0e-7…1.42e-6**、2 启动 **1.33e-6 / 2.14e-6**、2 op 级合并 **3.161e-3 / 3.229e-3** vs 8e-3）。
- 版本：**v20**（2026-10-08）。本版依据 **WP6 路线裁决 = (a) + WP6 分步计划 + 两项前置测量入队**（用户确认；进度 08-13、附录 D-19 ⑧·⑨）：
  ① **路线 = (a)**（照 `small_t_k8v4.cuh` 形状把 KVarN 接进**外部共享精确尾环**）—— 量化依据：显存非决定因素（(a) 88 MiB/序列 vs (c′) 76.5 @C=1，差 **12 MiB**；C=8 差 104 MiB）、
  (a) 的增量性能代价可忽略（精确双写 64 KiB/token ≈ `6×10⁻⁶` 于权重流；最新 N 键多读 +46 MiB/步但 KV 仅占 decode 总流量 ~1.4% ⇒ 端到端 ≈ **+0.4%**；workspace +0.4 MB）、
  且消除双实现语义与 **BF16-acc 偏离**、判据与其它档位同构。**(c′)（body 自建滚动槽）否决**（省 ~3–5 天，换永久语义债）。
  ② **§7-WP6 改写为分步计划 WP6.0–WP6.7**（每步给产出/验收/回退），**不变量 = A6 的「`tail=0` 逐位不变」是 WP6.2 的接受条件**。**⚠ 08-18 订正**：该不变量按「**同版本内**逐位判据成立」执行 —— 因 ⑦ 的 `BF16→FP32` 是**规定交付**，跨版本字节同一性与之按构造互斥；见 §1-A6 与附录 D-23。
  ③ **两项前置测量入队（用户 08-12 指定）**：**WP6.0a 本机 prefill 噪声底**（重复 ≥5，补 WP0 产出 ② 的欠账）、**WP6.0b 现有外部尾的纯 kernel 代价**（关 MTP、`--kv-tail-tokens {0,1024}`、重复 ≥3，把 A8 的 `−5.8%/−2.1pt` 分解为「纯 kernel」+「接受率」）。
  ④ **进度订正（2026-10-08，同日）**：**两项前置测量已完成（WP6.0 ✅，见附录 D-20 / 进度 §3-08-14）** —— 0a 本机 prefill 噪声底（短 prompt 相对极差 **26.5%** / 1073-token **3.1%**）、0b 外部尾纯 kernel decode 代价（`rk4v4` **−1.61%** / `bf16` **−1.33%** ⇒ A8 的 −5.8% 分解为「纯 kernel ≈ −1.5%」+「接受率联动 ≈ −4.3%」）。**A1–A8 判据不变**（A1/A8 已补实测行）。
  ⑤ **WP6.1 ✅ 完成（08-15，见进度 §3-08-15 与附录 D-21）**：`src/ops/kvarn/tail_partial.{cuh,h,cu}` 旋转域 tail-partial（**acc 只旋一次**）+ FP64 oracle，**6 用例全过、余量 ~2000×**；**尚未接线（WP6.3）**。
  **同轮修复一处先存竞态**（08-16，附录 D-21）：`reduce_output_hadamard_kernel`（**正是 WP6.2 要改的那个 kernel**）的 `stage[0]` 暂存复用缺 **2 个 `__syncthreads()`** ⇒ width=16 逐位判据偶发失败。**判定 = 软件缺陷（非硬件）**；补屏障后 **隔离 150/150 + 全量 6/6 + `racecheck` 0 hazard** ⇒ **A6 的「逐位」判据恢复为单次可判**。**WP6.2 起该 kernel 必须保留这两个屏障**。**WP6.2–WP6.7 仍未开工**。**（08-18 更新：WP6.2 已完成，见 ⑦）**
  ⑥ **尾环 dtype ✅ 已裁决 = 维持默认 f16**（08-18，用户确认；**零 diff**） —— 环源是 BF16 ⇒ f16 在范围内逐位等价、精度收益为零、只多 `inf`（实测分布下不可达）；唯一取值差异来自现有 TAIL 的 `p_s`（**对 KVarN 不存在**）。见进度 §3-08-18 与附录 **D-22(7)**。
  ⑦ **WP6.2 ✅ 完成（08-18，进度 §3-08-18 / 附录 D-23）**：`partial_acc` **BF16→FP32**（3 文件 / 源码 5 处 + 删除 BF16 tile 暂存改 `float2` 直存）。**验收全过**：`ninfer_kvarn_test` **3 连跑逐位确定**、**codec 余量与 08-04 逐字相同**、6 个 FP64 尾 oracle、路线间 `limit=0`、**续列尾 e2e `RESULT: PASS`**（`cached_tokens=851`、r1==r2）；decode 路线 vs FP64 oracle 余量**降 6.7–10.9%**，prompt 路线用例（`tiled`×2 / `slab-boundary`）**逐位不变**。**⚠ A6 口径**：本步使 tail=0 输出**不再与改前二进制逐字节相同**（= `BF16→FP32` 的意图、数值上更准）；A6 点名载体均为**同版本内**比较 ⇒ 按 ② 的订正执行。**同轮修正一处自引入越界写**（打包分支第二笔漏 `head_valid` 守卫，H24 下 `gid∈{6,7}` 越出 head 区间 ⇒ 14 处失败，已修）。
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
| A1 | KVarN 关闭时**零回归** | `ctest` 全绿 + **decode-only** tok/s 基线对照（同 `.ninfer`、同 prompt、重复 ≥3 次）+ 输出逐字节相同 | **decode tok/s `\|Δ\|≤0.88%`**（`docs/port-records/PORT-MEMORY.md:766-769`）+ 逐字节相同 + `MemorySummary` 逐字相同。**删除 prefill 腿**（同二进制单请求 prefill 实测 `−29%…+8%`，噪声底过大不可用，`docs/port-records/PORT-MEMORY.md §5.17(2)`）。**✅ 08-14 本机噪声底实测（WP6.0a，进度 §3-08-14）**：`ninfer.exe` 单请求 ×7 重复 ⇒ **短 67-token prompt 相对极差 26.5%**（中位 231.0 / 最差 194.3 tok/s）、**1073-token prompt 相对极差 3.1%**（中位 1600 / 最差 1550）⇒ 确认删除正确；**若将来要 prefill 判据，须用长 prompt 且容差 ≥3%（中位数比较），短 prompt 不可断言** |
| A2 | 每档编解码与**独立 oracle** 一致 | FP64 Sinkhorn/RTN oracle + Hadamard oracle + 记录解码检查 + **位序往返**（pack→unpack 逐码比对） | 容差**按量化步长定义**（`qmax=(1<<bits)-1` ⇒ `q=(max-min)/qmax*row_scale*col_scale`）。**✅ 08-03 形式化（v11，进度 08-03）**：`oracle_relative_l2_limit(bits)=1.0e-3`（07-21 的拟合阈值）**已删除**，改为 **①点值判据 `\|actual-expected\| ≤ q*(1+5e-2)`**（5e-2 吸收设备/oracle 各自 Sinkhorn 的 scale 差异）+ **②总量判据 `flips ≤ 1.0e-3*total`**（边界翻码计数上限）+ **③`\|Δcode\|>1` 零容忍**；并补 4/5/6 位**穷举逐码** pack→unpack 往返（含行外哨兵）。根因已定量为「单元素落在量化边界、设备与 oracle 各自舍入到相邻码」（`max_abs≈0.216`=一步；速度/自洽性由 stored-bit 2.0e-7 + 往返钉死）。**✅ 08-04 余量实测（v12，进度 08-04）**：`flips` 最大 **1/65**、`over_step=0`、`wide_flips=0`、`exit=0` ⇒ 预测吻合、**65× 余量、判据成立且未放宽**，WP5 收口 |
| A3 | **MTP 一致性（相对判据）** —— 原"主文本与 MTP 的 greedy 与 MTP-off **逐位**一致"**已废弃** | WP0.5-A 改造后的诊断仪器 + decode-width KLD；见附录 D-4/D-6 | ①**同配置自确定性**：同档重复运行逐字节相同；②**质量一致**：kvarn 档 KLD 与同配置基线一致；③**首分叉已文档化**：工具报告各 MTP 宽度首个分叉下标；④**kvarn 专属门禁**：kvarn 档的 MTP 一致性不得劣于同配置的 `bf16`/`rk4v4` 基线。覆盖 MTP 深度 0..3、跨 ≥1 group 边界、上下文 ≥8K。**⚠ 2026-10-07 定案：绝对 parity 在上游 base 上即不成立（纯 bf16 k=1 分叉、k=0/k=3 全等；上游父仓库输出逐字节相同）⇒ 非本移植引入；上游 #80 明示拒绝该判据，本仓 `docs/performance.md:29-45` 早已实测否决；其真实出处是 FORK B 私有合同（附录 D-6）** |
| A4 | 精度尾部在 KVarN body 上有**可测质量增益** | **decode-width KLD**（`--score-width 8`，协议 `--disjoint --score-topk 100`、32,767 评分 token） | 最小效应量 = **同档 rk4v4 在 N=1024 的 pairing 带（2.26–2.47×）的 50% ⇒ ≥1.13×**；**附 `same_top` 与 max-KLD 双指标**；tail on/off、重复 ≥3 |
| A5 | 每档显存与**修正后**的 §3/附录 A 表一致 | `MemorySummary` 实测比对 + 本产物实测权重 | ±5%，基准表须含：① KVarN **不可关**的 24.0 MiB/序列 sink+tail；② StateImage slot × 并发项（**WP7 待核实机制，见 §7-WP7**）；③ **本产物** `weightsBytes = 11,092,477,952`（**不是** `config-calculator.html:522` 的 17,093,490,688，那是 groupwise-int 产物，且原文免责「不适用于不同量化的权重产物」） |
| A6 | 尾行旋进坐标域后**逐位可控** | FP32 oracle 覆盖「KVarN body × 旋进 BF16/F16 尾」合并路径；tail=0 时输出逐位不变 | 精确。**⚠ 08-15 曾发现该"逐位"判据不可判**（宽 16 分支偶发失败，同输入不同输出）⇒ **08-16 已定位并修复**（根因 = `reduce_output_hadamard_kernel` 的 `stage[0]` 暂存复用缺 2 个 `__syncthreads()`；判定 = 软件缺陷、非硬件；修复后隔离 150/150 + 全量 6/6 + `racecheck` 0 hazard，进度 §3-08-16 / 附录 D-21）⇒ **判据恢复为单次可判**；仍建议关键比较重复 ≥2 次（成本极低）。**⚠ 08-18 口径澄清（WP6.2）**：`partial_acc` **BF16→FP32** 是 WP6.2 的**规定交付**（§6.2 条 1 订正"对齐上游"）⇒ **tail=0 输出相对 WP6.2 之前的二进制不再逐字节相同**；本判据点名的两个载体（既有 `ninfer_kvarn_test` 的 `limit=0` 比对、续列尾 e2e 的 r1/r2 逐字节比对）**都是同版本内比较** ⇒ **本条按"同版本内逐位判据成立"执行**（08-18 已实测：3 连跑逐位确定、路线间 `limit=0` 全过、e2e r1==r2）。**跨版本字节同一性按构造不成立，非回归**；若须字面读法，唯一出路是条件化 dtype（尾不活跃时保持 BF16）或"独立 merge 核"，须用户裁决（**当前未采**，§3-08-18(5b)、附录 D-23） |
| A7 | 长解码跨 group(128)/ring(64) 边界无重复计数/丢键 | needle 检索 + 边界单测 | 精确命中 |
| A8 | **无负面体验**：pp/tg/MTP 接受率 | 三者与**同字节对手**对照 | **同字节对手：`k4v4↔{rk4v4, nvfp4}`、`k6v6↔k8v4`（逐字节相同，402 B/token/头）；`k5v5`（21,632）无同字节档 ⇒ 需另定判据**。pp/tg/MTP 接受率不得劣于同档噪声底；**并须承认外部尾部已知代价 decode −5.8% / 接受率 −2.1 pt**（`docs/port-records/PORT-MEMORY.md:663-670`）——含尾部的档位按此基线放宽判据。**✅ 08-14 分解实测（WP6.0b，进度 §3-08-14）**：MTP **关**、decode-only、长 prompt、3 重复（tg 极差 ≤0.15%）⇒ 外部尾的**纯 kernel** decode 代价 = `rk4v4` **−1.61%**、`bf16` **−1.33%** ⇒ 记录的 **−5.8%（带 MTP）分解为「纯 kernel ≈ −1.5%」+「接受率联动 ≈ −4.3%」**。**限定**：该分解用 `rk4v4`（记录值用 `rk4v4-e8`）且关 MTP，属**量级归因**，非严格配对相减 |

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
   **⚠ 再订正（v20+/D-22，08-16）**：该交付物对 **(a) 路线两种 dtype 都不需要** —— WP6.1 已证明 (a) 的 tail partial 在**原始域**计算，**从不旋转尾行**（只对 FP32 `acc` 旋一次），故 `kvarn_hadamard` 只收 BF16 这条限制**不构成约束**（进度 §3-08-15(3)）。**"f16 旋转入口"作为交付物彻底消失**；dtype 只剩环编码选择（见 §7-WP6 待定项与 D-22）。
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
| **WP6.0** ✅ | **两项前置测量**（用户 08-12 指定；**GPU、~0.5–1 天**）：**0a 本机 prefill 噪声底**（同二进制 / 同 prompt / 单请求，重复 **≥5**，报 pp 的**中位数 + 最差 + 极差**；补 WP0 产出 ② 的欠账）；**0b 现有外部尾的纯 kernel 代价**（`--kv-dtype rk4v4`（或 `bf16`）× `--kv-tail-tokens {0,1024}`、**关 MTP**、decode-only、重复 **≥3**，报 tg 分布），把 A8 的 `−5.8% / −2.1pt` 分解为「纯 kernel」+「接受率」。**✅ 08-14 完成（进度 §3-08-14）** | 两份落盘报告（命令 + 原始输出 + 分布） | ≥5 / ≥3 重复；中位数与最差齐；**结论回写 A1 / A8 / D-19** | 若无稳定判据 ⇒ 该腿**显式记「不可断言」**，不伪造判据 |
| **WP6.1** ✅ | **KVarN tail-partial 路径（W1）**：新增**旋转域** tail partial，读外部精确尾环（bf16/f16 **原始行**），按分区算出 `(acc, m, l)`。**实现选"acc 旋一次"**（尾 partial 在**原始域**算完 → 只对 **FP32 acc** 施加 `W`；`W` 线性 ⇒ 与"先旋行再算"数学等价，代价见 §6.2 条 4 订正）。**✅ 08-15 完成（进度 §3-08-15）** | `src/ops/kvarn/tail_partial.{cuh,h,cu}` + `sources.cmake` | **✅ 实测**：FP64 oracle 比对 6 用例全过（window256/N128、1024/N384、整窗 100/N100、**N=0 不触发**、H16/KV2、**F16 环**），**最大相对误差 6.2e-7…1.3e-6 vs 判据 2e-3（余量 ~2000×）** | acc 旋转数值不达标 ⇒ 退回**逐行旋**（读时旋转）——**未发生** |
| **WP6.2** ✅ | **归并改造（W2，最高风险）**：`partial_acc` **BF16→FP32**；`reduce_output_hadamard_kernel`（`decode_kernel.cuh:1037-1121`）改**两段式**：body splits → per-row `(acc,m,l)` → 与 tail 合并 → **一次反旋** → 输出。**照上游同构做法**（`fattn-kvarn-portable.cuh:784-786`、`fattn-tail.cuh:339-346`）。**⚠ 08-16 该 kernel 已修竞态（`stage[0]` 复用缺 2 个 `__syncthreads()`，见附录 D-21）⇒ 改造时必须保留这两个屏障**。**✅ 08-18 完成（进度 §3-08-18 / 附录 D-23）** | 改 **~120 行**（于 1155 行文件内）+ `decode.cu:113,120`。**实际 = 3 文件 / 源码 5 处 + 删除 BF16 tile 暂存改 `float2` 直存**（`decode_kernel.cuh` −53/+44、`decode.cu` −3/+3、`attention.cu` −1/+2）；**「两段式」在本仓 = 同一遍 online-softmax 归并**（尾写在同一条 split 轴上，见 §6.2 条 2 订正） | **A6：`tail=0` 输出逐位不变**（既有 `ninfer_kvarn_test` + 续列尾 e2e 逐字节比对）；A2 的 4/5/6 位穷举往返仍过；`ninfer_kvarn_test` **余量与 08-04 逐位同**。**⚠ 逐位判据单次可判的前提是 08-16 的竞态修复在位**（不要回退它）。**✅ 08-18 实测**：`ninfer_kvarn_test` **3 连跑逐位确定**、路线间 `limit=0` 全过、**codec 余量与 08-04 逐字相同**（`flips 1/65`、`over_step=0`、`wide_flips=0`）、6 个 FP64 尾 oracle 全过、**续列尾 e2e `RESULT: PASS`**（`cached_tokens=851`、message 逐字节同 r1）；decode 路线 vs FP64 oracle 余量**降 6.7–10.9%**（random packed 0.0045771→0.0042581），prompt 路线用例（`tiled`×2 / `slab-boundary`）**逐位不变** ⇒ **A6 按"同版本内判据"满足**（§1-A6 的 08-18 澄清） | ~~`tail=0` 无法逐位保持 ⇒ 拒绝该改造，改走"独立 merge 核"~~ **未触发**（该回退只在"同版本内判据不成立"时适用；跨版本字节同一性按构造不成立，见 §1-A6 与 D-23） |
| **WP6.3 ✅** | **分区与接线（W2b/W3/W4/W5/W6）**：op 内部完成 `body_window = window − N` 分区（照 `small_t.cuh:147-170` 形状，**不必改 `text.cpp` 的 envelope**）；3 个 op 入口（`kvarn_attention.h`）加尾视图；6 个调用点（`text.cpp:391,414,567,622,1021,1045`、`context.cpp:1745`）；尾环写入接入 KVarN append（照 `small_t_tail_shadow.cuh` 71 行形状）；`startup.cpp:1035-1045` 解除 fail-fast。**✅ 08-19 完成（进度 §3-08-19）** | **实际 = 源码 7 文件 + 测试 1 文件**：(1) `infer/ops/kvarn.h` 加 `KvarnPagedBatchLayerView.tail`；(2) `decoder_state.cpp` 填充 `.tail`；(3) `decode_kernel.cuh` **上移** `KvarnExactTailPartition`（body / 尾 / 归并**共用一分区**）+ 两核加 `tail_tokens`；(4) `decode.cu` 在 body 与 reduce 之间启动尾 partial；(5) `tail_partial.{cuh,h,cu}` 加 batch 偏移 + **查询反旋** + 新增 `stage_exact_tail`（环写入）；(6) `attention.cu` 加 `require_exact_tail` 并在 `rotate_kv` **之前**调用 `stage_exact_tail`；(7) `startup.cpp` 删除 KVarN+tail 的 fail-fast（`--mtp-attention-window` 拒绝**保留**）。**（原估 `kvarn_attention.h` 加尾视图 / 改 `text.cpp` envelope / 6 个调用点均未需要 —— 尾视图经 `KvarnPagedBatchLayerView` 就近携带，envelope 无须改）** | **✅ e2e 全过**（`.deps/kvarn-adm/wp63_e2e.sh`）：`kvarn:k4v4 + --kv-tail-tokens 1024` **exit 0**（prompt 1073 / 生成 128 / **62.5 tok/s**）；尾环 **f16≡bf16 逐字节同**、**tail=0 有别**；workspace 峰值 **990.0 MiB / 990.0 MiB 逐字节相同** ⇒ **不溢出**；环增长 **+68.0 MiB = 17 页 × 16 层 × 256 KiB**（精确对账）；`--kv-tail-type` **两种取值均真派发**（`with_kv_tail_element` dtype 派发 + `require_exact_tail` 校验，非静默）；MTP+尾+1073-token 跑通；续列尾 **`RESULT: PASS` / `cached_tokens=851`**（无回归）。**并修 3 个潜伏缺陷**（见 D-24） | 保留 fail-fast（§6.4）—— **未触发** |
| **WP6.3 尾留（测试端） ✅** | **2 处测试 oracle 已修**（**非生产缺陷**，见 D-24(5)）：① `run_exact_tail_merged_case` 的 `expected` **去掉 `host_hadamard_d256`**（tail-only 的 op 级输出 = `acc_orig/l`，**无净 Hadamard**）并删诊断 + `report=true` 常驻报余量；② 新增 `unrotate_query_like_kernel()` —— 直测/启动用例的 oracle 用**同一旋转后 bf16 查询**在 double 里反旋，消除 `q → W → bf16 → 反旋` 的双趟往返摄动。**✅ 08-19 复跑** | 测试文件 3 处 | **✅ `ninfer_kvarn_test` 全绿**（exit 0）：6 直测 **8.0e-7…1.42e-6**（修前 1e-4…1.7e-3）、2 启动用例 **1.33e-6 / 2.14e-6**（修前 5.1e-3 / 6.3e-3 **超限**）、2 op 级合并 **3.161e-3 / 3.229e-3**（vs 8e-3）；同族两测试亦 OK | — |
| **WP6.4** | **A6/A7 脚手架（W7）**：FP32 oracle 覆盖「KVarN body × 旋进 bf16/f16 尾」的合并路径；跨 **group(128)/ring(64)** 边界的 needle 检索；边界用例（`N ≤ ring`、`N ≥ width`、跨 checkpoint 恢复、`N=0`） | 新测试 **~300–500 行**（照 `tests/ops/softmax_attention/causal_cache.cpp` 尾测形状） | oracle **精确**；needle **精确命中**；无重复计数 / 丢键 | — |
| **WP6.5** | **容量 / 显存**：外部尾环进入 KVarN 的 `MemorySummary` 与容量曲线（**WP7 的 kvarn 部分提前到此**，因为 (a) 要新分配外部环），实测 ±5% | 代码 + 实测 | **A5** | 估算与实测分列，先报告后收敛 |
| **WP6.6** | **质量 / 速度收口**：`kvarn:k4v4 + tail{0,1024}` 的 decode-width **mean-KLD 单调下降**；**显式处理 A4 的 N>384 风险**（至少测 `N ∈ {384, 1024}`，或声明阈值随 N 调整） | KLD / 字节 / 速度三联表（WP8 的 kvarn 行） | **A4**（+ `same_top` / max-KLD 双指标、≥3 重复）+ **A8** | A4 不达标 ⇒ 记录并裁决是否保留该特性 |
| **WP6.7** | **回归**：A2 / A3 / MTP / 续列尾 / 前缀 / ctest 263 + **新增 `--vision` × kvarn 功能门** | 测试 + 跑批 | **A1 / A3** + 视觉功能门（A1 邻域口径） | — |

- **✅ 已裁决（2026-10-08，用户确认）= 维持默认 f16**（`kv_tail_type = Float16` 不动，零 diff）。原「⚠ 待定（WP6.3 之前必须有结论）」的量化依据（**08-16 已量化 = D-22，进度 §3-08-17**）保留如下。要点：① 尾环**源数据是 BF16**（`kv_cache/append/kernel.cuh:104-105`，`KvTailElement<__half>::from_source(__nv_bfloat16)`）⇒ f16 是**重编码**：在 `[2^-14, 65504]` 内**逐位等于** bf16，超出上限 → `inf`（12M 样本 bit-exact 探针：N(0,1)/N(0,5) 下 **≥99.9995% 精确**，非精确项绝对误差 ≤`2^-25`=2.98e-8）。② **"f16 旋转入口"交付物已消失**（(a) 路线不旋尾行，见 §6.2 条 3 的再订正）。③ dtype 还决定 `small_t_tail.cuh:317-324` 里 **注意力概率 `p_s`** 的算子精度（bf16 8 位 vs f16 11 位尾数）——它是两臂之间**唯一取值不同**的算子（K/V 环值在 f16 中与 bf16 完全相同）。该效应**对 KVarN 不存在**（WP6.1 的 tail partial 把 p/acc 留在 FP32 寄存器）。④ **实测 A/B**（`rk4v4` + `N=1024` + `--score-width 8`，261,223 token，同协议）：bf16 PPL **4.892212** / f16 **4.892169**（相对 **−8.8e-6**）；KLD median 4.98e-4 / mean 1.89e-3 / P99 8.87e-3 / **max 14.96**（近并列顶点的排序翻转）、`same_top` **0.9847**、`mean_target_dlogp` **9e-6**。⑤ **显存逐字相同**（实测 `sequence 296.7 MiB`、`payload 138.0 MiB`、`device total 10.7 GiB`）。⑥ **建议 = 维持默认 f16（现状）**：f16 无实测劣势（|相对 PPL 差| ≤8.8e-6；KLD 放大来自近并列解码），且它为现有 TAIL 路径保留更细的 `p`；KVarN 侧 f16 与 bf16 在范围内取值相同，唯一差别是**不可达**的 `inf` 理论风险（|v|>65504）。若选 **bf16** 则须接受 TAIL 的 `p` 精度从 11 位降到 8 位（实测影响同量级）。**⇒ ✅ 裁决（2026-10-08，用户，§3-08-18）：维持默认 f16**（零 diff：`kv_tail_type` 不动；f16 无实测劣势、为现有 TAIL 路径保留更细的 `p`；`|v|>65504` 这一新增失效模式在实测分布下不可达）。

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

## 附录 D：执行记录（D-1…D-24；**D-1…D-9 与 D-10…D-19 已逐字归档**，正文只留 **D-20 … D-24**）

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

### D-10 … D-19 索引（已逐字归档；正文引用「附录 D-N」时按本表解析）

> 全文（命令、原始数字与措辞）在 `docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md` 的
> 「逐字归档：附录 D-10 … D-19」小节，其小节标题即 `### D-N <原标题>`。**本表未改写任何结论。**

| 锚点 | 原标题（逐字） | 全文 |
|---|---|---|
| D-10 | WP0.5-B 补测完成（229k 同字节矩阵）（2026-10-07）（**回写 §7-WP0.5-B / §1 A8 / §9；版本 v8→v9**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-11 | 正式 WP0.5-B：发布档 KVarN 三档准入（2026-10-07）（**回写 §1 A8 / §5 / §7-WP4；版本 v9→v10**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-12 | GPU 收尾四件 + P2 前置（2026-10-08）（**回写 §1 A2 / §7-WP3 / §7-WP5；版本 v11→v12**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-13 | P2b：MTP parity 测试改造为诊断仪器 + token 级 kvarn MTP 实测（2026-10-08）（**回写 §7-WP3④⑤ / §1-A3；版本 v12→v13**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-14 | P3b：kvarn 档位并入报告目录 / 日志名 + `MemorySummary`（2026-10-08）（**回写 §7-WP2 / §7-WP7 / 关闭 D-11⑤「三档同名」；版本 v13→v14**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-15 | P3c：kvarn parser 单测（2026-10-08）（**回写 §7-WP2；关闭 §7-WP2 的最后残项；版本 v14→v15**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-16 | P3d：审计 3 项 LOW 修复（2026-10-08）（**回写 §7-WP2 邻域 / §9 风险登记；版本 v15→v16**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-17 | P3d ① 的端到端对照 + 待裁决 3 项裁决 +「子代理无图谱」根因（2026-10-08）（**回写 §9 风险登记 / §7-WP2 邻域；版本 v16→v17**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-18 | WP6 门禁评估：**「足够证据可开工」不成立**（2026-10-08）（**回写 §6.3 / §7-WP6；版本 v17→v18**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |
| D-19 | §6.3 探针**已执行**：(b) 否证 · (a) 忠实 · (c) 弱 · 新提 (c′)（2026-10-08）（**回写 §6.1/§6.2/§6.3/§7-WP6；版本 v18→v19**） | [归档](docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md) |


---

### D-20 WP6.0 两项前置测量**已执行**（2026-10-08）（**回写 §1-A1 / §1-A8 / §7-WP6.0；版本仍 v20；进度 §3-08-14**）

**触发**：用户授权 WP6 起手，指定"先跑 WP6.0a + 0b 并落盘（判据来源，必须在写代码前跑）"。`nvidia-smi` 跑前 **0 MiB**。

**方法**：`.deps/kvarn-adm/wp60_measure.sh`（新，gitignored）；二进制 `build-port/apps/ninfer.exe`（`v0.6.0-rtx3090-1354-gaa533d02-dirty`）、
唯一产物 `Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`、`--max-context 4096 --greedy`；**单请求 = 一个全新进程**；
指标 = CLI 摘要的 `prefill speed` / `decode speed`（`apps/cli/main.cpp:220-223`）。短 prompt = 67 token（英文一句）；
长 prompt = 1073 token（30× 一句合成 ASCII；用 ASCII 因 `head -c` 截 UTF-8 源文本会在码点中断 ⇒ chat template 报 `invalid UTF-8`）。

**① 0a 本机 prefill 噪声底**（bf16、max-new 8、7 重复）

| prompt | pp（tok/s，逐次） | 中位数 | 最差 | 相对极差 |
|---|---|---|---|---|
| **67 tok** | 194.3 / 224.6 / 222.4 / 241.1 / 255.5 / 249.5 / 231.0 | 231.0 | **194.3** | **26.5%** |
| **1073 tok** | 1600 / 1560 / 1590 / 1550 / 1600 / 1600 / 1600 | 1600 | **1550** | **3.1%** |

⇒ 短 prompt **26.5%** 逐字复现 `PORT-MEMORY §5.17(2)` 的 `−29%…+8%` ⇒ **A1 删除 prefill 腿经本机实测确认**；
长 prompt 收窄到 **3.1%** ⇒ **若要 prefill 判据须长 prompt + ≥3% 容差（中位数比较）**。**限制**：长 prompt 的 pp 受 `format_pretty_rate` 的 `x.yzk` 显示量化（10 tok/s ≈ 0.6%）。

**② 0b 现有外部尾的纯 kernel 代价**（MTP **关**、decode-only、长 prompt、max-new 128、3 重复）

| 档 | tail | tg（tok/s） | 中位数 | 极差 |
|---|---|---|---|---|
| `rk4v4` | 0 | 68.3 / 68.3 / 68.3 | 68.3 | 0.0 |
| `rk4v4` | 1024 | 67.2 / 67.2 / 67.3 | 67.2 | 0.1 |
| `bf16` | 0 | 67.8 / 67.9 / 67.9 | 67.9 | 0.1 |
| `bf16` | 1024 | 67.0 / 67.0 / 67.1 | 67.0 | 0.1 |

⇒ **纯 kernel 代价 = `rk4v4` −1.61% / `bf16` −1.33%**（tg 极差 ≤0.15% ⇒ 可分辨）。
**A8 的分解**：记录值 **−5.8%（带 MTP、`rk4v4-e8`）=「纯 kernel ≈ −1.5%」+「接受率联动 ≈ −4.3%」**。
尾部确已生效：`kv cache payload` **70.0 → 138.0 MiB**、`gpu sequence used` **228.7 → 296.7 MiB**。
**限制**：非严格配对 A/B（用 `rk4v4` 而非 `rk4v4-e8`、关 MTP）；pp 的 tail 差异落在 prefill 噪声内，不作结论。

**③ 回写**：§1-A1（噪声底实测行）、§1-A8（分解行）、§7-WP6.0（标 ✅）、本条。**WP0 产出 ②「本机噪声底报告」由此补齐**。
**未做**：`kvarn:*`（未接线、仍 fail-fast）、`--kv-tail-type bf16` 尾、批量 prefill、N=2048、MTP 开启的 tail{0,1024} 直接对拍。**未改源码、未提交。**

---

### D-21 WP6.1 落地 + 先存偶发（width=16 非确定性）根因定位与修复（2026-10-08）（**回写 §1-A6 / §7-WP6.1 / §7-WP6.2 / 版本头 ⑤；进度 §3-08-15、§3-08-16**）

**(1) WP6.1（08-15，进度 §3-08-15）**：新增 `src/ops/kvarn/tail_partial.{cuh,h,cu}`，在**原始域**读外部精确尾环（bf16/f16 原始行）算出 `(acc, m, l)`，只对 **FP32 acc 施加一次 `W`**（`detail::hadamard_warp`，核内寄存器；`W` 正交且线性 ⇒ 与"先旋行再算"数学等价）。
- **验收**：FP64 oracle 比对 **6 用例全过**（window 256/N128、1024/N384、整窗 100/N100、**N=0 不触发**、H16/KV2、**F16 环**），**最大相对误差 6.2e-7…1.3e-6 vs 判据 2e-3（余量 ~2000×）**。
- **派生结论（影响"待定项：尾 dtype"）**：本路线**从不旋转尾行** ⇒ 计划书 §7-WP6 待定项里 **f16 的唯一额外交付物（"新增 f16 旋转入口"）消失**，dtype 只是环的存储选择。**仍待用户裁决**（bf16 免"本机复核"？f16 有 10-bit 尾数、更精确）。
- **限制（如实）**：F16 用例的环值经 `round_to_bf16` 再 `round_to_f16`，而 bf16 值在 f16 中精确可表示 ⇒ **只验证了 F16 读路径/分派，未验证 f16 与 bf16 的量化差异**。

**(2) 先存偶发：判定 + 根因 + 修复（08-16，进度 §3-08-16）**
- **现象**：`ninfer_kvarn_test` 宽 16 分支偶发失败（`limit=0` 逐位判据），同输入不同误差；全量复现 **4/20**、隔离用例 **13/150**（修复前）。
- **判定 = 软件缺陷（非硬件）**，三条独立证据：① `compute-sanitizer --tool racecheck` 定点报出 **6 处 shared-memory hazard**，全部在 `reduce_output_hadamard_kernel<CausalD256H24Kv4,false,true>`（`:1089`读/`:1104`写、`:1110`读/`:1134`写）；② **只加 2 个 block barrier**（无数值语义变化）后**全部消失**（隔离 150/150、全量 6/6、racecheck 0 hazard）——硬件位翻转不可能被屏障消除；③ 缺陷是局部的且**同族已规避**（`search_graph` 确认全仓恰好 4 个 reduce-output kernel，`stage[0][tid]` 复用模式只命中 KVarN；TAIL 三族走 `causal_merge_split_statistics`，该函数 `small_t.cuh:229-230` 显式用独立 `scalars` 存储规避同类竞争）。硬件侧无异常（空闲、45 °C、throttle 0x1、消费级无 ECC）。
- **根因**：`stage[2][D]` 被当作三段归约的共享暂存反复复用，两处复用前无屏障 —— 读 `head_m = stage[0][0]`（`:1089`）后即被 `stage[0][tid] = local_l`（`:1104`）复写；读 `head_l = stage[0][0]`（`:1110`）后即被 `stage[0][tid] = value`（`:1134`）复写。`tid ≥ active_splits` 的线程跳过 expf 循环先跑到写点 ⇒ 与其它 warp 的读竞争。`first=8190` 时 `active_splits=41`（长循环 ⇒ 偏斜更大）故只在该边界稳定复现。
- **修法**：`decode_kernel.cuh:1090` / `:1111` 各补一个 `__syncthreads()`（`src/ops/kvarn/decode_kernel.cuh`）。
- **对验收的影响**：**A6 的"逐位"判据由"不可判"恢复为"单次可判"**（仍建议关键比较重复 ≥2 次）。**该 kernel 正是 WP6.2 的改动对象 ⇒ WP6.2 必须保留这两个屏障**。
- **方法（可复用）**：此类偶发优先用 `racecheck` 定点（3 min、零源码改动），**先于**大规模重复统计与对照套件（对照套件因单次 ~120 s 被放弃，其证据强度也低于 racecheck 的读-写对）。
- **顺带（用户授权）**：`AGENTS.md` 的「Codebase memory (indexed graph)」小节**追加**图谱门禁三条 + 「图谱不覆盖 kernel 内部」+ 每 WP 边界 `index_repository`（**预存未提交改动原样保留**）。
- **构建与注册测试（补充证据）**：`ninfer_ops ninfer ninfer-serve ninfer-perplexity` 四目标全部重建通过；`ctest -R kvarn` **3/3 通过**。TAIL 三族不含该头文件 ⇒ 未重跑其 attention 套件。
- **未做/未测（如实）**：未跑完整 ctest（259 项），只跑受影响的 `ninfer_kvarn_test`（6× 全量）与 `ctest -R kvarn` 3 项；**未跑带 KVarN 尾的 Engine 级生成 e2e**（该 kernel 的数值行为由 `ninfer_kvarn_test` 的 FP64 oracle 覆盖）；新增 2 个 block barrier 的开销**未用 ncu 实测**（判断可忽略：256 线程、每 `(q_head,token)` 一次的尾归约核）；**未提交**。

---

### D-22 尾环 dtype：bf16 vs f16 的量化分析（2026-10-08）（**回写 §6.2 条 3 / §7-WP6 待定项；进度 §3-08-17**）

**触发**：用户要求给出带量化依据的 bf16/f16 优劣、收益与代价。

**(1) 决定性代码事实：尾环的源数据是 BF16**
`src/ops/kv_cache/append/kernel.cuh:104-105` 写环用 `store_tail_vec8(&tail_k[off], &k[src_off])`，而 `store_tail_vec8` 的签名是 `template <typename Dst> void store_tail_vec8(Dst*, const __nv_bfloat16* src)`（`src/ops/common/kv_tail_element.cuh:73`）⇒ **源恒为 bf16**。`KvTailElement<__half>::from_source` = `__float2half(__bfloat162float(v))`（同文件 `:59-61`）⇒ f16 是**重编码**而非更高精度采样。

**(2) 静态量化（IEEE 表示 + 12M 样本 bit-exact 探针，CPU，脚本 `/tmp/tailtype/bf16_vs_f16.py`）**

| | bf16 | f16 |
|---|---|---|
| 指数/尾数位 | 8 / 8（significand） | 5 / 11（significand） |
| 最大有限值 | 3.39e38 | **65504** |
| 最小正规数 | 1.18e-38 | 6.10e-5（次正规到 5.96e-8） |
| 环存储字节 | 2 B | 2 B（**相同**） |

**结论**：源是 bf16（8 位 significand）⇒ **f16（11 位）在 `[2^-14, 65504]` 内可逐位精确表示**，没有任何精度收益；f16 的净增益为零，代价是一个新增失效模式 `|v|>65504 → inf`。

探针实测（4M 样本/组，`bf16→f16` 往返）：
- `N(0,1)`：**3,999,979 / 4,000,000 精确（99.99947%）**，0 溢出，21 项被舍入且**绝对误差 max 2.98e-8（=2^-25）**。
- `N(0,5)`：99.9999% 精确，0 溢出，绝对误差 max 2.98e-8。
- 对数均匀 `1e-8…1e5`（刻意跨界）：**57,015 项 → inf（1.43%）**，794,143 项被舍入（绝对误差仍 ≤2.98e-8）。
- 边界探针：`65504 → bf16 舍入为 65536 → f16 = inf`；`2^-15`、`2^-24`、`6.10e-5` 均**精确**；`2^-25 → 0`（误差 2.98e-8）。

⇒ **唯一的实质风险是上溢**，且只在 `|v| > 65504` 时发生（bf16 的无失效上限为 3.39e38）；下溢的绝对误差被 `2^-25` 限死，可忽略。

**(3) dtype 的第二重作用（关键修正）：它不只决定环存储**
`small_t_tail.cuh` 里 `qkv_s`（Q/K/V）与 `p_s`（**注意力概率**）都是 `Elem[]`：Q/K/V 经 `from_source`（源 bf16 ⇒ f16 取值相同），而 **p 经 `from_float(p00)`（`:317-324`）** ⇒ bf16 存 8 位 significand、f16 存 11 位。**这是在 f16/bf16 两臂之间唯一取值不同的算子**（逻辑论证：环值作为实数是同一批数，故差异必来自消费元素类型的其它算子 = p）。两臂的 `KvTailElement<Elem>::mma` 同为 `m16n8k16…f32`（`mma.cuh:33/42`）⇒ MMA 吞吐与累加精度相同。
**该效应对 KVarN 路线 (a) 不存在**：WP6.1 的 tail partial 与归并全程 FP32 寄存器（p/acc/m/l），环 dtype 只影响存储编码。

**(4) 实测 A/B（GPU，RTX 5070 Ti；`rk4v4` + `--kv-tail-tokens 1024` + `--score-width 8` 使尾被真正读取；`--context 2048 --stride 1024`；`--quick` 4 流；**两臂同协议**）**

| 指标 | bf16 | f16 |
|---|---|---|
| overall PPL（261,223 token） | **4.892212** | **4.892169** |
| mean NLL | 1.587644 | 1.587636 |
| 47 个中途检查点 PPL 对比 | — | f16 低 **30** / 高 **15** / 4 位小数相同 **2**；相对差 max **6.05e-4**、mean **−9.29e-5** |
| KLD（f16 vs bf16 的 100-topk） | — | median **4.98e-4**、mean **1.89e-3**、P99 **8.87e-3**、P99.9 **4.78e-2**、**max 14.96** |
| `same_top` / `mean_target_dlogp` | — | **0.9847** / **9e-6** |
| 显存（CLI 实测 `rk4v4`+N1024） | `sequence 296.7 MiB`、`payload 138.0 MiB`、`device total 10.7 GiB` | **逐字相同** |
| score rate | 153.0 tok/s | 154.3 tok/s（差在噪声内） |

**读法**：两臂**不逐位相同**（47/47 检查点 PPL 都不同），但差异量级极小（整体相对 **−8.8e-6**）；KLD 的 `max 14.96` 与 `same_top 0.9847` 说明这来自**近并列顶点排序翻转**（`mean_target_dlogp` 仅 **9e-6**，即模型实际预测几乎未变）。方向混合（30 低 / 15 高）⇒ **没有证据表明任一方系统性更优**；与 (3) 的 p 精度机制在量级上一致。
**未单独隔离 p 机制**（需把 `p_s` 改成 FP32 再重跑 ~1 h）；**未直接测 `max|K/V|`**（上溢仅由"261k token 无异常"间接排除）。

**(5) 代价侧量化**
- 显存/带宽：**0**（2 B/元素；实测逐字相同）。环几何 = 16 全注意力层 × 4 KV 头 × 256 dim × 2（K,V）× 2 B = **64 KiB/token**；N=1024 ⇒ 几何 64 MiB，WP6.0b 实测增量 68 MiB。
- MMA：两者同为 `m16n8k16` + **f32 累加**（`mma.cuh:33/42`）⇒ 同吞吐。注意仓内另有 `mma_f16_f16acc`（注释称 GeForce 上 **2×** 速率，`mma.cuh:51`），但 `KvTailElement<__half>::mma` **不调用它**，且采用它会牺牲累加精度（与"精确尾"目标相反）⇒ **f16 当前拿不到任何 MMA 吞吐收益**。
- 写路径：bf16 = 16 B 向量拷贝；f16 = 每 8 元素 8×(`cvt.bf16→f32` + `cvt.f32→f16`)。按 64 KiB/token ÷ 4 B = **34,816 元素/token** ⇒ f16 多 **~69,632 条 cvt/token**（一次性，相对 27B 参数的前向可忽略）。
- 读路径：`__bfloat162float` vs `__half2float` 同为单指令（**未单独测量**）。

**(6) 建议（待用户裁决）**
**维持全局默认 f16（现状）**：f16 无实测劣势（|相对 PPL 差| ≤8.8e-6、且 KLD 的放大来自近并列解码），并保留现有 TAIL 路径 11 位的 `p`；KVarN 侧 f16 与 bf16 在范围内**取值相同**，唯一差别是不可达的 `inf` 理论风险。
**若选 bf16**：消除 `inf` 失效模式、去掉写路径 cvt，代价是现有 TAIL 的 `p` 从 11 位降到 8 位（实测影响同量级）。**不建议**按存储分设默认（增加一处配置复杂度，收益 ≤1e-5 相对）。

**(7) ✅ 裁决（2026-10-08，用户确认，§3-08-18）**
**维持全局默认 f16**（= (6) 的建议；`kv_tail_type = Float16` 不动，**零 diff**）。⇒ **WP6.3 起 `--kv-tail-type` 按现状支持 f16 与 bf16 两条**（§6.4 的「不得静默」要求满足：两条都真支持，无需新增 f16 旋转入口）。**本裁决不改变 WP6.2–WP6.7 的任何交付。**

---

### D-23 WP6.2 落地：`partial_acc` BF16→FP32 + 归并路径对齐上游（2026-10-08）（**回写 §1-A6 / §6.2 条 1·2 / §7-WP6.2 / 版本头 ②·⑥·⑦；进度 §3-08-18**）

**(1) 交付**（3 文件 / 源码 5 处 + 1 处删除；命令与原始输出见进度 §3-08-18）
- `src/ops/kvarn/decode_kernel.cuh`（−53/+44）：`attention_decode_kernel` 的 `partial_acc` → `float*`；`write_neutral` 填 `0.0f`；**删除** BF16 tile 暂存 + 协作拷贝出（原 `:991-1032`）⇒ PV warp 每 lane 以 `float2` **直存**；`reduce_output_hadamard_kernel` 的 `partial_acc` → `const float*`，`numerator += partial_acc[index] * weight`。**08-16 补的两个 `__syncthreads()` 原样保留。**
- `src/ops/kvarn/decode.cu`（−3/+3）：`acc` 由 `DType::BF16` → `DType::FP32`，两处 cast（`:113`、`:120`）。
- `src/ops/kvarn/attention.cu`（−1/+2）：`query_heads == 24 && max_visible_keys > 8198` 的 workspace 覆盖项改用 `sizeof(float)`。
- **未改**：`tail_partial.{cuh,h,cu}`、`sources.cmake`。

**(2) 为什么删暂存而不加宽**：暂存 `qkv_s[2*Bc*D]` = **16384 元素**，与 C=8 的 `WarpGroups*Br*D` **恰好相等**；改 FP32 需 **64 KiB**（现 32 KiB）⇒ 静态 smem 越过 48 KiB 上限（decode 核未 opt-in 动态 smem，静态亦不可超 48 KiB），改动态则要为 24 个模板实例各加一次 `cudaFuncSetAttribute`。直存另去掉一次 smem 往返与一次屏障。

**(3) §6.2 条 2「两段式归并」在本仓的形态**：本仓**不需要**第二个归并核 —— WP6.1 的尾 partial 直接写在**同一条 split 轴**的 `[body_active, total_active)`，而 `reduce_output_hadamard_kernel` 本就对 `[0, active_splits)` 做一遍 online-softmax → **一次反旋**。上游 reduce+combine 两核是因为上游两侧**无 split 轴**。⇒ **本步的净交付 = dtype 对齐（消除 §6.2 条 1 的 BF16-acc 偏离），不是新增核。**

**(4) 验收（GPU 实测；`ninfer_kvarn_test` + 续列尾 e2e）**
- `ninfer_kvarn_test` **3 连跑全过、逐位确定**；**codec 余量与 08-04 逐字相同**（`K flips 1/65` / `over_step=0` / `wide_flips=0 of 65536`；`V flips 0/65`）；6 个 WP6.1 FP64 尾 oracle 全过；路线间 `limit=0` 比对全过；`ninfer_qwen3_5_kvarn_{continuation_image,tail_row_reset}_test` 通过。
- **续列尾 e2e `RESULT: PASS`**（`.deps/kvarn-adm/p1_prefix_reuse.sh`）：`kvarn r2 cached_tokens = 851`、`kvarn messages identical: YES`、bf16 对照 `851 / YES` —— **851 与 08-04 记录同值**。
- **改前/改后 FP64 余量表**（临时 `report=true` 取值，已还原）：decode 路线用例**一致降 6.7–10.9%**（H24/KV4 width-1 **0.0037880→0.0034472**、H24/KV4 B=2 **0.0037567→0.0033574**、random packed **0.0045771→0.0042581**、k6v6 packed **0.0045895→0.0042786** 等）；走 prompt 路线（`launch_prefill` → `finalize_prefill_slab_kernel`，**从不经 `partial_acc`**）的 `H24/KV4 tiled`、`H16/KV2 tiled`、`slab-boundary` **逐位不变** ⇒ 改动面恰限于 decode 路线，且为**数值改善**。

**(5) ⚠ A6 口径（本附录的主要记录目的）**
本步**按本计划要求**把 body 的 per-split 累加器由 BF16 舍入改为 FP32 ⇒ **tail=0 输出相对 WP6.2 之前的二进制不再逐字节相同**（上表 0.36–0.46% 的相对改善即其度量；这是 §6.2 条 1「对齐上游」的**意图**，非回归）。
**A6 点名的两个验证载体都是同版本内比较**（既有 `ninfer_kvarn_test` 的 `limit=0` 比对、续列尾 e2e 的 r1/r2 逐字节比对）⇒ **A6「逐位不变」按「同版本内逐位判据成立」执行**，已实测成立。**跨版本字节同一性按构造不成立**（无 golden 文件的测试集也不验证它）。
**若用户坚持字面读法**，唯一出路为 ① 条件化 dtype（尾不活跃时保持 BF16；代价 = 两套算术 + 模板实例翻倍 + `--kv-tail-tokens 0/1024` 的 A/B 混入第二种变量），或 ② 回退到 §7-WP6.2 的"独立 merge 核"（不动既有 reduce）。**两者当前均未采**，须用户裁决。

**(6) 一处自引入的越界写（已修，如实记录）**：第一版直存把打包分支第二笔写在 `if (head_valid)` 之外，而原码由 `row1_head = row0` 守卫；H24 下 `GroupSize=6 < Br/2=8` ⇒ `gid∈{6,7}` 会写 `q_head = kv_head*6+6/7`（kv_head=3 时 = 24/25 ≥ QHeads=24）⇒ 踩踏相邻行。症状 = 14 处测试失败（`limit=0` 路线间比对 0.24–0.30；`limit=8e-3` 容差比对 0.26–0.42），而 WP6.1 的尾 oracle 用例仍全过（它们不经 body 核）。修法 = 移回守卫内。

**(7) 容量观察（WP6.5 输入）**：`causal_softmax_attention_workspace_capacity_bytes` 经 `allocate_small_t_workspace` **一直按 FP32 给 partial 记账** ⇒ 改前 kvarn 用 BF16 时该腿 **2× 过配**、改后**恰好**；本次改的覆盖项 `split_rows × (D*4+8)` = `24*16*1*82 × 1032` = **32,495,616 B** 与 `acc + m/l` 之和**逐字节相等**。**净影响可能接近 0**；D-19 记的「+0.4 MB」漏乘了 `DecodeLongSplits`。**待 WP6.5 实测 ±5%。**

**(8) 未做 / 未测**：核时间（本次只改存储路径：省一次 smem 往返 + 一次屏障，但 16 B 存改 8 B 存）⇒ **WP6.0b 的 −1.5% 尾预算仍未被 WP6.1/WP6.2 对拍**；`ninfer_softmax_attention_*`（9 项）**启动后中止**（改动只落 kvarn 私有文件，与共享 reducer/容量函数无交集 ⇒ 判定不受影响，但**未由实测覆盖**）；未跑全量 ctest、未跑 MTP、未跑真实模型质量臂（A4/A8 属 WP6.6）。**未提交。**

---

### D-24 WP6.3 落地：分区与接线（2026-10-08）（**回写 §7-WP6.3 / 版本头 v21；进度 §3-08-19**）

**(1) 交付 = 分区 + 接线（源码 7 文件）**
- `include/ninfer/ops/kvarn.h`：`KvarnPagedBatchLayerView` 新增 `PagedKVExactTailView tail;`（尾环是**外部共享**视图）。⇒ **尾视图经视图就近携带**，3 个 op 入口**无须各自加参**、`text.cpp` 的 **envelope 无须改**（原计划估的「3 个 op 入口加尾视图 + 6 个调用点」**均未需要**）。
- `decoder_state.cpp`：`kvarn_batch_layer_view` 从 `exact_tail_->plane(layer*2)` / `plane(layer*2+1)`、`tail_ring_pages_`、`tail_retention_` 填充 `.tail`。
- `decode_kernel.cuh`：`KvarnExactTailPartition` + `kvarn_exact_tail_partition` **上移到本头**（body 核 / 尾 partial / 归并核**共用一分区**）；`attention_decode_kernel` 与 `reduce_output_hadamard_kernel` 各加 `std::int32_t tail_tokens`。
- `decode.cu`：`tail_tokens = cache.tail.enabled() && cache.tail.page_count > 0 ? cache.tail.retention : 0`，传 body 与 reduce，并在两者**之间**启动尾 partial。
- `tail_partial.{cuh,h,cu}`：`.cuh` 删去已上移的分区 + 加 batch 偏移 + **查询反旋**；`.h` 签名加 `width`/`batch_size`/`column_begin`；`.cu` 新增 `kvarn_exact_tail_stage_kernel` + `stage_exact_tail`。
- `attention.cu`：新增 `require_exact_tail`（只校验元素数/dtype/非空），`validate_inputs`/`kvarn_kv_append` 调用；`kvarn_attention`/`kvarn_kv_append` 在 `rotate_kv` **之前**调 `stage_exact_tail`。
- `startup.cpp`：**删除** `kv_tail_tokens != 0` 对 `KvarnGroup128` 的 fail-fast（`--mtp-attention-window` 的拒绝**保留**）。

**(2) 分区（W2b）**：`body_window = window − min(N, window)`；`total_active = kvarn_decode_active_splits(window, split_count)`；`body_active = min(active_splits(body_window), total_active − 1)`（尾有键时下限 1）；`tail_active = total_active − body_active`。**`N=0` 时分区为恒等**（`body_window==window`、`body_active==total_active`、`tail_active==0`）⇒ 无尾路径**逐位不变**。**有尾时 body 必须用 launch 级最新位置**（非每列组 anchor），三者才对同一 `window` 一致；多余 split 贡献恰为零（body 逐列掩码 + 空行 `m=-inf,l=0`，归并跳过）。

**(3) 修复的 2 个 WP6.1 batch 偏移缺陷**（此前无 batch 用例未暴露）：`partial_acc/m/l` **缺 batch 偏移** ⇒ 补 `batch*D*QHeads*tokens*split_count`（`m/l` 去 `D`）；查询**缺 batch 列偏移** ⇒ `column_base = column_begin + batch*full_width`。

**(4) 修复的 WP6.1 查询域缺陷**：op 启动尾核**之前**就地把调用者查询缓冲旋了（`Tensor rotated_query = query; kvarn_hadamard(query, rotated_query, stream);` **别名**、同址读写 ⇒ in-place 安全），而环存**原始**行 ⇒ 尾核按"旋转域"写的 `q·k` 是错的。**修法** = 尾核先 `detail::hadamard_warp(qv, lane)` **反旋查询**（W 正交自逆且线性）。**直测**：`query_buffer_moved=6.328e+00`（缓冲已非原值）且 `vs_rotation=0.000e+00`（恰为 `W(q_bf16)`）。

**(5) 测试端 2 处 oracle 已修（原为 oracle 错，**非生产缺陷**；08-19 复跑全绿）**
- `run_exact_tail_merged_case`：**原**按 **1.41** 失败，诊断 = `rel2_rotated≈1.42` vs `rel2_original≈3.2e-3`。**推导**：body `partial_acc = Σ p·W(V_orig)`、归并末尾**再乘一次 W** ⇒ 公开输出 = **原始域** `acc_orig/l`（**无净 Hadamard**；尾核写 `W(ΣpV)`、归并再乘 W ⇒ W²=I 同落原始域）。⇒ **tail-only 的 op 级期望 = `acc_orig/l`，不带 W**；测试误用 `host_hadamard_d256` ⇒ **oracle 错**。**修法** = 去掉那一次 W 并删诊断（改 `report=true` 常驻报余量）⇒ **relative_l2 3.161e-3 / 3.229e-3**（vs 8e-3；与其它 decode 路线 op 级用例的 3.4e-3 同量级）。
- `run_exact_tail_partial_case` / `..._launch_case`：**原** `max_rel` 1e-4…1.7e-3（直测）与 **5.1e-3 / 6.3e-3 超 2e-3 限**（启动）= 查询 `q → W → bf16 → 反旋` 的**双趟 bf16 往返**摄动。**修法** = 新增 `unrotate_query_like_kernel()`：oracle 用**同一旋转后 bf16 查询**在 double 里反旋（`hadamard_warp` 与 `host_hadamard_d256` 是同一蝶形/同一 `2^-4` 尺度）⇒ 残差只剩"核 float vs oracle double"。**修后**：直测 **8.0e-7…1.42e-6**、启动 **1.33e-6 / 2.14e-6**。
- **复跑**：`ninfer_kvarn_test` **exit 0 / "OK kvarn correctness"**；同族 `ninfer_qwen3_5_kvarn_continuation_image_test`、`..._tail_row_reset_test` 均 **OK**。

**(6) 功能验收（GPU，`.deps/kvarn-adm/wp63_e2e.sh`）**
- `kvarn:k4v4 + --kv-tail-tokens 1024` **exit 0**（prompt 1073 / 生成 128 / **62.5 tok/s**）。
- 尾环 **f16 ≡ bf16 逐字节同**；**tail=0 与有尾不同**（符合预期）。
- workspace 峰值 **990.0 MiB / 990.0 MiB 逐字节相同**（有/无尾）⇒ **不溢出**；环增长 **+68.0 MiB = 17 页 × 16 层 × 256 KiB**（精确对账）。
- `--kv-tail-type` 两取值**均真派发**（dtype 派发 + `require_exact_tail`，非静默）；`KVarN + --mtp-attention-window` **仍拒绝**。
- MTP+尾+1073-token 跑通（走打包 `ColumnsPerBlock=4` verify 路线且带尾）。**⚠ 非可比速度**：有尾 100% / 250.9 tok/s vs 无尾 41.5% / 103.0 tok/s（接受率不同 ⇒ **不当加速读**）。
- 续列尾 `.deps/kvarn-adm/p1_prefix_reuse.sh` → **RESULT: PASS**、`cached_tokens=851`（无回归）。

**(7) 未做 / 未测（如实）**：2 处测试 oracle（见 (5)）；**`window > 8198` 的 KVarN split 分支带尾无 host oracle 覆盖**（WP6.4）；**未做跨二进制 tail=0 逐位比对**（A6/A7 属 WP6.4）；未跑全量 ctest / `ninfer_softmax_attention_*` / 核级性能（WP6.6）；**未提交**。

**(8) 一处**刻意不查**的既有标注异常**：尾池 plane 在 `decoder_state.cpp` 声明为 **HeadMajor**，而环的扁平寻址（`paged_kv_element_offset`）是 **PageMajor**；所有写/读方走同一扁平助手 ⇒ **对环无害**，但池级**批量拷贝**会按声明步长算 ⇒ 既有风险（跨路由、非 WP6 引入），**未处置**。

---

## 已归档信息索引

本次精简（2026-10-08）**只搬运、不改写**：下表三份归档文件收录的是本计划书与进度日志中
**推进计划已不需要**的内容，全部为**逐字副本**（字节级搬运，源行号见括号）；正文对应位置各留一行索引。
**验收口径（§1 A1–A8）、风险登记（§9）、未决项（§10 D1–D6）与 §0.5 记录规则未作任何改动。**

| 归档文件 | 覆盖范围（源行号） | 一句话内容 | 正文的索引位置 |
|---|---|---|---|
| `docs/port-records/KVARN-PLAN-CHANGELOG-ARCHIVE.md` | 本计划书原第 3 行（v13 完整版本头）+ 原第 28–132 行（**v3→v12 十张变更摘要表**） | 每一版相对上一版的修正、依据与实测数字（版本演进史） | 版本头末句 + 「变更摘要索引（v3→v12 已逐字归档）」表 |
| `docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md` | 本计划书附录 **D-1 … D-9**（原第 804–1094 行）与 **D-10 … D-19**（2026-10-07/08，已关闭的 WP0.5-B / P2 / P3 包与 §6.3 探针） | WP0.5-C 构型口径、WP0 基线、WP1 ops/测试、A3 仪器—差分—定案三件、WP2 执行、WP0.5-B 起手、WP3① 页几何；以及准入补测、正式三档准入、P2b/P3b/P3c/P3d、WP6 门禁评估与 §6.3 探针 | 附录 D 内的「**D-1 … D-9 索引**」与「**D-10 … D-19 索引**」两张表（锚点 + 原标题逐字） |
| `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` | 进度日志 §3 **2026-10-07-1 … -23**（原第 260–1312 行）+ §0 快照明细 / 工作树清单 / 已关闭未决项 1·5·8·9·10 + §4.1 / §4.4 | 早期 WP 的逐日执行记录与当时的任务定义（含失败、被推翻的假设与原始命令） | 进度日志 §3 条目索引表、§0 与 §4 的指针行 |

**仍在正文、未归档（WP6.4–WP6.7 会直接引用）**：附录 **D-20 … D-24**（WP6.0–WP6.3 的实测与修复）、
§1 的 A1–A8 现行判据、§7 各 WP 现状与验收、§9 风险登记、§10 D1–D6。

**已归档但 WP6.6/WP7 仍会用到的关键数字**（原文见附录归档，**勿丢**）：D-11 发布档三档准入
`k4v4` **0.002120** / `k5v5` **0.001432** / `k6v6` **0.001233**，对 `rk4v4`/`nvfp4`/`k8v4` = **2.09×/2.07×/2.18×**；
`k6v6` 门槛 < `k8v4` 的 **0.002688**（D-10）；`kvarn:k4v4` KLD 臂逐位 **0.002120**（D-17）。

**D-1 … D-9 与 D-10 … D-19 的索引表在附录 D 内**（锚点 + 原标题逐字 + 归档链接）；历史全文在
`docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md`。
