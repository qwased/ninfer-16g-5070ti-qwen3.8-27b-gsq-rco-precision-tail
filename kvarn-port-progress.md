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

| 项 | 现值 | 详见 |
|---|---|---|
| WP0 基线与环境 | 环境/构建已核实；整树构建**不通过**（先存 `LNK2019`，与本工作无关）⇒ 按目标构建；模型冒烟 + KLD 基线**未做** | 附录 D-2、§3-2026-10-07-2、归档 §C |
| WP0.5-A MTP 一致性仪器 | **定案 + 仪器改造完成（08-05）**：A3 按 O1 替换（非本移植引入、非上游缺陷，属 Fork B 私有合同）；仪器去逐位门禁 + 自确定性门禁 + 首分叉报告 + 并入 `rk4v4`/`kvarn` 档 + `--quick` | 附录 D-6/D-13、§3-2026-10-08-5、归档 §C |
| WP0.5-B 准入实验 | **完成**（07-18 代理档 / 07-22 正式档）：代理门禁同字节 **4.15×** 为正 ⇒ WP4 GO；发布档三档全过 —— `k4v4`(274 B) **0.002120**、`k5v5`(338 B) **0.001432**、`k6v6`(402 B) **0.001233**，对 `rk4v4`/`nvfp4`/`k8v4` = 2.09×/2.07×/2.18×；`k6v6` 门槛 < **0.002688** | 附录 D-10/D-11（正文未归档）、§3-2026-10-07-18/-22 |
| WP0.5-C 构型口径 | **完成**：本机为 **native 口径**（`NINFER_SM120_NATIVE=ON`）；compat 口径噪声底未测（未决项 2） | 附录 D-1、§3-2026-10-07-5 |
| WP1 kvarn ops 移植 | **完成**：ops 零告警编译；`ninfer_kvarn_test` 容差项由 WP5 收口（08-04 余量 65×）；kvarn bench **未做**（需重配，未决项 3） | 附录 D-3、§3-2026-10-07-3/-4、归档 §C |
| WP2 页几何 + 存储枚举 | **完成**：`KvarnGroup128` + `KvarnBits` + 三处 parser + 6 处名字 switch + 指纹 `;kvbn=` + 校验放宽 64\|128。回归：合成 242/245、真实模型 10/16，**失败均非本移植引入**（`stash` 重建基线对照）。**残余（parser 单测）已由 P3c 关闭（08-07）**：cli/serve 两处 parser 单测落地全过 | 附录 D-7、§3-2026-10-07-13、§3-2026-10-08-7 |
| WP3 模型接入 | **完成（08-05）**：① 地址空间页几何（07-16，ctx8192/229,348 token 跑通）② 续列尾（08-02 实现；08-04 host+device 单测全绿 + Engine 级 e2e `cached_tokens=851`、message 逐字节同）③ `--mtp-attention-window` 规划期拒绝（07-21）④ MTP 激励（08-04 `mtp accepted 84/113`、1.50× ≈ bf16 1.52×）+ 仪器（08-05）⑤ 措辞订正 ⑥ `--help`/docs（07-22）。08-03 独立审计：**无 HIGH/MED**（3 项 LOW 为 FORK 继承） | 附录 D-9/D-12/D-13、§3-2026-10-08-1…-5 |
| WP4 位宽参数化 K=V∈{4,5,6} | **完成（07-22）**：核按 `(KBits,VBits)` 模板化（发布档只实例化 `(b,b)`）+ `bits` 贯穿 + 测试扩 4/5/6 实跑全绿 + parser 发三档并删 `Bits2`/`k4v2` + 规划期拒绝（tail / mtp-window）+ 三档准入全过。三档单次测量的口径已由 08-04 的 P3a **3× 重复**收口（质量指标逐位相同、极差 0） | 附录 D-11/D-12、§3-2026-10-07-19…07-22 |
| WP5 oracle 与容差 | **完成（08-04）**：量化步长判据（点值 ≤ `q*(1+5e-2)` + `flips ≤ 1.0e-3*total` + `\|Δcode\|>1` 零容忍）+ 4/5/6 位穷举逐码往返；**实测余量**：`flips` 最大 **1/65**、`over_step=0`、`wide_flips=0` ⇒ **65× 余量、判据未放宽**。口径已回写 `docs/maintainer/op-development.md §6.3` | §3-2026-10-08-3/-4、§1 A2 |
| WP6–WP9 | **未开始**；WP6（旋转域尾部合并，10–15 天）门禁见计划书 §6 / §7-WP6 | 计划书 §6 §7 |

**最近四轮**：08-06 P3b 档位并入名字（**六展示面 GPU 实测全过**）+ 图谱使用纪律落盘（AGENTS.md + skill）
→ 08-07 P3c kvarn parser 单测（cli/serve 两处全过；第三处 perplexity parser 结构性不可单测，已在 P3b e2e 覆盖）
→ 08-08/08-09 图谱复核（`KvarnBits` 消费方与 grep 一致、`reset_kvarn_tail_row` callers=0 复现；两条工具边界回写 AGENTS.md）
+ 订正审计 LOW②（**只有 commit 侧静默**，capture 是 abort+重抛、activate 直接上抛）
→ **08-10 审计 3 项 LOW 修复落地**（① CausalScore 路由补 `reset_kvarn_tail_row` **生产变更** · ② commit 发布失败加诊断 · ③ 注释补 KVarN）
+ 新增 `ninfer_qwen3_5_kvarn_tail_row_reset_test`（**实跑 PASS，ctest 262→263**）。**① 的端到端对照未跑（用户指示）**。全文见 §3。

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
3. kvarn bench 是否纳入：需 `-DNINFER_BUILD_BENCHMARKS=ON` 重配，与「不重配」冲突，**待显式决定**。
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
> 本节正文以下只保留 **2026-10-08-1 … -10**。**追加新记录请接在最新一条之后，不要回填归档。**

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


### 2026-10-08-1 — 全量 `ctest`（261 项，**带 artifact**）复核：245 通过 / 7 跳过 / 9 失败；**9 项全部非 kvarn 引入**（含 1 项**从未取过基线**的新观测）

**触发**：用户指示检查上一窗口已跑完的全量 ctest 成果。

**结果**（`/tmp/ct_full2.log`、`build-port/Testing/Temporary/LastTest.log`，总时长 1990.61 s）：注册 **261**，
**245 通过 / 7 跳过 / 9 失败**（`97% tests passed`）。**该次为并发运行**（首个结果前有 **23** 个 `Start`
⇒ 显存密集的 real 测试互相争抢）。**`ninfer_kvarn_test` 已通过**（07-21 按位宽重定容差后收口；WP1 遗留项关闭）。

**9 项失败逐项对照基线（`LastTestsFailed.log` 全 9 项）**

| # | 测试 | 实测消息 | 判定 | 依据 |
|---|---|---|---|---|
| 20 | `ninfer_device_sync_empty_test` | `invalid sync setting did not fail before CUDA initialization` | **先存**（改动面外） | 与 D-7 逐字相同；`tests/cmake/CoreTests.cmake:82-83` 的 `ENVIRONMENT` 未把空串交给 `getenv` |
| 84 | `ninfer_qwen3_5_prefix_real_test` | `registered tokenizer/chat template has no prompt golden` | **产物结构性** | 该产物无 prompt golden（D-7 同） |
| 85 | `ninfer_qwen3_5_hybrid_prefix_real_test` | 全量：`... only 0 bytes are available for runtime capacity`；**隔离复跑**：`Qwen3.5 config: missing component dflash2` | **产物结构性**（全量那条是**并发显存争抢**的次生现象） | 隔离复跑消息与 D-7 逐字相同 ⇒ 全量差异来自 `-j` 并行 |
| 89 | `ninfer_qwen3_5_vision_workspace_test` | `Vision workspace exceeded the single-item bound: at=1205905409 above=1205905409` | **先存 / artifact 门控 / 首次观测**（见下） | 代号见下 |
| 90 | `ninfer_qwen3_5_dflash2_real_test` | `Qwen3.5 config: missing component dflash2` | **产物结构性** | 产物缺 DFlash2 组件（D-7 同） |
| 91 | `ninfer_qwen3_5_moe_real_test` | `35B Engine construction has an invalid load summary: target=Qwen3_5ForCausalLM` | **产物结构性** | 本产物非 35B MoE（D-7 同） |
| 93 | `ninfer_qwen3_5_dflash_real_test` | `FATAL: Qwen3.5 config: missing component dflash` | **产物结构性** | 产物缺 DFlash 组件（D-7 同） |
| 101 | `ninfer_qwen3_5_mtp_greedy_parity_real_test` | `bf16 k=1 sample=0 row=0 prompt=68 mismatch at token 91: expected=2466 actual=2640` | **已知 A3** | 与 D-4/D-7 逐字相同 |
| 119 | `ninfer_gdn_gating_proj_test` | `qwen3_6_27b/35b_a3b: GDN control interval missed a route endpoint` | **先存**（改动面外） | 与 D-7 逐字相同 |

**`ninfer_qwen3_5_vision_workspace_test`（#89，唯一新观测）判定为先前存在，四条依据：**
1. **改动面外**：测试文件 `tests/models/qwen3_5/test_vision_workspace.cpp` 与其数值来源
   `ops::causal_softmax_attention_workspace_capacity_bytes`、`src/ops/softmax_attention/dense/causal_cache/*`
   **均不在本次工作树改动清单内**（`git status` 无此二处）。
2. **我们改动的规划代码对该测试逐位等价**：`build_workspace_plan`/`page_count`/`maximum_main_page_groups`/
   `resolve_host_cache_budget` 全部以 `kv_page_tokens(storage)` 参数化，而本测试用
   `KvCacheStorage::Fp8E4M3Row256` ⇒ `kv_page_tokens` 恒 **64**（非 kvarn 逐位不变）；新增的
   `attention_workspace` lambda 对非 kvarn **原样转发**同一函数同一实参。
3. **失败的是主机标定上限，不是功能契约**：失败断言是 `at_item_limit > kWorkspaceCeiling(827 MiB)`
   （`test_vision_workspace.cpp:60-64`）；真正的单件上界契约（`above_item_limit == at_item_limit`）**通过**（两者均
   `1,205,905,409`）。
