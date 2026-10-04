# ninfer 移植 KV cache precision tail 实施计划

- 计划日期：2026-10-05
- 对象：`D:\ninfer\ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco`（下称 ninfer）
- 依据：`kvarn-kv-tail-feasibility-report.md`（v2）第 5.2 / 5.3 / 6 节
- 性质：实施计划（设计 + 工作包 + 里程碑 + 验证），尚未改动任何源文件
- 来源参考：beellama.cpp v0.4.7 的 KVCPT（`--kv-tail-tokens`），工作树 HEAD `58a162927`
- 基准装配：两个成品目录（`D:\ninfer\llamacpp` 与 `D:\ninfer\ninfer-package`）以同一模型 IQ3_XXS 对照，**llamacpp 侧不开启 MTP**，见 §0.5

> **一句话**：tail = 给"最近 N 个 attention 可见 token"保留一份精确（BF16）的 K/V 影子池，attention 时把 body（量化）与 tail（精确）各算一次 FP32 partial 再合并。ninfer 已有 partial reducer 与多异构池先例，最小形态可**不新增 Op 家族**地落地。

---

## 0. 目标与非目标

### 目标
1. 在 ninfer 上以"质量档"形式提供 precision tail，对现有 `--kv-dtype`（rk8v4 / rk4v4 / int8 …）任一量化 body 生效。
2. 显存代价可控且**可核算**（进入 `MemorySummary` 与 `config-calculator`）。
3. decode 性能代价落在噪声带内；不改量化 body 的 plane schema 与不变量。
4. 与 KVarN 解耦：body 用 ninfer 现有格式，不引入任何 beellama ggml/kernel/state。

### 非目标（本计划不做）
- KVarN 本体、Sinkhorn 归一化、`KVRN` state、多 GPU placement（见报告 §5.1）。
- bodyless（tail 覆盖整窗时省 body）——ninfer full-attention 窗口 262144，无价值（报告 §6 Phase 3）。
- Metal/HIP 等后端；本计划只覆盖 CUDA（sm_86 / sm_89 / sm_120a）。
- MLA/DSA 或任何 latent cache（ninfer 当前模型族不涉及，但 Op 层需 fail closed）。

---

## 0.5 统一基准：两个成品的同模型对比（本次测试装配）

本计划 M0 的 KVarN 收口实验与 M1 的质量验证**都在同一套装配**上做：两个原项目各自的**成品目录**，用**同一个模型**（Qwen3.8-27B GSQ-RCO **IQ3_XXS** + MTP 头）互相对照。

### 成品与基准模型

| 侧 | 成品目录 | 入口 | 基准模型文件 |
|---|---|---|---|
| **llamacpp**（llama.cpp 系成品，含 KVarN + KVCPT） | `D:\ninfer\llamacpp` | `llama-server.exe` / `llama-cli.exe` / `llama-perplexity.exe` / `llama-bench.exe` | `model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-mtp.gguf`（10.44 GB） |
| **ninfer** | `D:\ninfer\ninfer-package` | `engine\ninfer-serve.exe`（`NInferManager.exe` 为托盘/管理器） | `model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`（11.09 GB） |

- 两侧同为 **IQ3_XXS**、同带 MTP 头，仅容器格式不同（`.gguf` / `.ninfer`），**视为同一模型**，可作为同模型对照基准。
- 包装差异须知：ninfer 侧文件名含 `vision-bf16`（视觉塔以 bf16 随包），llamacpp 侧视觉塔是独立 `mmproj`。**纯文本质量对照不受影响**；一旦涉及图像，需另按 `docs/rtx-5070ti-windows.md` 说明。

### 本机硬件基线（以本机为基准）

