# PORT-MTP-PARITY-A3-INVESTIGATION — MTP k=1 与 greedy 分叉的上游归属调查

- 日期：2026-10-07
- 对象：`D:\ninfer\ninfer-precision-tail`（TAIL，工作树含 WP0.5-A/WP1 未提交改动）
- 触发：WP0.5-A 移植 Fork B 的真实模型测试 `tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`
  后实跑失败——纯 `bf16` 下 MTP k=1 与 k=0 greedy 在 **token 91** 分叉
  （`expected=2466 actual=2640`），k=3 却 512/512 全等；`--no-cuda-graph` 完全复现
  （见 `kvarn-port-progress.md` §3-2026-10-07-8、-10）
- 问题：**该问题在最上游 `Neroued/ninfer` 是否已被记录（文档及 issue）、是否已被修复**
- 性质：**只出结论。不改任何代码、不改任何既有判据、无任何网络写操作。**
  本文件是本次调查的唯一新增产物
- 结论落盘：调查结论已写入 `kvarn-port-progress.md` §3-2026-10-07-11 并更新 §0 快照与未决项 5；
  **已回写（2026-10-07）**：计划书 §1 A3、§7 WP0.5-A/WP3/WP9 验收、§9 风险表均已按 **O1** 更新
  （计划 v4 附录 D-6）；本报告 §9 的 O1 已被采纳。主代理复核见计划书附录 D-6

> 纪律：GitHub 侧每个编号都能由 §7 的一条命令复现；代码侧每个论断都带 `文件:行`。
> **本次没有跑 GPU**：真机数字取自 §3-2026-10-07-8/-10 的既有实测，本轮新增的是联网调研与静态代码判定。

---

## 0. 结论一览

| 论点 | 结果 |
|---|---|
| **C1** 该症状（MTP k=1 ≠ greedy）在上游有对应 issue | **无**。枚举全部 **370 个 issue+PR**：无任何标题含 `parity`；无 `token 91`/`2466`/`2640`/`5070 Ti` 命中；中文词（投机/不一致/分叉/对不上/贪心/确定性/复现）**0 命中** |
| **C2** 同族机制在上游有记录 | **有且开放**。**issue #265**（open，2026-09-16，**0 评论、timeline 空 ⇒ 维护者从未回复**）：verify 宽度 ≥2 的 GDN 输入投影按宽度查 schedule 表，"computes each column differently from how the width-1 decode pass computes it"，layer-0 分歧 **0.72%** → 隐态 **~3.5%** → **翻转约 5% 的 argmax 决策** |
| **C3** 是否被修复 | **未修复**。`gdn_input_record_schedule`/`decode_order` 全库 0 命中；`compare 68c54356...070fa61a` ⇒ dev 领先 23 个提交，**无一触及 GDN 输入投影**；**仓库无 release、无 tag** ⇒ 不存在"某版已修" |
| **C4** #265 是否就是本机成因 | **不是**。本机权重是 **IQ3_XXS GGUF**，`gdn_input_proj.cpp:603` 的 nvfp4 路由要求 `QType::NVFP4`；IQ3_XXS 走 `gguf_project`（`gdn_input_proj.cpp:1336-1347` / `:1439-1443`，同一个 `gguf_gdn_project`），唯一宽度键是 `t <= 8`（`gguf_linear.cpp:183`、`ggml_bridge.h:49`）⇒ **宽度 1/2/4 落同一个 MMV kernel**，不查宽度表 |
| **C5** 本机真实机制 | bf16 causal-cache **SmallT attention 的 CTA 内归约形状随 verify 宽度改变**（`small_t.cu:383-395`：宽度 1→`<1,2>`，宽度 2→`<2,4>`，宽度 4→`<4,4>`；第二参 = `WarpsPerCta`）＋ decode 用 cached/unmasked 入口（`execution/text.cpp:571`）、verify 用 batched/masked 入口（`execution/text.cpp:965`）。bf16 的 split 数只由 window 决定、与宽度无关（`small_t.cu:66-86`、`:131`）⇒ 分歧在 CTA 内 ⇒ **near-tie argmax 翻转**。**k=3 全等是没撞上平票，不是等价** |
| **C6** 上游是否认为这是缺陷 | **明确不认为**。#80 由维护者 **Neroued** 关闭并写：**"Not in scope. Greedy does not mean batch invariant. And batch invariant limits optimizations."**；PR #220 关闭不合并；`qwen3_5-model.md:328-330`（三仓逐字相同）：**"This does not impose token or logits equality between different quantization, prefill or kernel paths."**；`op-development.md:420`："Re-running a shorter projection can choose **different arithmetic and is not an equivalent reference**" |
| **C7** A3 判据的真实出处 | **Fork B（`ninfer-rtx5090-mobile`，Mirko Covizzi）的私有合同**，且以其自有 kernel 改动为条件（`7d566547`、`d476fafa` 的 "canonical per-query arithmetic profile"）。**这些提交不在 TAIL 的提交图内**；TAIL 只含上游 `a7818988`（宽度特化 switch 的源头，Neroued 本人） |
| **C8** 最关键的一条 | **TAIL 本仓早已实测并书面否决了 A3**：`docs/performance.md:29-45`（23 构型 × 3 重复）"**the width-1 greedy path produced a hash matched by no speculative configuration**"、"what is guaranteed is **per-configuration determinism, not cross-configuration equality**"；`docs/archive/TODO.md:4462-4464`：**"A test asserting bit-identity to greedy would fail permanently by design, which is worse than no test."** ⇒ §3-2026-10-07-8 把它当作"首次暴露的 TAIL 固有缺陷"是**误判** |
| **C9** 对 kvarn 移植的影响 | **不阻塞**。kvarn 的 decode/speculative 路线需要的是"同配置可复现 + 质量不劣化"，两者实测成立；"跨配置逐位相等"上游与本仓均已书面否认，且达成它要付的性能代价**从未被量化** |

