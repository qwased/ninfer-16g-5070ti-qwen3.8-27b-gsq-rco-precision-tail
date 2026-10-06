# 在 ninfer-precision-tail 内移植 KVarN（K4V4 / K5V5 / K6V6）+ 精度尾部：实施计划

- 版本：v3（在 v2 基础上按 `kvarn-port-into-precision-tail-plan-review.md` 审阅报告 + 主代理一手复核修正）
- 日期：2026-10-07
- 目标仓库：`D:\ninfer\ninfer-precision-tail`（下称 **TAIL**）
- 目标硬件：**仅本机** NVIDIA GeForce RTX 5070 Ti 16 GB / **sm_120a** / Windows
- 目标模型：`D:\ninfer\ninfer-precision-tail-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`（实测 **11,092,477,952 B / 10.33 GiB**，mtime 2026-10-02 05:59）
- 一句话目标：**在 TAIL 内移植 beellama 式 KVarN 4/5/6（K=V 同宽）+ 精度尾部，prefill / decode / MTP 核心体验零负面影响。**

---

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
| 11 | 未引用同仓既有负面裁决 | **新增 WP0.5-B：准入实验前置**，正面引用并反驳 `PORT-DOD.md:23-32` | commit `8e34ad12` |
| 12 | §9「无公布吞吐/质量」 | 不成立：FORK `docs/performance.md:145,153-154,159` 已公布 KVarN K4V2-G128 的 tok/s 与「约 0.8% decode 代价」 | 原文核实 |
| 13 | 附录 A「`config-calculator.html:490` 是 KV 表」 | :490 是**权重 profile 散文**；每格式 KV 表在 **:529**；:550 为 `mtp3 kvRatio`；:563 为 35B 表 | 详见附录 A |
| 14 | 附录 A「1 页 sink + 3 个 BF16 尾槽」 | 实际**共 3 槽 = 1 sink + 2 动态尾**；24.0 MiB 数值按 3 槽算是对的 | `FORK paged-kv-cache.md:100-101` |
| 15 | 附录 B 命令 | vcvars 导入改用 `.deps/env-port.bat`；冒烟档位改 `rk4v4`；perplexity 补真实 `--corpus` 与 `--disjoint --score-topk 100`；ctest 正则补 `ninfer_kv_cache_append_test` | 逐条实测 |
| 16 | `state_image.cpp:120-146` 存 24 MiB 尾（v2 引用） | **该文件无 kvarn/tail 引用**（是 `continuation_hidden` 与 DFlash-local K/V）；尾池几何真源在 `decoder_state.cpp:118-161`。StateImage 是否复制尾池 → **WP7 待确认**，不再作为既成结论 | 一手核验 |

> 其余仍正确的原文内容（字节表、路线排除、两类 tail 歧义、显存修正、验收换仪器方向）在本版保留。

---

## 0. 结论摘要

| 项 | 判定 | 依据 |
|---|---|---|
| 路线 | **只有 R2 一条路：在 TAIL 内移植** | §2 —— 原计划推荐的 R1（以 `ninfer-rtx5090-mobile` 为基线）对本目标**不可行** |
| 移植难度 | **中高**：设备侧接口兼容（已证 `wave_splits` 默认参），但**宿主侧注册、页几何、共享面、旋转域为硬改造** | §4.1 —— kvarn 只依赖 5 个 device helper + 2 个 host 入口，但两处 host 硬 throw、44 文件共享面、页 64 进内核寻址、FORK 20 文件接入面 |
| 主要工作量 | **三项**：① 位宽参数化 k4v2→K=V∈{4,5,6}；② 旋转域尾部合并（WP6）；③ 共享面/宿主注册改造 | §4 / §5 / §6 |
| 与尾部协同 | 可组合，但要过**旋转域门禁 WP6**；未过之前 `kvarn + --kv-tail-tokens` 必须 **fail-fast** | §6 |
| prefill/decode/MTP 零负面影响 | 有支撑（纯增量 + 关闭态门禁），但**吞吐/质量必须本机自测**，且**已知外部尾部有代价**（−5.8% decode / −2.1pt 接受率） | §7 A1/A8 |
| 总工期 | **约 34–52 人日**（含 WP0.5 前置门 2 天；GPU 实测另计） | §7 |

**与 v1 的三处根本差异（重写主线，保留）：**

