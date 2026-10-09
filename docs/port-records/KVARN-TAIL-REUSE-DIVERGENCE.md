# 精确尾环 × 前缀复用的确定性失配（KVarN 精度尾 / WP6.4 开口缺陷）

> **本文是该问题的唯一权威记录**（症状、证据、上游取证、方案取舍、④ 实施与未决项）。
> `kvarn-port-progress.md` 与 `kvarn-port-into-precision-tail-plan.md` 只保留**索引指针**。
> 状态：**④「降级披露」已实施**；**根因未定**（见 §7）；**影响面已扩大到「命中即不逐位」（与尾无关，见 §4.5）**；
> **用户已于 2026-10-09 裁决「接受」**（附条件与未测项，见 §5.1）⇒ 本问题**不再作为阻塞缺陷**，诊断线挂起。
> 首次记录：2026-10-08（进度 §3-08-22 / -23 原文即本问题全过程）。

---

## 1. 现象与判定边界

同一 `ninfer-serve` 会话内，发**两次完全相同的 greedy 请求**（prompt ≈ 856 token，`--derive-session-keys`），
第二次按设计复用前缀（日志 `cache 851 (99.4%, turn closure)`）。**r2 与 r1 的 message 是否逐字节相同**：

| 配置 | `cached_tokens`(r2) | r1==r2（`kvarn:k4v4`） | r1==r2（`bf16` 对照） | 判定 |
|---|---|---|---|---|
| 无尾（基线，08-04/08-18/08-19 记录） | 851 | **YES** | **YES** | PASS |
| `--kv-tail-tokens 1024`（**> prompt ⇒ 整段在环内**，`body_window=0`） | 851 | **YES** | **YES** | PASS |
| `--kv-tail-tokens 256`（**< prompt ⇒ body+tail 并存**，`body_window=600`） | 851 | **NO** | **NO** | **FAIL** |
| 同上 + `--kv-tail-type bf16` | 851 | **NO** | **NO** | **FAIL** |
| 同上 + `max_tokens=16` | 851 | **YES** | **YES** | PASS |

⇒ **逐位同一只在「尾不活跃（`tail=0`）」或「整段落在尾环内（`tail ≥ prompt`）」时成立**；
一旦 **`0 < tail < prompt`**（body 与 tail 并存），复用输出与新鲜输出**确定性失配**。

### 失配刻画（全部实测，非推断）
- **非抖动**：`kvarn r1` 两次独立运行逐位相同、`kvarn r2` 两次独立运行亦逐位相同 ⇒ 两条路径**各自确定**，是**系统性差异**。
- **与 KVarN 无关**：`bf16` 对照臂同样失配，且更早（reasoning 第 **27** 字符 vs kvarn 第 **207** 字符）。
- **与尾 dtype 无关**：f16（默认）与 bf16 的失配点与长度**完全一致**（`@207`、`1016/1027`）。
- **失配点随 `max_tokens` 变**：`max_tokens=16` 两臂全同；bf16 臂 `m16 r1 == m200 r1[:75]`（新鲜路径前缀自洽）
  但 `m16 r1 != m200 r2[:75]`（**复用路径自身随请求长度改变**）⇒ 不是固定位置的数值偏移。

## 2. 复现命令与产物

```bash
# 尾 < prompt（失配）
TAIL=256 bash .deps/kvarn-adm/wp64_prefix_reuse_bodytail.sh      # → RESULT: FAIL
# 尾 > prompt（一致）
bash .deps/kvarn-adm/wp64_prefix_reuse_tail.sh                    # → RESULT: PASS
# 无尾基线
bash .deps/kvarn-adm/p1_prefix_reuse.sh                           # → RESULT: PASS
```

产物（均 gitignored）：`.deps/kvarn-adm/wp64_prefix_reuse_bodytail.sh`（参数化 `TAIL/TAILTYPE/MAXTOK/SUFFIX`）、
`wp64bt*-{kvarn,bf16}-{r1,r2}.json`、`wp64bt*-serve.log`；日志 `/tmp/wp64_e2e.out`、`/tmp/wp64_bt256{,_tbf16,_rep,_m16}.out`。

## 3. 上游 beellama 取证（只读，源码 `D:/ninfer/beellama.cpp` @ `58a162927`）

图 project `D-ninfer-beellama.cpp`（60,529 nodes / 287,716 edges，索引 2026-10-07）⇒ 新鲜复用。
两个 Tier-2 子代理并行取证；**★ 三处最重断言由本人逐字复核**。

