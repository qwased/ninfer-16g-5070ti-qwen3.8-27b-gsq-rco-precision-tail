# KVARN-PLAN-CHANGELOG-ARCHIVE — KVarN 计划书 v3→v12「变更摘要」与旧版本头归档

- 归档日期：2026-10-08
- 来源：`../../kvarn-port-into-precision-tail-plan.md`（版本 v13，归档前的提交状态 `6aea191b`）
- 内容：**逐字搬运**的两块——① v13 时点的完整版本头（原来压缩成一行前的原文）；
  ② 「v3 相对 v2」…「v12 相对 v11」共 10 张变更摘要表
- 归档原因：这些是**版本演进史**（每一版的 v(n-1)→v(n) 差异），不再承载当前推进判据。
  现行判据以计划书 §1（A1–A8）、§7（WP 状态与验收）、§9（风险）、§10（D1–D6）为准；
  **v13 相对 v12 的摘要留在计划书正文**。
- 体例：以下每一节都是源文件的**字节级副本**，未改写任何数字、结论或措辞；
  计划书正文对应位置只留一行索引并指回本文件。

## 索引

| 版本 | 依据（一句话） | 本文件位置 |
|---|---|---|
| 版本头（v13 全文） | v13/v12/v11/v10/v9/v8/v7/v6/v5/v4 各版依据的逐字长行 | §原版本头 |
| v3 相对 v2 | 16 项：难度、工期、路线证据、§3–§6 数字更正、A1/A4/A5/A8 口径、WP0.5-B 前置 | §逐字归档：v3→v12 |
| v4 相对 v3 | A3 判据按 O1 替换（绝对逐位 parity → 相对判据），parity 测试降级为诊断仪器（附录 D-6） | 同上 |
| v5 相对 v4 | WP2 实测：D1/D2 定案、A1 的 `ctest` 口径收窄、§4.4 页-shift 陷阱不可达（附录 D-7） | 同上 |
| v6 相对 v5 | WP3 起手实测：挂载点改为 `execution/text.cpp` 分派、`kvarn:k4v2` 内部档、地址空间页几何为硬阻塞（进度 07-14） | 同上 |
| v7 相对 v6 | WP0.5-B 起手：准入实验被 WP3① 前置；首个 KVarN 数据点为正（附录 D-8） | 同上 |
| v8 相对 v7 | WP3① 完成：页几何按存储贯穿，`kvarn:k4v2` ctx8192 / 229,348 token 跑通（附录 D-9） | 同上 |
| v9 相对 v8 | WP0.5-B 补测完成：229k 同字节矩阵 ⇒ 代理门禁为正、`k6v6` 门槛量化为 < 0.002688（附录 D-10） | 同上 |
| v10 相对 v9 | WP4 完成 + 正式 WP0.5-B 发布档三档全过（附录 D-11） | 同上 |
| v11 相对 v10 | WP5 容差形式化 + P1 续列尾专项单测 + 独立审计（进度 08-03） | 同上 |
| v12 相对 v11 | GPU 收尾四件 + P2 前置（附录 D-12、进度 08-04） | 同上 |

---

## 原版本头（v13 时点，逐字）