---

## 1. 被调查的现象

| 项 | 取值 |
|---|---|
| 测试 | `tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（自 FORK B 移植，557 行；注册于 `tests/models/qwen3_5/tests.cmake:188-191`） |
| 断言 | 同一 prompt 同一模型下，`--spec` 关闭（verify 宽度 1）与 `--spec mtp --draft-tokens k`（宽度 k+1）的 **greedy token 序列逐位全等**，512 token |
| 模型 | `Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`（11,092,477,952 B），`--kv-dtype bf16` |
| 设备/构型 | RTX 5070 Ti 16 GB，`build-port`：`120a` / `NINFER_SM120_NATIVE=ON` / `NINFER_COMPAT_PATH=OFF`（native 口径，见 §3-2026-10-07-5） |
| 实测 | k=0 重复全等 ✓；**k=3 全等 ✓**；**k=1 在 token 91 分叉 `expected=2466 actual=2640` ✗** |
| 已排除 | CUDA Graph 复用（`--no-cuda-graph` 同 token 同数值）⇒ **确定性** |
| 归属判定（前序） | 上游包（源码 `b06908ba`）与 TAIL 包跑同一模型：**同位置分叉、输出逐字节相同** ⇒ **非 precision-tail 引入**（§3-2026-10-07-10） |
| 模型架构 | Qwen3.5 型混合：`src/models/qwen3_5/config.cpp:130-136` 把 `layer_types` 解成 `MixerKind::FullAttention`/`LinearAttention`，`:145-156` 建 `GdnConfig` ⇒ **确含 GDN（gated delta net）线性注意力层**，故 #265 谈的 pass 区分（record vs snapshot）在本模型真实存在，但其机制不适用（§3） |

---

## 2. 上游 GitHub 侧：是否记录、是否修复

### 2.1 #265 — 同族机制，开放且无人回应

- **标题**：`Proposal: artifact-declared GDN input record schedule (decode_order verify alignment)`
- **状态**：**open**，`closed_at: null`；2026-09-16T16:58:35Z 由 `cometkim`（`author_association: NONE`）提出；**0 评论**，`/timeline` 与 `/events` 均返回 `[]` ⇒ **无标签、无交叉引用的修复 commit/PR，维护者从未回复**
- 作者原文（关键三句）：
  > "The GDN input projection record pass (the one that runs at **verify widths >= 2**) today selects its GEMM schedule from a **width-keyed table**. Each width gets its fastest schedule, but that computes each column differently from how the width-1 decode pass computes it."
  > "Layer-0 GDN output: **0.72% relative divergence** at verify column 0 versus the matching decode step, compounding to **~3.5% at the final hidden state**, which **flips roughly 5% of argmax decisions**."
  > "GDN convolution and gate/state semantics are bit-identical across widths (verified by controlled probes), but the input GEMM's **accumulation order and activation precision differ per width**."
- `per_width` 默认语义（作者自述）："**No cross-width numerical consistency is implied.**"
- 作者报告的"对齐后"收益（greedy tg128 bench，RTX 5090，NVFP4 产物）：K1 58.02→58.75、K2 40.43→46.97、K3 34.57→38.64；**从未主张宽度 4 与宽度 1 轨迹一致**，宽度 4 只被说成对齐核的"~2x the cost"
- 该 issue **不点名任何源文件**，只描述机制；按其描述在 dev 上对应 `src/ops/gdn_input_proj/nvfp4/nvfp4_gdn_input_a16.cu`（`tokens==1 → decode_launch`、`tokens<=2 → small_t_launch`、`<=8/16/24/32/64/96 → launch_nvfp4_a16_sliced_k_mma<...>`，K=5120）
- **无复现命令行**

⇒ **#265 的价值是前瞻性的**：若 kvarn 路线将来改用 NVFP4 产物，它会正面命中；**对本机的 bf16 + IQ3_XXS 场景无解释力**（§3）。

### 2.2 维护者的合同立场

| 编号 | 状态/关闭者/日期 | 关键原文 |
|---|---|---|
| **#80** `Design: batch-invariant greedy decode (opt-in fixed decode width)` | closed **by Neroued**，2026-08-27（`state_reason: completed`） | **"Not in scope. Greedy does not mean batch invariant. And batch invariant limits optimizations."** 正文实测边界：Q4 [34816×5120] 在 **t=2** 越界；并指出"even `--max-concurrency 2` with MTP crosses them"；提议的 `--decode-invariant` 开关**从未落地** |
| **PR #220**（GDN conv record rounding） | closed **unmerged** by Neroued，2026-09-10 | **"This is not a numerical requirement: the Op contract explicitly allows private convolution intermediates to remain unrounded. Exact equivalence is required only for ReplaySSM record/fold versus the corresponding full computation."** |

### 2.3 相邻 issue 与反向先例

- **#105**（closed，**已修**）："改了 greedy 输出"被真实测试抓到，维护者答复 *"Fixed in `fd48e2fae270200793cb26029851b7d37befad38` … caused by numerical variation from different prefill chunk splits."*
  ⇒ **保持公允的证据**：上游**会**修真实回归，只是不认"跨宽度逐位等价"是需求。
- **#349**（closed 2026-10-01，`not_planned`，**由提交者 DuncanBetts 自开自闭**，6 条评论全是其粘贴的模型评审，**无维护者立场**）：`proposal_q` lookup miss 时的"静默过接受"；结论"at temperature 0; worst case, probability of ~3e-9"。其 guard 修复**只存在于第三方 fork** 的 `350136a8`（ValerioDolci，2026-10-05，"upstream issue #349"，改 `src/ops/kernel/speculative_round.cuh`），**不在 dev/master**。
- **#374**（open，2026-10-05）：`linear_add` 结果随 batch 宽度变化（FMA 结合）——**同一类成因**（宽度改变算术），非 parity 合同问题。
- **#119**（open，2026-08-30）：MTP 按草稿位置的自洽/接受率，**性能**议题，非 parity。
- **#375**（open，2026-10-06，`qwased`）：断言"MTP speculation untouched … byte-identical"——与本轮 C5 不冲突（它讲的是改动面，不是跨配置等值）。
- **#92**（closed 08-26→08-27）：MTP5 重复环问题，提交者自述 "Sorry, it's fork related."
- 其余涉及 MTP/投机的 **#208 / #221 / #226 / #292 / #52 / #294**：崩溃、graph 拓扑、性能、XGrammar+投机——**均与 parity 无关**。

### 2.4 负证据（查过且为空，属实质结论）

- 370 个 issue+PR 的标题+正文中：`parity` 标题 **0**；`2466|2640|token 91|parity_real|mtp_greedy` **0** 真实命中（#112 的性能表为误命中）；中文词 **0**；评论级 `diverge`(open) **0**；`5070 Ti` open/closed 均 **0**。
- **Releases：空。Tags：空** ⇒ 上游无版本化"修复"可指认。
- Discussions 共 6 个，无一涉及 MTP/投机；**全仓不使用任何 label**。
- 上游 `master` 与 `origin/dev` 上**都不存在**任何真实模型 MTP greedy-parity 测试：
  `tests/models/qwen3_5/` 只有 `speculative_page_boundary.h`、`test_dflash_prefill_real.cpp`、
  `test_engine_dflash{,2}_real.cpp`；`tests/ops/` 只有 `test_mtp_pack.cpp`、`test_mtp_round.cpp`、
  `test_speculative_round.cpp`。**没有任何断言"投机与非投机 token 序列相同"的测试。**
  并且 `tests/README.md:288-289` 自陈：固定 Engine fixture "**does not define bit parity across arbitrary floating-point routes**"。

### 2.5 dev 分支近况（是否悄悄修了）

- `master` = `68c54356fd490ab329bd1475d48957f886bb7dd1`（2026-10-05，与本地克隆 HEAD 一致）
- `origin/dev` = `070fa61a3d937dd662859a58579505ed308d7ed6`（2026-10-06T16:33Z，
  "feat(ops): support and tune bf16 linear 2560x2560"）；相对 master **23 ahead / 0 behind**，
  内容全是 xgrammar vendoring、GBNF/JSON/tool-call 约束解码、线性形状调优
- 窗口内唯一触及投机文件的提交：`08902f733ce5aba02c6b06d82db742adaf046116`（2026-10-05，GBNF 约束解码），
  改了 `speculative_round.cuh`、`target_verification.cpp`、`program/speculative/mtp.cpp`、`execution/draft.cpp`，
  但**只是给既有 greedy 快路径加 `cfg.mask.words == nullptr` 守卫**（`while (a < extent && row_targets[a] == row_drafts[a]) ++a`），
  **无数值/宽度改动**，且 **dev-only**（未进 master）
- GDN/投影相关提交均在窗口之前，且不改 schedule 合同：
  `0784e76f64`（09-26 `perf(gdn): replace chunked path with two-stage kernels`）、
  `fc3993d8af`（09-26 `perf(ops): unify nvfp4 templates and add a16 mma routes`，`nvfp4_gdn_input_plan.cpp`/`_a16.cu`/`_small_t.cu` 的最后一次编辑）、
  `40bfe7dc2b`（09-28 `perf(ops): tune fp8 fused projections with tma split-k`）、`9e163eee4b`/`594930e7b6`（q4/q5 输入投影路由）

---

## 3. 为什么 #265 不命中本机（静态代码判定）

| 判据 | 证据 |
|---|---|
| nvfp4 宽度表只在 NVFP4 权重下可达 | `src/ops/wrapper/gdn_input_proj.cpp:603`（要求 `weight.qtype == QType::NVFP4`）；`src/ops/linear/gguf/gguf_linear.cpp:98` `QType::GGUF_IQ3_XXS → GgmlType::IQ3_XXS`；`src/core/weight.h:33,62` |
| 本机 GDN 投影走 GGUF 路径，且 snapshot/record 用**同一个**调用 | `gdn_input_proj.cpp:1336-1347`（snapshot）与 `:1439-1443`（record）同调 `gguf_gdn_project`；`gguf_project` 实现 `linear/gguf/gguf_linear.cpp` |
| GGUF 路径唯一的宽度键是阈值 8，不查 schedule 表 | `gguf_linear.cpp:183` `t <= gguf::kMaxVectorColumns`（=**8**，`ggml_bridge.h:49`）→ `vector_product`（MMV）；否则 MMQ / `kDequantizedVectorColumns=64`（`:17,247`）。`linear/gguf/` 全目录 **无任何 route_table 引用** |
| 结论 | 宽度 1 / 2 / 4 全部 `t<=8` ⇒ **同一个 `ggml_vec_iq3_xxs.cu` MMV kernel**（`ggml_bridge.cu:82`）⇒ **#265 的"每宽度不同算术"在本机不成立** |
| GDN record pass 本身是"宽度门控"而非"schedule 门控" | `src/models/qwen3_5/execution/text.cpp:1020-1022`（`UpdateInPlace` 且 `width != 1` 直接 `throw`）→ `:1036` 用 `gdn_projection_record`；`gdn_input_proj.cpp:88-93` `kMinimumWidth = 2` 硬拒宽度 1；`src/models/qwen3_5/program/speculative/target_verification.cpp:14` 设 `RecordForReplay` |
| GGUF/IQ3 是 TAIL 独有面 | `grep -rl IQ3_XXS`：上游 `src`+`include` **0 命中**；FORK B `src` **0 命中**；TAIL **8 命中**（`src/ops/linear/gguf/{ggml_bridge.cu,ggml_bridge.h,ggml_mmq_iq3_xxs.cu,ggml_vec_iq3_xxs.cu,gguf_linear.cpp}`、`src/core/weight.h`、`src/artifact/formats.cpp`）⇒ **FORK B 的 parity 保证从未在本机权重格式上验证过** |

---

## 4. 本机真实机制：attention 的 CTA 内归约形状随宽度变化

| 环节 | 证据 | 含义 |
|---|---|---|
| 宽度 → kernel 形状 | `src/ops/softmax_attention/dense/causal_cache/small_t.cu:383-395`：`switch (invocation.width)` 于 `:383`，`case 1: DISPATCH(1,2)`；`case 2: DISPATCH(2,4)`；`case 3: DISPATCH(3,4)`；`case 4: DISPATCH(4,4)` | 宏第二参是 **`WarpsPerCta`**：宽度 1 用 2 个 warp，宽度 2–8 用 4 个 ⇒ **同一 query 的 softmax 归约划分方式不同** |
| 形状如何变 | `small_t.cu:212` `constexpr int kBlock = 32 * WarpsPerCta`；`small_t_bf16.cuh:29-30` `Wc = WarpsPerCta`、`Br = Wc * 16`；tail warps `kCausalSmallTTailWarps<TokenTile>` | 宽度变化同时改变 `TokenTile`、行块高度与参与归约的 warp 数 |
| 入口不同 | decode：`src/models/qwen3_5/execution/text.cpp:571` `causal_softmax_attention_cached`（unmasked）；verify：`text.cpp:965` batched masked `causal_softmax_attention`（同类调用另见 `:390,:404,:974`） | 不只是宽度，**整条调用入口都不同** |
| 跨 split 归约树**相同** | `small_t.cu:88-131` `causal_small_t_split_count(window, tokens, storage)` 中所有 token 相关分支都门在 `i8_family`（`:114-130`）；bf16 落到 `:131` 的 `causal_small_t_split_upper_bound<Geometry>(window)`（`:66-86`，**只看 window、无 tokens 形参**）；`narrow = tokens <= 5`（`:274`）只在 `if (batch_size > 1)`（`:271`）块内参与 CTA 目标数 | ⇒ 分歧**不在** split 之间，而**在 CTA 内部**（`WarpsPerCta`/`TokenTile`/`Masked`） |
| 路由命中 | `src/ops/softmax_attention/dense/causal_cache/causal_softmax_attention.cpp:392`（`width <= 8` → `SmallT`，否则 `ChunkedSmallT`）、`:394`（`width <= 6` → `SmallT`）、`:42-45`（bf16 宽度 9–12 的分块特例） | k=1→宽度 2、k=3→宽度 4 **都在 SmallT 带内**，故确实命中 `case 2` vs `case 4` |
| 后果 | 浮点加法不满足结合律 ⇒ 末位 ulp 差异 ⇒ **平票处 argmax 翻转** ⇒ 后续轨迹整体分岔 | 与"draft/accept 计数两模一致但提交 token 不同"完全吻合（宏观率不变，逐点轨迹变） |
| **k=3 全等的正确解读** | 宽度 4 用 `<_,4,4>` ≠ 宽度 1 的 `<_,1,2>`，**不存在"宽度 4 恰好等价"的性质** | ⇒ **通过与否是抛硬币**，不能当作"该宽度正确"的证据；也说明本机 flip 率远低于 #265 的 5%（512 步内没撞上）。**#265 的 5% 属 nvfp4 GDN 路径，不可移用** |

**没有可拨的开关**（逐条核过读取点，无法在不改代码的前提下把宽度-1 算术变成全宽度规范算术）：
`NINFER_LINEAR_ROUTES=legacy|unified`（`route_table.cpp:117-123`，对 GGUF **无效**）、
`NINFER_GDN_TWO_STAGE`（`gated_delta_net.cpp:212-219`，prefill `kMinTokens`）、
`NINFER_SMALLT_PV_F16`（`small_t_i8_launch.cuh:25`，**i8-only**）、
`NINFER_PROMPT_{PV_F16,PACK_GQA,FAST}`（`prompt.cu:31,43,238`）、
`NINFER_DEVICE_PROFILES`（`device_profile.cpp:133`，驱动 `fused_route_table`）、
`force_linear_route_table`（仅测试用）、`ProposalHead{Full,Optimized}`（`load_options.h:16`，只选草稿头）、
`mtp_attention_window`（`execution/text.cpp:387,401,441`，只裁草稿 KV）。

---

## 5. A3 判据的真实出处：Fork B 的私有合同

### 5.1 Fork B 的使能改动（作者 Mirko Covizzi，除注明外）

| SHA | 日期 | 主题 | 规模 | 作用 |
|---|---|---|---|---|
| `7d566547` | 08-23 | `fix(mtp): make greedy verification width-invariant` | 31 文件 **+342/−117** | **首次加入 parity 测试**（+122 行）；把 bf16 宽度 switch 钉成固定形状 `kTokenTile=1; kWarpsPerCta=2`（blame `small_t.cu:102-105`）；触及 `gqa_attention_decode*`、linear_add/gdn plans |
| `1388b7c2` | 08-29 | `fix(mtp): preserve greedy output across compact batches` | 20 文件 +176/−136 | attn_input_proj / gdn / linear_add plans |
| `114b0fcb` | 09-05 | `fix(kvarn): preserve greedy parity across speculative decode` | 20 文件 **+1031/−434** | `kvarn/decode_kernel.cuh` ±327，测试 +197 |
| **`a7818988`** | 09-06 | `perf(ops): qualify variable-width causal cache attention` — **Neroued／上游，三仓皆有** | 19 文件 +1062/−890 | **宽度特化 `switch(invocation.width)` 的源头**，即 TAIL 现在遵循的调度 |
| `d476fafa` | 09-19 | `fix(mtp): align greedy decoding across kv cache formats` | 25 文件 +380/−258 | 加入 `small_t.cu:230-233` 的 `tokens=1; batch_size=1` 与 `softmax_attention.h:129-131` 的 "**canonical per-query arithmetic profile**"；注意守卫条件是 `cache_storage != BFloat16 && != Int8Group64` ⇒ **不覆盖 bf16**，bf16 由 `7d566547` 覆盖；测试重写 172 行 |
| `dff96dca` | 09-22 | `feat(mtp): add adaptive draft widths` | 96 文件 +1850/−548 | 守卫扩到 `tokens<=16`，测试 +215，新增 `test_mtp_adaptive.cpp` |
| `16e12737` | 10-04 | `test(mtp): qualify compressed execution…` | 4 文件 +56/−25 | 仅 NVFP4 scale-compression 用例 |

### 5.2 TAIL 的对照

- TAIL **只含 `a7818988`**；`7d566547` / `d476fafa` / `dff96dca` / `114b0fcb` / `1388b7c2` **在 TAIL 中不是对象**（不在提交图内）。
- 结构已分叉：TAIL 的宏是 `NINFER_CAUSAL_SMALL_T_DISPATCH(TOKENS, WARPS)`（`small_t.cu:355`，bf16 臂 `:364`）且**携带 `wave_splits`**；FORK B 的是 `(TOKENS)`（`small_t.cu:279`）、`wave_splits` **0 处**、并带注释 `:278` "**BF16 uses its fixed canonical-column geometry**"。
- FORK B 测试的判据自述（`test_engine_mtp_greedy_parity_real.cpp:230-231`）：
  > "Every MTP width and fresh repeat uses **ordinary greedy as oracle**. DFlash retains its same-width repeatability check; **its wider target arithmetic has a separate contract**."
  ⇒ 作者自己就把 DFlash 排除在逐位等值之外，**只把 MTP 留作合同**，而该合同靠上面这些自有 kernel 改动撑着。

### 5.3 Fork B 合同的自相矛盾与代价（反面证据）

- **同一文件自相矛盾**：`qwen3_5-model.md:269-272` 声称 "the fork requires **exact committed-token parity** between ordinary greedy decode and MTP draft windows 1..15"，而**同一文件 `:358`** 保留了上游那句 "This does not impose token or logits equality between different quantization, prefill or kernel paths."
- **自陈存在分叉**（**Fork B** 的文件；TAIL 无此行）：`docs/maintainer/paged-kv-cache.md:188` — "A 128-token smoke misses the **reproduced cross-backend divergence at output index 357**."
- **代价从未量化**：FORK B 无任何 canonical-vs-宽度特化的基准对比。最接近的只是 `docs/performance.md:387-388` "The merge also **preserves canonical-column BF16 attention and single-column INT8 MTP attention**"（即拒绝上游调度），`:364` "The short INT8 MTP3 workload **regressed 7.2%**"（归因于同步导入），`:294` "Compact GDN-output batches retain their qualified width-invariant A16 profile"（此前一个 A4 变体 "**failed the independent numerical oracle**"）。
  ⇒ 与 #80 维护者 "**batch invariant limits optimizations**" 的立场正面相对 ⇒ **移植 A3 判据等于移植一个未定价的性能代价**。

### 5.4 上游/本仓的合同文本（本轮逐字核过）

- `docs/maintainer/qwen3_5-model.md`：**上游 `:328-330` / TAIL `:366-369` / FORK B `:355-358` 三仓逐字相同** —
  "…**This does not impose token or logits equality between different quantization, prefill or kernel paths.**"
- `docs/maintainer/op-development.md`（TAIL 行号）：`:223` "…**bitwise equality unless an exact semantic format requires it**"；
  `:420` "**Re-running a shorter projection can choose different arithmetic and is not an equivalent reference.**"；
  `:525` "**If one decomposition requires a different arithmetic profile, qualify it separately** against the oracle…"
- `docs/maintainer/replayssm-gdn.md:455-460`：exactness 只界定在"**同一物理 verify block**"，并明说"**验证 bitwise clone 时，最终文本或 BF16 output parity 都不够。**"
- `docs/maintainer/dflash.md:240`："…**their logits need not match one another.**"
- `docs/maintainer/engine-architecture.md:125-126`：投机路径的数值/状态正确性"**按 Op 合同及独立 oracle 验证**"，而非按等值验证
- **全仓无一处主张 MTP==greedy 逐位**：上游与 TAIL 对 `committed token sequence` / `greedy parity` / `same token IDs` 的 grep **皆空**；三个 `AGENTS.md`/`README.md`/`docs/README.md` 对 `parity`/`bit-exact`/`determinis` **零命中**；`CLAUDE.md` 不存在。

---

## 6. 最重要的一条：TAIL 早已书面否决 A3

`docs/performance.md:29-45`（2026-09-09，**23 个投机构型 × 3 次重复**，
`scripts/sweeps/dflash2-draft-tokens-realtext.ps1` 的 `content_sha256` 实测）：

> "**Speculative decoding is not bit-identical to non-speculative decoding here, it is not required to be, and every configuration is nonetheless deterministic in itself.**"
> "**every configuration reproduced its own output exactly, 23 of 23**, three runs each"
> "**the width-1 greedy path produced a hash matched by no speculative configuration**"
> "Verification evaluates k+1 columns in one pass while plain decode evaluates one, so **the reductions run in a different order and a near-tie argmax can flip**; a different draft count is a different width and so a different order again. That is why **bit-identity to greedy is not a target here: it would require computing the accepted column with the width-1 kernel on every round, which is the work speculation exists to avoid.** It costs nothing in quality — swapping an MMA tile for a different reduction order **leaves perplexity bit-identical to twelve significant figures** — so **what is guaranteed is per-configuration determinism, not cross-configuration equality.**"

`docs/serving.md:82-83`："attention splits its reduction by the **verify width**, so a near-tie can resolve differently at another width."
`docs/maintainer/quality-trade-experiments.md:251-253`："greedy text can differ at the reduction-order level."

`docs/archive/TODO.md`（同仓既有条目）：

> `:4462-4464` — "**A test asserting bit-identity to greedy would fail permanently by design, which is worse than no test.** A test asserting self-determinism and cross-run stability would be worth having, and `dflash2-draft-tokens-realtext.ps1`'s `content_sha256` column is the mechanism for it."
> 推荐保证项：`:4457` **self-determinism**、`:4459` **quality parity**、`:4461` **documented divergence**
> `:4452` "changed reduction order… flips an argmax only at a near-tie"；`:4533` "A different accumulation order… is **a property of the algorithm, not a defect**"

⇒ **本轮失败是该既有实测的一次重现，不是新发现**；"width-1 哈希无任何投机构型匹配"甚至预测了 k=1 会失败。
§3-2026-10-07-8 把它记作"TAIL 固有 MTP 问题被新仪器首次暴露"，**定性错误**，已在 §3-2026-10-07-11 更正（原条保留不删）。

---

## 7. 复现命令

```bash
# —— 联网（全部必须走代理）——
export HTTPS_PROXY=http://127.0.0.1:7897 HTTP_PROXY=http://127.0.0.1:7897
gh api repos/Neroued/ninfer                          # has_issues/has_discussions/默认分支
gh api repos/Neroued/ninfer/issues/265               # 状态：open；closed_at:null
gh api repos/Neroued/ninfer/issues/265/comments      # []（0 评论）
gh api repos/Neroued/ninfer/issues/265/timeline      # []（无交叉引用提交/PR）
gh api "repos/Neroued/ninfer/issues?state=all&per_page=100&page=N"   # 全量 370 枚举
gh api repos/Neroued/ninfer/releases                 # 空
gh api repos/Neroued/ninfer/tags                     # 空
gh api "repos/Neroued/ninfer/compare/68c54356...070fa61a"   # dev 23 ahead / 0 behind

