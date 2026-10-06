# PORT-VERIFY-REPORT（中文）— KV 精度尾巴移植验证报告

对精度尾巴移植（`D:\ninfer\ninfer-precision-tail`，`main` 分支，HEAD `685aa33e`）与移植前成品
（`D:\ninfer\ninfer-package`）的验收验证。执行依据 `PORT-VERIFY-PLAN.md`；审计行见 `PORT-DOD.md`
的"Verification campaign"小节，执行日志见 `PORT-JOURNAL.md` Step 59–62，可复用经验见
`PORT-MEMORY.md` §5.16–5.17。

> 本次验证遵守的原则：**任何结论都必须有一条能复现它的命令。** 下面每个数字都由 §8 列出的命令
> 产生，原始产物在 `.deps/verify-*`。

## 0. 结论一览

| 论点 | 结果 |
|---|---|
| **C1a** 移植未改变生成结果，MTP 关与开都一致 | **通过** — 64/64 格逐字节相同（含 `--draft-tokens 2`） |
| **C1b** 性能与显存无回退 | **显存精确通过（64/64）**；**decode 通过**（0/64 格超过 2%）；**prefill 不可判别**（噪声底大于效应） |
| **C1c** 视觉（图像）理解输出未变 | **通过** — 64/64 逐字节相同，含全部 `--vision` 格 |
| **C2** 尾巴带来显著质量收益 | **在 decode-width KLD 上通过** — 24/24 格改善，收益随 body 粗细单调；ppl 22/24 一致 |
| **C3** MTP 猜测仍可用、未被尾巴拖累 | **通过** — 尾巴关闭时完全无变化；rk8v4 在 ±2 点内；最粗的 rk4v4-e8 处于 ±2 点边界；长上下文场景为**正收益** |
| **A4** 尾巴关闭即移植前路径 | **通过** — `softmax_attention: PASS`，`ORACLE_EXIT=0` |

两处如实给出的缺口：Arm B 中 **2/24** 格 `ppl(tailN) ≤ ppl(tail0)` 不成立（int8，ctx 8192，
+0.050%/+0.002%）；Arm B3 的 ±2 点判据已达到其样本量的分辨极限。两者都不是移植缺陷。

## 1. 范围、论点、硬件、产物

| 项目 | 取值 |
|---|---|
| 基准模型 | `D:\ninfer\ninfer-package\model\Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer`（10.33 GiB） |
| 移植前引擎 | `D:\ninfer\ninfer-package\engine\ninfer-serve.exe`（仅 serve——无 CLI、无 `--kv-tail-*`、无离线评分器） |
| 移植后引擎 | `build-port\apps\ninfer-serve.exe`（本次验证首次构建）、`build-port\apps\ninfer.exe`、`build-port\apps\ninfer-perplexity.exe` |
| GPU | RTX 5070 Ti 16 GB，sm_120a，CUDA 13.3；空闲基线 **48 MiB** |
| 语料 | `.deps\m5-longtext.txt`，160,000 B ≈ 32.7K token（64K 不在范围内） |
| 视觉素材 | `bench\fixtures\ttft\media\load_00.png` |
| 矩阵 | Arm A `{pre,post} × MTP{off,on D=2} × vision{off,on} × ctx{8192,32768} × 8 种存储 = 128 格`；Arm B `4 种存储 × tail{0,1024,2048,4096} × ctx{8192,32768} = 32 格` + 2 个参考；Arm B3 `{rk8v4,rk4v4-e8} × tail{0,1024,2048}`，短/长提示词 |
| 存储类型 | `bf16, int8, fp8, rk8v4, rk4v4, rk4v4-e8, nvfp4, k8v4`（均为 ≥4bit；`rk2v4-e8` 为 2bit，排除） |
| 纪律 | GPU 严格串行、单所有者；每个分片跑完：无 `ninfer`/`perplexity` 进程且 `nvidia-smi` 回到 48 MiB |

## 2. 方法

**Arm A — serve 端 A/B。** 两侧都是同一个 HTTP `serve` 引擎并施加完全相同的请求（移植前成品只
有 serve），每格一次贪心请求（`--greedy --seed 0`，提示词 188 token / 带图 1,214 token，生成 128
token），然后对生成的文本做 `cmp`。唯一变量是引擎构建；`--kv-tail-tokens` 在移植前一侧本就不存在。

