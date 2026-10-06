# PORT-E8-INVESTIGATION — rk4v4-e8 比 rk4v4 严重劣化的根因调查

- 日期：2026-10-06
- 对象：`D:\ninfer\ninfer-precision-tail`（`main`，HEAD `11a9ab2c`）
- 触发：移植验收（`PORT-VERIFY-REPORT.zh.md` Arm B）显示「本应是 rk4v4 上位的 rk4v4-e8 比 rk4v4 劣化许多」
- 性质：**只出结论：不改任何代码、不改任何既有报告。** 本文件是本次调查的唯一新增产物
- 结论证据：`.deps/verify-*/report.json`（既有 campaign 原始产物）+ `D:\ninfer\.e8-investigation\`（本次新增的复刻脚本与参考实现快照）

> 纪律：下面每个数字都能由 §7 的一条命令复现。**本次没有跑 GPU**：真机数字取自既有的验收产物，
> 新增的是对编解码器的离线复算。

---

## 0. 结论一览

| 论点 | 结果 |
|---|---|
| **C1** rk4v4-e8 劣化的直接原因 | **成立** — key 编码器把 E8 吸附的结果 rint 进 int4，而 int4 装不下 E8 的半整数陪集；该步骤在 47.6% 的 8 维块上把"陪集更近"的优势整个丢掉，**落点比直接四舍五入更差** |
| **C2** 量化幅度 | 仿真 MSE **3.05×** rk4v4；真机 `mean_target_logprob_delta` **2.94×**、`KLD max` **2.17×**、`mean KLD` **1.69×** |
| **C3** 是否移植缺陷 | **否** — 参考实现 `TertiumOrganum1/ninfer-3090 a7fb3033` 的该段代码与本仓库**逐字相同**；Arm A 的 8 个 rk4v4-e8 格移植前后 **8/8 逐字节相同** |
| **C4** 修好 coset 能否反超 rk4v4 | **不能** — 上限 MSE≈0.0099（rk4v4 的 **1.11×**）。4 bit/维下 E8 的点阵增益（0.58 dB）小于 Lloyd-Max 相对均匀量化的整形增益（1.15 dB） |
| **C5** 「收益随 body 变粗单调」 | **是误读** — 该判据把 C1 的缺陷编码成了特性。e8 的 tail0 KLD 更大只是因为它的 **body 本身更差**，于是尾巴"修得更多" |
| **C6** 缺陷为何长期静默 | 没有任何测试把 e8 的质量与 rk4v4 或同尺度纯 int4 对照；既有断言只覆盖"两条分支都跑到"和宽松的 FP32 容差 |

---

## 1. 被调查的对象

| 项 | 取值 |
|---|---|
| 存储枚举 | `include/ninfer/types.h:65-68` `RotatedInt4KeyInt4ValueE8`（CLI `--kv-dtype rk4v4-e8`） |
| 姊妹存储 | `include/ninfer/types.h:61-64` `RotatedLloyd4KeyInt4Value`（`rk4v4`，本 fork 的 4-bit 默认，commit `0b4f9588`） |
| 编码器 | `src/ops/kv_cache/int8_g64_codec.cuh:220-227` `kv_cache_int4_e8_key_code`，由 `:442-458` 的 `KvKeyCoding::Int4E8` 分支调用 |
| 解码器 | `src/ops/kv_cache/int8_g64_codec.cuh:232-244` `kv_cache_int4_unpack_i8x16`（**普通 int4**，与 E8 无关） |
| 平面几何 | 与 rk4v4 **完全相同**（同一个 profile：U8/U8 键码、G64 FP16 键 scale、rk8v4 的 G32 int4 值平面）。因此两者的差异**只在 key 编码**，值平面误差是共同的底 |
| 引入 | `eb9f7a23`（2026-09-24，TertiumOrganum1）："adapted from TertiumOrganum1/ninfer-3090 a7fb3033" |
| 本文档的权威来源 | 参考实现快照 `D:\ninfer\.e8-investigation\ref_int8_g64_codec.cuh`（`raw.githubusercontent.com/TertiumOrganum1/ninfer-3090/a7fb3033/src/ops/kv_cache/int8_g64_codec.cuh`） |

---

## 2. 机理

E8 = D8 ∪ (D8+½)。编码器先求最近 E8 点，再把它塞进 int4：

```cpp
// src/ops/kv_cache/int8_g64_codec.cuh:220-227
const float scaled = inv_scale == 0.0f ? 0.0f : __fmul_rn(x, inv_scale);
const float point  = kv_cache_e8_nearest(scaled, lane);   // D8 或 D8+1/2，取更近者
int q              = __float2int_rn(point);               // ← 半整数被抹掉
q                  = max(kKVCacheInt4KeyMin, min(kKVCacheInt4Max, q));
```

两个分支都受损：

1. **陪集分支（实测 47.6% 的 8 维块）**：`point` 的 8 个分量都是 k+0.5，`__float2int_rn` 让每个分量移动
   0.5（8 维合计 √2，缩放域平方误差 +2）。当初正是因为陪集明显更近才选中它，rint 之后这个优势**整个消失**，
   落点甚至差于"直接对 y 取整"。
2. **整数分支（其中 23.6% 的块 `rint(y)` 奇偶性为奇）**：D8 的偶校验约束强制把最大舍入误差的那一维 ±1。

代码注释与文档都如实写着这一点，说明这是**设计如此**，而非笔误：

- `src/ops/kv_cache/int8_g64_codec.cuh:163-165` —"…the stored code is then that point rounded to an
  integer and clamped to [-8, 7]. **No coset bit is kept**, so a coset point loses its half on the way
  into the code. The read side is therefore exactly the plain packed int4 decode."
- `docs/maintainer/consolidated-line.md:48` —"…rounded per octet to the nearest E8 point, stored as
  int4 nibbles (**the coset bit is not stored**)"
- 对照 `docs/maintainer/consolidated-line.md:67`：姊妹格式 `rk2v4-e8` 存 root + log-radius + 残差轴，
  读侧**能**重建真正的格点，所以没有这个问题。

---

## 3. 量化证据

### 3.1 离线复刻（G64 分组、i.i.d. 高斯 ≈ Hadamard 旋转后的键统计）

复刻脚本 `D:\ninfer\.e8-investigation\sim_codecs.py` / `sim_mechanism.py`，语义逐条对齐仓库自带的
host oracle `tests/ops/test_kv_cache_append.cpp:575-626 encode_full_row_e8_keys`（absmax/7 的 FP16-RNE
scale、逐 8 维块 D8/陪集取近、`round_even` 后 clamp [-8,7]）。

| codec | MSE | × rk4v4 | rel-L2 |
|---|---:|---:|---:|
| `rk4v4` Lloyd-Max + LS scale（现实现） | 0.008893 | 1.00 | 0.0937 |
| **`rk4v4-e8` E8 吸附 + rint（现实现）** | **0.027087** | **3.05** | 0.1617 |
| 纯 int4 absmax/7 + rint（对照） | 0.011597 | 1.30 | 0.1066 |
| E8 但保留 coset（int4 装不下，仅上界） | 0.010138 | 1.14 | 0.0997 |

**误差分解**（缩放域，每 8 维块平方误差均值）：

| 分支 | E8+rint | 直接 rint | 占比 |
|---|---:|---:|---:|
| 全部块 | 1.5437 | 0.6564 | — |
| 陪集分支 | **2.6152** | 0.8218 | 47.59% 的块 |
| 整数分支 | 0.5709 | 0.5063 | 52.41% 的块 |

E8 步骤引入的额外 MSE（0.1254）= 陪集被 rint 的 0.1206（**96.2%**）+ 奇偶修正的 0.0048。
另：被 `[-8,7]` 截断的码仅 0.26%，即劣化不来自截断。

**自我校验（证明复刻可信）**：我的 E8 复刻相对纯 int4 好 0.58 dB，与 E8 理论空间填充增益 0.65 dB 吻合。

### 3.2 真机既有产物（`.deps/verify-*-t0-c{8192,32768}/.../report.json`）

| ctx | 存储 | ppl | mean KLD | KLD max | mean_target_logprob_delta | same_top |
|---|---|---:|---:|---:|---:|---:|
| 8192 | rk8v4 | 5.980890 | 0.00264652 | 0.48189 | −0.00126096 | 0.976010 |
| 8192 | rk4v4 | 5.986203 | 0.00386914 | 1.56008 | −0.00214894 | 0.971463 |
| 8192 | **rk4v4-e8** | 6.011151 | **0.00652166** | **3.38330** | **−0.00630781** | 0.965969 |
| 32768 | rk4v4 | 5.682110 | 0.00356046 | 1.56008 | −0.00303985 | 0.972686 |
| 32768 | **rk4v4-e8** | 5.694664 | **0.00631572** | 0.65803 | **−0.00524675** | 0.963378 |

比值：`mean KLD` **1.69×/1.77×**、`KLD max` **2.17×**、`mean_target_logprob_delta` **2.94×/1.73×**、
`same_top` −0.55/−0.93 点。其中 `mean_target_logprob_delta`（近似线性于误差）的 2.94× **与仿真的
3.05× MSE 比吻合**；KLD 因饱和而偏小，方向与量级一致。

注：rk4v4 与 rk4v4-e8 的**值平面完全相同**，所以上表差异全部来自 key 编码。

---

## 4. 为什么这不是移植缺陷（三条独立证据）

1. **参考实现逐字相同。** 拉取 `TertiumOrganum1/ninfer-3090` 的 `a7fb3033`，其
   `kv_cache_e8_nearest_d8` / `kv_cache_e8_nearest` / `kv_cache_int4_e8_key_code`（含
   `__float2int_rn` 与 `[-8,7]` 截断）与 `kv_cache_int4_e8_store_key_group` 与本仓库**无差异**。
2. **移植前成品行为一致。** Arm A 的 128 格 A/B 中，rk4v4-e8 的 8 格（MTP×视觉×ctx）
   `verify-a-compare.log` 全部 `IDENTICAL`，即移植前后**逐字节相同**。
3. **群组内也没有分叉。** `git log --follow src/ops/kv_cache/int8_g64_codec.cuh` 表明 e8 编码只由
   `eb9f7a23` 引入、`ad26b362` 追加姊妹格式，此后未再改动。

⇒ 该缺陷存在于**上游参考与移植前成品**中，移植忠实地把它搬了过来。**修它不是移植收尾，而是产品决策。**

---

## 5. 为什么"收益随 body 变粗单调"是误读

`PORT-VERIFY-REPORT.zh.md` §4.1 B3 判据、`README.md:21,120`、以及 Manager UI
（`apps/windows-manager/web/src/parameterHelp.ts:42`）都写"收益随 body 量化变粗单调增大"，并把排序
`int8 ≤ rk8v4 ≤ rk4v4 ≤ rk4v4-e8` 当作该特性的证据。该排序的最后一格不是"更粗的档位"，而是
**被实现写坏的档位**：

- 若把 E8 步骤去掉（直接 RNE），e8 的质量立刻回到纯 int4（MSE 0.0116，比现状好 2.3×），**仍劣于** rk4v4 1.30×；
- 即使把 coset 位完整保住，上限也只有 rk4v4 的 1.11×（见 §6）。

因此正确的读法应是：**rk4v4-e8 的尾巴"收益更大"，只是因为它先把 body 弄坏了。** 这不是收益排序，
而是缺陷的指纹。3.2 中 `mean_target_logprob_delta` 与 KLD 的比值差异（2.94× vs 1.69×）也说明：越接近线性
的仪器越能看出 body 本身差多少。

---

## 6. 4 bit 下 E8 结构上赢不了 Lloyd-Max

| 量 | 数值 | 说明 |
|---|---:|---|
| E8 相对纯 int4 的增益 | **−0.58 dB**（E8 更低） | 与 E8 理论空间填充增益 0.65 dB 吻合；**点阵仅优于均匀标量** |
| Lloyd-Max 相对纯 int4 的增益 | **+1.15 dB** | 非均匀电平 + 每 G64 组 LS scale 的整形增益 |
| ⇒ Lloyd-Max 相对 E8 | **+0.57 dB** | 即在 4 bit/维、本数据分布下，**固定码率非均匀标量量化优于 E8 格** |

原因：点阵量化的优势主要在"高码率 + 熵编码"区间；在 2–4 bit 这一档，为高斯源定制的 Lloyd-Max
（本例还带每组的 LS scale）本身已接近固定码率最优，而 E8 只能对标**均匀**标量。所以
"rk4v4-e8 是 rk4v4 的上位"这一前提在 **4 bit 这一档不成立**——E8 家族真正有意义的位置是更低码率的
`rk2v4-e8`（2 bit/维，root+radius+axis 的表示才有空间优势）。

---

## 7. 复现命令

```bash
# 0) 既有真机产物（无需 GPU）→ §3.2 的四个表
cd /d/ninfer/ninfer-precision-tail
python - <<'PY'
import json,glob
for tag in ['int8','rk8v4','rk4v4','rk4v4-e8']:
    for ctx in ['8192','32768']:
        p=glob.glob(f'.deps/verify-{tag}-t0-c{ctx}/**/report.json',recursive=True)
        if p: d=json.load(open(p[0])); print(tag,ctx,d['domains'][0]['perplexity'],d.get('kld'))