- **GPU：RTX 5070 Ti 16 GB**（16275 MiB，compute capability 12.0 → **sm_120a**），CUDA 13.x。所有数字**只在本机测得**，不外推到 3090/5090。
- 两侧成品均为 **sm_120a 构建**：llamacpp 侧为其 `build-win-cuda-sm_120`（handover 记录 `CUDA0 RTX 5070 Ti, compute capability 12.0`）；ninfer 侧为 `Native SM120a Release` 引擎（`docs/rtx-5070ti-windows.md:87`）。
- **因此报告 §0 的"目标 ABI 矛盾"（`AGENTS.md` sm_86/3090 vs `README.md` sm_120a/5070 Ti）在本次测试中不再构成阻塞**：测试基线钉死 `sm_120a / 5070 Ti`。仓库内部文档矛盾仍建议后续单独修正，但不影响本计划。
- **对 tail dtype 的影响**：本机（5070 Ti）上 beellama 实测 **F16 tail 明显优于 BF16**（报告 §4.1），故 tail 的**目标默认取 F16**；BF16 仅作为"复用 ninfer 现有 `small_t_bf16.cuh` 路由、最快打通合并链路"的起步形态（见 §1.1），不作为 5070 Ti 上的最终默认。

### 硬约束

1. **llamacpp 侧一律不开启 MTP**（用户明示：其 MTP 草稿-验证机制会导致质量损失）。
   - 质量测量（`llama-perplexity.exe`）本身不做投机，天然不含 MTP。
   - 速度/端到端测量（`llama-server.exe` / `llama-cli.exe`）必须显式 **`--spec-type none`**，不要传 `--spec-type draft-mtp`。
   - 基准模型只有带 `-mtp` 头的 `...IQ3_XXS-mtp.gguf`（本目录**没有**不带 MTP 的 IQ3_XXS）；其 nextn 权重在 `--spec-type none` 时**不装载**，故 baseline 就用该文件 + `--spec-type none` 即可，无需另找无 MTP 文件。
2. **ninfer 侧同时测"有投机 / 无投机"两态**（用户明示）：profiles 默认 `--spec mtp --draft-tokens 4`（见 `config/profiles/gsq-vision-rk8v4-120k.json`），另跑一态**显式关投机**。
   - **质量**对照以"无投机"态为主口径（隔离 KV/tail 效果）；"有投机"态也测并可交叉报告。
   - **速度/显存**两态都给，便于对比 MTP 的收益与代价。
   - llamacpp 侧**只有无投机一种**（MTP 被禁），故跨产品比较只在"两侧都无投机"这一态上做。
3. **同口径**：同一 corpus、同一 `-c` / 批大小、同一指标（KLD 或 ppl），指标在 **M0 先定死**。llamacpp 的 `llama-perplexity` 支持 KLD；ninfer 侧走其 perplexity 通道（源树 `apps/perplexity`，`docs/perplexity.md:23` 已覆盖 7 种格式）。**不得**拿一侧 KLD 对另一侧 ppl（报告 §3.2 缺口）。
4. **串行、单流、独占 GPU**：llamacpp 侧沿用其成品目录既有纪律（单拥有者锁、`-np 1`、`-lm none`）；ninfer 侧 `--max-concurrency 1`、`--device-snapshot-slots 1`。
5. **联网检索走代理**：需要查资料时用 `http://127.0.0.1:7897`。
6. **可复现记录**：两成品的 exe 哈希、模型文件哈希、完整命令行、`-c`/批大小、corpus、指标数值，一并入库。
7. **许可证不在本次评估范围**（用户指示）：不作为门槛、不列入 DoD，也不再做 derived-from / 上游 commit 的登记要求。

### 测试矩阵（本次）

| 目的 | 侧 | 档位 | 投机 | tail |
|---|---|---|---|---|
| KVarN 收口（M0） | ninfer | `rk4v4` | on / off | 0 / 1024 |
| KVarN 收口（M0） | llamacpp | `kvarn4` | **off** | 1024 |
| **tail 机制验证（M1）** | ninfer | **`rk8v4`、`rk4v4-e8`、`nvfp4` 三档** | on / off | 0 / N（默认 1024） |
| tail 参照系 | llamacpp | `kvarn4`+tail | **off** | 1024 |

- **ninfer 侧测尾部精度机制固定这三档 body：`rk8v4` / `rk4v4-e8` / `nvfp4`**（用户指定）。三档分别记 `tail=0` 与 `tail=N` 的 ppl 与显存，得到"每档 tail 收益"曲线。
- 档位别名以 ninfer CLI 为准：`--kv-dtype rk8v4|rk4v4-e8|nvfp4`（`docs/cli.md:251` 列 8 个已接受值，`rk4v4-e8` 是 E8 格点 INT4 key，与 `rk4v4`（Lloyd-Max）**不同格式**，勿混）。

### 与 tail 计划的关系