4. **隔离复跑（独显空闲、单测）**：**同样失败且数值逐位相同**（`at=1205905409 above=1205905409`，exit=1）
   ⇒ **确定性、非并发争抢**。
   （残留：**未做**一次「复原到 HEAD 的基线重建」以取得旧数值——但由 1+2 可知基线数值必同为 `1205905409`，
   即基线同样超限；该测试此前在所有基线运行中均为 **Skipped**（未设 `NINFER_TEST_ARTIFACT`），从未取过基线。）

**结论**：**本次全量 ctest 未发现任何由 kvarn 移植引入的失败**；A1 的「全绿」口径须补入 #89 这一
**artifact 门控的先存失败**（已回写计划书 §1-A1）。**未修 #89**（先存、且与 kvarn 交付面无关；属文档/测试校准范畴）。

**未做（如实记录）**：未做「复原 HEAD 的基线重建」（见上残留，代价 ~20–40 min，判为不必要）；
未串行重跑全量（仅隔离复跑 #89 与 #85 两项有歧义者）；**未改任何源码**；**未提交**。
**产物**：`/tmp/ct_full2.log`、`/tmp/vision_isolated.log`、`/tmp/hybrid_isolated.log`。

### 2026-10-08-2 — WP3② 续列尾**实现落地**（构建绿；e2e 验收待 GPU 空出）+ P3a 并行开跑

**触发**：用户裁定下一步 = **P1（WP3② 续列尾）+ 并行 GPU 补强**（见 08-01 的 AskUserQuestion 结果）。

**P3a（并行，GPU，后台）**：新建 `.deps/kvarn-adm/run6.sh`（协议与 run5.sh 逐字相同：`--corpus … --quick --context 8192 --disjoint --score-width 8 --score-topk 100`，共享 `bf16-t0.topk`），
`kvarn:k4v4|k5v5|k6v6` 各 **3 次**（共 9 臂）⇒ 产出 mean KLD / tok/s 的均值±散布（收口 D-11 的**单次测量**口径）。
00:36 启动（`run6.log`），GPU 11.6 GiB / 94%，预计 **~2 h**。

**WP3② 续列尾实现（6 文件，以 FORK `ninfer-rtx5090-mobile` 为参考，逐点核对后移植）**

| 文件 | 改动 |
|---|---|
| `state/state_image.h` | `+KvarnContinuationStateSpec`/`KvarnContinuationImageLayout`/`KvarnContinuationStateLayout`；`StateImageSpec.kvarn`；host layout `kvarn`+`kvarn_layout`；device layout `kvarn`；`StateImageDevicePool` 增 `has_kvarn`/`kvarn_text_layers`/`kvarn_mtp_layers`/`kvarn_text_tail`/`kvarn_mtp_tail` + 4 个成员 |
| `state/state_image.cpp` | `checked_i32`；`same_kvarn_spec`/`same_kvarn_layout`；`plan_kvarn_continuation_image`；`same_host_layout`/`plan_host_state_image`/`state_image_transfer_work` 并入 kvarn；设备池在 **rank 0** 加 `{image_bytes, slot_count}` U8 张量；ctor 绑定/校验并回填 `device_spec.kvarn`；`kvarn_tail_view` + 两个访问器；`zero_slot`/`zero_all`/`copy_slot`/`copy_to_host`/`copy_from_host` 与 `for_each_host_component` 的 **Rest** 分支均并入 kvarn |
| `program/program_impl.h` | 声明 `restore/capture/activate_sequence_kvarn_tail`（紧邻 `trim_sequence_kv`） |
| `program/storage/context.cpp` | `#include ninfer/ops/kvarn_attention.h`；`trim_sequence_kv` 尾部调 `restore_sequence_kvarn_tail`；三个函数实现（**`KvarnGroup128`** 取代 FORK 的 `KvarnK4V2Group128`；`compute_streams[0]` 取代 FORK 的 `device.stream`；用 `addresses.execution_row()` + `execution.kvarn_layer_view()`；restore 按 16 层一批） |
| `program/prefill.cpp` | 复用路径 `bind_sequence_kv` 后加 `activate_sequence_kvarn_tail(sequence)`（对应 FORK prefill.cpp:636） |
| `program/transactions/{capture,commit}.cpp` | `state_store->freeze(...)` **前**加 `capture_sequence_kvarn_tail(...)`（对应 FORK :684 / :635） |
| `program/planning/startup.cpp` | `KvarnGroup128` 时填 `state_image_spec.kvarn = {full_attention_layers, mtp?1:0, kv_heads, head_dim}`（对应 FORK :155-161） |

**适配点（TAIL vs FORK 的结构差异）**：① TAIL 的 `StateImageDevicePool` 是**多 rank 分片**（`shards`/`RankStreams`/`StateImagePart`/分段拷贝），FORK 是单 rank ⇒ kvarn 镜像绑 **rank 0**（`backings.front()`），并在 `for_each_host_component` 的 `Rest` 分支以 `rank=0` 暴露，**分段拷贝自动覆盖**；② 流参数：`compute_streams[0]`；③ 枚举名 `KvarnGroup128`；④ `checked_i32` 在 TAIL context.cpp **不存在** ⇒ 新增文件内 `kvarn_frontier_i32`；⑤ TAIL 的 host-layout 一致性检查在 ctor 中**先于** DFlash 段 ⇒ kvarn 绑定插在检查之前。

**构建**：`ninfer_ops ninfer_tests ninfer`（42 步）+ `ninfer-serve` **exit 0、0 error**。
**合成回归**（低显存，与 P3a 并存）：`ninfer_qwen3_5_state_image_test` / `ninfer_qwen3_5_state_image_layout_test` / `ninfer_state_store_test` **均 exit 0** ⇒ 未破坏既有 state-image 管道。

**未做 / 待做（如实记录）**：① **e2e 验收未做**（前缀复用 / 检查点往返）——需 GPU 加载 27B，而 P3a 正独占 GPU（~11.6 GiB）⇒ 待 P3a 完成（预计 02:36）；② `ninfer-perplexity` 未重链（其 exe 被 P3a 持有；P3a 用的是**改动前**的二进制，但其测量路径属 fresh 单请求、不经本改动的 capture/activate ⇒ 结论不受影响）；③ 「续列往返逐 token 对拍」单测**未写**；④ **未提交**。
**产物**：`.deps/kvarn-adm/run6.sh`、`run6.log`、`<spec>-t0-r<rep>.log`（进行中）。

### 2026-10-08-3 — WP5 容差形式化（构建绿；GPU 侧未跑）+ P1 续列尾专项单测（host 段实跑绿 / device 段待 GPU）+ P1 移植独立审计（无 HIGH/MED）

**触发**：用户裁定本轮做**纯 CPU**工作（GPU 被 P3a 独占）：(A) P1 续列尾专项单测、(B) WP5 容差形式化、(C) P1 移植只读审计。

**(B) WP5 容差形式化 —— `tests/ops/test_kvarn.cpp`**

- `codec_oracle` 返回值由 `std::vector<double>` 改为 `CodecOracle{decoded, step, code}`：`step` 是**逐元素理论量化步长**
  `q = (max-min)/qmax * row_scale * column_scale`（`qmax=(1<<bits)-1`，即相邻码的值距），`code` 是 oracle 在均衡域选定的整数码。
- **删除**拟合阈值 `oracle_relative_l2_limit(bits)=1.0e-3`，改为 `check_step_criterion(...)`：①**点值判据** `|actual-expected| <= q*(1+kStepSlack)`，`kStepSlack=5e-2`（吸收设备与 oracle 各自 Sinkhorn 的 scale 差异）；②**总量判据** `flips <= kMaxFlipFraction*total`，`kMaxFlipFraction=1.0e-3`（65536 元素 ⇒ 上限 65）；③`wide_flips`（`|Δcode|>1`）**零容忍**。口径与 `docs/maintainer/op-development.md §6.3`「pointwise bound + finite gross pointwise-error cap」一致，**已回写该文档**。
- 新增 `run_codec_bit_order_case()`：4/5/6 位**穷举逐码** pack→unpack 往返（每个可表示码 × 每个行内字段偏移，约 4.3 万组合），并置**行外哨兵 `0xff`** 证明码读写不越出所属行；另校验 pack 不污染未触碰字节。
- **实测**：`ninfer_tests` 目标构建 **exit 0 / 0 error / 0 warning**。**未跑**（`ninfer_kvarn_test` 是 CUDA 测试，GPU 被 P3a 独占）⇒ **判据的数值余量未实测**：`kMaxFlipFraction` 与 `kStepSlack` 由**分析**给出（预估设备/oracle 的 Sinkhorn 相对差 ~2e-6 ⇒ bits=4 约 1 次翻码、bits=5 约 2、bits=6 约 0–4；与 07-21 实测 bits=4 K `rel_l2=6.09e-4`（单元素翻码）、bits=6 `rel_l2=0` 自洽）。**必须在 GPU 空出后首跑确认**；若超限即为**真实发现**（记录并定量定标），**不得事后放宽**。

**(A) P1 续列尾专项单测 —— 新文件 `tests/models/qwen3_5/test_kvarn_continuation_image.cpp`**

- 注册 `ninfer_qwen3_5_kvarn_continuation_image_test`（`tests/models/qwen3_5/tests.cmake`）⇒ **ctest 261 → 262**。
- **host 段（无设备，本轮实跑）**：kvarn 续列镜像几何 vs **分页 KV 尾几何**对拍（`tail_layer_bytes == BF16[D,G,Hkv*3]` 字节数；`marker_layer_bytes == I32[3]`；`Hkv*3*D*2*G` 覆盖一整层尾；**16 层 K+V == 24.0 MiB**，与附录 A 口径一致）；六个组件 256B 对齐、互不重叠、落在镜像内；MTP=0 变体无 MTP 偏移；`state_image_transfer_work` 载荷含镜像且**镜像恰为 1 次拷贝操作**；无 kvarn 的 spec 真报 `!kvarn`；非法 spec（head_dim≠256 / text_layers=0 / kv_heads=0）被拒；**capture→activate 往返逐字节 + 逐 token 对拍**（源按分页尾几何、目的偏移取镜像布局 ⇒ **非自证**）。
- **device 段（需 GPU，待跑）**：真实 `StateImageDevicePool` + kvarn —— `zero_all/zero_slot`（K/V 清 0、markers 复位 `0xff`）、`copy_slot` D2D 携带整层尾、D2D 拷回分页形缓冲后**逐字节对拍**、MTP 池与 text 池**不互为别名**、越界层/槽抛 `out_of_range`、尾视图几何 == 分页尾几何。
- **实测**：`ninfer_tests` 构建 exit 0；**host 段实跑**（`CUDA_VISIBLE_DEVICES=99` 强制无设备 ⇒ 只走 host 段、**零显存占用**）**exit 0**。
  首跑曾**暴露我方一处错误假设**：误以为 host 镜像偏移 == device 区域偏移；实为**两套独立 `LayoutBuilder`**（ctor 以「由 device 组件重建 host 布局并比对」`state_image.cpp:459` 兜底）⇒ 断言已按实测修正为「host 镜像 == **单槽**字节数」。**device 段未跑**。
