# 给 ninfer 引擎引入 beellama.cpp 的 KVarN 量化与 KV cache precision tail：可行性调研报告

- 调研日期：2026-10-05
- 修订：v2（2026-10-05，依据回源核验修正 v1 的事实错误与引用漂移，见附录 A）
- 调研对象：`D:\ninfer\beellama.cpp`（llama.cpp 分支，v0.4.7，工作树 HEAD `58a162927`）与 `D:\ninfer\ninfer-16g-5070ti-5080-5090-qwen3.8-27b-gsq-rco`（下称 ninfer）
- 基准装配（本次）：两成品目录 `D:\ninfer\llamacpp`（llama.cpp 系成品）与 `D:\ninfer\ninfer-package`（ninfer 成品），同一模型 Qwen3.8-27B GSQ-RCO **IQ3_XXS**；**llamacpp 侧不开启 MTP**。详见 `precision-tail-port-plan.md §0.5`
- 硬件基线：本机 **RTX 5070 Ti 16 GB / sm_120a**（本次测试以此为准）
- 环境：需要联网检索时走代理 `http://127.0.0.1:7897`
- 性质：纯调研，未修改任何源文件
- 方法：4 个并行 Explore 子 agent 分别负责「KVarN 实现面」「precision tail 实现面」「ninfer KV/attention 现状」「文档·实测日志·许可·血缘」；作者对下列决定性数字做了独立复核（见 §7 核验说明）

> **v2 修订摘要（相对 v1 的实质性改动）**：① ninfer `rk4v4` **有**公开 perplexity（`docs/performance.md:460`），v1 的"未单独公布 ppl"错误，核心不确定点改写为"指标不可换算 + 无 tail"；② §4.3 的"近全量 CPU offload"被日志证伪，改为显存超额提交；③ §3.5 的"sm_89 无 FP8 tensor core"改正；④ §4.2 吞吐区间由被裁剪的 0.983–1.176× 改为完整 0.839–1.330× 并标注官方免责声明；⑤ §1.4/§3.5 多处引用指向错误文件已改正；⑥ §0 判定语气下调，与 §7 对齐；⑦ 新增 §2.4 tail 显存定量与并发乘数、§5.3 第 4 条难点（INT8/F16 值域合并）。

---

## 0. 结论摘要

**两个机制的可行性结论相反，应当拆开决策，不要当成一个特性包。**

| 项 | 判定 | 一句话理由 |
|---|---|---|
| **KVarN 量化本体** | **在 Phase 0 实验给出证据前不做整体移植**（最多借用其归一化算法） | ninfer 已有 9 种 KV 存储格式，最低档（rk4v4 = 27.3% bf16）已落在 KVarN 推荐梯队的同等字节档位；而移植需重写 4 个 ggml op + 约 1.1 万行 CUDA kernel + 私有 state 格式，且 ninfer 对 ggml 零运行时依赖，等于接口层 + 全部 kernel 重写 |
| **KV cache precision tail** | **建议移植，优先级高** | 与 KVarN 弱耦合可独立；ninfer 恰好已有它需要的两块基座（FP32 split-partial + reducer、多套异构 KV 池并存先例）；质量收益明确且近乎性能免费 |
| **硬件基线** | **本次已定为：本机 RTX 5070 Ti 16 GB / sm_120a** | ninfer 仓库内部文档自相矛盾（`AGENTS.md:34` 写 sm_86/RTX 3090，`README.md:17` 与 `docs/rtx-5070ti-windows.md` 写 sm_120a/RTX 5070 Ti），但按本次指示测试基线钉死本机 5070 Ti，**该矛盾不再阻塞本项目**；建议后续单独修正文档 |

**核心不确定点（唯一需要实测才能收口的判断）**：beellama 用 KLD 衡量 KV 质量、ninfer 用 perplexity，两者不可直接换算；且 ninfer 现有格式全部**没有 tail**，而 KVarN 官方阶梯**全部带 1024 tail**。因此"KVarN 是否比 ninfer 的 rk4v4 每字节质量更好"目前**无定论**。注意：这**不是**因为 ninfer 缺数据——ninfer 已公布 rk4v4 的 ppl（`docs/performance.md:460`：4.352432，+0.214% vs int8；这与本报告 §3.2 的字节档位互相独立），**而是因为两边的指标口径（KLD vs ppl）、模型小版本、tail 配置都不同**。若要收口，只需一次同模型同口径的三方对比（rk4v4 / rk4v4+tail / KVarN k4v4+tail，并把指标统一），成本远低于移植。

---

## 1. 机制一解剖：KVarN 是什么

### 1.1 算法
KVarN 是 beellama 引入的 Huawei 系免标定（training-free / calibration-free）方差归一化 KV 量化器（原文 "KVarN is Huawei's calibration-free, variance-normalized KV-cache quantizer"，`docs/beellama-features.md:11`）。对 RoPE 后的 K 与 V **整个 128-token tile** 执行：

1. per-head 归一化 Walsh-Hadamard 变换（自逆）— `ggml/include/ggml.h:2719`、`src/llama-kvarn.cpp:750-788`（`:750` = `llama_kvarn_hadamard_64`，`:770` = `..._128`；256/512 维走组合出的"完整逻辑 head Hadamard"，见 `src/llama-kvarn.cpp:128` 附近）
2. Sinkhorn 式行/列**双轴**方差归一化，迭代取 imbalance 最优（默认 `sinkhorn_iters = 16`，定义在 `src/llama-kvarn.cpp:198`；实现 `:824-879`）
3. 每行 min/max 仿射量化 — `src/llama-kvarn.cpp:913-930`

scale 布局：位打包 payload + F16 `scale[rows]` + F16 `zp[rows]` + F16 `s_col[cols]`，即 **per-row scale/zp + per-column 第二轴因子**；无 outlier 通道、无嵌套二次量化（记录级布局 `make_record_layout` `src/llama-kvarn.cpp:673-688`，第二轴因子字段名为 `other_off`；tile 级布局 `make_layout` `:634-671` 字段次序不同，勿混）。

- group 固定 **128 token**：`src/llama-kvarn.h:10`（`constexpr uint32_t KVAR_N_GROUP = 128;`）
- "N" = 位宽档 ∈ {2,3,4,5,6,8}，K/V 独立 → **36 个** `LLAMA_KVARN_KxVy_G128` 枚举：`include/llama.h:201-219`（值在 205-216，6×6 全叉积）
- head_dim 64 用矩形记录 K 64×128 / V 128×64；128/256/512 用 128×128 切片：`src/llama-kvarn.cpp:690-698`
- **注意力在 WHT 旋转域进行**（Q 与 tail 也做 WHT）：`src/llama-graph.cpp:3778, 4234`（两处均为 `ggml_kvarn_wht_aux(...)`；注意旋转受 `use_kvarn_q_rot` / domain 门控，`ggml.h` 同时定义 `ORIGINAL` 与 `ROTATED_K_ORIGINAL_V` 域，并非无条件旋转）
- **不可关闭的内建开销**：128-token attention-sink + 最新 128 token 的 F16 精确 suffix + F16 stage：`docs/beellama-features.md:18`、`src/llama-kv-cache-kvarn.h:29-82`（非 SWA 层实为 **2×** 128 F16 stage 组）