- **M0 收口实验**：`ninfer rk4v4`（有/无投机）× `tail 0/1024` 对比 `llamacpp kvarn4+tail`（`--spec-type none`），统一指标。
- **M1 质量验证**：ninfer 侧在上述 **三档 body（rk8v4 / rk4v4-e8 / nvfp4）** 上各自 `tail=0` vs `tail=N`，同一 `.ninfer`、无投机为主口径。
- **显存/速度**：两侧各自以 `-c` 夹逼 + 日志 buffer 行取数（llamacpp 侧 `KVarN tail buffer size` 恒定 64.25 MiB @ tail 1024 可作 §2 内存模型的旁证）。

---

## 1. 设计总览（最小可用形态）

### 1.1 数据模型：第二个同几何 exact 池
- 新增一个**独立页池**，`KvCacheStorage::BFloat16`、`PagedKVPlaneOrder::HeadMajor`、2 plane（K/V），几何与主池一致（D256、GQA [256,24,4] / [256,16,2]、page = 64）。
- 每个序列一份 arena，容量固定为 `round_up(N, 64) + rollback_reserve`，**不随 decode 前移改变**（这是 CUDA Graph 稳定的前提，见 §1.5）。
- 先例：`src/models/qwen3_5/program/planning/startup.cpp:253-286` 的 DFlash full-attention 池（硬编码 `BFloat16` + `HeadMajor` + 2 plane）与 `decoder_state.h:114,128` 的 `mtp_kv` 第二池。**不需要给页引入"精度"维度**，从而绕开最难的设计冲突。

**tail dtype 选择（本机基线已定）**：测试/目标硬件固定为 **RTX 5070 Ti / sm_120a**（§0.5），而 beellama 在**同一张 5070 Ti** 上实测 **F16 tail 明显优于 BF16**（报告 §4.1：同 tail 同 depth，bf16 pp 低 7–11%），故 **tail 的目标默认定 F16**。
- **M1 起步形态**仍先用 BF16：ninfer 的 attention Op 契约（`include/ninfer/ops/softmax_attention.h`）已声明 q/k/v/out 为 BF16，且已有 `small_t_bf16.cuh`（其第 7 行注释即 "a reducer combines FP32 split-local partials"），BF16 tail 可直接复用该路由，是**最快打通合并链路**的形态（只验证"body+tail 双 partial 合并"这一件事）。
- **M2 落 F16**：新增 F16 路由后切为默认；M1 的 BF16 仅作链路验证，不作为 5070 Ti 上的最终默认。

### 1.2 写入：fused 双写（在同一算子内）
- 现有写入点：prefill 走 `kv_cache_append_batch_launch`（`src/ops/kv_cache/append/launch.cu:219`），decode 在 small-T kernel 内 fused append，MTP 走显式 `ops::kv_cache_append`（`launch.cu:201`）。
- 扩展为：写量化 body 的**同一 kernel** 内，把未量化的 BF16 K/V 再写一份进 exact 池。参考 beellama 的 `ggml_set_rows_with_shadow`（`src/llama-kv-cache.cpp:3258,3290`）语义：一次读写、无双份量化、无重量化。
- 同一 batch 内重复写同一 slot 需去重（beellama 用 `keep_last_writes`，`src/llama-kv-cache-tail.cpp:12-24`），避免多行竞争同一页。

### 1.3 attention：body 与 tail 各算 partial，FP32 合并
- 现有 Op 契约明确："Op owns no cache allocation, frontier, request identity, or commit authority"（`include/ninfer/ops/softmax_attention.h`）——**因此 tail 池的生命周期、frontier、提交必须在 Program/state 层，不在 Op 内**。
- 合并语义（严格对齐 beellama `ggml/src/ggml-cuda/fattn-tail.cuh:339-389`）：body、tail **各算一次 partial**，用各自的 `(row max m, denominator l)` 做一次 FP32 online-softmax 合并；**不是各自归一后相加**。
- ninfer 的 small-T kernel 已经在写 `partial_acc / partial_m / partial_l`（`src/ops/softmax_attention/dense/causal_cache/small_t_i8.cuh:69` 签名，`:945-953` 写回），reducer 在 `small_t.cuh`（`small_t_i8.cuh:945-953` 与 `small_t_bf16.cuh` 共享）。新增的是**跨 body/tail 两次 partial 的合并**，复用 `causal_attention_small_t_reduce_output_kernel` 的模式。