- 版本：v13（v12 基础上按 **P2b：MTP parity 测试改造为诊断仪器**（2026-10-08，进度 08-05）：去逐位门禁、加**自确定性门禁**、加首分叉报告，`kKProfiles` 并入 `rk4v4`/`kvarn:k4v4|k5v5|k6v6` 并贯通 `options.kvarn_bits`；加 `--quick` 且 `tests.cmake` 注册 `TEST_ARGS --quick`（全扫仍可手动跑）。token 级实测：**kvarn 自确定性全档成立**；`bf16@k1` 分叉 413/512、`rk4v4@k3` 408/512、**`kvarn:k4v4`@k1/k3 与 greedy 逐字节相同**、@k15 418/512（bf16 同宽 412/512、同首分叉 token 91）⇒ **分叉不劣于基线**；WP3 ⑤ 措辞订正；见「v13 相对 v12 的变更摘要」与附录 D-13）。v12（v11 基础上按 **GPU 收尾四件 + P2 前置实测**（2026-10-08，进度 08-04）：**①P3a 3× 重复收尾**——`k4v4/k5v5/k6v6` 的**质量指标逐位重复（极差 0）**、与 D-11 单次值**逐位相同**，仅吞吐 ≤1.3% 散布 ⇒ D-11 单次口径被证实；**②P1 e2e 验收 PASS**（`p1_prefix_reuse.sh`：kvarn r2 `cached_tokens=851`、message 逐字节同 r1）；**③WP5 余量实测**（`flips` 最大 1/65、`over_step=0`、`wide_flips=0` ⇒ 判据收口）；**④续列尾 device 段** 首跑 FAIL(2)→定位为**测试自身 3B/12B pattern bug**→修正后绿；**P2 前置**：kvarn MTP provisional 路径**首次激励通过**（`mtp accepted 84/113`、加速比 1.50× ≈ bf16 1.52×）；见「v12 相对 v11 的变更摘要」与附录 D-12）。v11（v10 基础上按 **WP5 容差形式化 + P1 续列尾专项单测 + 独立只读审计**（2026-10-08，进度 08-03）：A2 容差改为**量化步长判据**（点值 ≤ 1 步 + 边界翻码计数上限 + `|Δcode|>1` 零容忍）+ 4/5/6 位穷举逐码往返；新增 `test_kvarn_continuation_image.cpp`（**host 段实跑绿 / device 段待 GPU**，ctest 261→262）；P1 移植审计**无 HIGH/MED**（3 项 LOW 为 FORK 继承）；见「v11 相对 v10 的变更摘要」）。v10（v9 基础上按 **WP4 执行 + 正式 WP0.5-B 实测**：核按 `(KBits,VBits)` 模板化、`bits` 贯穿、测试扩 4/5/6 全绿、parser 发三档并删 `k4v2`；**发布档准入三档全过** —— `k4v4`(274 B) mean KLD **0.002120**、`k5v5`(338 B) **0.001432**、`k6v6`(402 B) **0.001233**，分别对 `rk4v4`/`nvfp4`/`k8v4` 为 **2.09×/2.07×/2.18×**；见「v10 相对 v9 的变更摘要」与附录 D-11）。v9（v8 基础上按 **WP0.5-B 补测完成实测**：229k 同字节矩阵齐备，同字节 `kvarn:k4v2`(210 B) mean KLD **0.010513** vs `rk2v4-e8`(216 B) **0.043619** ⇒ **4.15×**（与 1438-token 单窗口的 5.7× 同向同量级）⇒ **代理门禁为正，裁决投 WP4**；`k6v6` 门槛量化为 < `k8v4` 的 **0.002688**；见「v9 相对 v8 的变更摘要」与附录 D-10）。v8（v7 基础上按 **WP3① 执行实测**：**地址空间页几何已按存储贯穿**，`kvarn:k4v2` 在 ctx8192 / 229,348 token **28 窗口全量跑通**（PPL 4.72225 vs `bf16` 4.69317）；见「v8 相对 v7 的变更摘要」与附录 D-9）。v7（v6 基础上按 **WP0.5-B 起手实测**：准入门被 **WP3① 页几何**前置、首个 KVarN 数据点为正；见「v7 相对 v6 的变更摘要」与附录 D-8）。v6（v5 基础上按 **WP3 起手实测**：挂载点改为模型执行层 `text.cpp` 分派、host 硬抛改显式守卫、`kvarn:k4v2` 内部档、**地址空间页几何**列为 WP3 硬阻塞；见「v6 相对 v5 的变更摘要」与进度记录 07-14。v5 = v4 基础上按 **WP2 执行结果**：D1/D2 定案、A1 的 `ctest` 口径实测修正、§4.4 页-shift 陷阱实测收窄、WP2 标记完成；见「v5 相对 v4 的变更摘要」与附录 D-7。v4 = v3 基础上按 `docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md` + 主代理复核：A3 判据替换、WP0.5-A 改造为诊断仪器、WP3/WP9 验收同步）

## 逐字归档：v3 相对 v2 … v12 相对 v11 的变更摘要

<!-- 以下自「## v3 相对 v2 的变更摘要」起为源文件第 28–132 行的逐字副本 -->