### 1.2 实现面规模（已复核）
- 命名含 kvarn 的文件 **102 个**（`git ls-files | grep -i kvarn | wc -l` = 102）
- 分类行数：src 4,997 / ggml-cuda 10,941 / vulkan 2,025 / tests 7,324 = **25,287**；若按"约 2.56 万行"口径，另 301 行在 `common/fit-kvarn-tail.{cpp,h}`(59) 与 `scripts/hip/validate-kvarn-runtime.py`(242)。**（v1 把四项直接写成 25,588 是加总错误，四项本身正确。）**
- `src/llama-kv-cache-kvarn.cpp` 单文件 145,641 B / 3,241 行
- 版本 `CMakeLists.txt:6-8` = 0.4.7；工作树 HEAD `58a162927`（`v0.4.7-1-g58a162927`）。注意本报告引用的 build-logs 来自 `0ba48c55a`/build #11860，**比工作树落后一个提交（`build-logs/build-win-cuda-sm_120.log:16`）**。

### 1.3 新增的 ggml 契约（已复核 op 名）
- 4 个新 op：`GGML_OP_KVARN_WHT / STORE / VIEW / MATERIALIZE`（`ggml/include/ggml.h:606-609`，注释在 `:604-605` 明确"不是 GGUF tensor type、不参与权重序列化"）；构造在 `ggml/src/ggml.c:6711-6903`（含 `ggml_kvarn_materialize`，`:6858` 起）
- `FLASH_ATTN_EXT` 扩展参数：实际新增 **5** 个 op_param——DOMAIN=4 / TAIL_BODYLESS=5 / TAIL_HISTORY_SLOTS=6 / WINDOW_CHUNK=7 / NON_CAUSAL_MASK=8（`ggml/include/ggml.h:452-459`，v1 只列了 3 个）
- 跨后端能力查询协议 `ggml_backend_kvarn_capabilities`（`ggml/include/ggml-backend.h:181-203`；逐后端注册见 `ggml/src/ggml-cuda/ggml-cuda.cu:6110-6111`、`ggml/src/ggml-cpu/ggml-cpu.cpp:716-717`）
- **KVarN 没有新增 ggml_type**（枚举止于 `Q2_1=48`/`COUNT=49`，`ggml/include/ggml.h:440-441`）；记录走 `GGML_TYPE_I8` 字节张量。GGUF / `convert_hf_to_gguf.py` / gguf-py **零 kvarn 命中**，无需改权重转换链。**注意**：beellama 分支本身为其他特性新增了 8 个 ggml_type（Q1_0/Q2_0/Q6_0/Q6_1/Q3_0/Q3_1/Q2_0S/Q2_1），"未新增 ggml_type"仅对 KVarN 成立
- 仅有私有 prompt-cache state 格式：magic `KVRN` = 0x4E52564B（`src/llama-kv-cache-kvarn.cpp:121`）、version 16、min 12（`:133-134`，写入 `:2295`）

### 1.4 kernel 与约束
- CUDA 六路由：`DECODE_SPLIT / DECODE_VECTOR / GENERIC_MMA / PROMPT_PREFILL / PORTABLE_NATIVE / UNAVAILABLE`（`ggml/src/ggml-cuda/fattn-kvarn-route-policy.h:21-28`，选择点 `fattn-kvarn-dispatch.cu:1231-1235`，分支派发 1237-1248）
- 硬约束（**均在 `fattn-kvarn-route-policy.h`，v1 误标为 dispatch.cu**）：`d_k == d_v` 且 head_dim ∈ {64,128,256,512}（`:195-212`）；split decode 最大 Q：D64=16，其余 8（`:227-230`）；128 列宽 MMA 仅 `n_q>8 && n_q<=16 && gqa>4`（`:253-259`）。specialized 路由要求 `turing_mma_available`（cc ≥ 7.5；定义 `ggml/src/ggml-cuda/common.cuh:355`，`dispatch.cu` 判定在 :61-67 块内，约 `:66`）——**无 Hopper/Blackwell 专有指令**，因此 sm_80/86/89/120 全部合格。注意 Vulkan 侧另有同名 `ggml-vulkan/fattn-kvarn-route-policy.h`，引用时须带目录
- CPU 有完整等价实现（`ggml/src/ggml-cpu/ops.cpp`：store/materialize 11725-12351、KVARN 直接消费记录的 flash-attn 12573-13580，另有 `:8759` 附近挂接；v1 的单一区间 11742-12600 偏窄）；Vulkan 有 5 个 `.comp` shader
- 构建门：`GGML_CUDA_KVARN`（默认 ON，`ggml/CMakeLists.txt:207`；默认 15 个均衡快速解码对，含 `FATAL_ERROR` 计数校验；`GGML_CUDA_FA_ALL_QUANTS=ON` 展开为 `GGML_CUDA_KVARN_ALL_PAIR_COUNT 36`，`:207-264`）

### 1.5 集成形态
`llama_kv_cache_kvarn : public llama_memory_i`（`src/llama-kv-cache-kvarn.h:215`）——是 **与 `llama_kv_cache` 平行的另一套实现，不是子类**，内部组合一个 metadata-only 的 `llama_kv_cache` 来复用 cell/pos/seq。iSWA 用 `llama_kv_cache_kvarn_shared` 包装并按组选路（`src/llama-kv-cache-iswa.cpp:15-91`）。`src/llama-graph.cpp` 213 处 kvarn 提及集中在 rot/mat_idxs 输入回填、Q/tail 的 WHT 插入、native vs materialize 分支；`src/llama-context.cpp` 68 处做参数校验落地、tail policy 计算、私有 workspace 累计（两处计数经 `grep -ic kvarn` 复核）。

---

## 2. 机制二解剖：KV cache precision tail 是什么

### 2.1 定义
**保留"最近 N 个 attention 可见 token"的 K/V 为精确 F16/BF16（exact tail），其余走低精度 body。** N 是固定 token 数（`--kv-tail-tokens` 的 **CLI 默认是 0**，`common/arg.cpp:2756`；只有 `auto` 解析为 1024，`src/llama-kv-tail-request.cpp:117`），不是比例、也**不按 attention 质量自适应**；但"哪些行属于 exact"是 **per-query** 计算的（每 query 取最新有限且因果可见的 N 条，`src/llama-kv-cache-tail.cpp:463-488`）。

默认精度：标准量化 cache → BF16（`src/llama-kv-cache.cpp:417`，文档 `common/arg.cpp:2764`），KVarN → F16（`src/llama-model.cpp:2273`、`src/llama-kv-cache-tail.cpp:246-254`）。

**物理分离**：每层独立张量 `cache_k_tail_l%d` / `cache_v_tail_l%d`（`src/llama-kv-cache.cpp:1143-1144, 1153-1154`），tail 内用 `seq_id*arena_stride+local` 的 slot arena 索引，不是给 cell 打标记（`src/llama-kv-cache-tail.cpp:694`，范围 `685-709`）。4 种形态 OVERLAY / NATIVE_EXACT / COMPACT_OVERLAY / COMPACT_NATIVE_EXACT（`src/llama-kv-cache-tail.h:115-121`，另加 DISABLED）。tail 覆盖整窗 SWA 时可**完全省掉 body**（bodyless，`src/llama-kv-cache-tail.cpp:390-405`）。