- **未做（如实记录）**：**未**做「复用路径 vs 一次性 prefill 的 **Engine 级 27B** 逐 token 对拍」（需 artifact + 显存，即 `.deps/kvarn-adm/p1_prefix_reuse.sh` 的缓存命中/PPL 口径，仍待 GPU）。

**(C) P1 移植独立审计（只读子代理，vs 上游 FORK）**：结论 **无 HIGH/MED 缺陷**。全部 4 个调用点齐备（trim `context.cpp:1714` / prefill `prefill.cpp:661` / capture `transactions/capture.cpp:726` / commit `transactions/commit.cpp:660`，相对位置与 FORK 相同）；`plan_kvarn_continuation_image`（`state_image.cpp:92-119`）与 `kvarn_tail_view`（`:501-522`）与 FORK 逐点相同；与 TAIL `decoder_state` 的 kvarn 板（`{256,128,rows*Hkv*3,layers}` / `{3,rows,layers}`）一致；**分片架构由规划期守卫兜底**——KVarN 要求所有 attention 层同 rank（`decoder_state.cpp:53-61`）⇒ 绑 rank 0 安全；`zero_slot/zero_all/copy_slot/copy_to_host(_segments)/copy_from_host(_segments)` 的 Rest 分支**无重复计数/漏项**；非 kvarn 逐位不变（三处早退 + `startup.cpp:241` 门控）。

**三项 LOW（均为 FORK 继承，非本次引入）**：① `reset_kvarn_tail_row`：TAIL **零调用者**（`decoder_state.cpp:360`），FORK 有 **2 处且都在同一测试** `tests/models/qwen3_5/test_prefill_precision_real.cpp:236,284` —— **该测试未被移植到 TAIL**（TAIL `tests/models/qwen3_5/` 无此文件）⇒ 既存在**陈旧 marker** 隐患（行重用若不由 store op 重写 markers），又说明**移植漏掉了一个激励它的 FORK 测试**。**⚠ 本条由 codebase-memory 图谱（`trace_path` inbound）首发现、再以 `grep` 复核修正**：原审计写的「两树均无调用者」**不准确**（FORK 有测试调用者）。② `capture/activate_sequence_kvarn_tail` 在 `!has_kvarn()` 时抛 `logic_error`，而其调用者位于 `catch(...) { return false; }` 内（`capture.cpp:726` / `commit.cpp:660`）⇒ 规划不一致会**降级为静默 capture/commit 失败**；**⚠ 2026-10-08-9 订正（读源码实测）**：本句**只对 commit 成立** —— `commit.cpp:660` 确在 `publish_active_continuation(...) noexcept` 的 `catch (...) { return false; }`（`commit.cpp:674`）内；`capture.cpp:726` 的调用者（`capture.cpp:1302-1308`）是 `catch (...) { abort_active_capture(...); throw; }`（**abort + 重抛，非静默**）；`prefill.cpp:661`（activate）在 `start_sequence`（`prefill.cpp:205`）内且**该函数体无 try/catch**（**直接上抛**）。详见 §3-2026-10-08-9。~~降级为静默 capture/commit 失败~~③ 文档面：`state_image.h:158-170,198-200` 的 `StateImagePart`/`StateImageDevicePool` 注释未提 KVarN。**处置**：①②属既有风险（**未修**，记录在案）；③属文档（本轮未改）。

**本轮全为纯 CPU**：仅**按目标构建** `ninfer_tests`（未重配 build-port、未全树构建）；host 段测试以 `CUDA_VISIBLE_DEVICES=99` 运行（零显存占用）；P3a 未受影响（01:47 仍 11,640 MiB；**run6.sh 每 rep 一个新进程**，故 PID 会变：01:32 为 18880 / 01:47 为 19104）。
**P3a 当时的进度（01:47）**：**6/9** —— k4v4 r1–r3 与 k5v5 r1–r3 均 `exit=0`（k4v4 mean KLD **0.002120**、k5v5 **0.001432**，与 D-11 逐位相同；tok/s 287.5–292.8），`k5v5-t0-r3` 进行中（01:42:54 起）；剩 k6v6 r1–r3 ⇒ 预计 **~02:35** 结束。
**收尾未做（待 GPU 空出，本轮被 P3a 阻塞）**：① 解析 run6.log 9 臂（均值±极差）；② `p1_prefix_reuse.sh` 的 **Engine 级 P1 e2e 验收**（kvarn r2 `cached_tokens>0` 且 message 逐字节相同 + bf16 对照）；③ `ninfer_kvarn_test`（**WP5 判据的数值余量首跑确认**）；④ 新 `ninfer_qwen3_5_kvarn_continuation_image_test` 的 **device 段**。已挂后台等待进程，P3a 退出即续做。
**产物**：`tests/models/qwen3_5/test_kvarn_continuation_image.cpp`（新）、`tests/ops/test_kvarn.cpp`（改）、`tests/models/qwen3_5/tests.cmake`、`docs/maintainer/op-development.md`、`/tmp/tests_wp5.log`、`/tmp/tests_p1c.log`。**未提交**。

### 2026-10-08-4 — GPU 收尾四件（①P3a 9 臂 / ②P1 e2e **通过** / ③WP5 余量**实测** / ④续列尾 device 段**首跑 FAIL→定位为测试自身 bug→修正 PASS**）+ P2 前置：kvarn MTP provisional 路径**首次激励**

**触发**：用户指示 GPU 收尾 + P1/P2 推进。P3a（run6.sh）已 `### P3a repeats DONE Thu Oct 8 02:36:04 2026`，显存回落 0 MiB。

**① P3a 收尾：9 臂（`kvarn:k4v4|k5v5|k6v6` × r1–r3）全部 `exit=0`；质量指标逐位重复，仅吞吐有散布**

协议与 D-10/D-11 逐字相同（复用 `bf16-t0.topk`，229,348 评分 token / 28 窗口）。

| 档 | mean KLD（r1/r2/r3） | median | P99 | max | same_top | dlogp | PPL | tok/s（r1/r2/r3） |
|---|---|---|---|---|---|---|---|---|
| `k4v4` | 0.002120 / 0.002120 / 0.002120 | 0.000747 | 0.017190 | 7.175765 | 0.9817 | −0.000252 | 4.69436 ×3 | 289.1 / 291.9 / 292.8 |
| `k5v5` | 0.001432 / 0.001432 / 0.001432 | 0.000531 | 0.010355 | 7.599654 | 0.9846 | −0.000187 | 4.69405 ×3 | 290.6 / 287.5 / 289.5 |
| `k6v6` | 0.001233 / 0.001233 / 0.001233 | 0.000454 | 0.007896 | 8.354687 | 0.9858 | −0.000073 | 4.69352 ×3 | 289.3 / 289.9 / 289.5 |

**判定（收口 D-11 的单次测量口径）**
1. **质量指标（mean/median/P99/P99.9/max/same_top/dlogp/PPL）在三重复间逐位相同** ⇒ KLD 测量是**完全确定性**的，重复测量的**极差为 0**（远强于"±散布"的预期）。三档数值与 D-11 单次测量**逐位相同**（0.002120 / 0.001432 / 0.001233）⇒ **D-11 的单次测量被 3 次重复完全证实**，无需修正。
2. **唯一有散布的是吞吐**（唯一非确定性来自 wall-clock/调度）：`k4v4` 291.27±3.7、`k5v5` 289.20±3.1、`k6v6` 289.57±0.6 tok/s（极差 1.3–1.3%）。三档均值落在 bf16 296.1 的 −1.6%…−2.3% 内，与 D-11/D-12 的"位宽升高有单调真实代价"一致但不构成新结论。
3. ⇒ **D-11 的"A8 三档全过、不回退"结论在 3× 重复下稳固**；计划书 §1-A2/A8 与附录 D-11 无需改判，只补一句"重复测量极差为 0（质量）/ ≤1.3%（吞吐）"。

**② P1 e2e 验收（WP3② 的验收标准）—— `RESULT: PASS`**

命令：`bash .deps/kvarn-adm/p1_prefix_reuse.sh`（`ninfer-serve`，`--kv-dtype kvarn:k4v4` vs bf16 对照，`--max-context 4096`、`--greedy`，同 prompt ×2）。
**先修掉 harness 自身一个 bug**：`run_case` 的 `local tag=$1 port=$2 kv=$3 url=… log="…$tag…"` 在同一条 `local` 语句里引用**尚未赋值**的 `$tag` ⇒ 在 `set -u` 下报 `tag: unbound variable`（该脚本此前从未跑过，故首次暴露）。

| 判据 | 实测 |
|---|---|
| kvarn r2 `usage.prompt_tokens_details.cached_tokens > 0` | **851**（99.4%，`turn closure`）✓ |
| kvarn r2 `choices[0].message` 与 r1 **逐字节相同** | **YES** ✓ |
| bf16 对照证明 harness 确触发复用 | r2 cached **851** 且 r1==r2 ✓ |
| 旁证（独立于计数器） | kvarn req#1 TTFT 721 ms → req#2 **37.9 ms**（19×）；serve log 记 `capacity \| KV 4,096 tokens, **kvarn**, explicit` ⇒ kvarn 存储确在生效 |