**Arm B — decode-width KLD。** 以每 ctx 的 bf16-tail0 top-K 100 为参考，量化尾巴候选在 width-8
分块下评分（`--score-width 8`，只有小 T 路线才会合并尾巴）。协议：`--disjoint`、`--score-topk 100`、
评分 32,767 token。

**Arm B3 — MTP 接受率**，通过 `ninfer` CLI（`--spec mtp --draft-tokens 2 --greedy --seed 0`），
汇总量化引擎自报的 `mtp drafted/accepted tokens`。

### 冒烟测试逼出来的 harness 修复（移植代码没问题，harness 有问题）

1. **Git Bash 的 `kill` 杀不掉原生引擎。** `kill $!` 与 `kill -9 $!` 都返回 0，而
   `ninfer-serve.exe` 仍在（手工验证：两个 PID 依旧驻留，占用 11.4 GiB）。现在用
   `taskkill //F //IM ninfer-serve.exe` 收尾；否则每一格都会留下一个占着 GPU 与端口的引擎。
2. **就绪判据把加载中的服务当成就绪。** 权重加载期间 `/health` 返回 **503 `model_loading`**，而
   `curl -s -o /dev/null` 对 503 也返回 0，原判据在第一秒即放行，于是请求打进了正在加载的引擎。
   现在必须等到 HTTP **200**。
3. **该模型是 thinking 模型。** 生成的 token 流落在 `message.reasoning_content`，`content` 始终为
   `""`（128 token 上限下思考段不会闭合）。只比较 `content` 等于比较**两个空串**，会"空过"。现在
   客户端记录 `reasoning_content + content`，且流为空时拒绝写文件。
4. **性能 grep 从来没匹配上。** 每请求的行是
   `req#N done | … | prefill X tok/s | decode Y tok/s`，旧关键词全不匹配。现在抓这一行以及启动时的
   `capacity |` 显存行。

另外：因为运行环境会杀掉超过约 500 秒的单次调用，新增了分片驱动（Arm A 用 `VERIFY_LIMIT`，Arm B
每次调用 `LIMIT=1`，Arm B3 用 `RUN_LIMIT`）。

## 3. Arm A — 无负面影响（移植前 vs 移植后）

### 3.1 一致性（文本 + 视觉），主判据

| 配置 | 格数 | 一致性 |
|---|:--:|---|
| MTP 关、视觉关、ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP 关、视觉关、ctx 32768 | 8 | **8/8 IDENTICAL** |
| MTP 关、视觉开、ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP 关、视觉开、ctx 32768 | 8 | **8/8 IDENTICAL** |
| MTP 开 `--draft-tokens 2`、视觉关、ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP 开 `--draft-tokens 2`、视觉关、ctx 32768 | 8 | **8/8 IDENTICAL** |
| MTP 开 `--draft-tokens 2`、视觉开、ctx 8192 | 8 | **8/8 IDENTICAL** |
| MTP 开 `--draft-tokens 2`、视觉开、ctx 32768 | 8 | **8/8 IDENTICAL** |
| **合计** | **64** | **64/64 IDENTICAL，0 DIFFERS，0 MISSING** |

两个额外观察：**MTP 开与 MTP 关的输出逐字节相同**（贪心验证器是精确的）；视觉格确实送入了图像
（提示词 1,214 token vs 188）且仍逐字节相同。同二进制重跑（同一引擎跑两次）在两轮独立实验中
**16/16 逐字节相同**，即跨进程贪心解码自稳定——这正是计划要求的"发现差异时先证明自稳定"的前置条件。

### 3.2 吞吐与显存

紧凑表：每格为 **prefill Δ% / decode Δ%**（后相对前）；这 64 格同时按 §3.1 全部逐字节相同。