### 2.2 状态迁移（关键：**没有重量化步骤**）
每 token 在写量化 body 的**同一算子内 fused 双写** exact shadow（`ggml_set_rows_with_shadow`，`src/llama-kv-cache.cpp:3258, 3290`）；滑出 tail 就是 ring 丢弃（front entry 选为 victim 后 erase，`src/llama-kv-cache-tail.cpp:711-719` trim / `:764-773` victim-erase）。提交发生在 attention 之后（`graph_compute_finish → finish_tail_batch`，`src/llama-kv-cache.cpp:2966-2976` 调用点、`:6821-6823`），失败走事务回滚并可标 `DEGRADED_PAYLOAD_INVALID`（该宏定义在 `src/llama-kv-cache-tail.cpp:643`，v1 误标为 `llama-kv-cache.cpp:2966-2976`）。同 batch 重复写同一 slot 用 `keep_last_writes` 去重以避免 SET_ROWS 竞争（`src/llama-kv-cache-tail.cpp:12-24`）。

CUDA Graph：tail 身份（storage/type/N/R/stride/每层 route+bias/current/slots）是 graph 复用键的一部分（`src/llama-graph.cpp:641-706` capture/matches、`:773-801 can_reuse`）；ubatch 只进 `attention_stride = N+U` 与 transient，不改持久容量。

`llama-kv-tail-request` **不是异步请求队列**，也与 sampler/投机解码无关：它是把 `--kv-tail-tokens` 一次解析成模型无关的不可变请求（5 模式 DISABLED/NUMERIC/AUTOMATIC/POSITIONAL/NAMED）+ 与 model group 绑定的纯 resolver，目的是让 `-c/--fit` 探针与最终 context 不分叉（`src/llama-kv-tail-request.h:9-15, 22-29`）。

### 2.3 attention kernel 侧要求
必须支持一次读取混合精度 KV：`ggml_flash_attn_ext` 追加 5 个 src（`k_tail/v_tail/mask_tail/query_order/run_desc`，`ggml/include/ggml.h:2529-2535`）。做法是 **body 与 tail 各算一次 partial，再用各自的 (row max, denom) 做一次 FP32 online-softmax 合并**——不是各自归一后相加（`ggml/src/ggml-cuda/fattn-tail.cuh:339-389`，per-partial max/denom 与 `expf(bm.x-m)`；小 tail 走就地索引支路 `:183-337`）。无 native 入口时回落 generic 图（`get_rows_as + mul_mat + concat + soft_max_ext + add`，`src/llama-kv-cache.cpp:126-205`）。

**与 KVarN 弱耦合**：tail 面向"任意量化 body"，q5_0 等标准格式路径完全不碰 `fattn-kvarn-*`；KVarN 只是 body 的一种。反向唯一依赖：body kernel 必须对外导出 FP32 row-max 与 denominator 才能参与合并（`docs/beellama-features.md:369-373`）。

### 2.4 内存核算（最容易漏的一环）
- 公式：`arena_stride = round_up(N+U, 256)`；持久 exact = **`(N+R) × S`，与 ubatch U 无关**（U 只进 transient）— `src/llama-kv-cache-tail.cpp:79-119`、`docs/beellama-args.md:124-130`。`S` 为池规模，**必须按 `--max-concurrency` 计**：ninfer 支持并发 1..8（`include/ninfer/types.h:415`、`docs/ngram.md:223`），故常驻 exact 随并发线性放大。
- **ninfer 上的定量估算（本报告补）**：ninfer 仅 full-attention 层持有 paged KV，bf16 = 65,536 B/token（§3.2）；故 N=1024 的 exact tail ≈ **64 MiB / 序列**，加 rollback reserve R 与 64-token 页对齐后按 64–72 MiB 估；`C=1 → ~64 MiB`、`C=8 → ~512–576 MiB`。这与 beellama 日志中恒定的 tail buffer 64.25 MiB 完全自洽（见 §4.1），可作为量级交叉验证。
- bodyless 时 cache 物理容量被 tail 取代：`allocated_capacity_tokens = compact_layout.history_stride` 而非 `get_size()`（`src/llama-kv-cache.cpp:2178-2180`）
- 上报字段（公开名见 `docs/beellama-args.md:159-163`，struct 内部名不同，见 `src/llama-kv-memory-stats.h`）：`kv_exact_history_bytes / kv_rollback_reserve_bytes / kv_staging_bytes / kv_transient_bytes / kv_peak_bytes`（计算在 `src/llama-kv-cache.cpp:2109-2182`）
- fit 走"精确无分配校验 + 从 pristine 输入重启"的候选循环（`common/fit.cpp:955-1000`）

### 2.5 开关矩阵与互斥
CLI：`--kv-tail-tokens`（`0|auto|N|N0,N1|full=N,swa=N|full@l0=N`，非法即 fail context）、`--kv-tail-type f16|bf16`（`common/arg.cpp:2752-2768`）。cparams 侧 8 个字段（`src/llama-cparams.h:67-74`）。

重要互斥/降级：
- 无 native 入口则 **warn 并把 tail 归零**，除非 `LLAMA_KV_TAIL_ALLOW_GENERIC=1`（`src/llama-context.cpp:457-521`）
- native 需 flash_attn 且无 attention bias；**MLA/DSA(K-only) 拒绝正 tail**（`src/llama-kv-cache-tail.cpp:437-441`；`src/llama-model.cpp:1916` 只是 `graph_consumes_exact_kv_tail(){return !is_mla();}`）
- body 已是 F16/BF16 → `already_exact`，不建 overlay（`src/llama-kv-cache-tail.cpp:385-387`）
- `v_transposed` 的量化 V 不可写（`src/llama-kv-cache.cpp:152-153`）；`--split-mode tensor` 不支持 overlay 时抛
- 投机 draft context 强制 tail = 0（`common/speculative.cpp:2751`）

---

## 3. ninfer 现状基线

### 3.1 ninfer 已经有 KV 量化，而且不止一种
`KvCacheStorage` 枚举定义在 `include/ninfer/types.h:49-73`；其存储布局 `paged_kv_storage_layout()` 在 `src/core/paged_kv_storage.h:58-121`，恰好 **9 个 switch case**，对应：BFloat16、Int8Group64、RotatedInt8KeyInt4ValueGroup64（rk8v4）、RotatedLloyd4KeyInt4Value（rk4v4，Lloyd-Max 4-bit key）、RotatedInt4KeyInt4ValueE8（rk4v4-e8，E8 格点 key）、RotatedE8RootKeyInt4Value（rk2v4-e8）、Fp8E4M3Row256、Nvfp4Group16、Fp8KeyNvfp4Value（k8v4）。**注意 rk4v4 与 rk4v4-e8 是两种不同格式**（前者 Lloyd-Max、后者 E8 lattice，见 `README.md` 的 rk4v4/rk4v4-e8 段），不要把二者归为一个枚举。

格式为 **code plane + scale plane 分离**（每层 2 或 4 个 plane 共享一个全局 pool，`src/models/qwen3_5/state/decoder_state.cpp:48`，`planes_per_layer = scaled?4:2`），page = **64 token**（`src/core/paged_kv_cache.h:19`），且**所有量化格式的 K/V 都要求 `head_dim == kD256KVCacheHeadDim`（= 256）**，否则 `throw "unsupported paged KV-cache storage geometry"`（`src/core/paged_kv_storage.h:120`，v1 的消息文字与行号略偏）。

**关键**：ninfer 的量化 KV **已经在用 Hadamard 旋转**（int8 = "rotated INT8, group-64"，`docs/config-calculator.html:569`；`src/ops/kv_cache/hadamard_d256.cuh` 实现 Sylvester 变换 `hadamard_d32_columns_inplace`；权重侧也有 Sylvester WHT + `hadamard_signs`，`docs/maintainer/qwen3_5-model.md:216-235`）。所以 KVarN 的数学基元**不是新东西**，其真实增量只有：Sinkhorn 双轴归一化 + per-column 第二轴 scale + 更细的位宽菜单（K/V 独立 2–8 bit）。