## v3 相对 v2 的变更摘要（本次修订的依据）

| # | v2 的说法 | v3 修正 | 依据 |
|---|---|---|---|
| 1 | 移植难度「中低」 | **设备侧接口兼容（已证），共享面 + 旋转域为硬改造：中高** | 宿主侧两处硬 throw、44 文件共享面、页 64 内核寻址、FORK 20 文件接入面 |
| 2 | 工期 25–35 人日 | **34–52 人日**（WP4 +4–6，WP6 +5–7，新增 WP0.5 前置 2 天） | WP 区间自洽性 |
| 3 | §2 三条「独立」证据 | #1 独立（FORK 注册表）；**#2/#3 是 `outdate/` 旧审阅复述**；补最强独立阻断项 **FORK 是 Linux-only 构建** | `FORK README.md:31`、`cmake/Dependencies.cmake:3-5`、构建系统 `WIN32/MSVC` 命中 0 |
| 4 | §3「无名字 helper」 | **有 3 个生产 + 4 个基准穷举 switch**（`operational_log.cpp:129`、`request_log.cpp:134`、`apps/cli/main.cpp:83` + 3 个 bench）；但**只产生警告，不保证编译失败**（未开 `-Werror=switch`） | 逐文件核验 |
| 5 | §4.3「37 个 switch 站点」 | **7 个穷举 `switch(storage)` / 44 文件 248 行引用**；补站点清单与「枚举必须追加末尾」约束 | `types.h:67` |
| 6 | §4.4「64 vs 128 冲突已消解」 | 过度声明。两池独立属实，但**共享 body helper 用 `>> kPagedKVPageShift(=6)`**，128-body 经 64-shift helper 会静默读错页；须为 kvarn body 用独立 page-shift | `small_t_i8.cuh:234,244` |
| 7 | §5「14 处字面量 / 3 套位解包」 | **≈55–60 处位打包字面量 / 8 个 ops+头文件 / 4 套独立位解包**（第 4 套在 `attention.cu:354-360`） | 保守模式实测 27 处（计划 14） |
| 8 | §5「唯一实现参考是 beellama」 | 上游 `config.py:21-26` **确有 4 个 preset（含 `kvarn_k4v4_g128/g64`）**；仅 K5V5/K6V6 无上游参考。且**只借 beellama 位流，不借其 128 列切片几何** | 见 H7 |
| 9 | §6 方案 A「尾行旋进后一次合并」 | **按文义不可实现**（FORK partial 是 BF16、TAIL 是 FP32；>8198 split 策略分叉；`kvarn_hadamard` 拒 f16）→ 改为 **k8v4 形状** 或 **ROTATED_K_ORIGINAL_V**；工期 5–8 → **10–15 天** | 见 §6 |
| 10 | §1 A1/A4/A5/A8 | A1 删 prefill 腿；A4 用 N=1024 **配对带**；A5 补 24.0 MiB 固定尾 + 本产物实测权重；A8 明确同字节对手 + 承认已知尾部代价 | 见 §1 |
| 11 | 未引用同仓既有负面裁决 | **新增 WP0.5-B：准入实验前置**，正面引用并反驳 `docs/port-records/PORT-DOD.md:23-32` | commit `8e34ad12` |
| 12 | §9「无公布吞吐/质量」 | 不成立：FORK `docs/performance.md:145,153-154,159` 已公布 KVarN K4V2-G128 的 tok/s 与「约 0.8% decode 代价」 | 原文核实 |
| 13 | 附录 A「`config-calculator.html:490` 是 KV 表」 | :490 是**权重 profile 散文**；每格式 KV 表在 **:529**；:550 为 `mtp3 kvRatio`；:563 为 35B 表 | 详见附录 A |
| 14 | 附录 A「1 页 sink + 3 个 BF16 尾槽」 | 实际**共 3 槽 = 1 sink + 2 动态尾**；24.0 MiB 数值按 3 槽算是对的 | `FORK paged-kv-cache.md:100-101` |
| 15 | 附录 B 命令 | vcvars 导入改用 `.deps/env-port.bat`；冒烟档位改 `rk4v4`；perplexity 补真实 `--corpus` 与 `--disjoint --score-topk 100`；ctest 正则补 `ninfer_kv_cache_append_test` | 逐条实测 |
| 16 | `state_image.cpp:120-146` 存 24 MiB 尾（v2 引用） | **该文件无 kvarn/tail 引用**（是 `continuation_hidden` 与 DFlash-local K/V）；尾池几何真源在 `decoder_state.cpp:118-161`。StateImage 是否复制尾池 → **WP7 待确认**，不再作为既成结论 | 一手核验 |