1. **路线反转**：v1 推荐 R1（fork 为新基线）并列为「优点：kvarn 与尾部都在」。**该优点为假**（fork 无 `small_t_tail*`、全库 0 处 `kv-tail-tokens`）；更硬的一条是 **fork 根本没有 GGUF 权重路径**（`src/artifact/formats.cpp:10-20` 只注册 9 种格式到 `fp8_e4m3fn_row_bf16`，代码内 `gguf` 0 命中），**加载不了本目标模型**（其清单 1189 张量中 398 个为 `gguf_blocks_v1`）。⇒ R1 出局。
2. **验收指标换仪器**：A4 不能用 `apps/perplexity` 的 ppl —— 它评 prefill，**结构上看不见尾部**（`PORT-MEMORY.md:385-393`）。改用 TAIL **已有**的 **decode-width KLD**（`apps/perplexity --score-width ≤8 --kld-base`，真实解析在 `apps/perplexity/main.cpp:175-177`）。
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
| A1 | KVarN 关闭时**零回归** | `ctest` 全绿 + **decode-only** tok/s 基线对照（同 `.ninfer`、同 prompt、重复 ≥3 次）+ 输出逐字节相同 | **decode tok/s `\|Δ\|≤0.88%`**（`PORT-MEMORY.md:766-769`）+ 逐字节相同 + `MemorySummary` 逐字相同。**删除 prefill 腿**（同二进制单请求 prefill 实测 `−29%…+8%`，噪声底过大不可用，`PORT-MEMORY.md §5.17(2)`） |
| A2 | 每档编解码与**独立 oracle** 一致 | FP64 Sinkhorn/RTN oracle + Hadamard oracle + 记录解码检查 + **位序往返**（pack→unpack 逐码比对） | 容差**按位宽**定义：`qmax=(1<<bits)-1` 推导的理论量化步长；**注明现有 `3.0e-4` 只在 4-bit 有效** |
| A3 | 主文本与 MTP 的 greedy 与 MTP-off 一致 | `ninfer_qwen3_5_mtp_greedy_parity_real_test`（**需先移植，见 WP0.5-A**） | MTP 深度 0..3、跨 ≥1 个 group 边界、上下文 ≥8K |
| A4 | 精度尾部在 KVarN body 上有**可测质量增益** | **decode-width KLD**（`--score-width 8`，协议 `--disjoint --score-topk 100`、32,767 评分 token） | 最小效应量 = **同档 rk4v4 在 N=1024 的 pairing 带（2.26–2.47×）的 50% ⇒ ≥1.13×**；**附 `same_top` 与 max-KLD 双指标**；tail on/off、重复 ≥3 |
| A5 | 每档显存与**修正后**的 §3/附录 A 表一致 | `MemorySummary` 实测比对 + 本产物实测权重 | ±5%，基准表须含：① KVarN **不可关**的 24.0 MiB/序列 sink+tail；② StateImage slot × 并发项（**WP7 待核实机制，见 §7-WP7**）；③ **本产物** `weightsBytes = 11,092,477,952`（**不是** `config-calculator.html:522` 的 17,093,490,688，那是 groupwise-int 产物，且原文免责「不适用于不同量化的权重产物」） |
| A6 | 尾行旋进坐标域后**逐位可控** | FP32 oracle 覆盖「KVarN body × 旋进 BF16/F16 尾」合并路径；tail=0 时输出逐位不变 | 精确 |
| A7 | 长解码跨 group(128)/ring(64) 边界无重复计数/丢键 | needle 检索 + 边界单测 | 精确命中 |
| A8 | **无负面体验**：pp/tg/MTP 接受率 | 三者与**同字节对手**对照 | **同字节对手：`k4v4↔{rk4v4, nvfp4}`、`k6v6↔k8v4`（逐字节相同，402 B/token/头）；`k5v5`（21,632）无同字节档 ⇒ 需另定判据**。pp/tg/MTP 接受率不得劣于同档噪声底；**并须承认外部尾部已知代价 decode −5.8% / 接受率 −2.1 pt**（`PORT-MEMORY.md:663-670`）——含尾部的档位按此基线放宽判据 |

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

---

## 6. 精度尾部协同与旋转域门禁（WP6）

### 6.1 事实（已独立复核）