### 3.2 同模型族的字节对比（本报告最重要的表）
两边其实是**同一模型族、同一 KV 字节几何**：ninfer 的 27B 基线 bf16 = 65,536 B/token（`docs/config-calculator.html:490`，本人核验；`docs/performance.md:454` 在 Qwen3.8-27B 上独立给出同一值）；beellama 侧 bf16 参考 = 4096 MiB / 64K token = 65,536 B/token（`README.md:32-49`，本人核验），且其 KVarN 日志显示 `layers = 16`（`build-logs/combo/v1-kv54.log:2273`）——与 ninfer 的 hybrid 架构中"仅 16 个 full_attention 层持有 paged KV"一致（16 × 4096 B/token/layer = 65,536）。因此下列字节数字可直接比较。**但质量列不可直接比较**（见下方警示）。

| 方案 | B/token | 占 bf16 | 质量指标 | 出处 |
|---|---:|---:|---|---|
| bf16 基线 | 65,536 | 100% | KLD 0 | `README.md:34` |
| KVarN k5v5 + tail1024 | 23,808 | 36.3% | median KLD 0.000897 | `README.md:41` |
| KVarN k5v4 + tail1024 | 21,760 | 33.2% | KLD 0.000936 | `README.md:43` |
| KVarN k4v4 + tail1024 | 19,712 | 30.1% | KLD 0.000994（优于 q4_0 的 0.001057） | `README.md:44-45` |
| KVarN k4v3 + tail1024 | 17,664 | 27.0% | KLD 0.001112 | `README.md:46` |
| KVarN k3v3 + tail1024 | 15,616 | 23.8% | KLD 0.001316 | `README.md:47` |
| KVarN k2v2 + tail1024 | 11,520 | 17.6% | KLD 0.003811（"last resort"） | `README.md:49` |
| KVarN k5v5 实测斜率 | body 22,016 B/tok + tail 1,028 B/tok | — | — | `build-logs/combo/v2-proof.tsv`（本人对 4 个 ctx 点取差商：344 MiB / 16384 tok = 22,016 B/tok；tail 恒定 64.25 MiB） |
| **ninfer rk4v4** | **17,920** | **27.3%** | **ppl 4.352432（+0.214% vs int8；decode ≈ rk8v4）** | `config-calculator.html:490`；**质量 `performance.md:460`** |
| ninfer nvfp4 | 18,432 | 28.1% | **+0.36% ppl，decode 慢 12%** | `performance.md:406,459` |
| ninfer rk8v4 | 26,112 | 39.8% | **+0.083% ppl，~1% decode** | `performance.md:400,457` |
| ninfer k8v4 | 25,728 | 39.3% | **decode falloff 最差（-25% by 32k）** | `config-calculator.html:591` |
| ninfer int8 / fp8 | 33,792 / 33,024 | 51.6% / 50.4% | bf16 4.343225 / int8 4.343263 / fp8 4.347181 | `config-calculator.html:490`；`performance.md:454-456` |

> 换算口径（供审阅者复算）：beellama 行的 B/token = `MiB × 16`（基准 64K = 65,536 token，1 MiB / 65,536 = 16 B/token）；占 bf16 比例 = 该值 / 65,536。实测斜率行由 `v2-proof.tsv` 4 个 (ctx, buffer) 点取差商得到；其"总斜率"（含 tail）≈ 23,044 B/tok，与 README 阶梯的 k5v5 折算值 23,808 相差约 3%（不同模型小版本与 ctx 起点所致），两者可互为交叉验证。

**读表结论**：ninfer 的 rk4v4（27.3%）与 KVarN 推荐梯队的 k4v3（27.0%）处于同一字节档；而 KVarN 官方"Balanced default" k5v4（33.2%）落在 ninfer 的 rk4v4（27.3%）与 rk8v4（39.8%）之间。**所以"移植 KVarN 以省显存"这个动机不成立——但注意这仅是"字节档位"意义上的不成立，不等于"每字节质量更差"。**

**必须标注的三个证据缺口**（这是本报告核心不确定点的真正来源）：
1. **指标不同**：ninfer 用 perplexity，beellama 用 KLD，二者不可换算；
2. **模型小版本不同**：ninfer 为 Qwen3.8-27B，beellama 阶梯为 Qwen 3.6 27B Q5_K_S（`README.md:30`）；
3. **tail 配置不同**：ninfer 全列无 tail，KVarN 全列带 1024 tail。
4. **基线硬件**：上表两侧数字均在 **RTX 3090 / sm_86** 上测得（`README.md:30`、`performance.md:448-450`），**都不是本机 5070 Ti / sm_120a**——本次测试以本机 5070 Ti 为基准（§0），故上表仅作"字节档位/量级"参考，不直接当作本机结论。

因此"同字节档位下 KVarN 是否质量更优"目前**无定论**（见 §6 收口实验）。

### 3.3 ninfer 完全缺失的能力：按新近度混精度
全仓 grep `kvarn` = 0 命中；无任何 precision-tiered cache 的设计文档。ninfer 现有"分层"是 **residency 分层**（Host pinned slab / Device LRU / Disk），且**只做同 dtype 字节搬运，不做降精度转换**（`src/core/host_kv_arena.h:18-33`、`src/core/disk_kv_store.h:1-45`、`src/core/kv_loan.h:22-40`）。同一序列内按新近度混精度在 ninfer 当前的 pool 模型里**结构上不可表达**——这是 tail 移植的实质设计工作。

### 3.4 ninfer 已具备的 tail 承接条件（本人直接核验，这是判定"建议移植"的关键）
1. **online-softmax partial 合并机制原生存在**：`src/ops/softmax_attention/dense/causal_cache/small_t_i8.cuh` 第 69 行的 kernel 签名即 `float scale, float* partial_acc, float* partial_m, float* partial_l`，并在 `:945-953` 写回 split-local 的 m/l；reducer 在 `small_t.cuh:7`（"only what both share: layout constants, device helpers, and the split reducer"）、`small_t_bf16.cuh:7`（"a reducer combines FP32 split-local partials"）。**这正是 tail merge 需要的 (acc, max, denom) FP32 合并语义，ninfer 已经有了。** 唯一保留：ninfer 的 INT8 家族 route 在 **s8 tensor core 内直接吃量化码**，body partial 处于 dequant 值域、tail 为 F16/BF16，跨值域合并的数值细节见 §5.3 第 4 条。
2. **多套异构 KV 池并存已有先例**：`src/models/qwen3_5/program/planning/startup.cpp:253-286` 里 DFlash 的 full-attention 池被硬编码为 `KvCacheStorage::BFloat16` + `PagedKVPlaneOrder::HeadMajor` + 2 plane，与量化主池共存；`src/models/qwen3_5/state/decoder_state.cpp:107-118` 的 `plan_cache(...)` 已按 `spec.kv_storage` 参数化，并已有 `mtp_kv` 第二个池（`decoder_state.h:114,128`）。**因此"再加一个同几何 BF16 tail 池"不需要给页引入"精度"维度，绕开了最难的设计冲突。**
3. **架构门槛满足**：`CMakeLists.txt:9` 默认 `86`、`:14` 正则 `^(80|86|89|120a)$`，KVarN/tail 的 specialized 路由只要求 cc ≥ 7.5，无 Hopper/Blackwell 专有指令。
4. **质量回归脚手架现成**：`apps/perplexity`（`docs/perplexity.md:23` 明示 7 种 KV 格式已测）+ 独立 FP32/FP64 oracle 契约（`AGENTS.md` Verification 节、`tests/ops/softmax_attention/causal_cache.cpp:3682-3690` 的 rk8v4 段与 `kv_cache_lloyd4_oracle.h`/`kv_cache_e8_root_host.h`）+ `bench/ops/kv_cache_append_bench.cu:531-557` 已能扫全部 9 种 storage。

