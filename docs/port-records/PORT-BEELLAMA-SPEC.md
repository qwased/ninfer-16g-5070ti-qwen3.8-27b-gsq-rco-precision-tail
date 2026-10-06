# PORT-BEELLAMA-SPEC — verified KVCPT algorithm reference

Source: `D:\ninfer\beellama.cpp` rev `main @ 58a162927`, read-only. **Port the algorithm, never the code.**
This document is the reference for WP2 (write), WP3 (merge), WP4 (graph key), WP5 (accounting).

> Read-only memory doc. Update only with newly verified findings + line evidence.

## A. Merge math (WP3 — authoritative)

Pattern is **"compute two partials, then merge online-softmax statistics"** — no re-scoring.

- **Body partial** = an ordinary FA pass over quantized K/V. When enabled it exports per-row
  statistics `dst_meta[row] = float2(max, sum)` (`ggml/src/ggml-cuda/fattn-common.cuh:1346,1463`;
  KVarN path `fattn-kvarn-portable.cuh:785`); its `dst->data` is the **normalized** output `O_b`.
- **Tail partial** = exact F16/BF16 K/V, either a fused small kernel or a second FA pass producing
  `(m_t, l_t)` + normalized `O_t` (`ggml/src/ggml-cuda/fattn-tail.cuh:844-877`).

Generic merge (`k_flash_attn_ext_tail_partials_merge`, `fattn-tail.cuh:363-388`):
```
g  = max(m_b, m_t)                                   // 367
wb = l_b * exp(m_b - g)                              // 368
wt = l_t * exp(m_t - g)                              // 369
out = (O_b*wb + O_t*wt) / (wb + wt)                  // 370, 387
```
Unnormalized form (fused kernel `k_flash_attn_ext_tail_indexed_small`, `:307-335`) is identical:
```
out = (num_b*e^{m_b-g} + num_t*e^{m_t-g}) / (l_b*e^{m_b-g} + l_t*e^{m_t-g})
```

**Domain alignment (answers risk R1).** Q is F32. Body K/V are dequantized by the body FA into
**FP32 accumulators**; tail K/V are dequantized per element (`tail_value_to_float<half|nv_bfloat16>`,
`:169-177`). `dst` is F32 (`GGML_PREC_F32`, `src/llama-graph.cpp:3155`). The QK scale `s_q·s_k` is the
FA `scale` op-param, applied per pass (`dot*scale`, `:258`) — so both partials are dimensionally
identical before the merge. ALiBi slope added identically (`slope*mask`, `:262`).
→ The body's quantized domain is converted to FP32 inside the body FA; the merge is FP32↔FP32.
Our port must likewise merge in a common FP32 domain (body partial output is already dequantized+normalized).

**Boundaries.** Guards `bv = bm.y>0 && finite`, `tv = tail_sum>0 && finite` (`:308-309,365-366`).
Tail empty → `denom=wb`, output = body output. Body empty / `tail_bodyless` → `body_value=0,
bm=(0,0)`, `bv=false` → pure tail, and sinks are handed to the tail pass itself
(`tail_pass.src[4] = tail_bodyless ? dst->src[4] : nullptr`, `:854-856`). When the tail covers
everything (`p+1<=N`) the body partial still runs but its mask is −∞ so it contributes ~0 — it is
**not** structurally skipped. Non-window body rows are set to −∞ and copied out of the body mask
into the exact mask (`src/llama-kv-cache.cpp:7347-7348`).

**Masking / GQA.** Causal is enforced by the mask plus a host selection filter
(`position > query_position` → skip, `src/llama-kv-cache-tail.cpp:481`). GQA:
`ih_k = ih/(n_head/n_head_k)`, `ih_v = ih/(n_head/n_head_v)` (`fattn-tail.cuh:228-229`).

## B. Tail window / indexing (WP3 — **correction to plan §1.4**)

**The window is selected on the HOST, not computed on device from positions.** Beellama walks entries
newest-first, skipping `!finite[i] || (causal && position > query_position)`, retaining up to
`retention=N` (`src/llama-kv-cache-tail.cpp:480-486`). The device reads a flat descriptor:
`desc[4]=n_tail`, `desc[6+it]=slot` (`llama-kv-cache.cpp:7521-7526`); `desc[5]=n_body`,
`desc[body_map_offset+i]=flat` (`:7573-7577`).

Tail page addressing: `from_current = slot >= history_slots`;
`row = from_current ? slot-history_slots : slot`; source = `kt_current/vt_current` vs `kt/vt`
(`fattn-tail.cuh:28-34,240-243`). Slots `< history_slots` index the persistent exact pool;
`>= history_slots` index the current graph-local ubatch source (virtual `n_slots+row`,
`llama-kv-cache-tail.cpp:799-804`). Visibility (window+causal) rides in the `mt` mask built by
`set_input_kq_mask_tail_impl` (`llama-kv-cache.cpp:7227-7367`). `attention_stride` is a graph-local
padded extent (`:6959-7000`) and never inflates persistent storage.