**设计建议（关键）**：不新增 Op 家族，而是**在 small-T 家族内部**加一个"tailed"分支：调用点仍是 `causal_softmax_attention*`，内部先跑 body partial、再跑 tail partial、再合并。理由见 §1.5。

### 1.4 索引：per-query tail 窗口
- body 用 `block_tables` 页寻址（`src/ops/kernel/paged_kv_address.cuh:47-69`）。tail 同样用一份页表 `tail_tables` 指向 exact 池。
- 对第 b 行、绝对位置 p、tail 长度 N：tail 行集 = `[max(0, p+1-N), p]`，body 行集 = `[0, max(0, p-N)-1]`。因 p 随 query 行不同，需要一个**设备侧按行计算的窗口起点**（不能回读主机）。
- 当 `p+1 <= N` 时 body 窗口为空、tail 覆盖全部——kernel 必须对空窗口保持均匀，避免额外分支/家族切换。

### 1.5 CUDA Graph 契约（本设计的关键洞察）
- 现状：route family 由 `causal_softmax_attention_route_family(...)` 给出（0 small-T / 1 chunked small-T / 2 prompt），"families launch different node sequences, so CUDA Graph planners keep calls whose families differ out of one executable"（`include/ninfer/ops/softmax_attention.h:209,212`）。
- **把 tail 合并**放在 small-T 家族内部 ⇒ **不引入新的 family**，session 内的 family 序列（prompt → small-T）与今天完全相同，**只保留 prefill→decode 这一次已有的切换**。这消掉了报告 §5.3 第 1 条最担心的"tail 边界前移触发 family 反复切换"。
- 代价：prompt（prefill）路由不参与 tail 合并（prefill 时 tail 覆盖全部、body 为空，合并结果等于直接用 tail；为简单起见 M1 可让 prompt 直接读 body+tail 中的 tail，或 prefill 仍写 body 但不合并）。M1 采用"prefill 只写、decode 才合并"的降级，功能正确（prefill 输出走 prompt 路由），质量影响仅限 prompt 内部的 self-attention，可接受。
- 若实测发现必须让 prompt 也合并，则退化为报告 §7 结论 2 的"固定到 page 边界 + 同批双 kernel"形态。

---

## 2. 内存模型（定量，必须进 `MemorySummary`）

- 每 token 每 full-attention 层：`head_dim(256) × kv_heads × 2B × 2(K,V)`。GQA [256,24,4] ⇒ `256×4×2×2 = 4096 B/token/layer`。
- ninfer 27B 实测 16 个 full-attention 层 ⇒ `16 × 4096 = 65,536 B/token`（与 `docs/performance.md:454` 的 bf16 基线一致，交叉验证通过）。
- **exact 池常驻** = `round_up(N,64) × 65,536 × C`（C = `--max-concurrency`，`include/ninfer/types.h:415`，1..8）：

| N | C=1 | C=2 | C=4 | C=8 |
|---:|---:|---:|---:|---:|
| 512 | 32 MiB | 64 MiB | 128 MiB | 256 MiB |
| 1024 | 64 MiB | 128 MiB | 256 MiB | 512 MiB |
| 2048 | 128 MiB | 256 MiB | 512 MiB | 1024 MiB |

- 另需 rollback reserve R（beellama 量级 ~0.86 MiB，`docs/beellama-features.md:575`）与页对齐 padding；上表 +16 MiB/C 即够量级估算。
- **结论**：N=1024、C=1 时 tail 净增 ≈64 MiB，相对 16 GB 卡很小；C=8 时 ≈512 MiB，已不可忽略。因此 tail 必须以"质量档"发布，并**按并发出现在 `config-calculator.html` 与启动期容量规划里**。
- 接入点（改这条链）：`paged_kv_storage.h:51 physical_bytes_per_token_head()` → `src/models/qwen3_5/load.cpp:189-194` → `decoder_state.cpp:191 kv_payload_bytes` → `startup.cpp:347-359` 与 `:1372-1395 SequenceCapacityCurve{bytes_per_additional_main_page_group}` → `src/runtime/engine/kv_capacity.cpp:104,226` → `types.h:1294-1331 MemorySummary`。
- `SequenceCapacityCurve` 现在是**线性单系数**；tail 引入 `(N+R)×C` 常量项，需改为"常量项 + body 线性项"。这是 §3 WP5 的核心。