⇒ **P1（续列尾前缀复用）Engine 级验收达成**。连同 08-03 的专用单测（host 段 + 本轮 device 段，见④），WP3② 的**实现 + 验收闭环**完成。日志：`.deps/kvarn-adm/p1-{kvarn,bf16}-serve.log`、`{kvarn,bf16}-r{1,2}.json`。

**③ WP5 判据数值余量实测（首跑）—— 余量充足，判据成立**

`./build-port/tests/ninfer_tests.exe ninfer_kvarn_test`（GPU，非 `CUDA_VISIBLE_DEVICES=99`）：

```
KVarN K official oracle: step_ratio_max=1        flips=1/65 over_step=0 wide_flips=0 of 65536   (bits=4)
KVarN V official oracle: step_ratio_max=0.0049119 flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=4)
KVarN K official oracle: step_ratio_max=1        flips=1/65 over_step=0 wide_flips=0 of 65536   (bits=5)
KVarN V official oracle: step_ratio_max=0.0101499 flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=5)
KVarN K official oracle: step_ratio_max=0        flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=6)
KVarN V official oracle: step_ratio_max=0.0206313 flips=0/65 over_step=0 wide_flips=0 of 65536   (bits=6)
OK kvarn correctness
```

- **无一项超限**：`flips` 最大 **1**（上限 65 ⇒ **65× 余量**）、`over_step=0`、`wide_flips=0`（零容忍项通过）。判据 exit=0。
- **与 08-03 的分析值吻合**：预测"bits=4 约 1 次翻码、bits=5 约 2、bits=6 约 0–4"；实测 K 侧 bits=4/5 各 **1**、bits=6 **0**；V 侧三档全 **0**。`step_ratio_max=1`（K，bits 4/5）即**恰好一个量化步长**，正是"单元素落在量化边界、设备与 oracle 舍入到相邻码"的形态，与 07-21 的 `rel_l2=6.09e-4`（bits4 K）根因一致。
- ⇒ **§1-A2 / §7-WP5 的判据（逐元素 ≤ 1 步 + 边界翻码上限 1e-3 + `|Δcode|>1` 零容忍）经实测成立**，无需放宽；`kMaxFlipFraction=1.0e-3`、`kStepSlack=5e-2` 保留。WP5 **收口**。

**④ 续列尾 device 段 —— 首跑 FAIL(2) → 根因 = **测试自身 bug** → 修正后 PASS**

`./build-port/tests/ninfer_tests.exe ninfer_qwen3_5_kvarn_continuation_image_test` 首跑：
```
FAIL: the captured KVarN tail survives the slot copy
FAIL: the activated KVarN markers match the captured ones
FAIL kvarn continuation image   (exit=1)
```
**定位（决定性）**：两处失败**都只涉及 `marker_pattern`**；同一断言里的 K/V（`restored_k/restored_v`，尺寸 `tail`）**通过**。根因是我在 08-03 写该测试时把 `marker_pattern` 写成 **3 字节** `{0x11,0x22,0x33}`，而 `logical_pages` 是 `I32{ops::kKvarnTailSlots=3}` = **12 字节** ⇒ ① `copy_to_tensor` 从 3 字节 vector 读 12 字节（**越界读 / UB**）；② `read_tensor(...) == marker_pattern` 是"12 字节 vs 3 字节"的 vector 比较，**恒 false**。测试**自身**在第 341 行就断言 `logical_pages.bytes() == marker`(=12) 且该断言通过 ⇒ 12 是正确尺寸，**3 字节才是 bug**。
**修法**：`marker_pattern` 改为按 `marker` 尺寸程序化生成（与 `k_pattern`/`v_pattern` 同构）。**判据不变**（仍是"逐字节存活 + activate 后逐字节相同"）。重跑 **`OK kvarn continuation image`（exit=0）**。
⇒ **实现无缺陷**（K/V/markers 全部逐字节通过 slot copy 与 activate）；这是**测试侧**的尺寸 bug，非移植缺陷。**已如实记录**（未放宽任何阈值）。

**P2 前置（WP3④）：kvarn MTP provisional 路径首次激励 —— 通过**

新建 `.deps/kvarn-adm/p2_mtp_gen.sh`：对 `kv{:kvarn:k4v4|bf16|rk4v4}` × `spec{off, mtp k=1}` 各跑同一 greedy 请求 **2 次**（repeat），比较 message 文本（`.deps/kvarn-adm/p2-*.{json,txt,log}`）。

| 档 | MTP-off decode | MTP-on decode | mtp accepted | self-det(off) | self-det(on) | **MTP-on vs off** |
|---|---|---|---|---|---|---|
| `kvarn:k4v4` | 67.8 tok/s | **101.5 / 101.6** | **84/113 (74.3%)** | none | none | **none** |
| `bf16` | 68.3 | 104.1 / 104.3 | 87/111 (78.4%) | none | none | **none** |
| `rk4v4` | 68.5 | 97.6 | 79/119 (66.4%) | none | none | **none** |

（"none" = 两次/两配置的 message **逐字节相同**；三档 req#2 均 `cached 851`。所有 `exit=0`、`http=200`。）

- **首要目标达成**：`kvarn:k4v4 --spec mtp` **端到端跑通**且日志报 `mtp accepted 84/113` ⇒ **kvarn 的 MTP provisional 路径（WP3 落地但从未激励）首次被真正执行**，无规划/几何缺陷。
- **A3 相对判据（O1）有利**：kvarn 在 MTP-on 下**自确定性成立**（r1==r2 逐字节）；且本样本中 MTP-on vs off **逐字节相同**（无分叉）。
- **同量级**：MTP 加速比 kvarn **1.50×**（101.5/67.8）vs bf16 **1.52×**（104.1/68.3）、rk4v4 **1.42×**；接受率 kvarn 74.3% 介于 bf16 78.4% 与 rk4v4 66.4% 之间。

**边界（如实记录，勿过度解读）**：本轮 P2 前置是 **message 文本级、单 prompt、200 token** 的 e2e 观测；它**不能**替代 **token 级首分叉下标/分叉率** —— 后者需要把 `test_engine_mtp_greedy_parity_real.cpp` 改造为诊断仪器（**P2b，未完成**）。已知 **bf16 在同一 token 级仪器下于 token 91 分叉**（A3，D-4/D-7 逐字相同），而本 e2e 样本 200 token 未复现分叉 ⇒ **分叉是按 prompt/步数触发的 near-tie 翻转，非必然**。