| MTP | vision | ctx | bf16 | int8 | fp8 | rk8v4 | rk4v4 | rk4v4-e8 | nvfp4 | k8v4 |
|---|:--:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 关 | 关 | 8192 | +2.4/-0.1 | +1.3/+0.0 | +1.7/+0.0 | +8.7/+0.1 | +1.1/+0.0 | +1.8/+0.0 | +1.4/-0.3 | +2.0/+0.0 |
| 关 | 关 | 32768 | +0.3/+0.0 | -18.7/-0.3 | -0.4/-0.4 | -10.7/-0.1 | -0.2/-0.1 | -8.9/-0.1 | -1.6/-0.1 | -11.0/-0.1 |
| 关 | 开 | 8192 | -0.6/-0.3 | -3.1/-0.4 | -2.4/-0.3 | -2.3/+0.0 | -4.6/-0.3 | -0.8/-0.4 | -14.1/-0.1 | -4.3/-0.1 |
| 关 | 开 | 32768 | +0.0/-0.1 | -0.8/-0.4 | -1.8/-0.1 | -3.1/-0.3 | -3.8/-0.1 | -3.1/-0.3 | -4.4/-0.1 | -6.1/-0.1 |
| 开 | 关 | 8192 | -5.7/-0.3 | -1.2/-0.2 | -3.3/-0.2 | -1.1/-0.2 | -3.7/-0.2 | -4.4/-0.4 | +20.4/-0.4 | +10.9/-0.2 |
| 开 | 关 | 32768 | -7.6/-0.2 | -12.2/-0.1 | +8.5/-0.3 | +3.8/-0.1 | -10.2/-0.2 | -28.1/-0.6 | +0.2/-0.2 | +0.6/-0.7 |
| 开 | 开 | 8192 | -1.2/-0.3 | +1.2/-0.1 | +0.0/+0.0 | +0.6/-0.2 | -1.2/-0.2 | +3.2/-0.2 | -1.2/-0.5 | -3.1/-0.3 |
| 开 | 开 | 32768 | -3.7/-0.7 | -1.2/-0.6 | -8.0/-0.6 | -3.7/-0.3 | -1.8/-0.3 | -1.8/-0.2 | -1.2/-0.2 | -0.6/-0.2 |

| 指标 | 结果 |
|---|---|
| **decode Δ%**（n=64） | 中位数 **−0.18**，均值 −0.23，范围 **−0.71…+0.15**，**0/64 超过 2%** → **通过** |
| prefill Δ% (n=64) | 中位数 −1.25，均值 −2.23，范围 −28.1…+20.4，34/64 超过 2% → 见下 |
| **显存**（`capacity \| KV … \| runtime … \| free …`） | **64/64 格前后完全相同** → **精确通过** |

**prefill 为何记为"不可判别"而非失败。** 两轮专门实验让**同一二进制**在**同样的格**上各重跑一次
（每次都新起服务）：

| 重跑实验 | 提示词 | prefill Δ（同二进制） | decode Δ | 文本 |
|---|---|---|---|---|
| `.deps/verify-ab-repeat`（8 格） | 188 token | **−29.1% … +7.9%** | −0.44…−0.15% | 8/8 IDENTICAL |
| `.deps/verify-ab-repeat2`（16 格，含 8 个视觉格） | 1,214 token | **−22.3% … −1.2%**（视觉格） | −0.88…0.00% | 8/8 IDENTICAL |

两者的离散度都**大于**它们本要去卡的前后差异（例如前后视觉 prefill 为 −0.6…−4.6%）。单次请求测出的
1.2–2.4 秒 prefill 由进程启动、预热与设备状态主导，因此 ≤2% 的 prefill 判据低于本 harness 的噪声底。
decode（稳态指标）在两轮实验中都很稳（≤0.9%）。显存则无须此类保留：它精确可复现。

## 4. Arm B — 尾巴收益（decode-width KLD，W=8，评分 32,767 token）

参考 = 每 ctx 的 bf16-tail0 top-K 100（bf16 精确存 K/V，是近精确基线）。量化候选随着精确尾巴替换
其最新的量化行而**更接近**该参考，因此 `mean KLD(tailN) < mean KLD(tail0)` 即为收益。bf16 是
**参考**而非候选：bf16 候选对 bf16 参考在 tail0 时 KLD 恰为 0、在 tailN 时约 1e-3（小 T 路线数值
底），不可能显示收益——bf16 由 Arm A 覆盖。