---

## 3. 工作包（WBS，含文件锚点与改动要点）

> 约定：`[锚点]` = 已核实的现有文件/符号；`[新增]` = 计划新增。

### WP1 存储布局与 exact 池
- `[锚点]` `include/ninfer/types.h:49-73`（`KvCacheStorage`）、`src/core/paged_kv_storage.h:58-121`（`paged_kv_storage_layout()`，9 case）、`src/core/paged_kv_cache.h:22,34`（`PagedKVLayerView` / `PagedKVBatchLayerView`）。
- `[新增]` tail 池规格：在 `DeviceKVPagePoolSpec` 之外增加"tail 池"二等公民（或复用同一 spec + 标记），仍为 `BFloat16`/2 plane，**不改 plane schema**。
- `[新增]` `PagedKVLayerView` 增加可选 tail 分量（`tail_k_pages/tail_v_pages/tail_block_table/tail_len`），或定义配套的 `TailedPagedKVLayerView`。**注意**输入按值传递（见 1.3 的 Op 契约），不改别名/不重叠规则。

### WP2 写入（fused 双写）
- `[锚点]` `src/ops/kv_cache/append/launch.h`（`kv_cache_append_launch` / `_batch_launch`）、`launch.cu:201,219`、`src/ops/kv_cache/append/kernel.cuh`、各 `*_codec.cuh`。
- `[新增]` 在 body 写入 kernel 内追加 exact 写入（未量化 BF16 原值）。需要新的"带 shadow 的 append"入口，供 prefill/decode/MTP 三处共用。
- `[新增]` 同 slot 去重（`keep_last_writes` 语义）以支持一个 ubatch 内多行命中同页。
- `[锚点/参考]` beellama `src/llama-kv-cache.cpp:3258,3290`、`src/llama-kv-cache-tail.cpp:12-24`。

### WP3 attention 合并路由
- `[锚点]` `include/ninfer/ops/softmax_attention.h`（`causal_softmax_attention`、`..._cached`、`causal_softmax_attention_route_family`）、`src/ops/softmax_attention/dense/causal_cache/causal_softmax_attention.cpp:347-402`（`causal_attention_resolve_route`）、`launch.h:48-138`、`small_t_i8.cuh:69,945-953`、`small_t_bf16.cuh:7`、`small_t.cuh`（reducer）。
- `[新增]` 在 small-T 家族内部加 tailed 分支：
  1. 跑 body partial（现有 INT8 系列 kernel）→ 写 `(acc_b, m_b, l_b)` 到 workspace；
  2. 跑 tail partial（BF16 kernel，窗口见 §1.4）→ 写 `(acc_t, m_t, l_t)`；
  3. 合并 kernel：`m=max(m_b,m_t)`，`acc=acc_b·exp(m_b-m)+acc_t·exp(m_t-m)`，`l=l_b·exp(m_b-m)+l_t·exp(m_t-m)`，输出 `acc/l`。
- `[新增]` per-query tail 窗口起点的设备侧计算（由 `positions` 推出，见 §1.4）。
- `[风险]` 见 §6-R1：body 为 INT8、tail 为 BF16，两套 QK 缩放（`s_q·s_k`）与 PV 累加必须先统一到同一 FP32 参考域。

### WP4 CUDA Graph / route family
- `[锚点]` `include/ninfer/ops/softmax_attention.h:207-216`、`src/models/qwen3_5/program/planning/graph_profiles.{cpp,h}`、`src/models/qwen3_5/program/graph_execution.h`。
- `[新增]` 把 tail 身份（N、R、dtype、每层是否启用、tail 池 base/stride）纳入 graph key；**长度不进 key**（arena 静态）——对齐 beellama `src/llama-graph.cpp:641-706,773-801`。
- `[验收]` 断言 session 内 family 序列与未开 tail 时一致（仍是 prompt → small-T），仅一次切换。