PY

# 1) 移植前后一致性（rk4v4-e8 的 8 格）→ §4.2
grep -iE "rk4v4-e8" .deps/verify-a-compare.log

# 2) 离线复刻（本次新增，纯 CPU）→ §3.1
cd /d/ninfer/.e8-investigation && python sim_codecs.py && python sim_mechanism.py

# 3) 参考实现比对 → §4.1
curl -s -x http://127.0.0.1:7897 \
  https://raw.githubusercontent.com/TertiumOrganum1/ninfer-3090/a7fb3033/src/ops/kv_cache/int8_g64_codec.cuh \
  -o ref_int8_g64_codec.cuh && grep -n "float2int_rn\|kKVCacheInt4KeyMin" ref_int8_g64_codec.cuh
```

---

## 8. 证据与产物清单

| 产物 | 内容 |
|---|---|
| `D:\ninfer\.e8-investigation\sim_codecs.py` | 四编码器（rk4v4 / rk4v4-e8 / 纯 int4 / E8 保 coset）的 MSE、rel-L2 对照 |
| `D:\ninfer\.e8-investigation\sim_mechanism.py` | 陪集/整数分支分解、E8 步骤的额外 MSE 归因 |
| `D:\ninfer\.e8-investigation\ref_int8_g64_codec.cuh` | 参考实现 `TertiumOrganum1/ninfer-3090 a7fb3033` 的 codec 快照 |
| `.deps/verify-rk4v4{,-e8}-t0-c{8192,32768}/**/report.json` | §3.2 的真机 ppl / KLD / same_top |
| `.deps/verify-a-compare.log` | §4.2 的移植前后一致性（含 rk4v4-e8 × 8 格） |
| `tests/ops/test_kv_cache_append.cpp:535-537,605-610,733-736` | 现有 e8 断言：只要求 `coset_blocks>0 && parity_fixes>0` |
| `tests/ops/softmax_attention/causal_cache.cpp:139-143,2688` | 现有 e8 判据：相对 FP32 oracle 的宽松 rel-L2 1.5e-2 |

---

## 9. 处置选项（本次不实施，供决策）

| 方案 | 效果 | 代价 |
|---|---|---|
| **A 编码器止血**：去掉 E8 吸附，直接 RNE | MSE 从 3.05× 降到 1.30×（rk4v4 的） | 退化为均匀 int4；`rk4v4-e8` 仍劣于 `rk4v4`，格式无存在价值 |
| **B 删除 rk4v4-e8**（本次倾向） | 消除缺陷与误导性文档 | 触及 `include/ninfer/types.h`、`src/ops/kv_cache/*`、`src/serve/serve_options.cpp`、`apps/cli/options.cpp`、`apps/perplexity/main.cpp`、Manager（`parameterHelp.ts`/`App.vue`）、`src/runtime/engine/device_profiles.json` 等 `attn_i8_small/h24/rk4v4-e8/*` 条目、测试 oracle 与 `--rk4v4-e8-only`、`docs/*`；`rk4v4` 成为唯一 4-bit 键方案 |
| **C 实现真正保 coset 的 E8 块码**（32 bit/块对 E8 点集做有界枚举） | 最好仍比 `rk4v4` 差 1.11×（§6） | 内核与测试工作量大，收益为负 |
| **D 仅修文档与判据** | 保留格式但如实描述 | 缺陷仍在产品内 |

若采取 A 或 C，需同步更新 `tests/ops/test_kv_cache_append.cpp` 的 E8 oracle 与
`causal_cache.cpp` 的 `--rk4v4-e8-only` 期望；无论选哪个，建议新增一条"每种 4-bit 键编码不得劣于
同尺度纯 int4 对照"的质量守卫，避免同类缺陷再次静默通过。

---

## 10. 如实交代的边界

- **没有新跑 GPU。** §3.2 的数字取自既有验收产物；若需端到端复测，需按 `PORT-VERIFY-REPORT.zh.md`
  §8 的 Arm B 矩阵重跑（GPU 串行、单所有者）。
- **仿真是代理分布。** §3.1 用 i.i.d. 高斯块代理 Hadamard 旋转后的键统计；真实键可能存在跨组相关，
  绝对值会变，但"陪集分支占比高 + rint 抹掉半整数"这一机理与分布无关。
- **MSE 与 KLD 不同度量。** 仿真给 MSE（3.05×），真机 KLD 因饱和给 1.69×；两者靠近似线性的
  `mean_target_logprob_delta`（2.94×）对齐，未做进一步的端到端数值建模。
- **`rk2v4-e8` 未受影响、也未测。** 它的读侧能重建格点，机理上不适用本结论；但它不在 Arm B 矩阵内
  （2 bit 被排除），本次不予置评。
- **本文只做调查。** §9 的四个方案尚未实施，§5 指出的既有报告/README/UI 措辞**本次未改**。