### 3.5 ninfer 的 attention 现状与约束
- decode/verify = Small-T / ChunkedSmall-T **手写 `mma.sync`**（`src/ops/common/mma.cuh:36-100`：bf16/f16 m16n8k16、s8 m16n8k32、e4m3 kind::f8f6f4）；INT8 格式走原生 s8 tensor core，QK 直接吃量化码。
- **FP8 硬件差异需分清**：Ampere sm_86 **无** FP8 tensor core；Ada sm_89 **有** FP8 tensor core，但 ninfer 的 `kind::f8f6f4` 这条 route 被限定在 Blackwell，故在本 fork 的 sm_86/sm_89 目标上，fp8/nvfp4/k8v4 全部 dequant 到 BF16/FP16 再普通 MMA（`src/ops/fp8_sm86_stubs.cpp:15-18` 只覆盖 FP8；nvfp4/k8v4 的 dequant 见 `small_t_nvfp4.cuh:345,473,500`、`small_t_k8v4.cuh:7,352`）。**（v1 写"sm_86/sm_89 无 FP8 tensor core"对 sm_89 是错的。）**
- 选型表 `causal_softmax_attention.cpp:347-402`（`causal_attention_resolve_route`）+ per-storage launch 声明 `launch.h:48-138`；设备路由开关名 `attn_prompt_fast` / `attn_pv_f16` 实际定义在 `src/ops/softmax_attention/dense/causal_cache/prompt.cu:33,240`（另见 `launch.h:23`、`apps/windows-manager/config/device-profiles.json:400,406`），**不在** `src/ops/common/device_route.h`（该文件是通用 per-GPU profile 框架）。
- **最大约束：一个 kernel 只认一种 plane schema**——leading/head extent 是编译期模板常量 + `block_table` 页寻址（`src/ops/kernel/paged_kv_address.cuh:47-69`），`src/ops/kv_cache/plane_types.h:63-71` 把 storage 映射为 kernel 类型，错型即编译失败。且 **route family 不同的调用不能进同一个 CUDA Graph executable**（`include/ninfer/ops/softmax_attention.h:209,212`）。
- GQA 几何只有 `[256,24,4]` 与 `[256,16,2]` 两个注册组合（`softmax_attention.h:142`、`src/ops/kv_cache/append/geometry.cuh:8-13`）。
- 写入点链：`attn_input_proj → rmsnorm_rope →`（① prefill `kv_cache_append_batch_launch`；② decode 在 small-T kernel 内 fused append；③ MTP 显式 `ops::kv_cache_append`）。量化/双写的合适 hook 层是 `src/ops/kv_cache/append/launch.cu:201`（`kv_cache_append_launch`）与 `:219`（`kv_cache_append_batch_launch`）。

### 3.6 配置贯通链（新增开关要穿 5 层，有集中 struct）
`apps/cli/options.cpp:368` 与 `src/serve/serve_options.cpp:793` 解析 `--kv-dtype` → **集中 struct `ninfer::EngineOptions`（`include/ninfer/types.h:353-486`，`kv_cache` 字段在 `:422`，并发 `max_concurrency` 在 `:415`）** → `src/runtime/engine/model_instance.cpp:550`（并写进 artifact 身份 tag `:177` 的 `;kv=`）→ `startup.cpp:1352` → `DecoderStateSpec.kv_storage`（`decoder_state.h:23`）→ pool 几何 / workspace / route（`startup.cpp:453,527,567,669`）。manager 侧是 JSON 参数表（`apps/windows-manager/config/profiles/s-128k.json:15`）。

内存核算链（tail 必须同步改的点）：`paged_kv_storage.h:51 physical_bytes_per_token_head()` → `src/models/qwen3_5/load.cpp:189-194` → `decoder_state.cpp:191 kv_payload_bytes` → `startup.cpp:347-359` 与 `:1372-1395 SequenceCapacityCurve{bytes_per_additional_main_page_group}` → `src/runtime/engine/kv_capacity.cpp:104,226` → `types.h:1294-1331 MemorySummary` → `docs/config-calculator.html:490,524` 的每 token 字面表。

---

## 4. 实测收益与代价（含反面证据）

### 4.1 precision tail：质量换显存，几乎不动速度
- **质量**（Gemma 4 31B Q5_K_S，`docs/beellama-features.md:548-557`，本人核验）：q5_0 tail0→1024，median KLD **0.061747 → 0.038596**，P99 9.084101 → 7.410694，Same-top 76.802% → 80.185%；kvarn5 tail0→1024 median 0.041221 → 0.035761。
- **速度**（Qwen3.8-27B IQ3_S + kvarn5，RTX 5070 Ti，b2048/ub512，`build-logs/perf/v5-tail-speed-f16.jsonl`）：depth 16384 tail0 pp 1366.66 / tg 47.49 → tail1024 pp 1511.07 / tg 48.22（**+10.6% / +1.5%**）；depth 65536 986.18/39.33 → 1064.09/41.10。tail 0→4096 的 decode 变化约在 -3.4% ~ +3% 噪声带内。
- **精度选择敏感**：BF16 tail 同配置明显更差（depth 65536 tail1024 pp 947.72 / tg 38.47，`v5-tail-speed-bf16.jsonl`）→ 该卡必须用 F16 tail。
- **显存代价是净增**：tail **增加**常驻显存。q5_0 ctx16384 从 859.38 MiB 增至 1327.73 MiB（+468.36，其中 exact history+reserve 880.86、transient 249.75、peak 1577.49，`features.md:563-577`）。tail buffer 恒定不随 ctx 增长（64.25 MiB，`v2-proof.tsv`）——这与 §2.4 的 ninfer 估算（N=1024 ≈ 64 MiB）一致。
- **bodyless 是唯一真省显存形态**：tail 覆盖整个窗口时用 800 MiB exact 替换 412.50 MiB 量化 body（Gemma SWA 场景，`features.md:575`）；`v5-tail-memory.jsonl` 显示 tail 大于 group 时 KVarN 退化为纯 F16 cache，24 MiB staging 归零。**注意 ninfer 的 full_attention 层窗口极长（原生 262144 ctx），bodyless 分支基本用不上。**
- 反面：无 native 入口的设备（如 Metal）开 tail 反而使 pp 39→112、tg 5.8→9.0 变差，故 beellama 默认拒绝并归零（`src/llama-context.cpp:457-469`，原文注释 `:463-465`）。