| 面 | 上游做法 | 证据 |
|---|---|---|
| 尾随状态存取 | 尾行、records、provenance、write cursor、rollback、`tail_ordinal`、per-record `local_slot` **一起序列化**；恢复 = **原始字节拷贝**（无去量化/重量化）+ 按 `local_slot` 重映射 | `llama-kv-cache.cpp:4436-4474`、`4520-4552`、`5282-5300`、`6383-6411`；`llama-kv-cache-tail.cpp:1288-1323`、`1418/1425` |
| 序列复制 / 前缀复用 | `prepare_seq_cp`/`commit_seq_cp`/`cancel_seq_cp` **事务化**（拷贝成功才 commit，失败 cancel + `seq_rm(dst)`）；复用走 `state_seq_get_data_ext/set_data` 或 `mem.seq_cp` | `llama-kv-cache-tail.cpp:832/844/937/963`；`llama-kv-cache.cpp:1611-1628`、`2197-2201`、`2282-2285`；`server-context.cpp:372/395/863-864` |
| ★ 尾成员集的确定方式 | **主机侧、按序列**：`tail.source_candidates(seq_id)` 取最新 `retention()` 条 → `desc[4]`/`desc[6+i]` ⇒ **与批次形状无关** | `llama-kv-cache.cpp:7504-7530` |
| 一致性不变量 | 尾条目与 body 单元**同键** `(stream, cell, generation)`；重复物理单元抛 `"duplicate physical cell in KV tail sequence"`；`recycle` 在 generation 变化时失效 | `llama-kv-cache-tail.cpp:669`、`810-822`；`llama-kv-cache.cpp:2901-2904` |
| ★ 唯一不保证逐位的路线 | `--cache-reuse` = `seq_rm`+`seq_add` ⇒ KV 移位；对 `k_tail` 做与 body 相同的 rope **增量重旋转**（非重新 prefill）⇒ **主动打降级** | `llama-kv-cache.cpp:1814-1818`（`mark_degraded(..., LLAMA_KV_TAIL_DEGRADED_HISTORICAL_OP)`）、`4258-4266`、`set_input_k_shift_tail:6874` |
| ★ `COMPLETE` 的门槛 | coverage = `NONE`（`exact==0`）／`COMPLETE`（**仅当** `exact >= requested && flags == 0`）／否则 `PARTIAL` | `llama-kv-cache-tail.cpp:1146-1148` |
| graph key | `llm_graph_kv_tail_identity` 含 `storage_kind/exact_type/retention_tokens(N)/rollback_tokens/arena_stride/storage_slots/compact` + 逐层标志；**N 入键** | `llama-graph.h:326-346`、`llama-graph.cpp:646-671/678-710`、`3442-3466`、`6959-6999` |

### 结论
- **状态存取 / prompt-cache 复用路线：按设计没有该问题。** 尾与 body **同身份、同事务、随状态字节级往返**；
  尾成员集是**序列/位置**的函数、与 prefill 批次形状无关 ⇒ 新鲜 prefill 与恢复运行看到**同一分组**。
- **`--cache-reuse` 移位路线：有同类现象，但被显式披露** —— 上游承认该路线不保证逐位，于是打
  `DEGRADED_HISTORICAL_OP` 并把 coverage 降为 `PARTIAL`。⇒ **不逐位是被允许的，但必须可见。**

## 4. 与本仓的结构差异（据此收窄假设；**仍为假设**）

本仓的精确尾是**另一个独立 device 页池**（`DeviceKVPagePool exact_tail_`，`state/decoder_state.h:156/191`），
由**单独内核** `stage_exact_tail` 在 `rotate_kv` 前写入（WP6.3）；body/tail 分区在核内算，须用
**launch 级最新位置**（§3-08-19）。对照上游"同键同事务 + 主机侧按序列选集"，**最可疑的差异是
「尾成员/分组是否依赖批次形状」**。**本轮只证「上游设计上不具备该依赖」，未证「本仓失配即由其造成」。**

### 4.1 本仓取证（2026-10-08，只读）：**精确尾环不在任何检查点内**

两个 Tier-2 子代理独立取证，**本人逐字复核 ★ 四处**：