**构建**：`ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0**（10 步；P3a 释放后 `ninfer-perplexity` 重链成功）。
**产物**：`.deps/kvarn-adm/{p1_prefix_reuse.sh(修),p2_mtp_gen.sh(新),p1-*.log/json,p2-*.log/json/txt,run6.log}`（gitignored）。
**未做（如实记录）**：① P2b（仪器改造 + kvarn 档并入 `kKvProfiles` + `kvarn_bits` 贯通到 `EngineOptions`）**未做**；② 未跑 `k5v5/k6v6` 的 MTP；③ 未做 kvarn MTP 的 `k>1`（k=3/15）；④ **未提交**。

### 2026-10-08-5 — P2b 完成：MTP parity 测试改造为**诊断仪器**（去逐位门禁、加自确定性门禁 + 首分叉报告，并入 kvarn/rk4v4 档）；**kvarn 在 token 级仪器下自确定性成立、分叉不劣于基线**

**触发**：承接 08-04 的 P2 前置（激励已通过），本轮做 P2b = WP3④ 的仪器改造（用户裁定 ctest 范围取「**收窄默认档**」）。

**改造内容（`tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`，557 行）**

1. **去逐位门禁**：`verify_result`（跨配置 token 逐位相等，不符即 `throw`）**删除**；换成 `require_output_limit`
   （只保留"达到固定输出长度/`FinishReason::OutputLimit`"这一**结构契约**）+ `divergence(left,right)`
   → `{first, count}`（首分叉下标 + 分叉计数）。
2. **加自确定性门禁（A3/O1 的①，真正的 gate）**：每个宽度 `repeat 0` 记下本配置输出，`repeat 1` 必须**逐字节复现**
   （长度相同且 `divergence.count==0`），否则 `throw "... is not self-deterministic: first_diff=… diverged=…"`。
   （原实现把 `expected` 只在 `depth==0||dflash2` 时写并与 greedy 比 ⇒ 既不是自确定性、又是跨配置门禁。）
3. **跨配置降级为报告**：打印 `self_det=n/a|ok vs_greedy=identical|first_diff=N diverged=C/total`。
   tool-loop 的 adaptive-vs-off 断言同样降级为首分叉报告。
4. **并入档位**：`kKProfiles` **新增 `rk4v4`、`kvarn:k4v4`、`kvarn:k5v5`、`kvarn:k6v6`**（`KvProfile` 增
   `KvarnBits` 字段），`engine_options` 增 `kvarn_bits` 形参并写 `options.kvarn_bits`
   （**此前测试从不设 ⇒ kvarn 会静默用默认 Bits4**）；`--kv-dtype` 允许表与 usage 同步。
5. **`--quick`（用户裁定的 ctest 收窄）**：= 代表性档 `{bf16, rk4v4, kvarn:k4v4}` × `sample 0` × 宽度 `{0,3}`；
   `tests/models/qwen3_5/tests.cmake` 的注册加 `TEST_ARGS --quick`（该宏本就有 `TEST_ARGS` 约定）。
   **全扫**（8 档 × 3 样本 × 7 宽度 × 2 重复 ≈ 336 次生成 ≈ 1.5 h）仍可**无 `--quick`** 手动跑。

**实测（GPU，本机 27B 产物，`sample 0`、`prompt 68`、512 输出 token、C=1、`--greedy`）**

| 档 | k=1 `vs_greedy` | k=3 `vs_greedy` | k=15 `vs_greedy` | `self_det` |
|---|---|---|---|---|
| `bf16` | **first_diff=91, 413/512** | identical | **first_diff=91, 412/512** | ok（全） |
| `rk4v4` | identical | **first_diff=97, 408/512** | 未跑 | ok |
| **`kvarn:k4v4`** | **identical** | **identical** | **first_diff=91, 418/512** | ok（全） |
| `kvarn:k5v5` | **identical** | 未跑 | 未跑 | ok |
| `kvarn:k6v6` | **identical** | 未跑 | 未跑 | ok |

**全部 `exit=0`**（门禁已去除，仅自确定性/结构契约可致失败）。`ctest -R ninfer_qwen3_5_mtp_greedy_parity_real_test`
（`--quick`）**Passed 130.68 s**。

**判定（A3 相对判据 / WP3④）**

1. **自确定性成立（①）**：`self_det=ok` 在**所有**已测（档,宽度）组合上成立（含 kvarn 三档 + rk4v4/bf16，宽度 0/1/3/15）
   ⇒ 这正是 A3-O1 的**唯一绝对判据**，kvarn 满足。
2. **分叉是（档,宽度）特异的 near-tie 翻转，非 kvarn 独有**：`bf16@k1`、`bf16@k15`、`rk4v4@k3`
   各自分叉（首分叉 91/91/97）；**kvarn:k4v4 在 k=1/k=3 与 greedy 逐字节相同**（0 分叉）。
3. **kvarn 不劣于基线（④）**：k=15 处 kvarn `418/512` vs bf16 `412/512`（**首分叉同为 token 91**，差 6 token =
   1.2% 相对）；k=1/k=3 处 kvarn 更好（identical）。⇒ **同量级、不劣化**（**不主张 kvarn 严格更优**；k=15 略高于 bf16）。
4. **与 A3 既有定性一致**：分叉随宽度出现/消失、且只落在部分档位 ⇒ 与 `small_t.cu` 宽度特化 + `docs/performance.md:45`
   「per-configuration determinism, not cross-configuration equality」一致。**未做**决定性实验（把 `case 2` 改 `<2,2>`），
   故仍属**代码结构上最合理的解释**，非已证事实（同 D-6 边界）。
5. **首个 kvarn token 级 MTP 数据**：kvarn 的 MTP provisional 路径在 token 级仪器下**无结构/规划缺陷**（无异常、
   长度契约满足、自确定性成立）。

**边界（如实记录，勿过度解读）**：① 仅 `sample 0`（prompt 68）、单次运行；② 宽度只覆盖 0/1/3/15（`k5v5/k6v6` 只测 k=1）；
③ "identical" 是**该样本**下的结论，**不等于** kvarn 永不与 greedy 分叉（k=15 即分叉）；④ 测试名仍含 `parity`
（语义已变为"自一致性 + 诊断"，**未改名**，避免波及 ctest 名/既有记录引用——记为未决项）。

**变更面（本轮新增/修改）**：`tests/models/qwen3_5/test_engine_mtp_greedy_parity_real.cpp`（改造）、
`tests/models/qwen3_5/tests.cmake`（`TEST_ARGS --quick`）。
**构建**：`ninfer_tests` **exit 0**（0 error 0 warning）。

### 2026-10-08-6 — P3b 完成：kvarn 档位并入报告目录 / 日志名（`MemorySummary` + 四展示面）；**六展示面 GPU 实测 + 6 项 host 回归全过**；顺带由**图谱**发现一处**潜在（非可达）**身份缺陷；并落盘「图谱使用纪律」（AGENTS.md + skill）

**触发**：用户裁定从待办 1（P3b）开工；中途用户指出「为什么不用 codebase_memory_mcp」⇒ 本轮回溯原因并修正使用纪律（见 (3)）。

**(1) P3b 落地（7 源码文件）**：`include/ninfer/types.h` 的 `MemorySummary` 增 `KvarnBits kvarn_bits`（默认 `Bits4`）；
`program_impl.{h,cpp}` 存成员并写入 `memory_summary()`；四展示面各带 `KvarnBits` 渲染级别 ——
`apps/perplexity/main.cpp:kv_name`、`apps/cli/main.cpp:format_kv_cache`、
`src/serve/{request_log,operational_log}.cpp:kv_cache_name`。
**写法（用户裁定）**：**文本面 `kvarn:k4v4`**（与 `--kv-dtype` 逐字一致）、**报告目录分量 `kvarn-k4v4`**
（Windows 路径禁 `:`；由该站点既有的 `safe_component` 转换）。**非 kvarn 档名字逐字不变**。

**实测（GPU，2026-10-08 04:16–04:17；`ninfer-perplexity --text … --context 512 --stride 256`）**

| # | 展示面 | 实测 |
|---|---|---|
| 1 | 报告目录叶（perplexity 自动命名） | `kvarn-k4v4` / `kvarn-k5v5` / `kvarn-k6v6`（三档**各自独立**，取代原同名 `kvarn/`） |
| 2 | `report.json` `execution.kv_dtype` | `kvarn:k4v4` / `kvarn:k5v5` / `kvarn:k6v6` |
| 3 | perplexity stdout | `kv: kvarn:k4v4|k5v5|k6v6` |
| 4 | `ninfer` 生成摘要 | `kv cache dtype  kvarn:k5v5` |
| 5 | `ninfer-serve` operational log | `capacity | KV 512 tokens, kvarn:k5v5, explicit` |
| 6 | request-log JSONL | `"kv_cache":"kvarn:k5v5"` |

**构建**：`ninfer_tests ninfer ninfer-serve ninfer-perplexity` **exit 0 / 251 步 / 0 error**；
**本轮改动面 0 新告警**（既有 nvcc `#128-D`、MSVC `C4244` 计数不变，均在改动面外）。
**host 回归 6 项全过（0 失败）**：`ninfer_{request_log,load_report,bench_support,cli_options,serve_options,kv_capacity}_test`。
**范围外（如实）**：`bench/inference/ninfer_bench_support.cpp:951 kv_cache_name` **未改** —— bench 的 `parse_kv_cache`
不接受 kvarn（上限 `k8v4`）、`BenchOptions` 无 `kvarn_bits`、`NINFER_BUILD_BENCHMARKS=OFF`（该函数仅在单独的
`ninfer_bench_support_test` 编译，无 kvarn 用例）⇒ bench 无 kvarn 档可标级别（归未决项 3）。
**harness bug（记录，非产品缺陷）**：`--text` 且 `--context` 小于默认 stride 时必须显式 `--stride`，否则 CLI 拒绝
（`context/stride must satisfy context>=2 and 1<=stride<context`）；首跑三档皆 exit=1，加 `--stride 256` 后全绿。

**(2) 图谱发现（潜在、非可达；未改）**：`ProgramImpl::capture_identity_tag()`（`program_impl.h:788-792`）**不含
`kvarn_bits`**，而该 tag 写进**持久化磁盘**缓存身份 `DiskKVIdentity.tag`（`storage/disk_tier.cpp:113`）⇒ 理论上
k4v4/k5v5 的磁盘检查点可同 tag。**实测该隐患被目录名兜住**：`disk_profile_directory`（`disk_tier.cpp:37-42`）
把 bits 相关的 `main_stride`（KV 页 stride = 128×记录字节）编进目录名 ⇒ 三档目录本就分离；且同进程内 bits 恒定。
**未改 tag**（改它会使既有磁盘缓存全部失效，属身份语义变更）⇒ 并入 §5 待裁决项。
**该发现只由 `trace_path` inbound 给出**（我最初的 grep 完全没搜到这条调用面），是本轮「图谱纪律」的直接产出。

**(3) 图谱使用纪律根因与落盘（用户指示）**
- **根因实测确认**：SessionStart hook 报 `no indexed graph project matched this working directory` 的**原因是
  工作区根 `D:\ninfer` 是本仓 `D:\ninfer\ninfer-precision-tail` 的父目录** —— hook 只在「已索引工程的根是 cwd 的
  **祖先**」时匹配。实测三次：`cwd=D:/ninfer` → no match；`cwd=…/infer-precision-tail` 与 `…/src` → 均
  `graph project="D-ninfer-ninfer-precision-tail" is indexed`。`list_projects` 才是权威；`config list` 无 cwd/match 旋钮，
  故无法只靠配置修复（可选：把工作区开在本仓目录，或在 hook 前包一层子目录探测）。
- **AGENTS.md（本仓）**新增 `### Codebase memory (indexed graph)` 小节：list_projects 权威 / 改共享类型先枚舉消費方 /
  否定性結論必須走 trace_path+coverage / 字面量仍用 grep / `metadata_changed` 非判據。
- **skill `~/.qoder/skills/codebase-memory/SKILL.md`**：description 增 (f)「改共享類型的字段/枚舉/顯示名」觸發詞與中文觸發；
  Step 0 增「hook 的 no-match 是 cwd 作用域假象，先 list_projects」；新增 `## Field and type plumbing` 段；
  Freshness 增 `metadata_changed` 非判據；Hard rule 擴展到**自己寫的**否定性句子（不止委派審計）；When NOT to use 明寫
  「別把圖譜用於字面量檢索」。

**未做 / 未測（如實）**：① **P3c（kvarn parser 單測）未做**；② 儀器覆蓋殘留（`k5v5/k6v6` 只測 k=1、`sample 0` 之外未測、
測試名仍含 `parity`）**未動**；③ 計劃書 §7-WP7 的 `MemorySummary` **容量口徑**未動（本次只加**名字欄位**）；
④ 未重跑全量 ctest（只跑受影響的 6 項 + 四目標構建）；⑤ **已提交**（用户于本轮末指示提交）：
`6836b2de` feat（7 源码文件）/ `3e780a41` docs（计划书 v14 + 本日志 + 三份归档）/ `efdb1dce` docs（AGENTS.md）。
**產物**：源碼 7 文件；`.deps/kvarn-adm/{p3b_names.sh,p3b_perp.sh,p3b_text.txt,p3b-*.log}`（gitignored）；
`profiles/perplexity/**/kvarn-kXvX/**`（gitignored）；`AGENTS.md`、`~/.qoder/skills/codebase-memory/SKILL.md`；
計劃書 v14 + 附錄 D-14。

### 2026-10-08-7 — P3c 完成：kvarn parser 单测（`ninfer_cli_options_test` / `ninfer_serve_options_test`）；三档解析 + 未发布拼写拒绝 + Engine 贯通全过；**第三处 parser（perplexity）仍无单测**（结构性不可测，行为覆盖由 P3b e2e 提供）

**触发**：用户指示从待办 1（P3c）开工 —— 它是 P3b 的唯一残余（§0 未决项 7 / §4.1-P3c）。纯 CPU、无源码改动。

**改动（2 测试文件，+63 行；`git diff --stat` = `test_cli_options.cpp` +32、`test_serve_options.cpp` +31）**