> 其余仍正确的原文内容（字节表、路线排除、两类 tail 歧义、显存修正、验收换仪器方向）在本版保留。

### v4 相对 v3 的变更摘要（依据：`docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md` + 主代理复核，2026-10-07）

| # | v3 的说法 | v4 修正 | 依据 |
|---|---|---|---|
| 1 | A3 = "主文本与 MTP 的 greedy 与 MTP-off **逐位**一致"，parity 测试作门禁 | **判据替换（O1）**：①同配置自确定性 ②质量不劣化 ③首分叉已文档化 ④ kvarn 专属"不劣于同配置基线"；parity 测试**降级为诊断仪器** | 上游基线本身不满足该判据（差分实测 D-4/D-5）；上游明示拒绝（#80）；本仓 `docs/performance.md:29-45` 早已实测否决；实为 Fork B 私有合同、其使能提交不在 TAIL 提交图内（D-6） |
| 2 | §7 WP0.5-A 验收 = `ctest -R …parity…` 通过 | WP0.5-A 改为"仪器交付 + 改造为诊断"，验收 = 自确定性成立 + 能报首分叉下标；不再要求逐位全等 | 同上 |
| 3 | §7 WP3 / WP9 验收含 "parity 测试通过 / MTP greedy 一致" | 改为"kvarn 档不劣于同配置 `bf16`/`rk4v4` 基线（相对判据）" | 同上 |

### v5 相对 v4 的变更摘要（依据：WP2 执行实测，2026-10-07，附录 D-7）

| # | v4 的说法 | v5 修正 | 依据 |
|---|---|---|---|
| 1 | §10 D1「单枚举 + `KvarnBits`」待决 | **已决**：`KvCacheStorage::KvarnGroup128`（追加末尾）+ `KvarnBits{Bits4,Bits5,Bits6}`，随 `EngineOptions.kvarn_bits` 流动 | WP2 已实现并编译通过 |
| 2 | §10 D2 三处表述冲突待决 | **已决**：产品默认不变；**裸别名 `kvarn` ≡ `kvarn:k4v4`**；`k5v5/k6v6` 需写全；指纹 `;kvbn=` 恒记级别 | 三处 parser 实测 |
| 3 | §1 A1「`ctest` 全绿」 | **实测修正**：本机基线**非全绿**，2 项先前存在失败 + 1 项已知 kvarn 容差，A1 口径据此收窄；真实模型另有 5 项结构性不可跑 | 基线（stash 重建）逐项对照，附录 D-7 |
| 4 | §4.4「共享 `>>6` helper 陷阱」为 WP2 必办 | **实测收窄**：kvarn ops **0 处**用 `paged_kv_address.cuh`/`kPagedKVPageShift`，其自身用**逻辑页号**；陷阱在 WP2 **不可达**，成为真实风险的条件是 WP3 把 kvarn body 接进共享 `small_t_*`/`prompt_*` helper | `grep src/ops/kvarn/` = 0 命中；附录 D-7 |
| 5 | §7 WP2「未开始」 | **已完成**（16 文件；合成 242/245、真实 10/16，全部失败均非 WP2 引入）；遗留项转 WP3 | 附录 D-7 |
| 6 | A8/A1 引用「TAIL 共 259 个测试」 | 注册数现为 **261**（WP1 + kvarn、WP0.5-A + MTP parity） | `ctest -N` 实测 |

### v6 相对 v5 的变更摘要（依据：WP3 起手实测，2026-10-07，进度记录 07-14）

