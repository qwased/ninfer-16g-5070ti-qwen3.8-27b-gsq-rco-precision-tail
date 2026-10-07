# Perplexity evaluation

`ninfer-perplexity` measures the causal perplexity produced by a v3 `.ninfer` artifact.
It uses the artifact's tokenizer, Text model, selected Main KV representation, final normalization,
and main output head. It is an offline evaluator, not a serving endpoint or a logits-export API.
Only Text weights and resources are loaded; Vision and speculative components are not required.

## Run the fixed corpus

The repository includes `ninfer-ppl-1m-v1`, a fixed set of 16 independent UTF-8 streams covering
English reference text, English long-form text, Chinese reference text, and NInfer C++/CUDA code.
`full` selects all streams; `--quick` selects one stream from each domain.

```bash
./build/apps/ninfer-perplexity models/qwen3_8_27b_nvfp4.ninfer \
  --corpus eval/corpora/perplexity-1m/manifest.json \
  --quick \
  --kv-dtype int8
```

The default evaluation uses a 4,096-token context and a 2,048-token stride. Use `--context` and
`--stride` to change that protocol, or score one UTF-8 file with `--text FILE`. The available Main
KV representations are `bf16`, `int8`, `fp8`, `rk8v4`, `rk4v4`, `nvfp4`, `k8v4`, and KVarN
`kvarn:k4v4`, `kvarn:k5v5`, `kvarn:k6v6` (the bare spelling `kvarn` is `kvarn:k4v4`).

`--kv-tail-tokens N` enables the exact KV tail: the newest `N` tokens of every sequence stay
unquantized (BF16) in a second pool and attention merges an exact tail partial with the quantized
body. `N` defaults to 0 (off, no memory cost). The tail costs a fixed amount per sequence
independent of context length, so the quality it buys is measured by scoring the same artifact and
corpus twice, at `--kv-tail-tokens 0` and at the chosen `N`, and comparing the perplexity. The merge
is implemented for the storages whose decoded key plane is in original coordinates — `bf16` and the
INT8 family (`int8`, `rk8v4`, `rk4v4`, `rk4v4-e8`, `rk2v4-e8`); the rotated-value formats `fp8`,
`nvfp4` and `k8v4` allocate the pool but no attention route reads or writes it, so the tail is inert
there and a tail-on run of those bytes is identical to tail-off; `kvarn:*` rejects the tail outright
rather than allocating it unused.

The tail merge is implemented only on the small-T attention route, which the causal-scoring pass
reaches only for a query width of eight or fewer tokens; the default 1024-wide score tile runs the
prompt route, which never reads the tail. `--score-width W` scores in width-`W` attention query
tiles so a small `W` (one to eight) drives the small-T route and makes the tail's on/off difference
visible. It preserves the scored token count and ordering, and `W` is applied verbatim (it is not
rounded to the prefill-chunk alignment), so `W` of 1 and 7 stay unaligned. `W` defaults to 1024 (the
planned prefill chunk), which is exactly the pre-option behavior.

All seven have been measured on this corpus; the results, alongside each format's size and decode
speed, are in [`docs/config-calculator.html`](config-calculator.html).
`--fast-prefill-kernel` scores `int8` with the fast prompt-attention kernel (as
`ninfer-serve --fast-prefill-kernel` prefills); `report.json` records it as `fast_prefill_kernel`.

```bash
./build/apps/ninfer-perplexity models/qwen3_8_27b.ninfer \
  --text notes.txt \
  --context 16384 --stride 8192 \
  --kv-dtype int8
```

Run `./build/apps/ninfer-perplexity --help` for the complete command surface. The evaluator loads
the model once, reads and tokenizes every selected stream before scoring, and writes readable
startup, corpus, scoring, and per-stream summaries to stderr. Interactive weight loading and
scoring use one transient progress line; redirected scoring emits persistent progress every ten
seconds. `--log-level debug` exposes internal startup and stream-begin detail. The final
domain/overall table remains product output on stdout; the independent full-precision machine
report is `report.json` under `profiles/perplexity/` unless `--output` supplies an empty directory.

For KV-format comparisons, the recommended long-context profile is the full corpus with
`--context 65536 --stride 32768` and without `--quick`.

`--disjoint` replaces the sliding windows with back-to-back windows of `--context` tokens, each
scored from its second token on, and drops a partial window at the end. That is the protocol of
the WikiText-2 perplexities quantization papers and model cards quote (GPTQ lineage: the test rows
joined by blank lines, 2,048-token windows). With the test split written to a file:

```bash
./build/apps/ninfer-perplexity models/qwen3_8_27b_gsq_rco_iq3_s.ninfer \
  --text wiki.test.joined.txt --context 2048 --disjoint --kv-dtype bf16
```

Past the model's native window `--rope-yarn` applies YaRN at factor `--context` / native, and
`--rope-yarn-factor F` at a fixed factor in `[1,4]` whatever the window is (default `1`, native
RoPE). `--rope-scaling-factor F` instead interpolates positions past
`--rope-scaling-original-context` (default: the native window) linearly by `F` in `[1,32]`, keeping
the angles of every position up to it. None changes the artifact or the default 4,096-token window;
long-context extrapolation is not a quality guarantee, so keep the factor fixed while comparing other
numerical settings. The report records all four settings.

## Metric

For a stream `x[0..N)`, every token after `x[0]` is scored exactly once. A window `[b,e)` with target
suffix `[s,e)` contributes:

```text
log p(x[i] | x[b], ..., x[i-1])  for i in [s,e)
```

Each window starts from empty State and Main KV, so history before `b` is deliberately excluded.
The reported metric is therefore fixed-window, truncated-context causal perplexity:

```text
mean_nll = -sum(logprob) / scored_tokens
perplexity = exp(mean_nll)
```

The first window scores `[1,min(context,N))`. Each later window advances by `stride` targets while
retaining up to `context-stride` preceding tokens as local context. Streams never share history.

## KLD against a reference (top-K)

`--save-topk <path>` and `--kld-base <path>` add the distribution-level metric llama.cpp reports
next to perplexity. `--save-topk` persists this run's per-target next-token distribution as a KLD
reference (normally the `bf16` body with the tail off); `--kld-base` loads one and reports this
run's KLD against it. Either flag enables the top-K instrument at `--score-topk K`, default 100.

Each scored target then carries, besides its own log probability, the `K` most probable next tokens
with their log probabilities. The selection runs on the device over the output head's logits in the
same pass that produces the target log probability, so the two are in the same log-softmax domain
and a token that is both a top-K member and the target carries the identical value.

The comparison is `KLD(candidate || reference)` over the **union** of the two top-K sets plus the
target token itself, with each side renormalized over that support; a token a side did not select is
floored at that side's least probable top-K entry, the smallest value that side actually measured.
Mass outside the union is dropped on both sides, exactly as llama.cpp's top-K KLD does, so only the
*incremental* KLD between two runs is meaningful -- never its absolute value. Both flags report the
median, mean, P99, P99.9 and maximum of the per-target KLD, Same-top% (the share of targets whose
most probable token is identical, which is exact because the top-K argmax is the vocabulary argmax),
and the mean difference of the scored target token's own log probability.

The reference file is refused on load when its corpus, model, context, stride, score width, window
mode or scored-token count differs from the run, so a KLD always compares the same scored positions;
only the KV representation may differ, because that is what the comparison varies. Scoring a run
against a reference taken from itself reports KLD 0 and Same-top 100% -- the self-check to run after
any change to the instrument:

```bash
./build/apps/ninfer-perplexity models/qwen3_8_27b_gsq_rco_iq3_s.ninfer \
  --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --stride 4096 \
  --kv-dtype bf16 --kv-tail-tokens 0 --save-topk out/bf16-t0.topk

./build/apps/ninfer-perplexity models/qwen3_8_27b_gsq_rco_iq3_s.ninfer \
  --corpus eval/corpora/perplexity-1m/manifest.json --quick --context 8192 --stride 4096 \
  --kv-dtype rk4v4-e8 --kv-tail-tokens 1024 --kld-base out/bf16-t0.topk
```

The instrument costs `K` selection passes over the vocabulary per scored column, so a run with
either flag is measurably slower than the same run without one. Its result lands in `report.json`
as `kld`, with `topk` naming the reference paths and `execution.score_topk_tokens` the width.

## Comparing runs

For a numerical comparison, keep the corpus, context, stride, and execution settings fixed except
the variable being measured. Compare KV formats with the same artifact and weight formats with the
same KV format.

The corpus name is a workload scale, not an exact token count. Exact input and scored-token counts
are runtime results from the current artifact tokenizer and are recorded in each report. Reports
contain unrounded NLL/PPL values for every window, stream, domain, and the token-weighted overall
aggregate.

The schema-v5 report identifies the artifact's architecture, public name, actual weight formats
and prefill signature alongside the workload and numerical results; its execution configuration
records `rope_yarn`, `rope_yarn_factor`, `rope_scaling_factor`, `rope_scaling_original_context` and
`score_topk_tokens`.