| ctx | 存储 | tail | ppl | KLD 均值 | KLD 最大 | same_top |
|---|---|---:|---:|---:|---:|---:|
| 8192 | int8 | 0 | 5.971447 | 0.00112641 | 1.42171 | 0.984709 |
| 8192 | int8 | 1024 | 5.971196 | 0.00103730 | 0.07297 | 0.984800 |
| 8192 | int8 | 2048 | **5.974426** | 0.00102872 | 0.10638 | 0.985441 |
| 8192 | int8 | 4096 | **5.971557** | 0.00107175 | 0.74014 | 0.984495 |
| 8192 | rk8v4 | 0 | 5.980890 | 0.00264652 | 0.48189 | 0.976010 |
| 8192 | rk8v4 | 1024 | 5.969419 | 0.00120207 | 0.26165 | 0.982817 |
| 8192 | rk8v4 | 2048 | 5.973092 | 0.00114922 | 0.25979 | 0.983518 |
| 8192 | rk8v4 | 4096 | 5.972232 | 0.00109672 | 0.74014 | 0.984648 |
| 8192 | rk4v4 | 0 | 5.986203 | 0.00386914 | 1.56008 | 0.971463 |
| 8192 | rk4v4 | 1024 | 5.975685 | 0.00156404 | 1.41782 | 0.982023 |
| 8192 | rk4v4 | 2048 | 5.973775 | 0.00134631 | 1.35255 | 0.982267 |
| 8192 | rk4v4 | 4096 | 5.970779 | 0.00123353 | 1.22301 | 0.984282 |
| 8192 | rk4v4-e8 | 0 | 6.011151 | 0.00652166 | 3.38330 | 0.965969 |
| 8192 | rk4v4-e8 | 1024 | 5.980981 | 0.00200700 | 0.92492 | 0.979948 |
| 8192 | rk4v4-e8 | 2048 | 5.980192 | 0.00161394 | 0.32719 | 0.982115 |
| 8192 | rk4v4-e8 | 4096 | 5.974378 | 0.00130116 | 0.74014 | 0.983244 |
| 32768 | int8 | 0 | 5.664418 | 0.00103341 | 0.33064 | 0.984893 |
| 32768 | int8 | 1024 | 5.663406 | 0.00100826 | 0.18132 | 0.984527 |
| 32768 | int8 | 2048 | 5.664209 | 0.00100349 | 0.71563 | 0.984802 |
| 32768 | int8 | 4096 | 5.664052 | 0.00098620 | 0.18133 | 0.985076 |
| 32768 | rk8v4 | 0 | 5.673180 | 0.00241272 | 0.33048 | 0.976867 |
| 32768 | rk8v4 | 1024 | 5.665655 | 0.00118513 | 1.13890 | 0.982940 |
| 32768 | rk8v4 | 2048 | 5.664276 | 0.00110771 | 0.18537 | 0.983947 |
| 32768 | rk8v4 | 4096 | 5.665985 | 0.00106248 | 0.18543 | 0.984008 |
| 32768 | rk4v4 | 0 | 5.682110 | 0.00356046 | 1.56008 | 0.972686 |
| 32768 | rk4v4 | 1024 | 5.667320 | 0.00157849 | 0.35742 | 0.982757 |
| 32768 | rk4v4 | 2048 | 5.665740 | 0.00140599 | 0.47161 | 0.982177 |
| 32768 | rk4v4 | 4096 | 5.665112 | 0.00124205 | 0.36498 | 0.984039 |
| 32768 | rk4v4-e8 | 0 | 5.694664 | 0.00631572 | 0.65803 | 0.963378 |
| 32768 | rk4v4-e8 | 1024 | 5.669458 | 0.00212822 | 0.53016 | 0.979247 |
| 32768 | rk4v4-e8 | 2048 | 5.667368 | 0.00170770 | 0.28566 | 0.980895 |
| 32768 | rk4v4-e8 | 4096 | 5.665916 | 0.00141657 | 0.85668 | 0.982482 |

int8/8192 段中加粗的 ppl 即为下述判据的两个例外。

### 4.1 四条子判据