| # | v5 的说法 | v6 修正 | 依据 |
|---|---|---|---|
| 1 | §7 WP3「`small_t.cu:460-505` body 挂载（新增 `small_t_kvarn` 分支）+ `causal_softmax_attention.cpp` 路由」 | **实测推翻**：FORK-B 在**模型执行层** `execution/text.cpp` 的每个注意力调用点用 `if (storage==kvarn) ops::kvarn_attention(...) else ops::causal_softmax_attention(...)` 分派；**`small_t_kvarn` 与 causal-cache 路由均不存在**。WP3 实际改动面 = 状态层 + 规划页几何 + 5 处派发 | 跨仓 diff（本地只读远端 `forkb`，merge-base `f76e19c0`）；进度记录 07-14 |
| 2 | §7 WP3「**两处 host switch 注册**（`d256_profile.h:87`、`paged_kv_storage.h:65`）」 | **改以「显式分支 + 守卫」替代注册**：`plan_cache` 对 kvarn 绕过 profile 派生，`layer_rank`/`layer_view` 对 kvarn 短路/抛。伪造 D256 profile 会静默算错几何 | 进度记录 07-14（决议 2） |
| 3 | §7 WP3 验收档 `--kv-dtype kvarn:k4v2` | **已实现为显式内部档**：`KvarnBits` 增 `Bits2`；`kvarn`/`k4v4|k5v5|k6v6` 由 parser 明确拒绝（避免静默错标）。**D2 的「裸 `kvarn` ≡ k4v4」标注为 WP4 生效** | 只发布已实现的 k4v2；进度记录 07-14（决议 1） |
| 4 | §7 WP3「未开始」 | **进行中**：状态层/派发/页几何（规划侧）已落地并编译通过；**`kvarn:k4v2` 短上下文端到端跑通**（28 tok PPL 11.18 vs rk4v4 11.22）；**长上下文（4900 tok）失败**于**地址空间页几何**（`kv_pages_for_*` 硬写 64，47 处）——列为 WP3 唯一硬阻塞 | 进度记录 07-14；未决项 8 |
| 5 | §4.4 陷阱「WP3 挂载 kvarn body 时必须用 `kv_page_shift=7`」 | **实测收窄**：kvarn body **完全不经**共享 `small_t_*`/`prompt_*` helper（自带 128-stride kernel），故该陷阱在 WP3 **仍不可达**；但**地址空间/准入的 64 页算术**是真实同类缺陷（v5 未列） | 进度记录 07-14（决议/未决项 8） |

### v7 相对 v6 的变更摘要（依据：WP0.5-B 起手实测，2026-10-07，附录 D-8）

| # | v6 的说法 | v7 修正 | 依据 |
|---|---|---|---|
| 1 | §7 WP0.5-B「准入实验前置（**≤1 天**，ROI 最高）」（隐含可独立先跑） | **不成立**：`kvarn:k4v2` 在**多窗口/长上下文**下**无法运行**（`--corpus --quick` ctx8192 与 ctx1024 均 window 0 报 `text KV address space has no free active entry`；单流 `--text` 仅 ≤≈1.4k token 可用）⇒ **该门被 WP3①（未决项 8，47 处页算术）前置** | 附录 D-8；进度记录 07-15 |
| 2 | §1 A4/A8 隐含「KVarN 无质量优势」（据 `PORT-DOD.md:23-32`） | **不足为据，且首个数据点方向相反**：`kvarn:k4v2`（**210 B**）mean KLD **0.0049** ≪ 字节匹配 `rk2v4-e8`（216 B）**0.0280**（≈5.7×），接近 `rk4v4`（280 B，0.0032）；但**样本仅 1438 token / 单窗口**，**不作结论**，发布档 k4v4/k5v5/k6v6 仍未测 | 附录 D-8 |
| 3 | 基准噪声底（A8） | 补 ctx8192/229,348-token 实测锚点：`bf16` PPL **4.693174**；`rk4v4+tail1024` mean KLD **0.001724** / max 8.786 / same_top 0.9833 | 附录 D-8 |

### v8 相对 v7 的变更摘要（依据：WP3① 执行实测，2026-10-07，附录 D-9）