- `tests/test_cli_options.cpp`（在既有 `nvfp4`/`k8v4` 用例后）：`--kv-dtype` 的 kvarn 面 —— 裸 `kvarn` 与
  `kvarn:k4v4` ≡ `KvarnGroup128` + `KvarnBits::Bits4`；`kvarn:k5v5|k6v6` → `Bits5|Bits6`；默认 `kvarn_bits == Bits4`；
  拒绝 `kvarn:k4v2`（已删内部档）/ `kvarn:k3v3` / `kvarn:k4v5` / `kvarn:k7v7`；help 含 `kvarn:k4v4|k5v5|k6v6`。
- `tests/test_serve_options.cpp`（同位置）：同上解析面（含默认 `Bits4` 与四个拒绝）+ **`make_engine_options()` 贯通**：
  `kvarn:k5v5` → `EngineOptions.kv_cache == KvarnGroup128` **且** `kvarn_bits == Bits5`（这是 serve 侧唯一能覆盖
  parser→Engine 映射的断言点，`generation_service.cpp:263-264`）+ help 含三档。

**实测**：按目标构建 `ninfer_tests`（8 步；仅两个测试对象重编 + 链接）**`exit=0` / 0 warning**（日志 0 条 `warning`）。
host-only 运行（`CUDA_VISIBLE_DEVICES=99`，零显存）：
`./build-port/tests/ninfer_tests.exe ninfer_cli_options_test` → **exit 0**；
`./build-port/tests/ninfer_tests.exe ninfer_serve_options_test` → **exit 0**。
（`ninfer_tests.exe` 是 dispatch 型 runner：`build-port/tests/ninfer_tests_dispatch.cpp:380-384` 按名字转发、未知名字 `exit 2`
⇒ exit 0 是**真实执行**的结论，不是 no-op。）

**未做 / 未测（如实）**
1. **第三处 parser 未加单测**：`apps/perplexity/main.cpp:197-229` 的 `--kv-dtype` 是 **main.cpp 内联的 `else if` 链**，
   其 `Options` 结构体也是 main.cpp 局部（`:69-89`）；既有 `ninfer_perplexity_evaluation_test` 只编译
   `apps/perplexity/evaluation.cpp`（`tests/cmake/ProductTests.cmake:18-24`）⇒ **不重构出可测函数即无法单测**，
   属 P3c 定义范围之外（**未改**，不新增抽象）。该 parser 的**行为覆盖**由 P3b 的 GPU e2e 提供
   （§3-08-06：`kvarn-k4v4` 报告目录 + `report.json` + stdout 三面实测，其路径经此 parser）。
2. **未跑全量 ctest**：本轮**无源码改动**、只改两个测试文件 ⇒ 只跑受影响的两项（符合 AGENTS.md「按改动面选择检查」）。
3. **未提交**（用户约束）。

**产物**：`tests/test_cli_options.cpp`、`tests/test_serve_options.cpp`；`/tmp/p3c_tests.log`。**未提交**。

### 2026-10-08-8 — 图谱复核（实测，**无源码改动**）：`KvarnBits` 消费方集合与 grep **一致（图谱 0 独有文件）**；`reset_kvarn_tail_row` **callers=0 复现**；并实测两条**工具边界**（field 的 `trace_path` 恒 0；`USAGE`/`WRITES` 按名解析会混同同名域、且无访问点行号）⇒ 已回写 `AGENTS.md` 纪律

**触发**：用户提问「能否用 codebase_memory 替代大量 grep/read 以减少上下文占用」，并要求「试一试，然后继续推进」。

**做法**：① 工作树自 `2026-10-07T19:00Z` 后已变（P3b/P3c 未索引）⇒ 先 `index_repository(mode=full)` 刷新：
**45,923 节点 / 216,539 边**（旧 45,868 / 216,412）；② `search_graph` 定位 `KvarnBits` 图谱面；
③ `trace_path` inbound 复核既有 callable 结论；④ `query_graph`（Cypher `MATCH (a)-[r:USAGE|WRITES]->(b:Field)`）枚举域消费方。

**(1) 确认（正面）**
- **域是图谱一等节点**：`search_graph` 返回 13 个 `Field` 声明 —— `enum KvarnBits` + 12 个 `kvarn_bits`/`kvarn_bits_` 成员
  （`EngineOptions` / `MemorySummary` / `ProgramImpl` / `cli::Options` / `ServeOptions` / perplexity `Options` /
  `DecoderStateSpec` / `PagedKVCacheLayout` / `PagedKVCache` / `SequencePlanningInputs` / `SequencePlanImpl` / `KvProfile`）。
- **`reset_kvarn_tail_row` `callers_total=0`** —— 刷新后的图谱**复现**了 §3-08-03 的记录（TAIL 无调用者）。
- **`activate_sequence_kvarn_tail`**：direct（hop=1）调用者 **1** 个（`prefill.cpp:661`）；另两条出现在的
  `commit.start_request` / `materialization.progress_materialization_transaction` 是 **hop 2/3 的传递祖先**，
  **不是**调用点 ⇒ 与 §3-08-03 的「4 个调用点 = restore 1 + activate 1 + capture 2」一致。

**(2) 工具边界（本轮实测；已回写 `AGENTS.md` 的 codebase-memory 小节）**
- **`trace_path` 对 *field* QN 恒返回 0**：对 `MemorySummary.kvarn_bits`（其 `USAGE`/`WRITES` 边确实存在）做 inbound
  追踪得 `callers_total: 0` ⇒ 原句「否定性结论必须走 `trace_path` inbound」**对域消费方不适用**；域必须走
  `query_graph` 的 `USAGE`/`WRITES`。
- **`USAGE`/`WRITES` 的域归属按名解析 ⇒ 混同同名域**：`tests/test_cli_options.cpp` 读 `cli::Options::kvarn_bits`、
  `operational_log.cpp` 读 `MemorySummary::kvarn_bits`，图谱都回成 `ServeOptions.kvarn_bits`；且边**无访问点行号**
  （查询返回的是**所在符号的起始行**）⇒ 域级否定性结论只能作**指示性**证据，须与 grep 对拍后再落盘。
- **本仓 C++ 域的消费方集合，图谱不比 grep 更完备**：图谱源文件集是 grep 文件集的**子集**（0 个图谱独有文件）；
  唯一候选 `startup.cpp:128 persistent_layout` 经读源码证实**就是** grep 的 `startup.cpp:198`（同一处，只是图谱报
  函数起始行、grep 报访问行）。⇒「图谱找得到 grep 漏掉的读者」在**本例（域）**未被证实（反向：grep 更精确）。
  **与 D-14 不矛盾**：D-14 的发现属 **callable** 层（`capture_identity_tag` 的调用面，`trace_path` 在该层可靠）；
  本例说的是**域**消费方枚举（该层不可靠）。

**(3) 未做（如实）**：未对新索引重跑全量 `check_index_coverage`（仅按需查询）；**未改任何源码**；**未提交**。

**产物**：`AGENTS.md`（纪律订正两处）；本日志 §3-08-08 / §5 图谱段 / §6 索引。**无源码改动**。

### 2026-10-08-9 — 订正（读源码实测）：审计 LOW② 的「capture/activate 抛错被 `catch(...)` 静默化」**只对 commit 侧成立**；并补记 `reset_kvarn_tail_row` 的标记语义与 FORK 测试的真实用途（**无源码改动**）

**触发**：用户要求逐项解释 §5 待裁决项（问题 / 代价 / 后果）；读源码时发现 §3-08-03 的 LOW② 表述不准确，按「实测优先」回写。

**(1) LOW② 订正：三条调用路径中只有 commit 静默**

| 抛错源 | 调用点 | 包裹 | 结果 |
|---|---|---|---|
| `capture_sequence_kvarn_tail`（`context.cpp:1756`） | `capture.cpp:726` | 其唯一调用者 `capture.cpp:1302-1308`：`try { … } catch (...) { abort_active_capture(transaction); transaction.published = true; throw; }` | **abort + 重抛（上抛，非静默）** |
| `capture_sequence_kvarn_tail` | `commit.cpp:660` | `publish_active_continuation(...) noexcept`（`commit.cpp:645-674`）末尾 `catch (...) { return false; }`（:674） | **静默吞掉**：continuation 不发布、无日志 |
| `activate_sequence_kvarn_tail`（`context.cpp:1796`） | `prefill.cpp:661` | 在 `ProgramImpl::start_sequence`（`prefill.cpp:205` 起）内；`380-661` 区间**无 `try`/`catch`** | **直接上抛** |

⇒ **唯一真正的静默路径是 commit**。抛错前置条件是规划不一致（KV=`KvarnGroup128` 而 StateImage 池未带 KVarN），`has_kvarn()` 由 startup 的同一 `KvarnGroup128` 门控置位 ⇒ 当前**不可达**。§3-08-03 该句**已就地标注订正**（未删除）。

**(2) `reset_kvarn_tail_row` 的语义与 FORK 用途（供解释 ①/(b)）**
- **语义**：`PagedKVCache::reset_kvarn_tail_row(table_row, stream)`（`decoder_state.cpp:360-371`）把**某一执行行**在所有层的 `tail_logical_pages` 标记重置为 `0xff`（"不指向任何页"）。标记形状 `I32[kKvarnTailSlots]` 逐 `(layer,row)`；构造期即为全 `0xff`（`decoder_state.cpp:220-222`）。
- **标记运行期只被追加**：KVarN 注意力/staging 在**认领**某页尾槽时写入（`attention.cu:75` 以**非 const** `int32_t*` 传入；`mapped_tail_slot` `attention.cu:91-96` 用它匹配逻辑页）⇒ 热路径不清理。故该函数是**执行行复用**的卫生原语：行易主后若不重置，staging 可能误判"页 P 的尾槽已就位"而**复用上一占用者的尾 K/V**。
- **FORK 的两处调用都在测试里**（`test_prefill_precision_real.cpp:236,284`）：该测试 `execution_tables().acquire(0)` 后**跨多个子用例复用同一行 0**（每子用例 `state.zero_all(...)` + `reset_kvarn_tail_row(0, stream)`），并比较 conv/recurrent 状态与最终 logits（基线 / cold / cached 三档）⇒ **FORK 生产代码同样 0 调用者**。**移植缺的不是"生产行为"，而是那个测试。**
- **未做**：未追查 TAIL 是否存在"回收 KVarN 执行行且其标记随后被读取"的路径 ⇒ 可达性**未验证**。