- TAIL 的尾部合并**只对原始坐标 body 生效**（BF16 + INT8 族 + Prompt 路线）；旋转域 `fp8-e4m3 / nvfp4-g16 / k8v4` 的 `small_t_*.cu` **不含任何 tail/shadow 头**（被编进但不走）。
- **KVarN 就在旋转域**（K/V 都做 Hadamard，输出再旋回）。
- ⇒ `kvarn + --kv-tail-tokens` **不会自动生效**，若不处理就是**静默惰性**。**WP6 是独立门禁。**
- **fp8 无尾的真正原因是实现欠账，不是坐标障碍**：`append/kernel.cuh:35` 只旋 K（V 不旋），`small_t_fp8.cu:86` 用**未改动的共享 reducer** ⇒ fp8 只有一个被旋转的 KV 轴。

### 6.2 方案 A 按文义不可实现（三条一手反证）

1. **partial 累加器元素类型不同**：FORK kvarn 走 `decode_kernel.cuh:368 __nv_bfloat16* partial_acc`、`:926` reducer 收 `const __nv_bfloat16*`；TAIL tail 核走 `small_t_tail.cuh:56 float* partial_acc`、共享 reducer `small_t.cuh:262 const float*` ⇒ **不能共用一块 buffer**，「由 kvarn 自己的 reducer 一次合并」不成立。
2. **split 策略在目标工况分叉**：`decode_kernel.cuh:44-53 kvarn_decode_active_splits` 对 `QHeads==24`（正是本 27B）且 `window>8198` 覆盖 `cap = window<=122880 ? 41 : 82`（`config.cuh:16-18`）；TAIL 走 `small_t.cuh:145-180`。**长上下文下 body 与 tail 各自给出合法但不同的 split 集合**，而这恰是 A4/WP8 要测的区间。
3. **默认尾部 dtype 被旋转入口拒绝**：TAIL 默认 `kv_tail_type=Float16`（`types.h:469`），FORK `codec.cu:161-165 kvarn_hadamard` 对 `source.dtype != BF16` **抛 `invalid_argument`** ⇒ 必须**新写 f16 旋转入口**（独立交付物）。
4. **旋转时点未定**：读时旋转（每步对 tail 行做 Hadamard）≈ 尾部注意力 FLOPs 翻倍；**写时旋转**（append 期一次性，≈32768 元素/token）代价可忽略。两者差一个数量级。

### 6.3 修正后的方案（替代 v2 方案 A）

**优先 (b)，回退 (a)：**

- **(a) 照 TAIL `small_t_k8v4.cuh` 的形状做**（仓内已有模板）：K 与 V 都旋（`:208,229`）、核内旋 Q（`:263`）、归并 **FP32** partial（`:639-643`）、**归并后只做一次**反旋（`:653`）。即方案 A 想要的东西，改为沿用 TAIL 的 FP32 partial 契约。
- **(b) `ROTATED_K_ORIGINAL_V`**：若 V 不旋，KVarN 在结构上等价于 TAIL 已有的 fp8 body ⇒ 尾部接线退化为一个 launcher 分支，**可能零内核改动**。取证/落地前须确认上游契约提供该域（`ggml.h` 同时定义 `ORIGINAL` 与 `ROTATED_K_ORIGINAL_V`，见 `kvarn-kv-tail-feasibility-report.md:44`）。
- **(c) 最低形态**：直接暴露 KVarN 内建精确后缀（`kKvarnSinkPages=1` + `kKvarnTailSlots=3`，≈384 token 已近精确）作为「尾部」，把 `--kv-tail-tokens ≤384` 映射上去，属配置工作。
- **明确写时旋转**；**新增 f16 旋转入口为独立交付物**。
- **逐位一致这一要求本身可满足**：FORK `hadamard.cuh:9-31` 与 TAIL `hadamard_d256.cuh:46-65` 是**同一 Sylvester 顺序、同一符号约定、同一次 2⁻⁴ 归一**，两侧都未开 `use_fast_math`（TAIL `CMakeLists.txt` 只有 `/Zc:` 系列）。但注意 `hadamard_warp`（`hadamard.cuh:33`）是**死代码**，且 fork 是 256 线程/行、TAIL 是 1 warp/行，寄存器映射 `d = lane + 32r` 需重建。
- **执行顺序**：**在 WP4 之前做一次低成本探针**判定 (a)/(b)/(c) 哪条成立，而不是把门禁拖到 WP6 才暴露。