| # | v7 的说法 | v8 修正 | 依据 |
|---|---|---|---|
| 1 | §7 WP3①「地址空间页几何」（未决项 8，47 处硬写 64）**阻塞** kvarn 长上下文，验收 = 4900 tok 跑通 | **已完成**：`kv_pages_for_frontier`/`kv_pages_for_tokens`/`kv_tokens_for_pages` 增 `KvCacheStorage` 参数；`KVAddressSpaceStore`/`LogicalKVPageStore` 改为**按其池几何 `page_tokens`** 取页（非 kvarn 逐位不变）。`kvarn:k4v2` 在 **ctx8192 / 229,348 token（28 窗口 / 全 `--quick` 语料）跑通**：PPL **4.72225** vs `bf16` **4.69317**（+0.62%）；tok/s 294.5 vs 296.1（−0.5%） | 附录 D-9 |
| 2 | §7 WP0.5-B「准入实验」被 WP3① 前置、无法执行 | **解除**：kvarn 现可按 A4 协议（ctx8192 / 229k token / `--disjoint --score-width 8`）执行；WP0.5-B 的 kvarn 臂已具备运行条件（本轮已产出 `kvarn:k4v2` 的 ctx8192 KLD：mean **0.010513** / same_top 0.9624） | 附录 D-9 |
| 3 | §9 风险「**地址空间页几何未随存储变化**」（长上下文 kvarn 无法运行） | **已消除**（本行保留为已解决记录） | 附录 D-9 |

### v9 相对 v8 的变更摘要（依据：WP0.5-B 补测完成实测，2026-10-07，附录 D-10）

| # | v8 的说法 | v9 修正 | 依据 |
|---|---|---|---|
| 1 | §7 WP0.5-B「可执行（07-16）」但 229k **同字节**对照档缺失（D-9 只说「不足以对质量下结论」，未点名缺哪几臂） | **已完成**：补齐 `rk2v4-e8`(216 B) / `rk4v4-t0`(280 B) / `nvfp4-t0`(288 B) / `k8v4-t0`(402 B) 四臂（`run4.sh`，51 min，全部 exit=0）。**代理门禁为正**：同字节 `kvarn:k4v2`(210 B) mean KLD **0.010513** vs `rk2v4-e8` **0.043619** ⇒ **4.15×**（1.4k 单窗口为 5.7×） | 附录 D-10 |
| 2 | §1 A8 的对照集仅有 k4v4/k6v6 的**预期**对手，无 229k 实测锚点 | 补 229k 实测锚点：`rk4v4` **0.004426** / `nvfp4` **0.004385** / `k8v4` **0.002688**（PPL 4.6936 ≈ bf16 4.6932，dlogp −0.000091）；`rk4v4+tail1024` **0.001724** ⇒ 尾部 229k 增益 **2.57×**（合 A4 的 2.26–2.47× 带） | 附录 D-10 |
| 3 | §1 A4/A8 隐含「KVarN 每字节质量更高」仅有 1.4k 证据 | **升级为 229k 级证据，但性质收窄**：KVarN 的优势是**每字节**（少 25–27% 字节接近 `rk4v4`/`nvfp4`），**不是绝对质量**——绝对 KLD 上 `rk4v4`/`nvfp4` 仍更优。**k6v6 的门槛量化 = < 0.002688**（按 A8「不优于 k8v4 则移除」） | 附录 D-10 |
| 4 | §7 执行顺序「WP3① → 完整 WP0.5-B → 才考虑 WP4/WP6」 | **WP0.5-B 的代理门禁已过** ⇒ **下一步为 WP4**；**WP6 仍不启动**（须先有 k4v4 档的正式 WP0.5-B 结果） | 附录 D-10 |

### v10 相对 v9 的变更摘要（依据：WP4 执行 + 正式 WP0.5-B 实测，2026-10-07，附录 D-11）