**(3) 未改源码、未提交。产物**：本日志 §3-08-03 的就地订正 + 本条。

### 2026-10-08-10 — 审计 3 项 LOW **修复落地**（用户批准）：① CausalScore 路由补 `reset_kvarn_tail_row`（**生产行为变更**）· ② commit 发布失败不再静默 · ③ `state_image.h` 注释补 KVarN；新增专项单测（**ctest 262 → 263，实跑 PASS**）；**GPU 侧回归未跑**（用户指示）

**触发**：用户指示「按照你推荐的方法修好这些问题」，并对 FORK 仓（`ninfer-rtx5090-mobile`）给予完全授权。

**调查（2 个只读子代理 + 我的逐点复核）**
- **① 的真实缺口在 CausalScore 而非「函数是死代码」**：执行行 = lane 且**会被回收**（`context.cpp:103-107` release 抬代 → `commit.cpp:85-88,116-120` 准入复用）。**生成路径安全**：每次 start 由 `activate_sequence_kvarn_tail` **整行覆盖**标记（`context.cpp:1823-1825`，源为零清零/捕获的镜像）。但 **CausalScore 不走 `start_sequence`**（`trace_path start_sequence` 实测 direct 调用者仅 `commit.ProgramImpl::start_request`），却 `create_active(entitlement, 0, …)` **硬编码 row 0**（`program_impl.cpp:659`）并断言 `bound_row == 0`（`:663-665`），跨多次打分复用 row 0 而**不恢复任何镜像**；而标记在运行期**只被追加**（`attention.cu:75` 非 const 指针；`mapped_tail_slot` `:91-96` 用它匹配逻辑页）⇒ 上一占用者的标记会被当作本序列的读入。
- **② 的修法**（子代理 B）：`publish_active_continuation`（`commit.cpp:642-692`）为 `noexcept`，两个调用者（`finish` `:583`、`salvage_continuation` `:696`）皆 `noexcept` 且把 `false` 当**正常的最好努力拒绝**（abort→salvage/discard）⇒ `catch(...) { return false; }` 静默且与合法拒绝不可区分。仓内先例：`report_capture_release_drift`（`capture.cpp:601-612`）= **stderr 诊断 + 返回 false**。**(C) 去 `noexcept` 被否决**（一处规划不一致会升级为引擎级 `warning + recover`，失败所有在途请求，`engine_core.h:2453-2472,2193-2199`）。

**改动（3 源码 + 1 新测试 + 1 cmake）**
1. **①** `program_impl.cpp`（`causal_score`）：`bound_row` 断言之后、`ensure_mapped_to_tokens` 之前加
   `decoder->text_kv.reset_kvarn_tail_row(0, compute_streams[0]);`。该函数自述「No-op on a non-KVarN cache」（`decoder_state.h:133-134`）⇒ 无需显式门控。
2. **②** `commit.cpp`：`publish_active_continuation` 的 `catch(...)` 前插入 `catch (const std::logic_error& error) { std::fprintf(stderr, "active continuation publish refused: %s\n", error.what()); return false; }`；补 `#include <cstdio>`。（保留 `catch(...)`：`CUDA_CHECK` 抛 `CudaError : std::runtime_error`。）
3. **③** `state_image.h`：`StateImagePart` 与 `StateImageDevicePool` 的文档注释补上「KVarN body 的 full-attention sink/tail（含 MTP 池）」这一占用者（原先只列 GDN/hidden 与 DFlash）。
4. 新测试 `tests/models/qwen3_5/test_kvarn_tail_row_reset.cpp`（284 行，设备侧）+ `tests/models/qwen3_5/tests.cmake` 注册。

**实测**
- 构建：`ninfer_tests ninfer` **exit 0 / 234 步**（`state_image.h` 触发宽重编）；`ninfer-serve ninfer-perplexity` **exit 0 / 7 步**。
- **新测试 `ninfer_qwen3_5_kvarn_tail_row_reset_test` 首跑 PASS**（`OK kvarn tail row reset`，exit 0）：`DecoderStateSpec` → `plan_decoder_state` → 真 `PagedKVCache`；经**公开**的 `kvarn_batch_layer_view(layer).tail_logical_pages` 播撒标记；断言「重置行的每层标记为 `0xff`、其它行逐字节不变」「越界行 `-1`/`rows` 为空操作」「非 KVarN 缓存调用不抛且不动一字节」。测试自身的几何断言（`I32[kKvarnTailSlots, rows]`、行距 12 B）经我核对 `tensor.cpp:set_contiguous_strides`（**`nb` 以字节计**）与 `decoder_state.cpp:352-353` 成立。
- host（`CUDA_VISIBLE_DEVICES=99`）**4 项全过**：`ninfer_qwen3_5_exact_tail_capacity_test`、`ninfer_qwen3_5_state_image_layout_test`、`ninfer_cli_options_test`、`ninfer_serve_options_test`。
- `ctest` 注册数 **262 → 263**（`ctest -N` 实测）。

**未跑 / 未验证（如实）**
1. **① 的生产行为变更没有执行证据**（用户指示「先不跑需要 GPU 的测试」）。原计划的决定性对照 —— **改动后跑一个 `kvarn:k4v4` KLD 臂，与 P3a 记录的 mean KLD `0.002120` 对比** —— **已启动即被叫停**（无输出）。两种可能都**未测**：若数值仍为 `0.002120`，说明该路由上标记本已被清（本改动为无害 no-op）；若不同，则 **D-11 / P3a 的 KLD 数值需重测**（那将是本移植的一个真实质量缺陷被修）。现有证据仅为：逻辑论证（row 0 硬编码 + 生成路径之外无覆盖 + 标记运行期只追加）+ 上面那个**测原语而非测路由**的单测。
2. **未移植 FORK 的 `test_prefill_precision_real.cpp`**（用户批准的是「移植它」）。**偏离理由**：调查显示该测试练的是**已受保护的 prefill 路径**（生成路径每 start 整行覆盖），而真实缺口在**打分路由**；且 TAIL 已有 prefill/前缀复用覆盖（`test_engine_prefix_real`、P1 e2e）。故改为「生产修复 + 定点单测」。**如需仍要 FORK 那支测试（现已获授权），可再补。**
3. 其余 GPU 回归（`ninfer_kvarn_test`、续列尾 device 段、score real 等）**未跑**。
4. 顺带：`.qoder/`（本会话隔离 worktree 的目录）在仓内且未被 ignore，**未提交**。

**产物**：`src/models/qwen3_5/program/program_impl.cpp`、`src/models/qwen3_5/program/transactions/commit.cpp`、`src/models/qwen3_5/state/state_image.h`、`tests/models/qwen3_5/test_kvarn_tail_row_reset.cpp`（新）、`tests/models/qwen3_5/tests.cmake`、`/tmp/fix_build{1,2}.log`、`/tmp/host_*.log`。**已提交**：`b30c3a98` fix（本轮 3 项修复）/ `0754708e` test（P3c）/ `69f84b3e`+`7582d7ff` docs（见 §0 提交清单）。

---

## 4. 下一步（按计划书 WP 顺序）

> **状态**：WP4 于 07-22 收尾、07-23 完成 KVarN 本质三方核对、08-01 完成全量 ctest 复核（§3-08-01）、
> **08-07 完成 P3c（kvarn parser 单测）**。**计划内的功能项与收尾项均已落地**；全量 ctest（261 项，08-01 那次）
> 已比对完毕（见 §4.0-P0），此后**未再重跑全量**（注册数已由 261 → **262**，见 §3-08-03）。

### 4.0 主线判定与推荐顺序

- **排序依据**：**功能缺口已清零**（续列尾/前缀复用、MTP 路径均已落地并验收）⇒ 余下为判据补全（已完成）>
  覆盖补强（残留）> 评估。
- **推荐顺序：P1 → P2 →（P3 可并行）→ P4 → P5(WP6)** —— **P1–P4 全部完成**，仅剩 **P5 = WP6**（门禁后）。
- **WP6 门禁**：**仅当 P1/P2 完成（✅ 已完成）且用户明确授权后才启动**（10–15 天、最高风险、计划书 §6 / §7-WP6）。
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
| **P3d — 审计 3 项 LOW 修复** | **完成（08-10）**，**① 缺执行证据** | ① CausalScore 路由补 `reset_kvarn_tail_row(0, …)`（**生产行为变更**；row 0 硬编码、生成路径之外无覆盖、标记运行期只追加）· ② `commit.cpp` 发布失败加 stderr 诊断（不再静默）· ③ `state_image.h` 注释补 KVarN；新增 `ninfer_qwen3_5_kvarn_tail_row_reset_test`（**实跑 PASS**，ctest 262→263）。**未跑**：① 的 `kvarn:k4v4` KLD 臂对照（`0.002120`，用户指示不跑 GPU）；**未移植** FORK 的 `test_prefill_precision_real.cpp`（改为定点单测，理由见 §3-08-10） |
| P4a — WP5 容差形式化 | **完成**（08-03 形式化 / 08-04 实测） | 量化步长判据 + 4/5/6 位穷举往返；`flips` 1/65、`over_step=0` ⇒ 65× 余量、未放宽；口径写入 `op-development.md §6.3` |
| **P4b — kvarn bench 归属** | **待用户决定** | 需 `-DNINFER_BUILD_BENCHMARKS=ON` 重配，与 AGENTS.md「不要重配」冲突（未决项 3） |
| **P5 — WP6 旋转域尾部合并** | **未启动** | 10–15 天、最高风险；P1/P2 已完成 ⇒ 启动前仍需按计划书 §6 / §7-WP6 落实门禁 |
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
> 环境纪律、建议的第一条命令。**最后更新 2026-10-08（本轮 08-10）。**

**任务**：在 `D:\ninfer\ninfer-precision-tail` 继续 KVarN 移植 —— **计划内的 kvarn 功能项与收尾项均已完成**，
且 **08-10 已把审计的 3 项 LOW 修复落地**（含一处 **CausalScore 路由的生产行为变更**）。
剩余：**① 的端到端对照未跑**（最优先的待验证项）、**覆盖残留**、以及 3 个待用户裁决项（FORK 测试 / `capture_identity_tag` / bench 归属）。