### 4.2 KVarN：收益强依赖 tail，且官方自己标 experimental
- **tail=0 时 KVarN 比 q4_0 更费显存**（`docs/beellama-features.md:119-130`，本人核验原表）：Qwen 16K request=0 → +35.1%（292.50 → 395.21 MiB）；Gemma 16K request=0 → **+139.4%**（703.12 → 1683.66 MiB）；matched tail 1024 时为 Qwen16K −11.1%、Qwen64K −16.3%（**表中无 2048 行**，2048 仅出现在正文叙述，v1 把 1024/2048 并列表述有误）。文档明说 request-zero 是"explicit architectural tradeoff"（内建 F16 suffix + compression staging 不可关）。
- 吞吐：对同深度 bf16 的完整区间为 **0.839–1.330×**（Qwen 行 0.983/1.060/1.176，Gemma 行 0.839/1.116/1.330，`features.md:106-111`）。**v1 只取 Qwen 子区间 0.983–1.176× 属于裁剪**；且该文档紧接着声明这些比值"predate compact current-source storage…retained as baseline evidence, **not as a performance claim**"，引用时须带此免责。
- 成熟度：`CHANGELOG.md:101`（v0.3.1）原文 "Added **experimental** KVarN KV-cache compression"（本人核验）；v0.4.0 `:90` 才把 "New KV cache precision tail (KVCPT)" 列出；全仓无任何 stable 标记。
- 明确不支持：DSV4/MLA DSpark latent cache（非稠密 K/V，fail closed）、Self-Extend、Vulkan D64、HIP/Vulkan/异构多卡/多模态 DFlash draft KVarN 未合格；state 格式跨版本 fail closed 且无迁移 shim。
- **诚实标注证据缺陷**：`build-logs/combo/v1-kv54.log:2273` 打印 `"KVarN = 0.00 MiB, equivalent F16 = 16384.00 MiB"`（本人核验）——KVarN 侧 0.00 MiB 是预分配/fit 阶段的记账异常（同文件 12083 行有真实的 3359.19 MiB），**不可用作收益证据**；但 "equivalent F16 = 16384 MiB / 262144 tok = 65,536 B/tok" 这一折算仍可用于确认字节几何。该日志另一有效读数：16 GB 卡上 10.8 GB 权重 + **256K ctx 仅占 3,508 MiB context**。
- `build-logs/perf/v5-tail-*.tsv`（speed-f16/speed-bf16/memory）三个文件为 0 字节；`v5-tail-memory.jsonl` 是 `-d 0` 的 512-token 结构探针，不能用来算节省。

### 4.3 16 GB 目标卡上的实际数字（**v1 此处归因错误已改**）
RTX 5070 Ti 16275 MiB、Qwen3.8-27B IQ3_S、kvarn5：峰值 13,164 / 13,554 / 14,255 / **15,715** MiB @ 16K/32K/64K/128K（`build-logs/sweep/summary.tsv`）。同机 f16 KV 的 `c64k` 只能跑到 25.7/10.1 t/s，而 kvarn5 为 268.0/45.9（`sweep/summary2.tsv`）。

**关于 f16 KV 为何慢——v1 的"近全量 CPU offload"是错的，日志直接证伪**：`summary2.tsv` 每一行 `offload` 列都是 `65/65`；`c64k_f16kv.log:2166` 原文 `offloaded 65/65 layers to GPU`，`:5599` 的显存分解为 `CUDA0 | 16275 = 0 + (15259 = 10827 model + 4245 context + 186 compute) + 1016 unaccounted`——**所有层都在 GPU 上、0 MiB free**，且 fit 阶段报 `cannot meet free memory target ... need to reduce device memory by 1396 MiB`（`:2345`）。因此正确的说法是：**f16 KV 把 16 GB 卡推入超额提交（over-commit）状态导致严重退化，而非 CPU offload**；具体退化机制（分配重试/驱动换页）需重测确认，本报告不再给出因果结论。

**结论不变**：在 16 GB 卡上"KV 放不下"确实是真实瓶颈，量化 KV 的价值主要是避免超额提交/offload 而非省几个 GiB。但 ninfer 已用 rk8v4/rk4v4 占据了同一位置。

---

## 5. 移植可行性判定

### 5.1 KVarN 本体：**在 Phase 0 证据前不做整体移植**（判定：暂负，可被实验推翻）
1. **动机仅在"字节档位"意义上不成立**：显存维度被 ninfer 自己的 rk4v4/nvfp4 覆盖（§3.2）；数学基元（WHT 旋转）ninfer 已有。**但"每字节质量是否更优"无证据**，故这里是"不值得在无证据时投入全量移植成本"，而非"确定更差"。
2. **成本极高**：4 个 ggml op 的参数契约 + ~10.9k 行 CUDA kernel（六路由 + portable + MMA case）+ CPU 等价实现 + 私有 state v16 序列化 + `ggml_backend_meta` split-state + placement 组件。ninfer 无 ggml runtime 依赖（`CMakeLists.txt` / `src/CMakeLists.txt` grep `ggml|llama_` 命中 0，仅有 `third_party/ggml-quants` 权重 codec archive），等于**接口层 + 全部 kernel 重写**。
3. **契约冲突**：128-token record group vs ninfer 64-token page；D64/D128/D512 记录 ABI vs ninfer 量化 KV 一律 D256；KVarN 的内建不可关 128 suffix + 向上取整到 128 组，与 ninfer 的 frontier/checkpoint 语义冲突。
4. **成熟度风险**：官方全程 experimental、fail-closed 面宽、跨版本无迁移。

**唯一合理的借用姿势**：把 `src/llama-kvarn.cpp:824-930` 的 **Sinkhorn 双轴归一化 + per-column scale**（纯 C++、零 ggml 依赖）作为 ninfer 第 10 种 `KvCacheStorage` 实现——走 ninfer 自己的 plane schema 单一真源（`include/ninfer/types.h:49` + `src/core/paged_kv_storage.h:58`）、codec（`src/ops/kv_cache/append/*_codec.cuh`）、FP64 oracle 与 `bench/ops/kv_cache_append_bench.cu` 模板。**但前提是先做 §6 的收口实验证明每 bit 质量确实优于 rk4v4，否则不必动。**

### 5.2 precision tail：**建议移植**（判定：正）
理由见 §3.4 的四个既有承接条件。加上 tail 与 KVarN 弱耦合（可独立移植，body 用 ninfer 现有的 rk8v4/rk4v4 即可），以及收益形态契合 ninfer 的定位（16 GB 卡上质量档，decode 代价在噪声内）。

**必须接受的三条约束**：
1. tail 是**净增常驻显存**（`(N+R) × S`）。定量：ninfer 上 N=1024 ≈ **64 MiB/序列**（§2.4），`C=1..8` → **~64–576 MiB**；在 16 GB 卡上会直接挤占 ctx 预算 → 必须以"质量档"而非"省显存档"发布，并**按并发计**进入 `config-calculator.html` 的显存估算。
2. 只作用于 full_attention 层（ninfer 是 full + GDN linear 混合，约 1/4 层有 paged KV，实测 16/65）→ 收益分母比 beellama 小。
3. body kernel 必须导出 FP32 (row max, denom)。ninfer 的 small-T partial 已经是 `(acc, m, l)` 三元组（§3.4），**这一条近乎免费**——但跨 INT8/F16 值域的合并见 §5.3 第 4 条。