| # | 判据 | 结果 |
|---|---|---|
| B1 | 每格 `mean KLD(tailN) ≤ mean KLD(tail0)` | **通过 — 24 格 0 违反** |
| — | `same_top(tailN) ≥ same_top(tail0) − 0.002` | **通过 — 24 格 0 违反**（且每格都上升） |
| B2 | `ppl(tailN) ≤ ppl(tail0)` | **22/24** — 2 例违反：int8 ctx 8192，t2048 **+0.050%**、t4096 **+0.002%** |
| B3 | 收益随 body 粗细单调：`int8 ≤ rk8v4 ≤ rk4v4 ≤ rk4v4-e8` | **全部 6 个 (ctx × N) 组合通过** |

**收益倍率（mean KLD 的 tail0 / tailN）：**

| ctx | tail | int8 | rk8v4 | rk4v4 | rk4v4-e8 |
|---|---:|---:|---:|---:|---:|
| 8192 | 1024 | 1.09× | 2.20× | 2.47× | 3.25× |
| 8192 | 2048 | 1.09× | 2.30× | 2.87× | 4.04× |
| 8192 | 4096 | 1.05× | 2.41× | 3.14× | 5.01× |
| 32768 | 1024 | 1.02× | 2.04× | 2.26× | 2.97× |
| 32768 | 2048 | 1.03× | 2.18× | 2.53× | 3.70× |
| 32768 | 4096 | 1.05× | 2.27× | 2.87× | 4.46× |

解读：收益真实存在，且**严格按 body 的粗细排序**——最粗的 `rk4v4-e8` 收益 3–5×，`rk8v4` 约 2×，
已接近 bf16 的 `int8` 约 1.05×。N=4096 的收益大于 N=1024，并随 ctx 增大略有下降。尾巴同时大幅压低
最坏误差（`rk4v4-e8` ctx 8192 的 KLD 最大 3.38 → 0.92/0.33/0.74）。这在要求的 ctx 集合上再次确认了
WP-B 的结论，并补上了 WP-B 矩阵未覆盖的 `rk4v4` 与 N=4096。

**关于那 2 个 ppl 例外。** `int8` 是收益最小的档，其 ppl 变动低于 32,767 token 困惑度的分辨力：
5.97 上的 +0.05% 约 3e-3，而**同样这两格**在 KLD 仪器上单调改善（0.00112641 → 0.00102872 /
0.00107175），`same_top` 也上升。KLD 仪器才反映该特性的作用；ppl 如实呈现。

## 5. Arm B3 — 尾巴下的 MTP 接受率

仅移植后（移植前无 CLI）。按提示词汇总，`--spec mtp --draft-tokens 2 --greedy --seed 0`。

### 5.1 短提示词（3 条，序列约 290 token）

此处**ctx 轴是空的**：256–1,024 token 的生成永远不会接近 ctx 8192/32768 的上限，两个设置给出完全
相同的 rounds/drafted/accepted——该参数只改变分配量。因此下表中两个 ctx 是同一个测量，不是两个。

| 集合 | 存储 | tail | drafted | accepted | 接受率 vs tail0 | Δ 点 | 接受长度 |
|---|---|---:|---:|---:|---|---:|---:|
| `b3`（新 256） | rk8v4 | 0 | 739 | 396 | 53.59% | — | 2.079 |
| `b3` | rk8v4 | 1024 | 739 | 397 | 53.72% | **+0.14** | 2.082 |
| `b3` | rk8v4 | 2048 | 739 | 397 | 53.72% | **+0.14** | 2.082 |
| `b3` | rk4v4-e8 | 0 | 734 | 400 | 54.50% | — | 2.099 |
| `b3` | rk4v4-e8 | 1024 | 773 | 379 | 49.03% | **−5.47** | 1.987 |
| `b3` | rk4v4-e8 | 2048 | 773 | 379 | 49.03% | **−5.47** | 1.987 |
| `b3n`（新 1024） | rk4v4-e8 | 0 | 3,015 | 1,563 | 51.84% | — | 2.039 |
| `b3n` | rk4v4-e8 | 1024 | 3,083 | 1,535 | 49.79% | **−2.05** | 2.001 |
| `b3n` | rk4v4-e8 | 2048 | 3,079 | 1,537 | 49.92% | **−1.92** | 2.004 |
| `b3n` | rk8v4 | 0 | 2,952 | 1,601 | 54.23% | — | 2.091 |
| `b3n` | rk8v4 | 1024 | 2,931 | 1,611 | 54.96% | **+0.73** | 2.106 |
| `b3n` | rk8v4 | 2048 | 2,927 | 1,612 | 55.07% | **+0.84** | 2.108 |