### WP5 容量核算与 `MemorySummary`
- `[锚点]` 见 §2 接入点链；`types.h:1294-1331`。
- `[改动]` `SequenceCapacityCurve` 由线性单系数改为"常量项 `(N+R)×C` + body 线性项"；`MemorySummary` 增 `kv_exact_history_bytes / kv_rollback_reserve_bytes`（对齐 beellama 字段，`docs/beellama-args.md:159-163`）。
- `[改动]` `docs/config-calculator.html:490,524` 每 token 表增 tail 行。

### WP6 配置链
- `[锚点]` `apps/cli/options.cpp:368`、`src/serve/serve_options.cpp:793`、`include/ninfer/types.h:353-486`（`EngineOptions`，`kv_cache` 在 `:422`）、`src/runtime/engine/model_instance.cpp:550`（身份 tag `:177` 的 `;kv=`）、`src/models/qwen3_5/program/planning/startup.cpp:1352`、`decoder_state.h:23`、manager JSON `apps/windows-manager/config/profiles/s-128k.json:15`。
- `[新增]` CLI：`--kv-tail-tokens`（`0|auto|N` 起步，语法对齐 beellama 但**只实现被验证的子集**）、`--kv-tail-type bf16|f16`（M2）。非法值 fail context。
- `[新增]` `EngineOptions.kv_cache` 增 tail 字段；artifact 身份 tag 增 tail 维度（否则不同 tail 配置共用同一 engine 身份会错）。
- `[新增]` 投机 draft context 强制 tail = 0（对齐 beellama `common/speculative.cpp:2751`）。

### WP7 跨 tier 搬运与前缀复用
- `[锚点]` `src/core/host_kv_arena.h:18-33`、`src/core/disk_kv_store.h:27-45`、`src/models/qwen3_5/program/prefix/hybrid_cache.h:30,117-133`（内容寻址，block = 1 full page）、`docs/maintainer/paged-kv-cache.md §4.5`。
- `[M1 决策]` **exact 池只放 Device、不参与 host/disk tier**（最小形态，绕开"两套 stride"问题）。这样 WP7 在 M1 只做一件事：确认 exact 池不进 slab/LRU/磁盘，并更新 `§4.5` 的"pool 内 page group 等价"所有权边界描述。
- `[M4 可选]` 若要 exact 池也下 tier，则搬运、命中 digest、身份校验需同时容纳两套页尺寸——单独里程碑，不阻塞 M1–M3。

### WP8 事务 / checkpoint / rollback
- `[锚点]` `src/models/qwen3_5/program/transactions/`、`checkpoint_recovery.cpp`、`src/runtime/engine/kv_capacity.cpp`。
- `[新增]` exact 池写入必须在 attention 之后提交（对齐 beellama `graph_compute_finish → finish_tail_batch`，`src/llama-kv-cache.cpp:2966-2976`）；失败回滚并标 degraded。rollback reserve R 即为此。
- `[验收]` 回滚后 exact 池与 body 池一致性测试（见 §5）。

### WP9 文档
- `docs/cli.md`、`docs/serving.md`、`docs/config-calculator.html`、`docs/maintainer/paged-kv-cache.md §4.5`、`docs/performance.md`（发布 tail 档实测）。

### WP10 验证与基准
- 见 §5。

---

## 4. 里程碑

| 里程碑 | 内容 | 出口条件 |
|---|---|---|
| **M0 决策前置** | 硬件基线**钉死本机 RTX 5070 Ti / sm_120a**（§0.5，报告 §0 的 ABI 矛盾不再阻塞）；按 §0.5 装配做 Phase 0 收口实验：`ninfer rk4v4`（**有/无投机两态**）× `tail 0/1024`，对照 `llamacpp kvarn4+tail`（**`--spec-type none`**），统一指标 | KVarN 是否值得借算法有结论（决定是否开 Phase 2） |
| **M1 静态 tail（功能闭环）** | WP1–WP3 的 BF16 最小形态 + WP5/WP6 核算与开关；N 固定、prefill 只写不合并；在 **`rk8v4` / `rk4v4-e8` / `nvfp4` 三档 body** 上各测 `tail 0 vs N` | FP32 oracle 通过；三档均能跑通且质量提升可测 |
| **M2 F16 落地 + 图形稳定** | WP4（family 稳定断言）+ **F16 tail 落地为 5070 Ti 默认**（BF16 退回为链路验证形态） | graph 复用无额外重捕获；F16 vs BF16 在本机实测择定 |
| **M3 并发与投机** | exact 池按 C 分配；投机 draft 强制 tail=0；跨 tier 决策落地；ninfer **有/无投机两态**的端到端复测 | C=1..8 显存 = §2 预测；投机路径无回归 |
| **M4 可选** | exact 池下 host/disk tier；per-request tail 覆盖观测 API | 仅在 M3 后确有需求时启动 |