### 5.3 tail 移植的四个真难点
| 难点 | 具体位置 | 说明 |
|---|---|---|
| **frontier 每轮移动 × CUDA Graph 合同** | `include/ninfer/ops/softmax_attention.h:207-216`、`src/models/qwen3_5/program/planning/` 图配置 | route family 不同不能进同一 executable。tail 边界随 decode 前移会触发 family 切换。缓解：把 tail 池长度**固定为 N+U 的静态 arena**（beellama 正是这么做的：ubatch 只进 `attention_stride=N+U`，不改持久容量），使 graph key 只含 (N,R,type) 而非当前长度 |
| **容量曲线的线性假设** | `startup.cpp:1372-1395 SequenceCapacityCurve` → `src/runtime/engine/kv_capacity.cpp:104,226` | 现假设单一 `bytes_per_additional_main_page_group`。tail 引入 `(N+R)×S` 常量项（**须乘并发**）+ 非线性混精度 → curve 与 `MemorySummary`（`types.h:1294-1331`）、`load.cpp:189` 的 LayerCost 都要改 |
| **跨 tier 搬运与前缀复用** | `src/core/host_kv_arena.h:18-33`、`src/core/disk_kv_store.h:27-45`（identity+CRC）、`src/models/qwen3_5/program/prefix/hybrid_cache.h:30,117-133`（内容寻址，block = 1 full page） | 现按等长 page stride 搬运、按内容 digest 复用。tail 页与 body 页尺寸不同 → 搬运、命中判定、身份校验要同时容纳两套 stride；`docs/maintainer/paged-kv-cache.md §4.5`「pool 内 page group 等价」这条所有权边界要改写 |
| **INT8 body 与 F16 tail 的数值域合并**（**v2 新增**） | `small_t_i8.cuh:69,945-953`、`mma.cuh:64`、`small_t.cuh`（reducer） | ninfer 的 INT8 家族在 **s8 tensor core 内直接吃量化码**，body 的 QK/PV partial 处于"量化码 + per-row scale"域；tail 是 BF16/F16 域。合并必须把两者的 QK 缩放（s_q·s_k）与 PV 累加统一到同一 FP32 参考域后才能做 online-softmax。报告确认 partial 三元组存在，但"跨值域一致"这一条**没有被现有代码证明是免费的**，应作为 Phase 1 的第一个可证伪里程碑 |

另需自建而 ninfer 没有的概念：group（full/swa）分派、tail coverage 观测 API、rollback reserve、`keep_last_writes` 式同 slot 去重。

### 5.4 许可：**不在本次评估范围**（用户指示）
按本次指示，许可证/血缘不作为决策因素与门槛。仅备注供参考：beellama `LICENSE` = MIT（`Copyright (c) 2023-2026 The ggml authors, Anbeeld`），ninfer 根 `LICENSE` = Apache-2.0，两者兼容；KVarN 原文自述为 "Huawei's calibration-free … quantizer"（`docs/beellama-features.md:11`）。本报告后续不再对移植文件的 derived-from / 上游 commit 登记作要求。

---

## 6. 建议路线

**Phase 0（决策前置，1 天内可完成，成本极低）**
1. **硬件基线已定**（本次）：本机 RTX 5070 Ti 16 GB / sm_120a。仓库文档矛盾（`AGENTS.md:34` sm_86/3090 vs `README.md:17` sm_120a/5070 Ti）**不阻塞本项目**，建议后续单独修正。tail dtype 按 5070 Ti 实测取 **F16**（BF16 同条件低 7–11%，§4.1）。
2. **收口 KVarN 问题的实验**（这是决定 Phase 2 要不要做的唯一依据）：按 `precision-tail-port-plan.md §0.5` 的装配，在**同一模型（IQ3_XXS）**上跑对比 —— 成品 `ninfer-package` 侧 `rk4v4` / `rk4v4 + tail`（有/无投机两态），对照成品 `llamacpp` 侧 `kvarn4 + tail1024`（**`--spec-type none`，不开 MTP**），并统一指标口径（建议**两边都补 KLD，或都补 ppl**；ninfer 侧已有 rk4v4 ppl 基线 `performance.md:460` 可作对照）。若 rk4v4 在同字节档不劣于 KVarN，**永久放弃 KVarN 移植**。

**Phase 1：precision tail（推荐做）** —— 详见另文 `precision-tail-port-plan.md`，最小形态 = **第二个同几何 BF16/F16 exact 池 + 复用现有 split-partial reducer**，不改 plane 不变量。

**Phase 2（条件触发）**：仅当 Phase 0 实验显示每 bit 质量确有显著优势，才把 Sinkhorn 双轴归一化 + per-column scale 实现为 ninfer 的一种新 `KvCacheStorage`（走 plane schema 单一真源 + codec + oracle，**不移植 ggml op/kernel/state**）。

**Phase 3（可选）**：bodyless（tail 覆盖窗口时省 body）——对 ninfer 的长窗口 full-attention 层基本无价值，可跳过。

**明确不做**：KVarN 的 ggml op 家族、六路由 CUDA kernel、`KVRN` state v16 序列化、Vulkan/HIP 路径、`ggml_backend_meta` split-state、多 GPU placement。

---

## 7. 核验说明与证据局限（供审阅者参考）

**由报告作者直接读取原始文件/命令核验的关键结论**（可信度高）：
- ninfer 9 种 `KvCacheStorage`（枚举在 `types.h:49-73`，layout 9 个 case 在 `paged_kv_storage.h:58-121`）与 `head_dim == 256` 硬约束、throw 在 `:120`
- page = 64 token —— 读 `src/core/paged_kv_cache.h:19`
- `KVAR_N_GROUP = 128` —— 读 `src/llama-kvarn.h:10`
- 36 个 `LLAMA_KVARN_KxVy_G128` 枚举 + `llama_kvarn_params` 字段 —— 读 `include/llama.h:201-235`
- 4 个 `GGML_OP_KVARN_*` op 名及其"不进 GGUF"注释 —— 读 `ggml/include/ggml.h:604-609`
- kvarn 文件数 102 —— `git ls-files | grep -i kvarn | wc -l`
- ninfer 每 token 字节表（bf16 65536 / rk4v4 17920 / nvfp4 18432 / rk8v4 26112 / k8v4 25728 / int8 33792 / fp8 33024）—— 读 `docs/config-calculator.html:490`
- **ninfer rk4v4 ppl = 4.352432（+0.214% vs int8，decode ≈ rk8v4）—— 读 `docs/performance.md:460`（v2 新增）**
- beellama README 质量阶梯（bf16 4096 MiB、k5v5+1024 = 1488 MiB 36.3% KLD 0.000897 …）—— 读 `README.md:28-52`
- tail=0 反例表（Qwen16K +35.1%、Gemma16K +139.4%）—— 读 `docs/beellama-features.md:119-130`
- tail 质量表（q5_0 0.061747→0.038596、Same-top 76.802→80.185）—— 读 `docs/beellama-features.md:548-557`
- `CHANGELOG.md:101` 的 "experimental" 原文、`LICENSE` 版权行 —— 直接读取
- `v2-proof.tsv` 的 22,016 B/token 差商与 tail 恒定 64.25 MiB —— 由 4 个 ctx 数据点独立计算
- `v1-kv54.log:2273` 的 `layers = 16`、`KVarN = 0.00 MiB, equivalent F16 = 16384.00 MiB` —— grep 原文（并注意 0.00 为记账异常）
- `c64k_f16kv.log:2166` 的 `offloaded 65/65 layers to GPU` 与 `:5599` 显存分解、`:2345` fit 超目标 —— **读原文，推翻 v1 的 CPU-offload 归因（v2 新增）**
- ninfer 已有 `partial_acc/partial_m/partial_l` + shared reducer —— 读 `small_t_i8.cuh:69,945-953`、`small_t.cuh:7`
- ninfer 已有多异构池并存 —— 读 `decoder_state.cpp:107-118`、`startup.cpp:253-286`、`decoder_state.h:114,128`
- ninfer 支持架构 80/86/89/120a、默认 86 —— 读 `CMakeLists.txt:9,14`
- ninfer 无 ggml/llama runtime 依赖、`kvarn` 全仓 0 命中、无 precision-tiered 设计文档 —— grep