→ **Our plan §1.4 said "不能回读主机" (device-side window from positions). That is a divergence.**
Reconsider: a host-built descriptor (n_tail + slot list per query row) is the upstream approach and is
compatible with our plan's "views passed by value + runtime inputs" idea. Decide in WP3 design.

## C. Write path (WP2)

**Fused single-read dual write.** `ggml_set_rows_with_shadow` (`ggml/src/ggml.c:4049-4073`) emits ONE
`GGML_OP_SET_ROWS` node with `src[2]=body (quantized)`, `src[3]=shadow (F16/BF16)`,
`src[4]=shadow_indices`. Compute writes body via `from_float(src0_row)` **and** shadow via
`from_float(src0_row)` from the *same F32* row (`ggml/src/ggml-cpu/ops.cpp:5402-5426`) — the quantized
body is never re-read. Created in `cpy_k_with_tail` / `cpy_v_with_tail`
(`src/llama-kv-cache.cpp:3258,3290`). A compact path uses separate `cpy_k` + `cpy_k_tail`
(`ggml_set_rows_ordered`, `:3400-3457`).

**`keep_last_writes` dedup** (`src/llama-kv-cache-tail.cpp:12-24`): iterate rows descending, levels
descending; `written.insert(slot)` — the global-last occurrence keeps the slot, later duplicates set
to `-1` (SET_ROWS requires unique destinations). Applied for compact storage in `set_input_tail_idxs`
(`src/llama-kv-cache.cpp:3629-3635`).

**Commit timing.** Persistent slot assignment (`tail->commit`) happens in `apply_ubatch` **before** the
graph (`:2900-2906`); the payload write executes **after attention** ("The tail is updated after
attention", `:3630-3632`); the current ubatch's exact rows are fed to attention as a separate
graph-local source (`k_tail_current`, `fattn-tail.cuh:496`). Matches our plan §3 WP8.

**Rollback/reserve.** `begin_batch()` snapshots `sequences/slot_used/write_cursors/degradation/
recovery_commits` (`llama-kv-cache-tail.cpp:596-615`); `finish_batch(success,…)` restores on failure
and, if payload may be modified, releases affected slots and sets
`LLAMA_KV_TAIL_DEGRADED_PAYLOAD_INVALID` (`:617-649`). `history_stride = N + R` (`:103`); suffix
rollback only when `n <= rollback_tokens` (`:1152-1154`).

## D. Graph / lifecycle (WP4 — **correction to plan §1.5/WP4**)

Tail identity = `llm_graph_kv_tail_identity` (`src/llama-graph.h:326-338`), matched in `matches()`
(`src/llama-graph.cpp:675-712`): `storage_kind, exact_type, retention_tokens, rollback_tokens,
arena_stride, storage_slots, compact`, plus per-layer `has_body/has_current/body_execution_rows/
routes/explicit_bias`. `retention_tokens = get_tail_tokens() = tail_plan.effective_tokens`
(`src/llama-kv-cache.h:235-237`).

→ **`retention_tokens` (i.e. N) IS in the graph key.** Our plan WP4 said "长度不进 key" — that is
**wrong** vs upstream. What is excluded is the *dynamic per-query window*: `desc[4]` (n_tail), the slot
table, and the masks are runtime **inputs**, validated only by shape via
`attention_stride`/`desc_stride` (`src/llama-graph.cpp:773-801`). So one cached graph serves every
window occupancy, while a different N is a different graph. Since our arena is static per session and
N is startup-fixed, putting N in the key costs nothing.

The exact pool is **static per sequence**: shadow tensors allocated once as `[ne0, tail_slots]`
(`src/llama-kv-cache.cpp:1143,1153`); fixed per-sequence arena via `arena_stride`.

## E. Memory accounting (WP5)

Plan-time (`src/llama-kv-cache-tail.cpp:350-358`): `promotion_increment`, `overlay_increment`,
`compact_history_bytes = history_slots * overlay_bytes_per_row`,
`compact_rollback_bytes = rollback_tokens * n_seq_max * overlay_bytes_per_row`, `shadow_bytes`.
Runtime (`src/llama-kv-cache.cpp:2138-2161`):
```
rollback_bytes  = row_bytes * rollback_slots
history_bytes   = row_bytes * (slots - rollback_slots)
transient_bytes = row_bytes * (attention_stride - effective_tokens)
```
Reported as `native_exact_bytes` / `exact_tail_bytes` / `rollback_reserve_bytes` /
`transient_estimate_bytes`. Doc reconciliation: q5_0/1024 → 800 MiB native-exact SWA + 80 MiB global
overlay + 0.86 MiB rollback reserve (`docs/beellama-features.md:570-574`).
→ Our plan's `kv_exact_history_bytes` / `kv_rollback_reserve_bytes` names align with this split.

## Ambiguities flagged by the reader (do not invent)
1. The `[max(0,p+1-N), p]` window is **host-side selection**, not a device formula from positions.
2. No separate graph-key beyond `llm_graph_kv_tail_identity`; the plan's pointer
   `src/llama-kv-cache.cpp:2109-2182` is actually `kv_memory_stats`, not a key.