| # | 断言 | 证据 |
|---|---|---|
| ★1 | `StateImageSpec` **没有**尾环区域 | `src/models/qwen3_5/state/state_image.h:52-57` = `{linear, hidden, optional dflash_local, optional kvarn}`；host layout 区域 = `linear_conv` / `linear_recurrent` / `continuation_hidden` / `dflash_local_k,v` —— **无 exact tail** |
| ★2 | 尾环是**独立池**，与 body 池不是同一个 | `src/models/qwen3_5/state/decoder_state.h:191`：`std::optional<DeviceKVPagePool> exact_tail;`，注释 "no execution tables, addressed by position ring, device-only"；`attach_exact_tail` 在 `decoder_state.cpp:402-407` |
| ★3 | 寻址按**批次行**，不是按序列 | `src/ops/kvarn/tail_partial.cuh:186`：`ring = batch * ring_pages + ((key >> kPagedKVPageShift) % ring_pages)`，其中 `batch = blockIdx.z`（`:86`） |
| ★4 | 视图里**没有**占用/有效标记 | `src/core/paged_kv_cache.h:38-49`：`PagedKVExactTailView{k_pages, v_pages, page_count, retention}`，注释 "Addressing is implicit: the page for absolute position p is `(p/64) % page_count`, **so no block table is stored**" |

**补充（子代理取证，本人未逐字复核）**：`KvarnContinuationStateSpec`（由 `capture_sequence_kvarn_tail` 写入，`program/storage/context.cpp:1756`，调用点 `capture.cpp:726` / `commit.cpp:661`）拷的是 **KVarN 的 sink/tail slab**，**不是**精确尾环；三个入口均 kvarn-gated（`context.cpp:1733/1758/1797` ⇒ 非 KVarN 全部 no-op）。
负向声明所跑的查询：`query_graph` 对 `exact_tail_`/`exact_tail` → **3 个 Field 节点全在 `decoder_state.h`**；`grep exact_tail src/models/qwen3_5/program/**` → 仅 `startup.cpp`。覆盖度：所依赖文件均 `no_recorded_issue`（脏树 `metadata_changed`），被标记的 `parse_partial` 行已直读。

**结论**：尾环**不随任何检查点保存/恢复**（**对所有 storage，不限于 bf16**）；复用后该批次行槽位的环由**上一个占用者遗留的行**填充；**没有任何占用计数或有效标记**阻止读到从未写入的行（只有写入侧的 `kv_tail_row_in_ring` 守卫）。
⇒ 这是与上游（尾随状态字节级往返）的**实质性架构缺口**。
**但它不是失配的原因 —— 见 §4.2（已被判决实验否证）。**

### 4.2 判决实验（2026-10-08）：**「环残留」假说被否证**

**设计**：同一次服务会话内，先发一条**不同内容的长请求 P**（827 token，覆盖 B 读取窗口 256 槽中的 227 槽）把环弄脏，再发标准 B 两次；
把 B 的**第一次（新鲜，`cache 0`）**与**干净会话**的基线 r1 逐字节比较。脚本 `.deps/kvarn-adm/wp65_ring_residue.sh`（两个臂各一次会话）。

| 臂 | 新鲜（脏环 vs 干净环） | 复用（脏环 vs 干净环） | 同会话内 fresh vs reuse |
|---|---|---|---|
| `kvarn:k4v4` | **完全相同**（917/917） | **完全相同**（924/924） | **不同**（复现原失配） |
| `bf16` | **完全相同**（935/935） | **完全相同**（963/963） | **不同**（复现原失配） |

**判定**：
1. **新鲜请求对环的前置内容完全不敏感** ⇒ 新鲜 prefill 会写满它自己读取的每一个环槽（自洽）⇒ **「复用后读到上一占用者遗留的行」不是失配的原因**。
2. 复用的答案**同样**只由 `(prompt, checkpoint)` 决定，与环的前置内容无关；且 **r1/r2 的分歧在两个会话中逐字节复现**（长度 917/924、935/963 一致）。
3. ⇒ 失配是**恢复路径自身**的性质（"全新 prefill" vs "从检查点恢复后继续"），**不是环污染**。

**由此产生的新首选假说（未证）**：恢复路径可能对 body 做**再物化 / 重建**（`rebuild_work` / `rebuild_tail_begin` 等概念存在于 `SequenceState` 与 `prefill.cpp`），而重建走的是**另一条 kernel / 另一套 split 分组** ⇒ 舍入不同。
**该假说与全部已知事实一致**：① 只有 `0 < tail < prompt`（存在非空 body）才失配；② `tail ≥ prompt`（`body_window = 0`，**无 body 可重建**）逐位一致；③ 与 KVarN 无关（bf16 同现）。**下一步应先静态排查这条路径。**

### 4.3 第二轮取证（2026-10-08）：`rebuild_work` 假说**亦被否证**；候选收窄到「路线 / 分组」