依赖：M1 依赖 M0 的硬件基线（已由 §0.5 钉死 `sm_120a`，不再阻塞）；WP3 是 M1 的关键路径与最大风险。

---

## 5. 验证矩阵

| 层面 | 证据 | 锚点/命令 |
|---|---|---|
| 数值正确性 | tail 合并 vs **独立 FP32/FP64 oracle**（覆盖 `p+1<=N`、`p+1>N`、空 body 窗口、C>1 掩码行） | `tests/ops/softmax_attention/causal_cache.cpp`（`:3682-3690` 已有 rk8v4 段与 `kv_cache_lloyd4_oracle.h`/`kv_cache_e8_root_host.h` 模式） |
| 量化/状态边界 | body 与 tail 双写一致；同 slot 去重；回滚后一致性 | `tests/ops/softmax_attention/`、`tests/ops/`（新增 tail 用例） |
| 质量 | 按 **§0.5 装配**：ninfer 侧在**同一 `.ninfer`**（`Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`）上对 **`rk8v4` / `rk4v4-e8` / `nvfp4` 三档**各做 `tail=0` vs `tail=N`（无投机为主口径，有投机并测）；跨产品对照用 llamacpp 侧 `Qwen3.8-27B-GSQ-RCO-IQ3_XXS-mtp.gguf` + **`--spec-type none`**，统一到 KLD 或都补 ppl | `apps/perplexity`；llamacpp `llama-perplexity.exe`；`docs/perplexity.md:23`（7 格式已测） |
| 性能 | `bench/ops/kv_cache_append_bench.cu` 扫 body+tail 双写；端到端 decode 对比开/关 tail | bench 现有能扫 9 种 storage（`:531-557`） |
| 图/生命周期 | 断言 family 序列不变；graph key 不含 tail 长度；无额外重捕获 | `graph_profiles.cpp`、`causal_softmax_attention_route_family` |
| 显存 | 实测 `MemorySummary` 对 §2 表（C=1..8、N=512/1024/2048） | `types.h:1294-1331` |

**统一口径提示**（报告 §3.2 缺口）：ninfer 用 ppl、beellama 用 KLD，不可换算。若要宣称"tail 让每字节质量优于 KVarN"，必须同一 artifact、同一 corpus、同一指标；否则只陈述 ninfer 自身的 ppl 变化。

---

## 6. 风险与缓解

| 编号 | 风险 | 缓解 / 退路 |
|---|---|---|
| **R1** | **INT8 body × BF16 tail 跨值域合并**：body partial 在"量化码 + per-row scale"域，tail 在 BF16 域，QK 缩放（`s_q·s_k`）与 PV 累加需统一到同一 FP32 参考域 | 作为 M1 **第一个可证伪里程碑**：先只做"body=BF16 + tail=BF16"验证合并公式，再切入体=INT8；用 FP32 oracle 逐行验证 `(m,l)` 一致性 |
| **R2** | CUDA Graph 家族切换导致重捕获 | 设计已把 tail 合并放进 small-T 家族内部（§1.5），家族数不变；设 flag 兜底退化为"page 边界 + 双 kernel" |
| **R3** | `SequenceCapacityCurve` 线性假设被打破 | WP5 改为常量项 + 线性项；先用 §2 表做预测，再实测回归 |
| **R4** | 投机/checkpoint 与 exact 池交互 | draft tail=0；exact 写入在 attention 后提交 + reserve 回滚（WP8） |
| **R5** | 并发乘数被忽略导致 16 GB 卡 OOM | §2 按 C 预算；启动期容量规划与 fit 探针必须含 tail 常量项（对齐 beellama `common/fit.cpp:955-1000`） |
| **R6** | prefill 阶段 tail 覆盖全部 ⇒ body 为空 | M1 让 prefill 走 prompt 路由（不合并），decode 才合并；kernel 对空 body 窗口保持均匀 |
| **R7** | dtype 与基线不符（本机 5070 Ti 实测 F16 优于 BF16） | 硬件基线已钉死 `sm_120a`（§0.5）；M1 先 BF16 打通链路，M2 落 F16 为默认 |