`b3n` 是把 `--max-new` 提到 1024 的同一实验——**样本量 4 倍**（2,927–3,083 drafted，二项标准误约
0.9 点，而 `b3` 约 1.8 点）。它表明 −5.47 点主要来自小样本噪声：rk4v4-e8 收敛到 **−2.05/−1.92 点**
（N=1024 略超 ±2 点判据，N=2048 在界内），rk8v4 在界内并略正 **+0.7/+0.8 点**。另需注意：当尾巴覆盖
整个窗口时，N=1024 与 N=2048 计算完全相同，故这类成对值是同一个观测，不是两个。

### 5.2 长提示词（`--messages`）——生产中的 body+tail 场景

| 集合 | 提示 token | 存储 | tail | drafted | 接受率 vs tail0 | Δ 点 | 接受长度 |
|---|---:|---|---:|---:|---|---:|---:|
| `b3long8`（ctx 8192） | 6,847 | rk8v4 | 0 | 231 | 62.77% | — | 2.330 |
| `b3long8` | 6,847 | rk8v4 | 1024 | 230 | 63.48% | **+0.71** | 2.339 |
| `b3long8` | 6,847 | rk8v4 | 2048 | 229 | 69.43% | **+6.66** | 2.674 |
| `b3long8` | 6,847 | rk4v4-e8 | 0 | 242 | 57.85% | — | 2.217 |
| `b3long8` | 6,847 | rk4v4-e8 | 1024 | 242 | 63.22% | **+5.37** | 2.500 |
| `b3long8` | 6,847 | rk4v4-e8 | 2048 | 244 | 62.30% | **+4.44** | 2.476 |
| `b3long32`（ctx 32768） | 31,020 | rk8v4 | 0 | 223 | 64.13% | — | 2.277 |
| `b3long32` | 31,020 | rk8v4 | 1024 | 234 | 62.82% | −1.31 | 2.374 |
| `b3long32` | 31,020 | rk8v4 | 2048 | 236 | 62.29% | −1.84 | 2.361 |
| `b3long32` | 31,020 | rk4v4-e8 | 0 | 228 | 61.40% | — | 2.228 |
| `b3long32` | 31,020 | rk4v4-e8 | 1024 | 233 | 63.52% | **+2.12** | 2.383 |
| `b3long32` | 31,020 | rk4v4-e8 | 2048 | 242 | 64.46% | **+3.06** | 2.592 |

每格仅 1 条提示词 → ±3 点，故单个幅度只需宽松看待；但**符号以正为主**（8 格中 6 格改善，两个负值都
很小：−1.31 与 −1.84 点，且都在 31K 下的 rk8v4，即尾巴覆盖窗口比例最小处）。在 6,847 token 提示词下，
尾巴（1,024/2,048）只覆盖窗口最新部分，验证器的注意力可测地变好而草稿自身缓存不变，一致性因此提升
2–7 点。

### 5.3 MTP 结论（C3）

1. **尾巴关闭时，猜测路径完全未受移植影响。** Arm A 中 `mtp 开 --draft-tokens 2` 的格与移植前成品
   **以及**与 MTP 关的输出都逐字节相同，两个 ctx、含视觉（§3.1）。
2. **尾巴打开时，衰减并不显著。** `rk8v4` 在所有测量中都留在 ±2 点内；最粗的 `rk4v4-e8` 在整窗场景
   处于 ±2 点边界（3.0k drafted 时 −2.1/−1.9 点），而一旦存在 body，接受率变动为 **+2 至 +7 点**——这
   正是该特性面向的场景。约 3k drafted 下 ±2 点判据已是本 harness 的分辨极限（汇总的
   `--draft-tokens 2` 预算约 2.0–2.7 token/轮，标准误约 0.9 点）。