| # | v9 的说法 | v10 修正 | 依据 |
|---|---|---|---|
| 1 | §7-WP4 为「进行中：4.1/4.2 首步完成」，剩余 ①核模板化 ②`bits` 贯穿 ③测试 ④parser | **①②③④ 全部完成**：核按 `(KBits,VBits)` 模板化（发布档只实例化 `(b,b)`；`KBits==4` 保留 int4/nibble 快路径、V 侧统一位流）、`bits` 经 `EngineOptions→…→视图` 贯穿、`ninfer_kvarn_test` 扩 4/5/6 **全绿**、parser 发三档并删 `Bits2`/`k4v2`；顺带补 kvarn×tail / kvarn×mtp-window **规划期拒绝**。五目标构建绿 | §5「WP4 执行定案」、进度 07-20/07-21 |
| 2 | §1 A2 容差「现有 `3.0e-4` 只在 4-bit 有效」 | **收口并量化根因**：K oracle 在 bits=4 的 `relative_l2=6.0882e-4` **与重构前逐位相同**（⇒ 4-bit K 零回归），`max_abs=0.216` 恰为**一个量化步长**且 `sqrt(0.216²/Σ)≈8.4e-4≈实测` ⇒ 偏差来自**单元素边界翻码**，非 codec 缺陷；bits=6 与 oracle **逐位相同（rel_l2=0）**。容差按位宽重定为 `1.0e-3`（1.64× 余量），**WP5 待形式化为量化步长判据** | §1 A2、进度 07-21 |
| 3 | §1 A8 以「同字节对手」判 `k4v4↔{rk4v4,nvfp4}`、`k6v6↔k8v4`，`k5v5` 判据另定 | **判据执行完毕，三档全过**：`k4v4` 0.002120 vs `rk4v4` 0.004426 = **2.09×**、vs `nvfp4` 0.004385 = **2.07×**（且字节更少 274<280/288）；`k6v6` 0.001233 vs `k8v4` 0.002688 = **2.18×**（门槛达成）；`k5v5` 0.001432 落在单调曲线中间档。**不回退** | 附录 D-11 |
| 4 | D-10 判定 2「KVarN 优势性质是**每字节**，不是绝对质量」（基于 `k4v2` 代理档） | **被发布档取代**：`k4v4` 在**绝对** mean KLD 上同时优于 `rk4v4`/`nvfp4`（280/288 B）与 `k8v4`（402 B）⇒ 优势现在是「更少字节 **且** 质量更好」。**D-10 原句保留以存史**（它是代理档的实测） | 附录 D-11 |
| 5 | D-10 判定 5「无 KVarN 特有速度代价」 | **收窄**：`k4v4` −0.5% 成立；位宽升高有**单调真实代价** `k5v5` −1.5% / `k6v6` **−2.8%**（记录更大 ⇒ decode staging 字节更多）。A8「不得劣于同档噪声底」对 `k6v6` 须据此读 | 附录 D-11 |
| 6 | §10-D2/D4/D5：裸 `kvarn` ≡ k4v4 与删 `k4v2`「自 WP4 起生效」、`kvarn+tail` fail-fast「WP6 前」 | **均已落地（07-21）**：三处 parser 发三档、`KvarnBits::Bits2` 与 `kvarn:k4v2` 删除；`validate_target_options` 对 `kvarn` 拒绝 `--kv-tail-tokens>0`（此前会被**静默忽略**）与 `--mtp-attention-window != 0` | §10-D2/D4/D5、进度 07-21 |

### v11 相对 v10 的变更摘要（依据：WP5 容差形式化 + P1 续列尾专项单测 + 独立审计，2026-10-08，进度 08-03）