两个 Tier-2 子代理独立取证（结论一致）：

| 断言 | 结论 | 证据 |
|---|---|---|
| `PrefillWork` / `rebuild_work` / `rebuild_tail_begin` 会触发 body 重算吗 | **否 —— 纯成本账目** | `PrefillWork` = `{chunks, tokens, attention_pairs, vision_items, vision_patches}`（`src/runtime/contract/resources.h:18-26`），仅供 `resource_manager.h:803/874/957`、`pressure_planner.cpp:502` 的驱逐定价；`rebuild_tail_begin` 的**唯一读者**是 `advance_rebuild_work`（`storage/context_work.cpp:74-79`）且只做 chunk 算术；`advance_rebuild_work` 只在**追加 token** 时触发（`prefill.cpp:966`、`decode.cpp:1082`、`commit.cpp:397`），**不为恢复的前缀触发** |
| 复用会重算已复用前缀吗 | **否** | `transactions/materialization.cpp:343-344`：`.base = .cursor = (Root ? 0 : reuse_base)`；`prefill.cpp:1146-1148` 从 `staged.cursor` 起分块；保留路径（`prefill.cpp:518-561`）**就地保留**源 KV；body 页为字节拷贝（`copy_page`） |
| body 会经另一条 kernel 重物化吗 | **否** | 后缀走 `execution::prefill_text_chunk`（`prefill.cpp:1208-1215`），与新鲜同路；`rotate_kv`/`stage_exact_tail` 在 `ops/kvarn/attention.cu:614-617` 只作用于本步新 token |
| `rope_delta` 是同 prompt 的差异源吗 | **否** | 纯文本 prompt 恒 0（`frontend/frontend.cpp:341`），复用路径同样取 0（`prefill.cpp:672`）；消费点均为加法偏移 |

**⇒ §4.2 提出的「`rebuild_work` 驱动的 body 再物化」假说被否证。**

**候选收窄（**未证**）**：
1. **路线 / 分组差异**：attention 路由按**发射宽度**选 —— 宽度 ≥ `kCausalPromptBr=64`（`ops/softmax_attention/dense/causal_cache/prompt_common.cuh:21`）走 `launch_prefill`（16384-token slab），否则走 `launch_partial`（`ops/kvarn/decode.cu:146,178-191`）；且 **body/tail 分区由「发射窗口」决定**（`decode_kernel.cuh:39-60`、`small_t.cu:116-127`）⇒ **856-token 的新鲜 prefill 与 5-token 的续填不是同一条路径**。
2. **KVarN 侧确有再导出**：`kvarn_restore_tail`（`ops/kvarn/attention.cu:675-686`）含 `settle_encode_kernel`（把环页**重新编码**回 packed body 记录）与 `restore_tail_kernel`（把 frontier 页的 body 记录**去量化**进环槽）—— **每次非 Root 复用都会跑**；但它是 **KVarN-gated**（`context.cpp:1733`）⇒ **不能解释 bf16 臂**。

**仍未解的不一致**：候选 1 与「`tail ≥ prompt` 逐位一致」尚未调和（该情形下两条路径的宽度差异同样存在）。**定案仍需 logits 级或环内容级仪器。**

**候选 1 的可测预言与一条实测边界（2026-10-08 补）**：路由判据是 `query.ne[3] == 1 && query.ne[2] >= kCausalPromptBr`（`ops/kvarn/decode.cu:146`，`kCausalPromptBr = 64`，`prompt_common.cuh:21`）⇒ **预言**：若把**续填后缀做到 ≥ 64 token**（使续填也走 `launch_prefill`），分歧应消失；反之把新鲜 prefill 做到 < 64 也应消失。
**⚠ 实测边界**：**`--prefill-chunk` 无法用来构造后一种** —— 它被强制为 **128 的倍数**（`serve/serve_options.cpp:1201`），最小 128 ≥ 64，故任何前向分块仍走 `launch_prefill`。⇒ 只能从"加长续填后缀"一侧构造（例如用 `[user, assistant(部分内容 ≥64 token)]` 的多消息 prompt，使 turn-closure 检查点之后的后缀足够长）。

### 4.4 剂量-反应探针（2026-10-08）：**构造受阻、剂量未变 ⇒ 无判定力**

**设计**：把**续填后缀**做到 ≥ 64 token（使续填也走 `launch_prefill`），预期分歧消失。
**做法**：`.deps/kvarn-adm/wp65_prefix_len_probe.sh` —— 用带 `assistant` 尾的 prompt，希望 turn-closure 检查点落在该 assistant 消息**之前**，从而后缀 = assistant 内容长度。