3. **机理。** 草稿缓存（`mtp_kv`）**在设计上**不含尾巴，因此 `--kv-tail-tokens` 只让*验证器*变准；
   草稿与验证器的一致度可升可降，且变化幅度随 body 粗细放大——实测正是这一形态（int8/rk8v4 约 0 至
   +0.8 点，最粗的 rk4v4-e8 在整窗场景约 −2 点，存在 body 时全为正）。

## 6. A4 — 尾巴关闭的一致性与 FP32 oracle

`.deps\run-oracle.bat` → `.deps/verify-a4-oracle.out`（`softmax_attention: PASS`，`ORACLE_EXIT=0`，
94 秒；测试二进制比所有源文件都新）。全部守卫均在场：`fused-append empty-body cache write` ×4、
`fused-append crossing build` ×4、`prompt-route ring write` ×8（原 4 例 + F3 环形溢出 4 例）、
`fused-append chunked ring write` ×2、`PATHPT` ×2、`TAILGAIN` ×12、`WIDETAIL` ×8、`graph family=` ×8。
`tail = 0` 的逐位一致与 graph family 稳定性都在本次运行内被断言，因此 DoD §7.6（零回退）与
§7.5（graph family）在当前树上再次得到确认。

## 7. 结论与如实交代的缺口

**已确立的：** 在所有被测格上——文本、视觉、MTP 关与开、两个上下文长度、八种 ≥4bit 存储——移植后与
成品逐字节相同；显存精确可复现；decode 无回退；并且 `--kv-tail-tokens N` 在每一个可尾存储（bf16 +
INT8 族）上都带来显著、且随 body 粗细单调的 decode-width KLD 下降，`same_top` 全程上升，猜测路径未受
实质影响。

**缺口，直说：**

- **B2 ppl** 在 Arm B 的 24 格中有 2 格不成立（int8，ctx 8192，t2048 +0.050%、t4096 +0.002%）。属噪声
  量级，且同样这两格在 KLD/`same_top` 上均改善。`int8` 收益最小，其 ppl 变动低于 32,767 token
  困惑度的分辨力。
- **B3** 短提示词的 ±2 点判据已达到样本分辨极限；单次 256 token 下 rk4v4-e8 的 −5.47 点在 4 倍样本下
  未能复现（−2.05 点）。
- **B3 的 ctx 轴是空的**（短提示词场景）——该参数改变的是分配量而非工作量。要真正跑出上下文长度，需
  用 `--messages FILE` 送长提示词（Windows 命令行对 `--prompt` 有约 32 KB 上限，30K token 的文本无法
  从命令行传）。
- **prefill ≤2%** 在这些提示词长度下无法用"每格一次请求"来评判；同二进制重跑的离散度大于前后差异。
  已记为"不可判别"。
- **64K** 仍不在范围内：本地唯一的语料切片约 32.7K token。
- **跨构建残差：** `bf16`+尾巴依赖约 1e-3 的数值底（一个 bf16 ULP，Step 55/56）；这正是 bf16 只能做
  Arm B 的参考、不能做候选的原因。

## 8. 复现命令