---

## 7. 验收标准（Definition of Done，M1）

1. `--kv-tail-tokens 1024` 在 **`rk8v4` / `rk4v4-e8` / `nvfp4` 三档** body 上均可启动、可 decode、可 prefill。
2. tail 合并对 FP32 oracle 通过（含边界与 C>1 掩码行）。
3. `apps/perplexity` 显示三档各自相对 `tail=0` 的 ppl 改善（方向正确、幅度记录在案）。
4. `MemorySummary` 与 §2 预测在 ±5% 内；C=1 时 tail 净增 ≈64 MiB。
5. session 内 CUDA Graph family 序列与不开 tail 时一致。
6. 关闭开关（tail=0）时，输出与现状**逐 token 一致**（零回归）。
7. 文档与 `config-calculator.html` 更新。（许可证/血缘事项按 §0.5 第 7 条**不在本轮范围**。）

---

## 8. 明确不做

- 不移植 beellama 的 `GGML_OP_KVARN_*`、六路由 CUDA kernel、`KVRN` state v16、`ggml_backend_meta` split-state、多 GPU placement。
- 不改量化 body 的 plane schema / codec / oracle。
- 不为 hypothetical 后端（Metal/HIP/Vulkan）预留分支。
- 不在 M1 就做 host/disk tier 的 exact 池（WP7 留 M4）。

---

## 9. 参考锚点汇总（已核实）

**ninfer 存储/几何**：`include/ninfer/types.h:49-73`(KvCacheStorage)、`:353-486`(EngineOptions, kv_cache@422, max_concurrency@415)、`:1294-1331`(MemorySummary)；`src/core/paged_kv_storage.h:51,58-121`；`src/core/paged_kv_cache.h:19,22,34`；`src/models/qwen3_5/state/decoder_state.h:23,114,128`、`.cpp:48,107-118,191`；`src/models/qwen3_5/program/planning/startup.cpp:253-286,347-359,453,527,567,669,1352,1372-1395`。

**ninfer attention/写入**：`include/ninfer/ops/softmax_attention.h`(Op 契约、route family 0/1/2)；`src/ops/softmax_attention/dense/causal_cache/causal_softmax_attention.cpp:347-402`、`launch.h:48-138`、`small_t_i8.cuh:69,945-953`、`small_t_bf16.cuh:7`、`small_t.cuh`；`src/ops/kv_cache/append/launch.h`、`launch.cu:201,219`；`src/ops/kv_cache/plane_types.h:63-71`、`hadamard_d256.cuh`；`src/ops/kernel/paged_kv_address.cuh:47-69`。

**ninfer 运行时/前缀/容量**：`src/models/qwen3_5/program/prefix/hybrid_cache.h:30,117-133`；`src/core/host_kv_arena.h:18-33`、`disk_kv_store.h:27-45`、`kv_loan.h:22-40`；`src/runtime/engine/kv_capacity.cpp:104,226`；`src/models/qwen3_5/load.cpp:189-194`；`apps/cli/options.cpp:368`、`src/serve/serve_options.cpp:793`、`src/runtime/engine/model_instance.cpp:177,550`。

**测试/文档**：`tests/ops/softmax_attention/causal_cache.cpp:3682-3690`；`bench/ops/kv_cache_append_bench.cu:531-557`；`docs/perplexity.md:23`；`docs/performance.md:454-460`；`docs/maintainer/paged-kv-cache.md §4.5`；`docs/config-calculator.html:490,569,591`。

**beellama 参考**（只作设计参照，不移植）：`src/llama-kv-cache-tail.cpp:12-24,79-119,385-387,390-405,437-441,463-488,643,694,711-719,764-773`；`src/llama-kv-cache.cpp:1143-1144,1153-1154,152-153,126-205,2178-2180,3258,3290,2109-2182,2966-2976`；`src/llama-graph.cpp:641-706,773-801`；`ggml/src/ggml-cuda/fattn-tail.cuh:183-390`；`docs/beellama-features.md:119-130,548-577`；`common/arg.cpp:2752-2768`。