**主要来自子 agent 报告、作者未逐行复核**（数字有出处，但审阅者若要引用作决策依据建议回读原文）：
- KVarN 的 25,287 行分类统计、六路由名称与 sm 约束的具体行号（`fattn-kvarn-route-policy.h:195-259`）
- `llama-graph.cpp` 213 处 / `llama-context.cpp` 68 处引用的语义归类
- tail 的 state manifest 字段、`prepare/commit/cancel_seq_cp` 事务细节、bodyless 时 `allocated_capacity_tokens` 的具体行
- ninfer 的 GQA 注册几何、sm_86 FP8 stub 行为、route table 行号（`causal_softmax_attention.cpp:347-402`）
- `v5-tail-speed-*.jsonl` 与 `sweep/summary*.tsv` 的逐行读数（这些日志**未记录 n_ctx**，三档重复数字对应的上下文长度是推断的）

**已明确排除的无效证据**（不要采信）：
- `v1-kv54.log` 的 `KVarN = 0.00 MiB` 行不能作为显存节省证据
- `build-logs/perf/v5-tail-*.tsv` 三个文件为 0 字节
- `v5-tail-memory.jsonl` 是 512-token 结构探针，不能算节省比例
- `beellama` 引用的 `docs/development/std-quant-kv-precision-tail.md` 等开发文档在本仓库不存在
- KVarN 质量阶梯来自 **Qwen 3.6 27B / RTX 3090**，官方明写 "not a universal model guarantee"；beellama 与 ninfer 全部日志**均无同一 artifact 上的 ppl 直测**

**本报告的判断性结论（可被审阅者挑战）**：
1. "不整体移植 KVarN"依赖 §3.2 的**字节档位**对比 + "每字节质量无证据"这一事实缺口。**注意：这与 v1 的措辞不同——结论是"在缺乏 Phase 0 证据前不投入全量移植"，而非"确定更差"。** 若 Phase 0 实验显示 KVarN 在同字节档 KLD/ppl 显著优于 rk4v4，该结论需下调为"借算法，不移 kernel"。
2. "tail 建议移植"依赖 ninfer 的 partial reducer 与多池先例足以承接 —— 若 §5.3 的 CUDA Graph route-family 切换实测代价过高（图重捕获频繁），最小形态应退化为"tail 长度固定到 page 边界 + 同批双 kernel"，性能收益会缩水但功能与质量收益保留。**§5.3 第 4 条（INT8/F16 跨值域合并）是 v2 新增的最大技术风险，须作为 Phase 1 首个里程碑验证。**
3. ninfer 只有 full_attention 层有 paged KV（实测 16/65 层），因此 tail 的绝对显存/质量影响面**小于 beellama 上的实测值**；报告已补出量级估算（N=1024 ≈ 64 MiB/序列，随并发放大），但 **ninfer 上的精确数字需 Phase 1 自测**。

---

## 附录 A：v1 → v2 勘误表

| 位置 | v1 原文（错误） | v2 更正 | 证据 |
|---|---|---|---|
| §0/§3.2 | ninfer rk4v4「未单独公布 ppl」 | rk4v4 **有**公开 ppl：4.352432（+0.214% vs int8，decode ≈ rk8v4） | `docs/performance.md:460` |
| §4.3 | f16 KV 慢 =「近全量 CPU offload」 | 日志证伪：`offloaded 65/65 layers to GPU`、0 MiB free、fit 超目标 1396 MiB；改为超额提交 | `c64k_f16kv.log:2166,2345,5599`；`summary2.tsv` 全行 65/65 |
| §3.5 | 「sm_86/sm_89 无 FP8 tensor core」 | sm_89(Ada) **有** FP8 TC；真实约束是 `kind::f8f6f4` route 限定 Blackwell | `fp8_sm86_stubs.cpp:15-18` |
| §4.2 | 吞吐「0.983–1.176×」 | 完整区间 0.839–1.330×；并标注官方"baseline evidence, not a performance claim" | `features.md:106-111,116` |
| §4.2 | 「matched tail 1024/2048 → −11.1%/−16.3%」 | 表中无 2048 行；−11.1%=Qwen16K/1024，−16.3%=Qwen64K/1024 | `features.md:119-130` |
| §3.2 | k8v4「−25%」出处 `performance.md:576-594` | 该文件仅 521 行；真实出处 `config-calculator.html:591` | `wc -l`；grep 原文 |
| §1.2 | 行数合计 25,588 | 四项合计 25,287（301 行在 fit-kvarn-tail + hip 脚本） | 逐项 `wc -l` |
| §1.4 | 4 条 CUDA 约束在 `fattn-kvarn-dispatch.cu` | 3 条在 `fattn-kvarn-route-policy.h:195-259` | grep 原文 |
| §3.5 | `attn_prompt_fast`/`attn_pv_f16` 在 `device_route.h:7-44` | 实际在 `prompt.cu:33,240`、`launch.h:23`、`device-profiles.json:400,406` | `device_route.h` 无此串 |
| §2.2 | `DEGRADED_PAYLOAD_INVALID` 在 `llama-kv-cache.cpp:2966-2976` | 宏定义在 `llama-kv-cache-tail.cpp:643` | grep 原文 |
| §3.1 | `hadamard_d256.cuh` 在 `kv_cache/append/` | 实际在 `src/ops/kv_cache/hadamard_d256.cuh` | 文件系统 |
| §3.1 | `KvCacheStorage` 定义在 `paged_kv_storage.h` | 枚举在 `include/ninfer/types.h:49-73`；`paged_kv_storage.h:58-121` 是 layout 函数 | 读原文 |
| §1.3 | `ggml.c:6709-6850` | 实际 `:6711-6903`（含 materialize 构造） | 读原文 |
| §1.3 | FLASH_ATTN_EXT 新增 3 参数 | 实际 5 个 op_param | `ggml.h:452-459` |
| §1.1 | `sinkhorn_iters=16` 在 `:824-879` | 默认值在 `:198`，实现在 `:824-879` | 读原文 |
| §2.1 | 「CLI/auto 默认 1024」 | CLI 默认 0，`auto`=1024 | `arg.cpp:2756`、`llama-kv-tail-request.cpp:117` |
| §2.2/§2.5 | MLA/DSA 拒绝正 tail 在 `llama-model.cpp:1916` | 拒绝在 `llama-kv-cache-tail.cpp:437-441` | grep 原文 |
| §5.4 | 工作树 = `0ba48c55a` | 工作树 HEAD `58a162927`；`0ba48c55a` 仅对应 build-log | `git describe` |
| §2.4 | 未提并发乘数、无 ninfer 定量 | 新增：N=1024 ≈ 64 MiB/序列，×`max_concurrency`(1..8) | `types.h:415`；本文测算 |
| §5.3 | 3 条难点 | 新增第 4 条：INT8 body × F16 tail 跨值域合并 | `small_t_i8.cuh:69,945-953`、`mma.cuh:64` |
| §7 | 部分"本人核验"含无效日志数字 | 分级标注，无效日志单列 | — |