### 6.4 回退（硬约束）

- 若坐标域无法调和：**`kvarn:* + --kv-tail-tokens` 必须 fail-fast 拒绝**，交付缩回「仅 KVarN 三档、无尾部」。
- 禁止任何「分配了 ring 但不读不写」的静默路径进主干。

---

## 7. 实施计划（WP0 · WP0.5 前置门 · WP1–WP9）

> 每个 WP 给「产出 / 验收 / 回退 / 工期」。**WP0.5 为 v3 新增前置门，未过之前不投入 WP1–WP9。**

### WP0 — 基线与环境（1–1.5 天）
- 复用已配置的 `build-port/`（Ninja Release，`CMAKE_CUDA_ARCHITECTURES=120a`，CUDA 13.3，MSVC v143 14.44.35207，`BUILD_TESTING=ON`）；**不要重配**。
- **⚠ 构型口径**：`build-port/CMakeCache.txt:502 NINFER_SM120_NATIVE:BOOL=ON`，而 TAIL `CMakeLists.txt:11,26-30` 自述「120a is admitted locally as an **unqualified** target」「upstream's native routes … are a separate, unqualified code path, while the compatibility one is the **tested route**」⇒ 当前复用**未鉴定路径**。同时 TAIL `AGENTS.md:34,37` 仍写本仓目标 sm_86/RTX 3090/CUDA 12.8，并注「上游 route table 在 sm_86 重测误差 12–41%」——与 `wave_splits`/split-capacity 常量直接相关。**须在 WP0.5-C 决断。**
- 产出：① 现状冒烟（`ninfer-perplexity --score-width 8 --save-topk` 生成基线 topk）；② 一份「本机噪声底」报告（**decode-only** pp/tg 重复 ≥3 次的分布）；③ **取数前先确认独占**（本仓 AGENTS.md 提示常有人挂服务；审阅期间空闲显存波动 1901 MiB→15948 MiB）。
- **验收**：能对本机模型跑通一次 prefill+decode+MTP 与一次 KLD 基线。
- **回退**：无（前置门）。