**结果（kvarn / bf16 各 3 档，assistant 重复 0 / 2 / 40 次）**：

| 档 | prompt | cached | **后缀** | r1==r2 |
|---|---|---|---|---|
| s0 | 856 | 851 | **5** | 否 |
| s2 | 883 | 878 | **5** | 否 |
| s40 | 1225 | 1220 | **5** | 否 |

**⇒ 后缀恒为 5，剂量未变，探针对假说无判定力。** 另：复用类型由 `turn closure` 变为 `response replay`（检查点仍取 `generation_begin`，**不是** assistant 消息起点）⇒ 构造假设错误。
**附带数据点（真实）**：`response replay` 复用的 **6/6 全部失配** ⇒ 失配**不限于** `turn closure`。

**为何 serve 层做不出这个剂量（实测边界，记下以免重走）**：
- chat-completions 的缓存会话键**只能派生** —— `generation_service.cpp:443-445` 是唯一赋值点；`http_transport.cpp:119/131` 的 `session_id` 属 **ngram 原生会话**且被 `--ngram-native-sessions` 门控（`http_transport.cpp:104`），**不是**通用缓存键。
- 检查点偏移取 `generation_begin`（`chat_template.cpp:442-445`）⇒ 单条 user 消息时后缀 = 模板尾（≈5 token），**无法由请求侧拉长**。
⇒ 要在 serve 层做这个剂量**必须改代码**（增客户端会话键，或改检查点选取）。

**剩下能定案的只有两条**：(a) **op 级差分测试** —— 同一份 cache 上，把最后 5 行分别按「宽度 856 的 slab 调用」与「宽度 5 的 split 调用」各算一次并逐位比对（新增测试代码，约 1–1.5 h）；(b) logits 级仪器。

### 4.5 真实 agent 多轮形状的影响实测（2026-10-09）：**分歧不限于精确尾**

**动因**：用户问「一轮输出 → 工具结果（或长或短）→ 下一轮」这一典型 agent 场景的实际影响。§1–§4.4 全部只测了
**「同一请求重复两次」**（复用后缀 = 模板尾 5 token）这一子形状，**从未测过真实多轮形状**。

**设计（`.deps/kvarn-adm/wp66b_agent_hit_vs_cold.sh`）**：**同配置、同 turn-2 请求体，只改「有无可命中的检查点」**——
- 冷臂 C：turn-2 请求体是**该服务器第一个请求** ⇒ `cache 0`（Root，真冷）；
- 命中臂 H：先发 turn-1（`cache 0`，落在 851），再发**同一个** turn-2 请求体 ⇒ `cache 851`（命中）。
请求体 = `[user P(=§1 同款 856 token), assistant A1(turn-1 输出), user U]`，`U` 取两档（工具结果短 ≈223 token / 长 ≈1023 token）。

| 形状 | KV / 尾（`kvarn:k4v4` 除非注明） | 冷(cache 0) vs 命中(851) | 判定 |
|---|---|---|---|
| 重复同请求，后缀 5 | `tail=0` | — | **PASS**（§1 既有记录） |
| 重复同请求，后缀 5 | `tail=256` | 首差 `@27`(bf16)/`@207`(kvarn) | **FAIL**（§1 既有） |
| **agent 第 2 轮**，工具结果短（后缀 223） | `tail=256` | 980 vs 949，首差 **`@75`** | **FAIL** |
| **agent 第 2 轮**，工具结果长（后缀 1023） | `tail=256` | 1004 vs 957，首差 **`@49`** | **FAIL** |
| **agent 第 2 轮**，工具结果短 | **`tail=0`** | 900 vs 941，首差 **`@485`** | **FAIL** |
| **agent 第 2 轮**，工具结果长 | **`tail=0`** | 925 vs 1022，首差 **`@81`** | **FAIL** |
| **agent 第 2 轮**，工具结果短 | **`bf16` + `tail=0`** | 948 vs 949，首差 **`@442`** | **FAIL** |
| **agent 第 2 轮**，工具结果长 | **`bf16` + `tail=0`** | 993 vs 822，首差 **`@81`** | **FAIL** |