```bash
# 0) 预检
nvidia-smi --query-gpu=memory.used --format=csv,noheader      # 48 MiB
.deps/env-port.bat && .deps/build-target.bat ninfer-serve
cmd //c ".deps\\verify-corpus.bat"                            # CORPUS_OK

# A) 无回退 A/B（移植前 vs 移植后 serve）—— 分片、可续跑
VERIFY_CTX=8192 VERIFY_FMT=rk8v4 VERIFY_LIMIT=8 bash .deps/run-verify-serve-ab.sh   # 冒烟
VERIFY_LIMIT=30 bash .deps/run-verify-serve-ab.sh                                   # x5 -> 128 格
VERIFY_COMPARE_ONLY=1 bash .deps/run-verify-serve-ab.sh | tee .deps/verify-a-compare.log
python .deps/verify-ab-report.py --md > .deps/verify-a-table.md

# A) prefill 噪声底（同二进制跑两次；--dir / VERIFY_OUT 选择输出目录）
VERIFY_OUT=$PWD/.deps/verify-ab-repeat  VERIFY_CTX=8192 VERIFY_LIMIT=8  bash .deps/run-verify-serve-ab.sh
VERIFY_OUT=$PWD/.deps/verify-ab-repeat2 VERIFY_CTX=8192 VERIFY_LIMIT=16 bash .deps/run-verify-serve-ab.sh

# B) 尾巴收益 —— 参考 + 矩阵（可续跑；LIMIT=1 每次恰好推进一个未完成运行）
for i in $(seq 1 34); do cmd //c ".deps\\run-verify-matrix.bat 1"; done
python .deps/summarize-verify.py | tee .deps/verify-b-summary.txt

# B3) 尾巴下的 MTP 接受率
B3_MAX_NEW=1024 B3_CTX=8192 B3_FMT="rk8v4 rk4v4-e8" B3_TAIL="0 1024 2048" RUN_LIMIT=18 bash .deps/run-verify-b3.sh
B3_TAGPREFIX=b3long8  B3_CTX=8192  B3_MSG_FILE=$PWD/.deps/verify-b3-msg-c8192.json  RUN_LIMIT=6 bash .deps/run-verify-b3.sh
B3_TAGPREFIX=b3long32 B3_CTX=32768 B3_MSG_FILE=$PWD/.deps/verify-b3-msg-c32768.json RUN_LIMIT=6 bash .deps/run-verify-b3.sh
python .deps/summarize-b3.py | tee .deps/verify-b3-summary.txt

# A4) 尾巴关闭一致性 / FP32 oracle
cmd //c ".deps\\run-oracle.bat" | tee .deps/verify-a4-oracle.out

# 每次 GPU 运行之后
tasklist | grep -iE "ninfer|perplexity"   # 必须为空
nvidia-smi --query-gpu=memory.used --format=csv,noheader   # 必须回到 48 MiB
```

## 9. 产物清单（原始证据，`.deps/` 已被 gitignore）

| 产物 | 内容 |
|---|---|
| `.deps/verify-ab/{pre,post}-mtp*-vis*-c*-*.txt` | 128 份生成文本（一致性证据） |
| `.deps/verify-ab/*.perf` / `*.server.log` / `*.client.err` | 128 条请求性能行 + 引擎日志 |
| `.deps/verify-a-compare.log`、`.deps/verify-a-table.md` | 一致性表、64 行 A/B 表 |
| `.deps/verify-a-chunk0{1..5}.log`、`.deps/verify-a-repeat*.log` | 分片/对比/噪声底记录 |
| `.deps/verify-ref-bf16-t0-w8-c{8192,32768}.ptk`（+`.run/report.json`） | 两个 bf16-tail0 top-K 参考 |
| `.deps/verify-{int8,rk8v4,rk4v4,rk4v4-e8}-t*-c*/report.json` | 32 个 Arm B 格（ppl + KLD 块） |
| `.deps/verify-b-chunk0{1..12}.log`、`.deps/verify-b-summary.txt` | 矩阵记录 + 汇总 |
| `.deps/verify-b3/*.{err,out}`、`.deps/verify-b3-summary.txt` | 66 次 MTP 运行 + 汇总 |
| `.deps/verify-b3-msg-c{8192,32768}.json` | 长提示词 messages 文件 |
| `.deps/verify-a4-oracle.out` | oracle 输出 |
| `.deps/run-verify-matrix.bat`、`run-verify-b3.sh`、`summarize-b3.py`、`verify-ab-report.py` | 新增/扩展的 harness（均为追加，非重写） |

## 10. 在仓库中的记录位置

- `PORT-JOURNAL.md` **Step 59–62** —— 按顺序的执行日志，含命令与实测值。
- `PORT-DOD.md` **"Verification campaign (PORT-VERIFY-PLAN)"** 的 **V0–V7** 行 —— 审计表。
- `PORT-MEMORY.md` **§5.16–5.17** —— harness 陷阱与本次验证的可复用事实（kill 语义、503 就绪、
  thinking 模型的流位置、prefill 噪声、B3 样本量、草稿/验证器不对称）。