# —— 本地 ref 更新（只动远端 ref，不动工作树/HEAD）——
git -C /d/ninfer/ninfer -c http.proxy=http://127.0.0.1:7897 fetch --all --prune
git -C /d/ninfer/ninfer log -1 --format='%H %ad %s' origin/dev

# —— 代码判定 ——
grep -n "case 1:" -A 10 src/ops/softmax_attention/dense/causal_cache/small_t.cu | head -20   # 宽度→WarpsPerCta
grep -n "kMaxVectorColumns" src/ops/linear/gguf/ggml_bridge.h src/ops/linear/gguf/gguf_linear.cpp
sed -n '366,369p' docs/maintainer/qwen3_5-model.md     # 三仓逐字相同的"不施加等值"
sed -n '29,45p'   docs/performance.md                  # 23 构型实测
sed -n '4455,4464p' docs/archive/TODO.md               # "会永久按设计失败"

# —— GGUF/IQ3 只存在于 TAIL ——
grep -rl IQ3_XXS src include | wc -l          # TAIL：8
grep -rl IQ3_XXS /d/ninfer/ninfer/src /d/ninfer/ninfer/include            # 上游：0
grep -rl IQ3_XXS /d/ninfer/ninfer-rtx5090-mobile/src                       # FORK B：0