**控制臂（排除混淆，"冷/命中"两侧必须只差检查点）**：
1. **同配置 fresh 可跨实例复现**：turn-1（`cache 0`）在 4 个服务器实例 × 2 个脚本中均为 **917 字节、逐位相同** ⇒ 新鲜路径稳定。
2. **warm-no-hit 控制**（`.deps/kvarn-adm/wp66c_history_control.sh`）：先发一条**不可能匹配**的不同 prompt（不同首条 user ⇒ 派生键不同），再发同一个 turn-2 体（日志确认 `cache 0`）⇒ 与真冷臂**逐位相同**（`tail=256`: 980/980；`tail=0`: 900/900）⇒ **排除"服务器有历史请求 / 分配历史"**，分歧**确由检查点命中造成**。
3. **`--no-prefix-reuse` 非输出中性**（独立发现）：同一条 turn-1 请求，`--no-prefix-reuse` 的 fresh = **928** 字节，reuse-on 的 fresh = **917**，首差 `@652`。⇒ 该开关在**尾启用**时**会改变新鲜输出**，因此"开缓存 vs 关缓存"的直接 A/B **不是干净的对照**（本文 §4.5 的设计正是为绕开它）。

**结论**：
1. **agent 形状（长后缀）的命中 ⇒ 与冷启动输出不同 —— 且与精确尾无关（`tail=0` 同样分歧）、与 KV dtype 无关（`bf16` 同样分歧）。**
   即：本仓的**前缀复用对"多 chunk prompt"本来就不与新鲜 prefill 逐位一致**，这是**先存性质**，不是 WP6 引入的。
2. 精确尾的增量是**把分歧提前到更小的后缀**：后缀仅 5 token 时 `tail=0` 一致、`tail>0` 不一致 ⇒ **§1 的判定表只描述了「重复同请求」这一子形状，不是全貌**。
3. 与既有文档的关系：`docs/ngram.md:216-219` 把 fresh-vs-cached 不逐位只记为「**量化**路径、且只是可选诊断 `--strict-fresh`（"may return 2"）」；本次实测显示 **bf16 非量化路径在 agent 形状同样不逐位** ⇒ 该文档的口径**比实测窄**。
4. **性质**：两条路径**各自确定、可复现**（非抖动）；实测分歧是**同一意思的不同措辞**（例：`user sent long repeated sentence then "Request number one." We responded?` vs `long repeated pangram then Request number one. Now...`），随后分叉，**非乱码**；无显存/性能影响。**质量（KLD / 准确率）差异未测**。
5. **对 ④ 口径的含义**：④ 只在「尾启用 且 命中」时打标；实测表明**尾关闭 + 命中（agent 形状）也不逐位** ⇒ **④ 覆盖不全**。是否把口径改为「命中即披露」是**用户决策项**（代价：正常复用路径也会被打标）。

**未测（本轮显式留下）**：多轮传播（第 2 轮之后分歧是否一直携带）；工具调用**参数级**翻转（同一近平分机制的另一后果）；
质量（KLD / 准确率）的命中 vs 冷对比；上游 beellama 在"长后缀命中"形状下的逐位性（§7 未跑项的扩展）。

## 5. 方案取舍（三档，用户已裁决取第 ③ 条 ④）

| 档 | 内容 | 代价 | 收益 | 结论 |
|---|---|---|---|---|
| ① | 抄上游 ④：**降级披露**（标记"复用且尾环启用 ⇒ 未验证逐位"） | 小 | 把**静默**变**可见**；补上上游唯一在守的契约 | **✅ 采（本文 §6）** |
| ② | 抄上游 ③：分区改到**主机侧按序列/位置** | 中（3–6 天；动三处共用分区、op 接口、CUDA Graph 键、host 镜像测试） | 直击被怀疑的根因 | **未采**：根因未定，先改架构可能修不到病灶 |
| ③ | 抄上游 ①+②：尾与 body **同池同写同事务** | 大（数周，等于推翻 WP6 架构决定；可行性报告当初正为绕开"页精度维度"才选独立池） | 不变量最强 | **不采**：单机单用户项目，收益不抵代价 |

**用户裁决（2026-10-08）**：只做 ①（保守触发 + **仅运维日志**出口）。
**保守口径** = 只要「尾环启用 且 前缀来自检查点」即披露（**不**按 `0 < tail < prompt` 收窄）。
⇒ 已知代价：`tail ≥ prompt`（实测其实逐位一致）与"尾对该 storage 惰性"两类也会打标，**是有意接受的过报**。

### 5.1 用户裁决（2026-10-09）：**接受该现象**

裁决表述（原文要点）：「**如果只是贪婪解码时会产生翻转、不影响实际任务表现、且尾部精度能提供确实的收益，那么这还是可以接受的**」。

**逐条核对（三分之二的证据状态）**：