| # | v10 的说法 | v11 修正 | 依据 |
|---|---|---|---|
| 1 | §1 A2 容差「**WP5 待办**：形式化为量化步长判据（逐元素 ≤ 一步 + 边界翻码计数上限）」 | **已落地（08-03）**：`oracle_relative_l2_limit(bits)=1.0e-3` **删除**；改为 `check_step_criterion`——①点值 `\|actual-expected\| ≤ q*(1+5e-2)`（`q=(max-min)/qmax*row_scale*col_scale`，`qmax=(1<<bits)-1`）；②总量 `flips ≤ 1.0e-3*total`（65536 ⇒ 65）；③`\|Δcode\|>1` **零容忍**；另补 4/5/6 位**穷举逐码** pack→unpack 往返（含行外哨兵）。口径回写 `docs/maintainer/op-development.md §6.3`。**未跑**（CUDA 测试，GPU 被 P3a 独占）⇒ **余量为分析值、待 GPU 首跑确认**；若超限按**真实发现**记录定标，**不得事后放宽** | 进度 08-03、`test_kvarn.cpp` |
| 2 | §7-WP3 剩余 ②「续列尾」仅「已实现、构建绿」 | **补专项单测**：`tests/models/qwen3_5/test_kvarn_continuation_image.cpp`（ctest **261→262**）。**host 段已实跑绿**（几何 vs 分页尾、transfer work、capture→activate 逐字节/逐 token 往返、非法 spec 拒绝）；**device 段待 GPU**。② 的 **Engine 级 27B e2e 验收（PPL/缓存命中）仍开放** | 进度 08-03 |
| 3 | （未评）P1 移植的实现正确性 | **独立只读审计（vs FORK）结论：无 HIGH/MED 缺陷**；4 调用点齐备、`state_image` 布局与 FORK 逐点相同、分片由规划期守卫「KVarN 要求所有 attention 层同 rank」兜底。**3 项 LOW 均为 FORK 继承**，其中 ① **已由图谱（codebase-memory `trace_path`）+ grep 复核修正**：`reset_kvarn_tail_row` 在 TAIL **零调用者**、在 FORK 有 **2 处（`tests/models/qwen3_5/test_prefill_precision_real.cpp:236,284`）**，**该测试未随移植进入 TAIL** ⇒ 既属陈旧-marker 隐患，也是**移植遗漏的测试覆盖**（原审计「两树均无调用者」不准确）；② capture/activate 抛错被 `catch(...)` 降级为静默失败；③ `StateImagePart`/pool 注释未提 KVarN。**均未修** | 进度 08-03 |

### v12 相对 v11 的变更摘要（依据：GPU 收尾四件 + P2 前置实测，2026-10-08，进度 08-04，附录 D-12）

| # | v11 的说法 | v12 修正 | 依据 |
|---|---|---|---|
| 1 | §1 A2 / §7-WP5：判据「**余量为分析值、待 GPU 首跑确认**」 | **余量已实测**：`flips` 最大 **1/65**（65× 余量）、`over_step=0`、`wide_flips=0`、`exit=0`。预测（bits=4 约 1 翻码、bits=6 0）与实测吻合 ⇒ **判据成立、无需放宽**，WP5 **收口** | 进度 08-04、`ninfer_kvarn_test` |
| 2 | §7-WP3 剩余 ②「续列尾：host 段绿、**Engine 级 e2e 验收仍开放**」 | **② 完成并验收**：`p1_prefix_reuse.sh` → kvarn r2 `cached_tokens=**851**`、message 与 r1 **逐字节相同**、bf16 对照亦 851；`test_kvarn_continuation_image.cpp` **device 段亦绿**（首跑 FAIL 系**测试自身** marker_pattern 3B/12B 尺寸 bug，非实现缺陷） | 进度 08-04、附录 D-12 |
| 3 | §7-WP3 剩余 ④「MTP 路径激励与 A3 相对判据」**未做** | **激励完成**：kvarn MTP provisional 路径首次激励通过（`mtp accepted 84/113`、自确定性成立、本样本 MTP-on vs off 逐字节同、加速比 1.50× ≈ bf16 1.52×）。**仅余 P2b 仪器改造**（token 级首分叉率） | 进度 08-04、附录 D-12 |
| 4 | §7-WP3 的「`small_t.cu:460-505` 挂载 / `small_t_kvarn`」措辞 | **订正为「模型执行层 `execution/text.cpp` 分派」**（FORK-B 无 `small_t_kvarn`）——本项自 v6 起已反复标注，v12 正式把 §7-WP3 正文的剩余条款一并订正 | §7-WP3、进度 07-14 |
| 5 | D-11 ⑥①「各臂**单次**测量（未做 ≥3 重复）」 | **已收口**：P3a 3× 重复 ⇒ **质量指标三重复逐位相同（极差 0）**、与 D-11 单次值**逐位相同**；仅吞吐 ≤1.3% 散布。D-11 的 A8 三档判定**稳固** | 进度 08-04、附录 D-12 |