**先读（顺序）**：本文件 `kvarn-port-progress.md`（**§0 快照 / §3-2026-10-08-10（最新）/ §3-08-09 / §3-08-08 / §4.1 / §5**）
→ `kvarn-port-into-precision-tail-plan.md`（**版本头 v16**、§1 A2/A3/A8、§7-WP2·WP3·WP5·WP6、附录 D-11…D-16）。
二者是唯一权威；**冲突时以本文件的实测为准并回写计划书**（计划书 §0.5 规则）。

**当前状态（务必先核验，勿臆断）**
1. **GPU 空闲**（显存 0 MiB）；`ninfer_tests/ninfer/ninfer-serve/ninfer-perplexity` 均**最新构建绿**
   （08-10 构建：前者 234 步、后两者 7 步，均 exit 0）。
2. 工作树：**08-07…08-10 的全部改动已提交** ⇒ 冷启动时 `git log -1` 应是 08-10 的提交（清单见 §0）。
3. 测试：新增 `ninfer_qwen3_5_kvarn_tail_row_reset_test` **实跑 PASS**（ctest 注册 **262 → 263**）；
   host 4 项（`exact_tail_capacity` / `state_image_layout` / `cli_options` / `serve_options`）**exit 0**。
   **未跑（用户指示「先不跑需要 GPU 的测试」）**：任何 GPU 回归 —— 尤其 **① 的 `kvarn:k4v4` KLD 臂**（对照 P3a 的 `0.002120`）、
   `ninfer_kvarn_test`、续列尾 device 段、`ninfer_qwen3_5_score_real_test`。

**待办队列（按依赖序）**
1. **【最优先，需 GPU】验证 ① 的生产变更**：跑一个 `kvarn:k4v4` KLD 臂，协议与 `.deps/kvarn-adm/run6.sh` 逐字相同
   （`--corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --disjoint --score-width 8 --score-topk 100
   --kv-dtype kvarn:k4v4 --kv-tail-tokens 0 --kld-base .deps/kvarn-adm/bf16-t0.topk`），与 **P3a 的 mean KLD `0.002120`** 对比：
   - 若仍为 `0.002120` ⇒ 该路由上标记本已被清，本改动是**无害 no-op**；
   - 若不同 ⇒ **D-11 / P3a 的 KLD 数值需重测**（说明该路由此前确有陈旧标记缺陷，被本次修复）。
   另建议补跑 `ninfer_kvarn_test`、`ninfer_qwen3_5_kvarn_continuation_image_test`、`ninfer_qwen3_5_score_real_test`。
2. **待用户裁决（3 项，仍未答复）**
   (a) **是否仍要移植 FORK 的 `test_prefill_precision_real.cpp`** —— 已授权读 FORK 仓；本次**未移植**，改为 TAIL 定点单测 + 生产修复（理由见 §3-08-10）；
   (b) `capture_identity_tag()`（`program_impl.h:780-796`）**是否补 `kvarn_bits`** —— 潜在、非可达，**且代码注释已写明理由**（随 `6836b2de` 提交）；补则使既有磁盘缓存全部失效；
   (c) kvarn bench 归属：需 `-DNINFER_BUILD_BENCHMARKS=ON` 重配，与「不要重配」冲突（未决项 3）。
3. **覆盖残留（低优先，非交付面）**：仪器中 `k5v5/k6v6` 只测了 k=1；`sample 0` 之外未测；MTP 测试名仍含 `parity`（未改名）。
4. **WP6（旋转域尾部合并，10–15 天、最高风险）—— 仅当用户明确授权后启动**（计划书 §6 / §7-WP6 门禁）。
5. **计划书侧**：v16 已回写（本轮 3 项修复 + 附录 D-16）。

**08-03 的两个非显然发现（勿丢）**
- **`reset_kvarn_tail_row` 移植缺口**：TAIL **零调用者**；FORK 有 **2 处**，都在
  `D:\ninfer\ninfer-rtx5090-mobile\tests\models\qwen3_5\test_prefill_precision_real.cpp:236,284`，
  **该测试未随移植进入 TAIL**。⇒ 既属"陈旧 marker"隐患（行重用若不由 store op 重写 markers），
  也是**移植遗漏的测试覆盖**。**⚠ 原审计写"两树均无调用者"不准确**——由 codebase-memory 图谱 + grep 复核修正（§3-08-03）。
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
- 本轮新增了 `codebase-memory-port-auditor` 子代理（Tier 3，移植完整性/否定性结论审计）并改写了 `codebase-memory` skill；
  若新窗口看不到该 agent 类型，重开会话即可。结构性问题（"是否漏了调用点""是否未被使用"）**必须**用它，
  **不要**交给通用 Explore 类子代理（无 MCP，做不到结构完备性）。

**建议第一条命令**
```bash
cd /d/ninfer/ninfer-precision-tail && git status --short && git log --oneline -1 && nvidia-smi --query-gpu=memory.used --format=csv,noheader
```
GPU 空闲（0 MiB）⇒ **P3c 已收口**。建议从**待办 1 的裁决项**起步 —— 先向用户取 (a)–(e) 的决定
（尤其 (d) `capture_identity_tag` 身份语义、(e) WP6 授权）；若有空可并行做待办 2 的覆盖残留
（`k5v5/k6v6` 的 k=3/15、`sample 0` 之外的样本）。

**codebase-memory 图谱（2026-10-08 起可用，勿再被 SessionStart hook 误导）**：本仓已被索引
（`D-ninfer-ninfer-precision-tail`，**45,923 节点 / 216,539 边**，`indexed_at 2026-10-08`（08-08 重跑 `index_repository(mode=full)`，
已含 P3b/P3c 的代码与测试）。注意：**SessionStart hook 仍报 "no indexed graph project
matched this working directory"（陈旧的误报）**——直接 `list_projects` 即可看到本仓。使用纪律：
依赖某文件前先 `check_index_coverage`（`index_status` 报 `parse_partial 253` / `not_indexed 79` /
`parse_unusable 7`（多为 `third_party/`），CUDA `.cu/.cuh` 可能在 partial 列表内 ⇒ **miss 的行直接读源码**）；
**callable** 的否定性/完备性问题（"谁调用 X""X 是否未被使用"）**必须**走 `trace_path` inbound，
**不要**交给无 MCP 的 Explore 子代理。
**两条本轮实测的工具边界（勿踩，详见 §3-08-08、已回写 `AGENTS.md`）**：
① `trace_path` 对 **field** QN **恒返回 `callers_total: 0`**（即使 USAGE/WRITES 边存在）⇒ 域消费方必须走
`query_graph` 的 `USAGE`/`WRITES`；② 那些 `USAGE`/`WRITES` 边**按名解析**（混同同名域、无访问点行号）
⇒ 域级结论只作**指示性**，须与 grep 对拍。**本轮实测结论：`KvarnBits` 的消费方集合图谱与 grep 一致
（0 个图谱独有文件）**；图谱确认 `reset_kvarn_tail_row` 在 TAIL `callers_total=0`；`activate_sequence_kvarn_tail`
的 direct 调用者 1 个（`prefill.cpp:661`，其余为 hop≥2 的传递祖先，切勿当调用点）。
---

## 6. 已归档信息索引（2026-10-08）

本日志精简时**只搬运、不改写**：下列内容全部逐字（字节级）移入归档文件，正文各处只留一行索引/指针。
§1 环境与构建速查、§2 关键路径与事实索引、§3 的 2026-10-08-1 … -5、§4.0/4.2/4.3、§5 handoff **保留在正文**。

| 归档文件 | 覆盖范围（本文件源行号） | 一句话内容 | 正文索引位置 |
|---|---|---|---|
| `docs/port-records/KVARN-PROGRESS-ARCHIVE-2026-10.md` §A | §3 条目 **2026-10-07-1 … -23**（260–1312） | WP0 基线、WP1 ops/测试、A3 的四轮追查、WP2/WP3/WP4 全过程与 229k 准入实验的逐日记录（含失败与被推翻的假设） | §3 开头的条目索引表（23 行，条目号 + 原标题逐字） |
| 同上 §B | §0「最后更新」散文段（16–34） | 08-05 / 08-04 / 08-03 三轮的实测摘要 | §0 压缩说明 |
| 同上 §C | §0 WP 状态明细表（36–47） | 每包当时的详表状态与实测数字 | §0 现值表（已压缩为现值 + 指针） |
| 同上 §D | §0 工作树改动清单（49–131） | WP1–WP5/P1 的逐文件改动（函数、守卫点、switch 位置） | §0 的「工作树改动清单」指针 |
| 同上 §E–§I | §0 未决项 1 / 5 / 8 / 9 / 10（135–142 / 146–167 / 177–192） | 已关闭或已被取代的未决项原文（含 A3 定案依据、页几何根因与修法、续列尾「未实现」旧表述） | §0 未决项列表中对应编号的索引行 |
| 同上 §J | §4.1 任务清单（1574–1607）与 §4.4 历史 WP 状态（1615–1648） | P0–P5 的原始「目标/内容/验收/风险/估时」与当时的 WP 状态 | §4.1 现行表 + §4.4 指针 |
| 同上 §K | 正文里被**订正**的 5 行原句（计划书原 800 行；本日志原 6、1669–1671 行） | 订正前的原文（含 fenced 逐字节副本）：附录 D 的旧标题、「配套权威（方案 v3）」、「工作树**未提交**」三行 | §4.3 的订正注 + §5 当前状态第 2 条 |

**未归档（现行推进计划需要）**：§0 现值表、仍开放的未决项 2 / 3 / 4 / 6、§1 全部、§2 全部、
§3 的 2026-10-08-1 … -10、§4.0/4.1/4.2/4.3、§5。计划书侧的对应归档见
`docs/port-records/KVARN-PLAN-APPENDIX-ARCHIVE.md` 与 `docs/port-records/KVARN-PLAN-CHANGELOG-ARCHIVE.md`。
**§0.5 记录规则不变**：WP 边界仍须「快照 + 日期条目 + 回写计划」，检索路径为 §3 索引表 → 归档 §A。