| 前提 | 状态 | 依据 |
|---|---|---|
| 「只是贪婪解码时」 | **不准确** | 机制是 **logits 上的舍入扰动**，采样**不会消除**它（近平分处照样本可被推动），只是让它在多数位置与采样噪声难以区分。**准确表述**：贪婪是唯一把它变成**永久、确定、可复现**的场合 ⇒ **实际暴露主要集中在贪婪 + 对"缓存透明/可复现"有诉求的场景**。全部实测都是 `--greedy`；**温度 > 0 未测**。 |
| 「不影响实际任务表现」 | **未测** | 无质量对比（命中 vs 冷启动的 KLD/准确率**均未测**）；同一种近平分机制**完全可以翻掉工具名/工具参数**。实测样本显示为"同一意思的不同措辞"（§4.5 结论 4），但**样本不足以外推**。⇒ 这是本裁决**唯一的实质风险项**。 |
| 「尾部精度有确实收益」 | **有据** | 尾路径的数值代价已压到与量化档同级（`docs/performance.md`：bf16 tail 1024 post-fix KLD `9.4e-4` vs rk8v4 `1.1e-3`，tail-off 为 0）；纯 kernel 代价 **−1.5%**（WP6.0-0b）。 |

**接受的范围（记录清楚，避免日后误读）**：接受的**不只是**"尾相关的那一类"，而是 **§4.5 实测的更大事实 —— 本仓前缀复用对多 chunk prompt 不与新鲜 prefill 逐位**（含 `tail=0` 与 `bf16`，属**先存引擎性质**，非 WP6 引入）。
**接受的代价**：**缓存透明性 / 跨缓存状态的可复现性**丢失（同一请求在"命中"与"冷启动"下输出不同；每条路径各自确定可复现）。
**不改变**：④ 维持现状（触发 = 尾启用 且 命中），**覆盖缺口（尾关闭 + 命中不披露）记为"已知并接受"，不再作为缺陷跟踪**；根因诊断线（§7.1）仍挂起、不再阻塞交付。
**若日后要收紧**：唯一能把前提 2 从"未测"变成"有据"的是**任务级**质量对比（同一条 agent 轨迹在命中/冷两种状态下跑一组真实任务、比结果正确性），**未做**。

## 6. ④ 实施（A5 改动清单 / A7 实现 / A8 验证）

### 数据通路（侦查结论）
日志渲染读 `outcome.metrics`（`src/serve/operational_log.cpp:286`），`GenerationMetrics` 是 **serve 内部类型**
（`src/serve/generation_service.h:24`）；`GenerationService` 持有 `options_` 并暴露 `options()`
（`generation_service.h:135`），`ServeOptions::kv_tail_tokens`（`serve_options.h:71`）在 `run()` 内可用。
⇒ **不需要动公开头文件 `include/ninfer/types.h`，不需要新增数据通路。**
（已排除的陷阱：`sequence.tail_hidden_valid` 是 **MTP draft 的尾**，与精确尾环同名无关，`program_impl.h:466`。）

### 改动清单

| # | 位置 | 改动 | 可行性 | 风险 | 回退 |
|---|---|---|---|---|---|
| 1 | `src/serve/generation_service.h`（`GenerationMetrics`） | 加 `bool exact_tail_reuse_unverified = false;` | 可行 | 无（纯新增字段，serve 内部） | 删字段 |
| 2 | `src/serve/generation_service.cpp`（`GenerationService::run`，~:573） | `= result.prefix_reuse_path != ninfer::PrefixReusePath::Root && options().kv_tail_tokens > 0;` | 可行 | 低 | 删赋值 |
| 3 | `src/serve/operational_log.cpp`（~:286） | 复用路径段后追加 `", exact-tail reuse unverified"` | 可行 | 低（仅日志文本） | 删该行 |

**不做**：API 响应字段（属对外协议契约）、公开枚举 `PrefixReusePath` 扩项（Tier A 共享契约，代价大且语义正交）、
独立统计计数（本轮不需要）。

### A6 状态迁移矩阵（实施前）

| 状态 | 观测面：日志出现披露 | 观测面：既有行为不变 |
|---|---|---|
| `tail=0` + 复用 | 否（`kv_tail_tokens==0`） | 需 e2e 复核 |
| `tail>0` + 复用（`!=Root`） | **是** | 需 e2e 复核 |
| `tail>0` + 无复用（`Root`） | 否 | 需 e2e 复核 |
| `tail>0` + 复用 + 非流式/流式 | 是（两条路径共用渲染） | — |
| 拒绝/失败请求 | 不涉及（无 metrics） | — |

