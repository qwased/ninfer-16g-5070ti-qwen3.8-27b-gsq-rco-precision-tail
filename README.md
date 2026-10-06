# NInfer 16G · Qwen3.8-27B GSQ-RCO · KV 精度尾巴移植

**简体中文** | [English](README.en.md)

**RTX 5070 Ti / RTX 5080 / RTX 5090 · Windows 16GB · 本地推理**

本仓库 = **RTX 50 系列 Windows 16GB 的 NInfer 成品分支**（Qwen3.8-27B GSQ-RCO Q3、CUDA 13 Native 引擎、托盘管理器）**+ 已完成的「KV 精度尾巴（precision tail）」移植**。

本 README 记录本次**移植工作**的成果；产品安装、下载与部署请看 [RTX 5070 Ti Windows 指南](docs/rtx-5070ti-windows.md)（[English](docs/rtx-5070ti-windows.en.md)）与[下载说明](docs/rtx-5070ti-windows-downloads.md)。

- 本仓库：**[qwased/ninfer-16g-5070ti-qwen3.8-27b-gsq-rco-precision-tail](https://github.com/qwased/ninfer-16g-5070ti-qwen3.8-27b-gsq-rco-precision-tail)**
- 直接上游（本分支的基线）：**[Ryan-gsq/ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco](https://github.com/Ryan-gsq/ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco)**
- 上游汇总仓库：**[iamwavecut/ninfer-all](https://github.com/iamwavecut/ninfer-all)**；原始推理引擎：**[Neroued/ninfer](https://github.com/Neroued/ninfer)**

---

## TL;DR

- 把 beellama.cpp 的 **KV 精度尾巴（KVCPT，`--kv-tail-tokens`）** 移植进 NInfer：给每个序列**最近 N 个 token 保留一份精确（F16）K/V 影子环**，attention 时把量化 body 与精确 tail **各算一次 FP32 partial，再做 online-softmax 合并**。
- **零回退**：验收 64/64 格输出**逐字节相同**（含 MTP 投机、视觉、ctx 8192/32768、8 种存储），**显存逐位可复现**，decode **0/64** 格超过 2%。
- **收益已量化**：decode-width KLD 在 **24/24** 格下降，收益随 body 变粗**单调递增**（`int8` 1.02–1.09× < `rk8v4` 2.04–2.41× < `rk4v4` 2.26–3.14× < `rk4v4-e8` 2.97–5.01×）。
- 代价：N=1024、C=1 时**每序列 64 MiB** + 4 MiB 回滚预留；tail 打开时 decode 约 **−6%**（指示值），移植本身（tail 默认关闭）decode 中位 **−0.18%**。
- 过程中发现并修复了**两个环写入缺陷**和一个环形容量越界守卫——它们是"收益一开始测不出来"的真正原因。见 §5。

## 1. 精度尾巴是什么

量化 KV cache 里最新的若干行被量化误差污染，而 attention 对**最近的 token 最敏感**。精度尾巴把"最近 N 个 token"的 K/V **不量化**保存进一个**设备端精确池**（影子环），attention 时：

1. **body partial** —— 对量化 K/V 正常跑一遍 attention，得到 `(acc_b, m_b, l_b)`；
2. **tail partial** —— 对精确 K/V 再跑一遍，得到 `(acc_t, m_t, l_t)`；
3. **FP32 合并** ——

```
g   = max(m_b, m_t)
acc = acc_b·exp(m_b − g) + acc_t·exp(m_t − g)
l   = l_b  ·exp(m_b − g) + l_t  ·exp(m_t − g)
out = acc / l
```

合并严格对齐上游 `fattn-tail.cuh`：**不是各自归一后相加**，而是两份未归一 partial 的 online-softmax 合并；两侧都在 **FP32 域**完成（body 的量化域在 body FA 内已还原为 FP32 并归一）。

关键设计取舍（决定了零回退）：

- **不新增 Op 家族、不改 CUDA Graph family 序列**。tail 合并放在既有 **small-T 家族内部**：会话内的 family 序列（prompt → small-T）与不开 tail 时完全相同，因此没有额外 graph 重捕获。`retention_tokens`（即 N）进引擎身份 key，而**每个 query 的动态窗口只作为运行时输入**。
- **写入采用 fused 双写**：在写量化 body 的**同一个 kernel** 内，把未量化的 K/V 原值再写一份进精确环（一次读、无双份量化）；同一 ubatch 内重复命中同一 slot 时按"最后一次写入获胜"去重。
- **精确池只放 Device**，不参与 host/disk tier；draft（MTP）缓存**按构造不含 tail**，所以 `--kv-tail-tokens` 只让*验证器*变准。

## 2. 支持范围与显存代价

| 项 | 取值 |
|---|---|
| 合并生效的存储 | `bf16` 与 **INT8 族**：`int8`、`rk8v4`、`rk4v4`、`rk4v4-e8`、`rk2v4-e8` |
| 分配但**惰性**的存储 | `fp8`、`nvfp4`、`k8v4`（解码后的 key plane 处于旋转坐标系，未实现尾巴行旋转；开 tail 与关 tail 的字节完全一致） |
| 生效路由 | **small-T decode 路由**（query 宽度 ≤ 8）。prompt/prefill 路由**只写不合并** |
| 环元素类型 | `--kv-tail-type f16`（默认，10 位尾数）或 `bf16`（7 位尾数，链路验证形态）；两者同为 16-bit，池大小/页几何/`MemorySummary` 完全相同 |
| 精确环常驻 | `round_up(N,64) × 65,536 × C` 字节（C = `--max-concurrency`，1..8） |

| N | C=1 | C=2 | C=4 | C=8 |
|---:|---:|---:|---:|---:|
| 512 | 32 MiB | 64 MiB | 128 MiB | 256 MiB |
| 1024 | **64 MiB** | 128 MiB | 256 MiB | 512 MiB |
| 2048 | 128 MiB | 256 MiB | 512 MiB | 1024 MiB |

实测（27B 成品，C=1，N=1024）：`kv_exact_history_bytes` = 67,108,864 B（**64 MiB 整**，与模型预测 **0% 误差**），`kv_rollback_reserve_bytes` = 4,194,304 B（单页 4 MiB），`runtime_reservation_bytes` 净增 **68 MiB**。

## 3. 新增接口

| 入口 | 选项 | 说明 | 默认 |
|---|---|---|---|
| `ninfer` / `ninfer-serve` | `--kv-tail-tokens N` | 保留每序列最近 `N` 个 token 为未量化精确 K/V；`0` 关闭 | `0` |
| `ninfer` / `ninfer-serve` | `--kv-tail-type bf16\|f16` | 精确环元素类型 | `f16` |
| `ninfer-perplexity` | `--score-width W` | 以宽度 `W` 的 attention query tile 评分；`W ≤ 8` 才驱动 small-T 路由、让 tail 的开关差异可见 | `1024` |
| `ninfer-perplexity` | `--save-topk <path>` / `--kld-base <path>` | 把本次的逐目标 next-token 分布存为 KLD 参考 / 载入参考并报告 KLD（top-K 通道，对齐 llama.cpp） | — |

引擎身份 tag 增加了 tail 维度（否则不同 tail 配置会共用同一 engine 身份）；`config-calculator.html` 与启动期容量规划都按并发放进了 tail 项。

## 4. 验证结果

验收报告全文：**[PORT-VERIFY-REPORT.zh.md](docs/port-records/PORT-VERIFY-REPORT.zh.md)**（[English](docs/port-records/PORT-VERIFY-REPORT.en.md)）。原则是"**任何结论都必须有一条能复现它的命令**"。

- 硬件：RTX 5070 Ti 16 GB，`sm_120a`，CUDA 13.3，空闲基线 48 MiB
- 模型：`Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`（10.33 GiB）
- 对照：移植前成品引擎（`ninfer-package`，仅 serve）vs 移植后 `build-port` 引擎
- 纪律：GPU 严格串行单所有者；每片跑完 `nvidia-smi` 必须回到 48 MiB

### 4.1 Arm A —— 无负面影响（移植前 vs 移植后）

| 配置 | 格数 | 一致性 |
|---|:--:|---|
| MTP 关 × 视觉{关,开} × ctx{8192,32768} | 32 | **32/32 IDENTICAL** |
| MTP 开 `--draft-tokens 2` × 视觉{关,开} × ctx{8192,32768} | 32 | **32/32 IDENTICAL** |
| **合计（8 种存储）** | **64** | **64/64 IDENTICAL，0 DIFFERS，0 MISSING** |

| 指标 | 结果 |
|---|---|
| **显存** | **64/64 格逐位相同** → 精确通过 |
| **decode Δ%** | 中位 **−0.18**，范围 −0.71…+0.15，**0/64 超过 2%** → 通过 |
| prefill Δ% | 中位 −1.25，离散度 −28.1…+20.4 → **不可判别**（同二进制重跑的离散度大于前后差异，见报告 §3.2） |

补充观察：MTP 开与关的输出**逐字节相同**（贪心验证器精确）；视觉格确实送入图像（提示词 1,214 vs 188 token）且仍逐字节相同；同二进制重跑 16/16 逐字节相同，即跨进程贪心解码自稳定。

### 4.2 Arm B —— 尾巴收益（decode-width KLD，W=8，评分 32,767 token）

参考 = 每 ctx 的 `bf16`-tail0 top-K 100。**收益倍率（tail0 / tailN 的 mean KLD）：**

| ctx | tail | int8 | rk8v4 | rk4v4 | rk4v4-e8 |
|---|---:|---:|---:|---:|---:|
| 8192 | 1024 | 1.09× | 2.20× | 2.47× | 3.25× |
| 8192 | 2048 | 1.09× | 2.30× | 2.87× | 4.04× |
| 8192 | 4096 | 1.05× | 2.41× | 3.14× | 5.01× |
| 32768 | 1024 | 1.02× | 2.04× | 2.26× | 2.97× |
| 32768 | 2048 | 1.03× | 2.18× | 2.53× | 3.70× |
| 32768 | 4096 | 1.05× | 2.27× | 2.87× | 4.46× |

| # | 子判据 | 结果 |
|---|---|---|
| B1 | 每格 `mean KLD(tailN) ≤ mean KLD(tail0)` | **通过 — 0 违反 / 24** |
| — | `same_top(tailN) ≥ same_top(tail0) − 0.002` | **通过 — 0 违反 / 24**（每格都上升） |
| B2 | `ppl(tailN) ≤ ppl(tail0)` | **22/24** — 2 例例外（int8/ctx8192：t2048 +0.050%、t4096 +0.002%，噪声量级） |
| B3 | 收益随 body 粗细单调 `int8 ≤ rk8v4 ≤ rk4v4 ≤ rk4v4-e8` | **6/6 (ctx × N) 组合通过** |

尾巴同时大幅压低最坏误差：`rk4v4-e8` ctx 8192 的 KLD 最大 **3.38 → 0.92 / 0.33 / 0.74**。

### 4.3 Arm B3 —— MTP 接受率（仅移植后）

| 场景 | 结论 |
|---|---|
| **尾巴关闭** | 猜测路径**完全未受移植影响**：与移植前成品、与 MTP 关的输出都逐字节相同 |
| 整窗、短提示词（4× 样本） | `rk8v4` 在 ±2 点内并略正（+0.7/+0.8 点）；最粗的 `rk4v4-e8` 收敛到 **−2.05/−1.92 点**（±2 点边界，已到样本分辨极限） |
| **长提示词（生产 body+tail 场景）** | 8 格中 6 格为正，幅度 **+2 至 +7 点**（两个负值都很小：−1.31、−1.84 点） |

机理：草稿缓存 `mtp_kv` **按构造不含 tail**，`--kv-tail-tokens` 只让**验证器**变准；一致度变化幅度随 body 变粗放大——实测正是这一形态。

### 4.4 A4 —— 尾巴关闭的一致性与 FP32 oracle

`run-oracle.bat` → `softmax_attention: PASS`，`ORACLE_EXIT=0`（94 秒）。全部守卫在场：`fused-append empty-body cache write` ×4、`fused-append crossing build` ×4、`prompt-route ring write` ×8、`fused-append chunked ring write` ×2、`PATHPT` ×2、`TAILGAIN` ×12、`WIDETAIL` ×8、`graph family=` ×8。`tail = 0` 的逐位一致与 graph family 稳定性都在同一次运行内被断言。

### 4.5 结论一览

| 论点 | 结果 |
|---|---|
| **C1a** 移植未改变生成结果（MTP 关与开） | **通过** — 64/64 逐字节相同 |
| **C1b** 性能与显存无回退 | **显存精确通过**；**decode 通过**；prefill **不可判别** |
| **C1c** 视觉理解输出未变 | **通过** — 全部 `--vision` 格逐字节相同 |
| **C2** 尾巴带来显著质量收益 | **通过** — decode-width KLD 24/24 改善，随 body 粗细单调 |
| **C3** MTP 猜测仍可用、未被拖累 | **通过** — 关尾无变化；长上下文为正收益 |
| **A4** 尾巴关闭即移植前路径 | **通过** — `ORACLE_EXIT=0` |

## 5. 移植中发现并修复的写入缺陷

收益一开始**测不出来**，根因不在合并、而在**环的写入侧**——两个真正的缺陷加一个容量守卫，都是"只有跑起来才能发现"的：

1. **fused-append 的分片在 `body_window == 0` 时从不量化尾部行**（`56fc8384`）。当精确尾巴覆盖整个窗口时，分区把 `body_active = 0`，每个分片都在 fused-append 块之前返回，于是这些行**从未进入量化缓存**；之后 `window > N` 的 body 会读到一个永久空洞。
2. **`Prompt` 路由从不写环**（`b99ba8d5`）。prompt 路由只"写"，但它根本没写影子环，导致后续 small-T 步骤读不到。
3. **环形写入越界守卫**（`205ea411`）：把精确环的写入限制在其容量之内。

修复后：`bf16` body+tail 对 `bf16` 参考的 mean KLD 从 **0.13284577 → 0.00093566（−142×）**，最大 16.29 → 0.475，`same_top` 0.9004 → 0.9867；同一协议下 `rk8v4` 格在修复前后**逐位相同**，`bf16` tail-off 对照仍为 KLD 0 / `same_top` 1.0——即修复**只影响 tail 打开**的情形。新增守卫测试：`run_fused_empty_body_append_case`、`run_fused_crossing_case`、`run_prompt_ring_write_case`、`run_fused_chunked_ring_case`。

## 6. 里程碑与工作包

| 里程碑 | 状态 | 内容 |
|---|---|---|
| **M0** KVarN 决策 | DONE | 同模型对照（`llamacpp` vs `ninfer-package`，IQ3_XXS）：本机切片上 KVarN 相对 f16/q8_0 无可测 ppl 惩罚，亦无质量上的借用理由；测试基线钉死 `sm_120a / 5070 Ti` |
| **M1** 静态 BF16 tail 功能闭环 | DONE | FP32 oracle 对 BF16 + INT8 族（fused/cached）与 batched masked 全绿；显存与模型 **0% 误差** |
| **M2** F16 默认 + 图形稳定 | DONE | 环元素类型成为配置维度（`--kv-tail-type`）；F16 在全部 `WIDETAIL` 用例不低于 BF16，**取为默认** |
| **M3** 并发与投机 | DONE | `payload_bytes == round_up(N,64)*65,536*C + C*4 MiB` 对 C=1..8 精确成立；draft 缓存按构造无 tail；有/无投机端到端复测无回归 |
| **M4** 可选 tier（host/disk） | 不在范围 | — |
| **M5** 尾巴收益 + MTP 影响 | DONE | 建立仪器（`--score-width` + KLD），找到并修复 §5 的两个缺陷，收益全矩阵成立 |

工作包 WP1–WP7、WP9、WP10 全部 DONE；**WP8（事务/回滚）为有意推迟**（M1 功能闭环为 device-only、C=1，任何 failure 路径都未被触达），其回滚预留 `R` 已在 WP5 的容量核算里就位。

## 7. 用法示例

```bash
# 服务端 / CLI 开启尾巴（默认 f16 环，N=1024）
ninfer-serve models/qwen3_8_27b.ninfer --kv-dtype rk8v4 --kv-tail-tokens 1024
ninfer       models/qwen3_8_27b.ninfer --kv-dtype rk4v4-e8 --kv-tail-tokens 1024 --spec mtp --draft-tokens 2

# 离线：decode-width 评分 + KLD（对齐 llama.cpp 的 top-K 通道）
ninfer-perplexity models/qwen3_8_27b.ninfer --kv-dtype bf16     --kv-tail-tokens 0    --save-topk out/bf16-t0.topk
ninfer-perplexity models/qwen3_8_27b.ninfer --kv-dtype rk4v4-e8 --kv-tail-tokens 1024 --kld-base out/bf16-t0.topk --score-width 8
```

复现整套验收的命令见 [PORT-VERIFY-REPORT.zh.md §8](docs/port-records/PORT-VERIFY-REPORT.zh.md)。

## 8. 已知缺口（如实交代）

- **B2 ppl**：24 格中 2 格不成立（`int8`/ctx 8192，+0.050%/+0.002%），属噪声量级；同样这两格在 KLD 与 `same_top` 上均改善。`int8` 收益最小，其 ppl 变动低于 32,767 token 困惑度的分辨力。
- **B3 短提示词的 ±2 点判据**已达样本分辨极限；单次 256 token 下的 −5.47 点在 4× 样本下未复现（−2.05 点）。
- **B3 的 ctx 轴在短提示词下是空的**——该参数改变分配量而非工作量；真正跑上下文长度需用 `--messages FILE`。
- **prefill ≤2%** 在这些提示词长度下无法用"每格一次请求"评判，已记"不可判别"；decode（稳态）稳定。
- **64K 上下文**不在范围：本地唯一语料切片约 32.7K token。
- **数值底**：`bf16` body + tail 依赖约 1e-3 的 mean-KLD 数值底（一个 bf16 ULP），这也是 `bf16` 只能做 Arm B 参考、不做候选的原因。
- **性能特征化未完成**：body+tail 双写的 `kv_cache_append_bench` 扫描与更长的 decode 基准仍是计划 §5 的待办；§4.3 的 −6% 只是 24-token 的指示值。
- **`fp8` / `nvfp4` / `k8v4`** 上的 tail 为惰性（设计如此），DoD §7.3 已按"tail 可生效的存储"收窄。

## 9. 代码规模与文档索引

相对直接上游 `b06908ba`：**116 个提交**，**69 个文件**，**+10,920 / −191** 行。

核心新增/改动：

| 区域 | 文件 |
|---|---|
| 精确环元素泛型 | `src/ops/common/kv_tail_element.cuh` |
| tail partial kernel | `src/ops/softmax_attention/dense/causal_cache/small_t_tail.cuh` |
| fused 双写（shadow） | `.../causal_cache/small_t_tail_shadow.cuh` |
| small-T 家族分片/合并与环写入 | `.../causal_cache/small_t*.cuh`、`prompt.cu`、`small_t.cu` |
| 配置链 | `apps/cli/{main,options}.{cpp,h}`、`src/serve/serve_options.{cpp,h}`、`include/ninfer/types.h`、`src/runtime/engine/` |
| 评分仪器 | `apps/perplexity/{main,evaluation}.{cpp,h}`、`include/ninfer/ops/target_logprobs.h`、`src/ops/kernel/target_logprobs.cuh` |
| 容量/显存 | `src/core/paged_kv_cache.{cpp,h}`、`src/models/qwen3_5/program/planning/startup.{cpp,h}` |
| 测试 | `tests/models/qwen3_5/test_exact_tail_capacity.cpp`（新增）、`tests/ops/softmax_attention/causal_cache.cpp`、`tests/test_perplexity_evaluation.cpp` |

文档索引（已完成的 port 记录归档在 [docs/port-records/](docs/port-records/)；当前实施计划为 [kvarn-port-into-precision-tail-plan.md](kvarn-port-into-precision-tail-plan.md)）：

| 文档 | 内容 |
|---|---|
| [PORT-VERIFY-REPORT.zh.md](docs/port-records/PORT-VERIFY-REPORT.zh.md) / [.en.md](docs/port-records/PORT-VERIFY-REPORT.en.md) | **验收报告**（本文 §4 的全文与复现命令） |
| [PORT-DOD.md](docs/port-records/PORT-DOD.md) | DoD / 里程碑 / 工作包审计表（含 V0–V7 验收行） |
| [PORT-JOURNAL.md](docs/port-records/PORT-JOURNAL.md) | 按顺序的执行日志（含命令与实测值） |
| [PORT-MEMORY.md](docs/port-records/PORT-MEMORY.md) | 可复用经验与陷阱（harness、kill 语义、就绪判据等） |
| [PORT-M5-PLAN.md](docs/port-records/PORT-M5-PLAN.md) / [PORT-REVIEW-PLAN.md](docs/port-records/PORT-REVIEW-PLAN.md) / [PORT-VERIFY-PLAN.md](docs/port-records/PORT-VERIFY-PLAN.md) | M5 收益战役、独立代码评审、验收计划 |
| [PORT-BEELLAMA-SPEC.md](docs/port-records/PORT-BEELLAMA-SPEC.md) | 移植算法参考（只搬算法、不搬代码） |
| [precision-tail-port-plan.md](docs/port-records/precision-tail-port-plan.md) | 实施计划（设计 + WBS + 里程碑 + 验证矩阵） |
| [kvarn-kv-tail-feasibility-report.md](docs/port-records/kvarn-kv-tail-feasibility-report.md) | 可行性报告 |
| [docs/performance.md](docs/performance.md) §"KV precision tail" | 已发布的实测（显存 / ppl / F16-vs-BF16 / decode-width KLD） |

## 10. 快速开始、下载与许可

产品安装、Windows 部署、参数与模型转换请看 **[RTX 5070 Ti Windows 指南](docs/rtx-5070ti-windows.md)**；逐项可调参数（含精度尾巴的显存与收益）见**[可调参数说明书](docs/参数说明书.md)**；预编译引擎与配套 `.ninfer` 模型成品见**[下载说明](docs/rtx-5070ti-windows-downloads.md)**（**[夸克网盘](https://pan.quark.cn/s/28b896c4b0c0)**）。构建方式见 [AGENTS.md](AGENTS.md) 与[构建系统](docs/maintainer/build-system.md)。

本仓库的 Windows / RTX 5070 Ti 构建、显存策略与管理器来自直接上游 [Ryan-gsq](https://github.com/Ryan-gsq/ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco)；上游汇总线来自 [iamwavecut/ninfer-all](https://github.com/iamwavecut/ninfer-all)，原始引擎来自 [Neroued/ninfer](https://github.com/Neroued/ninfer)，各项改动保留原作者署名（[维护者与改动对应表](docs/maintainer/consolidated-line.md)）。精度尾巴算法移植自 beellama.cpp 的 KVCPT（**只搬算法、不搬代码**），参考 [PORT-BEELLAMA-SPEC.md](docs/port-records/PORT-BEELLAMA-SPEC.md)。

许可证见 [LICENSE](LICENSE)。