### WP0.5-A — 移植 MTP parity 测试（0.5–1 天）〔新增前置门〕
- `test_engine_mtp_greedy_parity_real` 在 TAIL **不存在**（0 命中），只存在于 FORK `tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（注册名 `ninfer_qwen3_5_mtp_greedy_parity_real_test`，`tests/models/qwen3_5/tests.cmake:61`）。
- 移植该测试并纳入 TAIL 测试包（TAIL 现有 MTP 测试为 `ninfer_mtp_pack_test / ninfer_mtp_round_test / ninfer_qwen3_5_mtp_adaptive_test / ninfer_qwen3_5_mtp_graph_profiles_test`）。
- **验收**：`ctest -R ninfer_qwen3_5_mtp_greedy_parity_real_test` 通过。否则 A3 无仪器。

### WP0.5-B — 准入实验前置（≤1 天，ROI 最高）〔新增前置门〕
- **同模型同口径 decode-width KLD 三方对照**：`rk4v4` / `rk4v4+tail` / `k4v4+tail`。
- **正面引用并反驳 `PORT-DOD.md:23-32`（commit `8e34ad12`, 2026-10-05）的负面裁决**：该 M0 门禁用 wikitext-00 / ctx 4096 / 4 chunks，得 `f16 5.3580 / q8_0 5.3577 / kvarn4 5.3559 / kvarn4+tail1024 5.3622`，噪声 ±0.136，结论「没有质量驱动的理由引入 KVarN」。要么指出前测方法缺陷（4 chunks、ppl、±0.136 噪声——**正是本计划论证的仪器问题**），要么把 WP8 从「收口实验」前置为「准入实验」。
- 同时引用 `docs/performance.md:754-764` 的 kvarn4 KLD 行与 `.deps/wpc/kld-kvarn4-*.out`。
- **验收**：给出三方 KLD 与裁决，明确是否继续。

### WP0.5-C — 构型口径声明（0.5 天，可与 A/B 并行）〔新增前置门〕
- 声明 `build-port` 是 `NINFER_SM120_NATIVE=ON`（TAIL 自述 unqualified）：要么改回 compat 路径重测噪声底，要么在 A1/A8 写明噪声底与 route 常量属于 native 路径；并处理 `AGENTS.md:34` 的 sm_86 文档矛盾。
- **验收**：一页说明，A1/A8 引用它。

### WP1 — kvarn ops 原样移植（2–3 天）
- 复制 `src/ops/kvarn/`（12 文件 2980 行）+ 两个公共头；接 CMake；移植 `tests/ops/test_kvarn.cpp`（**1710 行**）+ kvarn bench（**385 行**）；在**测试层**跑通（不改模型分发）。
- **验收**：`ninfer_kvarn_test` 通过（codec/hadamard/cached attention/prefill slab/batched/cache lifecycle/27B/tail staging/speculative boundary/publication settlement；容差见 WP5）。
- **回退**：若某单测依赖 fork 独有的 `paged_kv_cache` 断言，只在测试内适配，不动 ops。
- **注意**：WP1「照搬」只覆盖 ops + 测试，**不覆盖 20 文件接入面**（那是 WP2/WP3）。

### WP2 — 页面几何 + 存储枚举（2–3 天）
- 加 `KvarnGroup128` 枚举（**追加末尾**）；`kv_page_tokens()`；放宽 `paged_kv_cache` 校验到 64|128；**三个 CLI parser + 6 处名字 switch** + 身份指纹 `;kvbn=<bits>`。
- **为 kvarn body 提供独立 page-shift**（§4.4 陷阱），审计所有经共享 `>>6` helper 的路径。
- **验收（A1 前半）**：**非 kvarn 格式**全量 `ctest` 全绿（TAIL 共 259 个测试），page 仍为 64，输出逐字节不变。
- **回退**：枚举与几何解耦，先只加枚举 + parser，几何单独提交。

### WP3 — 模型接入，k4v2 端到端（3–5 天）
- `causal_softmax_attention.cpp` 路由 + `small_t.cu:460-505` body 挂载（新增 `small_t_kvarn` 分支）；`execution/text.cpp` 派发 `ops::kvarn_attention`；`program/decode.cpp` 的 group 边界；`state/decoder_state.*` 视图与 sink/tail 张量；`program/storage/context.cpp` 注入/恢复；`program/planning/startup.cpp` 容量；`graph_profiles` 断点（1K/120K）；**两处 host switch 注册**（`d256_profile.h:87`、`paged_kv_storage.h:65`）。
- **验收**：`--kv-dtype kvarn:k4v2`（内部临时档）**能跑**；`ninfer_qwen3_5_mtp_greedy_parity_real_test` 通过（A3）。
- **回退**：若某处共享面无法隔离，给 kvarn 独立池几何分支，不改公共路径。

### WP4 — 位宽参数化 K=V ∈ {4,5,6}（8–12 天）〔核心增量〕
- 收拢 `KBits` 死常量与全部 `4*item`/`&15`/`Group/2`/`D/4` 字面量（**≈55–60 处**）；`RecordBytes/各 Offset` 变 `bits` 的 `constexpr` 函数；`qmax=(1<<bits)-1`；**4 套位解包统一**（含 `attention.cu:354-360`）。
- 保留 4-bit nibble 快路径 + 5/6-bit 位流慢路径 + `if constexpr(bits)` 分派；**显式声明「借 beellama 位流、不借其 128 列切片」**（否则字节表整体作废，H7）。
- 发布 `kvarn:k4v4|k5v5|k6v6`；默认 `kvarn:k4v4`（§10-D2）。
- **验收（A2）**：三档各自 FP64 oracle 全绿；**位序往返测试**（pack→unpack 逐码比对）；`test_kvarn.cpp` 扩到 4/5/6。
- **回退**：若某档不达标，先只发 `k4v4`，其余挂起。**不回退到 k4v2（不在发布集）。**
- **前置**：先做 §6.3 的低成本探针（判 (a)/(b)/(c)）。

### WP5 — oracle 与容差规范（2–3 天）
- 容差**按位宽**定义：现有树内 `3.0e-4` 随 `qmax=(1<<bits)-1` 变，**只在 4-bit 有效**；5/6-bit 按该位宽理论量化步长定义。
- oracle 必须 host 侧 FP64、与 kernel **零共享代码**；packed 输入用**存储的 scale 独立解码**后再比。
- **验收**：每档 oracle + 往返 + 记录解码检查三件齐备。
- **回退**：无。

### WP6 — 旋转域尾部合并（门禁，**10–15 天**）〔最高风险〕
- 见 §6.3：优先 (b) `ROTATED_K_ORIGINAL_V`，回退 (a) k8v4 形状；**写时旋转**；**新增 f16 旋转入口**为独立交付物；`--kv-tail-type` f16/bf16 双支持。
- **验收（A6/A7）**：`kvarn:k4v4 + tail{0,1024}` 的 decode-width **mean-KLD 单调下降**，tail=0 **逐位不变**；跨 group(128)/ring(64) 边界无重复/丢键。
- **回退**：**fail-fast 拒绝** `kvarn + tail`（§6.4）。

### WP7 — 容量 / 显存核算落地（1.5–2 天）
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
- **验收（A3/A8）**：MTP greedy 一致；graph 复用键含 `(profile,bits,N,R,type)` 且无需频繁重捕获；pp/tg/MTP 接受率不劣于同字节档噪声底（含尾部的档位按 −5.8%/−2.1pt 放宽）。
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
| **同仓已有负面裁决**（`PORT-DOD.md:23-32`） | ROI 单点风险 | **WP0.5-B 前置准入实验**，先证再投 |
| **构型未鉴定**（SM120_NATIVE / sm_86 文档矛盾） | 噪声底与常量前提不成立 | WP0.5-C 决断 |
| 无公布吞吐/质量 | 计划不确定 | **部分不成立**：FORK `docs/performance.md:145,153-154,159` 已公布 K4V2-G128 tok/s（407.6→412.4、238.1→240.1）与「约 0.8% decode 代价」；仍**不引用 beellama ladder 作结论**（其 30.1/36.3/42.6% 含 sink+tail+slice，与纯 codec 26.8/33.0/39.3% 不可直接互比） |
| 发布集不含 k4v2 ⇒ 失去 20.5% 档 | 收益缩水 | 已知取舍（§10-D4） |

---

## 10. 待决项

- **D1 存储建模**：单枚举 + `KvarnBits` profile（**推荐**）还是三枚举？（推荐理由：7 个穷举 switch × 3 的注册成本）
- **D2 默认档**：三处表述冲突须一次决清——`kvarn:k4v4` 为默认候选 / `--kv-dtype kvarn`（裸别名）是否等价 `kvarn:k4v4` / **`默认 --kv-dtype 不变`**（区分 default-format vs family-default vs bare-alias）。**须指派 WP 去决。**
- **D3 尾部 dtype**：默认 **f16**（对齐 TAIL 现状，`types.h:469`）。
- **D4 是否补 k4v2**：仅作 oracle 对照（**不在发布集**，且**不加入 parser**；host oracle 不需要 CLI 档）。
- **D5 fail-fast 覆盖范围**：WP6 前，`kvarn:* + --kv-tail-tokens>0` 直接报错。
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
| 外部精度尾部 ring（N=1024） | **64 MiB + 4 MiB reserve**，×并发 | `PORT-MEMORY.md:394` |

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
| ppl 看不见尾部 / KLD 仪器 | `PORT-MEMORY.md:385-393`；`apps/perplexity/main.cpp:175-177` |
| 同仓既有裁决 | `PORT-DOD.md:23-32`（commit `8e34ad12`）；`docs/performance.md:754-764`；`.deps/wpc/kld-kvarn4-*.out` |
| 尾部已知代价 | `PORT-MEMORY.md:663-670`（−5.8%/−2.1pt）；`§5.17(2)`（prefill −29%…+8%、decode ≤0.88%） |
| rk4v4 tail 增益带 | `PORT-VERIFY-REPORT.en.md:189-194`、`README.md:21`、`.deps/verify-b-summary.txt`（2.26–3.14×，tail0→tailN） |
| 27B 几何 16 full-attn 层 / 4 KV 头 / D256 | `FORK docs/maintainer/qwen3_5-model.md:60-64`；`geometry.cuh:15-16` |
| 构建树与本机 | `build-port/CMakeCache.txt`（Ninja/Release/120a/CUDA 13.3/14.44.35207/**BENCHMARKS=OFF/SM120_NATIVE=ON**）；`nvidia-smi` 5070 Ti / cap 12.0 / 617.14；模型 11,092,477,952 B |

*本计划仅记录移植方案，未对任何仓库做写操作。*