### A8 验证
- 构建目标：`ninfer_tests`、`ninfer-serve`（按目标，禁全树）。
- e2e：`.deps/kvarn-adm/wp64_prefix_reuse_bodytail.sh`（`TAIL=256`）⇒ 日志应出现 `exact-tail reuse unverified`
  且 `cached_tokens=851`、r1/r2 关系与实施前一致（FAIL 与否**不因本改动改变**）。
- 回归：`.deps/kvarn-adm/p1_prefix_reuse.sh`（无尾）⇒ 日志**不应**出现该段。

## 7. 未决项（本文开放）

1. **根因未定**。**已否证的假说**：⓪「环残留」（§4.2 判决实验）；①「`rebuild_work` 驱动的 body 再物化」（§4.3 静态取证 —— 纯成本账目、无执行读者、复用前缀不重算）。
   **现存候选（未证）**：② **路线 / 分组差异** —— 路由按**发射宽度**在 `launch_prefill` / `launch_partial` 间切换，且 body/tail 分区由**发射窗口**决定（§4.3 候选 1）；
   ③ **KVarN 的 `kvarn_restore_tail` 再导出**（`settle_encode` 重编码 + `restore_tail_kernel` 去量化，每非 Root 复用都跑；**KVarN-gated，不能解释 bf16 臂**）。
   **两者都未能调和「`tail ≥ prompt` 逐位一致」** ⇒ **定案必须上 logits 级或环内容级仪器**。
2. **（2026-10-08 查实，见 §4.1/§4.2）** 尾环不随**任何** storage 的检查点保存/恢复（架构缺口，**但已否证为本失配之因** —— 判决实验：新鲜与复用答案都对环的前置内容完全不敏感）。
3. **未跑上游实测对照**（`D:/ninfer/llamacpp` 二进制 + 同族 GGUF，`tail=0 / ≥prompt / 0<tail<prompt` 三档），估 2.5–4.5 h。
4. **未做**：`--kv-tail-type` 惰性档（fp8/nvfp4/k8v4）的过报确认；`tail ≥ prompt` 过报的实测确认。
5. **（2026-10-09 新增，见 §4.5）** 「命中即不逐位」在 **agent 多轮形状**下与**尾、dtype 均无关**；**④ 的触发口径覆盖不全**（尾关闭 + 命中不披露）。**用户已于 2026-10-09 裁决「接受」（见 §5.1）** ⇒ ④ 维持现状，覆盖缺口记为**已知并接受**；**遗留的实质风险项 = 前提 2「不影响实际任务表现」未测**（任务级质量对比未做）。
6. **（2026-10-09 新增，见 §4.5 控制臂 3）** `--no-prefix-reuse` 在**尾启用**时**非输出中性**（fresh 928 vs 917，首差 `@652`）—— 独立于本缺陷，但**使"开/关缓存"的朴素 A/B 失效**，需另记（是否与本仓"发射窗口决定分区"同族，未查）。
7. **（2026-10-09 新增，见 §4.5）** 多轮传播、工具调用**参数级**翻转、质量（KLD/准确率）对比**均未测**。
8. **（2026-10-09 新增，WP6.7，进度 §3-10-09-05）** **`kvarn` + 活跃尾 + 小 `body_window` 的话题级翻转**：短 prompt（body ≲ 23）或「整窗在尾」（`N ≥ prompt`、body = 0）下，`kvarn:*` + `--kv-tail-tokens>0` 的贪婪输出**话题级离群**（幻觉出未出现的 user 语句；视觉题答成无关内容），而 **`bf16`+尾 与 `kvarn`+尾=0 在测过的 prompt 上逐字节相同**（唯一离群者是 `kvarn`+活跃尾）。**归因未定**：或为本文已知的「尾改变数值 ⇒ 贪婪翻转」的放大，或为小 `body_window` 下的分区/接线缺陷（线索：WP6.4 记录的 `neutral=48` 最小分割算术）。**WP6.6 的满窗（8192）KLD 协议不覆盖本区**；logit 级差分测试可判（同本 §1）。

## 8. 谁引用本文（索引）

- `kvarn-port-progress.md` §0（WP6–WP9 行）、§3-2026-10-08-22、§3-2026-10-08-23、**§3-2026-10-09-05（WP6.7：小 `body_window` 翻转）**。
- `kvarn-port-into-precision-tail-plan.md` §7-WP6.4 行、WP6.4 状态迁移矩阵「跨 checkpoint 恢复」格。