# —— Fork B 使能改动的溯源 ——
git -C /d/ninfer/ninfer-rtx5090-mobile show --stat 7d566547 d476fafa 114b0fcb dff96dca 1388b7c2 16e12737
git -C /d/ninfer/ninfer-precision-tail cat-file -t 7d566547      # 期望：fatal（不在 TAIL 提交图）
```

---

## 8. 证据与产物清单

| 类别 | 位置 |
|---|---|
| 真机失败原始输出 | `/tmp/run_wp05a.log`；`--no-cuda-graph` 复跑（§3-2026-10-07-8） |
| 上游/TAIL 活体差分 | `/tmp/mtpdiff/`（全部原始 JSON 与日志，§3-2026-10-07-10） |
| 进度落盘 | `kvarn-port-progress.md` §0 快照、未决项 5、§3-2026-10-07-8 / -9 / -10 / **-11** |
| 本文件 | `docs/port-records/PORT-MTP-PARITY-A3-INVESTIGATION.md`（**本次唯一新增产物**） |
| 上游基线 | `master 68c54356`(10-05) / `origin/dev 070fa61a`(10-06)；上游包源码基线 `b06908ba`(10-01 09:52Z) |
| GitHub 查询身份 | `gh` v2.97.0（认证 `qwased`，5000 core 配额，全程未触发限流）；`curl` 8.21.0 `--proxy` |

---

## 9. 处置选项（~~本次不实施~~ **O1 已于 2026-10-07 采纳并回写计划书 v4 附录 D-6**；O2 拒绝、O3 次优）

| 选项 | 内容 | 评价 |
|---|---|---|
| **O1（推荐）** | **A3 判据替换**为：①同配置自确定性（重复运行逐字节相同）②perplexity/质量与同配置基线一致 ③首个分叉下标被记录并文档化；另加 **④"kvarn 不得使 MTP 一致性劣于同配置基线"** 作为 kvarn 专属门禁。同时把 `test_engine_mtp_greedy_parity_real.cpp` 从"通过/失败门禁"改造为**诊断工具**（报各宽度的首个分叉下标、分叉率、perplexity 差） | 与本仓既有书面裁决（§6）一致；**零性能代价**；保留仪器价值；判据从"绝对等值"降到"可证伪且相关" |
| O2 | 把 FORK B 的 canonical per-query 算术面移植进 TAIL，使绝对 parity 真的成立 | 约 5 个核心文件 +380/−723（`small_t.cu` 155/264、`small_t.cuh` 15/97、`small_t_bf16.cuh` 105/110、`causal_softmax_attention.cpp` 95/230、`launch.h` 7/22）**加** `1388b7c2`/`114b0fcb` 的 linear_add/gdn/kvarn 片段约 +1200/−570；与 TAIL 自身的 `small_t_tail.cuh`/`_shadow.cuh`/`small_t_bf16.cuh` **宽度特化改动正面冲突，需重算而非合并**；**性能代价未知**；且上游明确不欢迎（#80/#220）⇒ **不建议** |
| O3 | 直接删除该测试，仅依赖 `docs/performance.md` 的既有实测 | 丢掉唯一能逐宽度定位分叉的仪器；对 kvarn WP 系列的"劣于基线"对照无用 ⇒ 次于 O1 |

**共同前提**：无论选哪项，**A3 都不应再作为 kvarn 移植的阻塞门禁**（C9）。

---

## 10. 如实交代的边界

1. **未跑 GPU**：本轮无新增真机测量。`--draft-tokens` 各宽度在**本机 IQ3_XXS + bf16** 下的分叉率、以及分叉后文本的 perplexity 差**未实测**；"12 位有效数字不变"引自本仓 `docs/performance.md:43-45` 的**既有实测**（2026-09-09，另一批构型），不能当作本轮数据。
2. **§4 的机制判定是静态代码结论**，未经运行时确认：没有用 Nsight/日志证明宽度 2 与宽度 1 实际各进了哪个 kernel，也没有做"手工把 `case 2` 改成 `<2,2>` 后 k=1 是否转为全等"这一**决定性实验**。**这是本结论最大的软肋**——若要把 C5 升为定论，需要该实验（属代码改动，需授权）。
3. **#265 的 nvfp4 路径未在本机复现**（无 NVFP4 产物），其对 kv 格式适用性仅由 §3 的代码门控推断。
4. **O2 的可行性把握不足**：FORK B 无 GGUF/IQ3 代码，其 canonical profile **从未在 IQ3_XXS 上验证**；移植后能否让本测试转绿**未知**。
5. **未做任何网络写操作**：没有向上游提交/评论 issue，没有 fork、没有 push。`git fetch` 只更新远端 ref。
6. **引文与行号的复核程度**：GitHub 侧引文可在 §7 命令处复核（REST/`gh`）。本地引文中，下列各条**本轮已逐字直读原文复核**：
   §5.4 与 C6 的全部合同句、§6 的全部实测句（`docs/performance.md:29-45`、`docs/serving.md:82`、
   `docs/archive/TODO.md:4455-4464`）、§3 与 §4 的全部代码行（含 `small_t.cu:212/:271-274/:66-86/:114-131/:383-395`、
   `small_t_bf16.cuh:29-30`、`execution/text.cpp:571/:965/:1020-1022/:1036`、
   `causal_cache/causal_softmax_attention.cpp:392/:394/:42-45`、`gdn_input_proj.cpp:88-93/:603/:1336`、
   `gguf_linear.cpp:98/:183`、`ggml_bridge.h:49`、`target_verification.cpp:14`）、
   以及 §5.2 的"6 个 Fork B 提交不在 TAIL 提交图"（`git cat-file -t` 实测 6 × `fatal: Not a valid object name`，
   `a7818988` 返回 `commit`）。
   **未逐条重读**的是：§4 末「无可拨开关」各读取点行号、§5.1 的每文件行数分解、§2 各 issue 正文细节
   （来自子代理检索，编号与日期已核）。行号会随工作树漂移，引用前建议按 §7 命令重取。
7. **char 与 token 下标不可直接换算**：活体差分的 char 538（≈token 138）与测试的 token 91 来自不同 prompt/采样基准，**承重的只有"上游同样分叉且逐字节相同"这一差分结论**。
8. 上游代码搜索 API（`search/code`）对本 token 全部返回 `total_count: 0`（该仓未被代码索引），文件存在性改用 Git Trees API 核实 ⇒ GitHub 侧"某文件不存在"类结论的依据是 Trees API，不是代码搜索。
