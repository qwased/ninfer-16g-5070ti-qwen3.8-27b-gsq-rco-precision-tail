#!/usr/bin/env bash
# Precision-tail experiment harness (plan §0.5, §5).
#
# Reproducible records + the M0/M1 measurement cells. Does NOT run anything by
# default; pick a cell explicitly. GPU runs are serial and single-owner: never
# invoke two cells at once.
#
#   port-tools/run-experiments.sh manifest            # hashes only, no GPU
#   port-tools/run-experiments.sh llamacpp-kvarn4     # baseline: kvarn4 + tail
#   port-tools/run-experiments.sh ninfer-tiers        # rk8v4/rk4v4-e8/nvfp4 x tail 0/N
#   port-tools/run-experiments.sh all                 # manifest + both (long, needs a free GPU)
#
# Every run writes a timestamped directory containing the exact command line and
# its stdout/stderr. See docs/port-records/PORT-MEMORY.md for the pinned baseline description.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LLAMACPP_DIR="${LLAMACPP_DIR:-D:/ninfer/llamacpp}"
NINFER_PKG_DIR="${NINFER_PKG_DIR:-D:/ninfer/ninfer-package}"

# Baseline models (plan §0.5). Override to re-pin.
LLAMACPP_MODEL="${LLAMACPP_MODEL:-$LLAMACPP_DIR/model/Qwen3.8-27B-GSQ-RCO-IQ3_XXS-mtp.gguf}"
NINFER_MODEL="${NINFER_MODEL:-$NINFER_PKG_DIR/model/Qwen3.8-27B-GSQ-RCO-IQ3_XXS-vision-bf16-mtp.ninfer}"

# Our build (task #2). The shipped product has no perplexity binary.
NINFER_PPL="${NINFER_PPL:-$REPO/build-port/apps/ninfer-perplexity.exe}"
CORPUS_MANIFEST="${CORPUS_MANIFEST:-$REPO/eval/corpora/perplexity-1m/manifest.json}"
# llama-perplexity takes one text file (the ninfer side reads the manifest). Pin the same
# wikitext stream so the M0 baseline is reproducible; override for other streams.
CORPUS_TEXT="${CORPUS_TEXT:-$REPO/eval/corpora/perplexity-1m/data/wikitext/00.txt}"

# Pinned protocol (plan §0.5 item 3): same -c / batch size / metric on both sides.
CTX="${CTX:-4096}"
STRIDE="${STRIDE:-2048}"
TAIL_N="${TAIL_N:-1024}"
KVARN="${KVARN:-kvarn4}"

OUT_ROOT="${OUT_ROOT:-$REPO/port-tools/results}"
STAMP="$(date +%Y%m%d-%H%M%S)"

die() { echo "error: $*" >&2; exit 1; }
need_file() { [ -f "$1" ] || die "missing file: $1"; }

new_run_dir() {
  local dir="$OUT_ROOT/$STAMP-$1"
  mkdir -p "$dir"
  echo "$dir"
}

# Record exact artifact identity. Plan §0.5 item 6.
record_manifest() {
  local dir; dir="$(new_run_dir manifest)"
  hash_or_warn() {
    if [ -f "$1" ]; then sha256sum "$1"; else echo "MISSING: $1"; fi
  }
  {
    echo "# precision-tail experiment manifest"
    echo "date=$STAMP"
    echo "host_gpu=$(nvidia-smi --query-gpu=name,compute_cap,driver_version --format=csv,noheader 2>/dev/null || echo unknown)"
    echo "repo_head=$(git -C "$REPO" rev-parse HEAD 2>/dev/null || echo unknown)"
    echo "repo_dirty=$(git -C "$REPO" status --porcelain 2>/dev/null | wc -l)"
    echo
    hash_or_warn "$LLAMACPP_MODEL"
    hash_or_warn "$NINFER_MODEL"
    hash_or_warn "$CORPUS_MANIFEST"
    hash_or_warn "$LLAMACPP_DIR/llama-perplexity.exe"
    hash_or_warn "$LLAMACPP_DIR/llama-server.exe"
    hash_or_warn "$NINFER_PPL"
    hash_or_warn "$NINFER_PKG_DIR/engine/ninfer-serve.exe"
  } | tee "$dir/manifest.txt"
  echo "manifest -> $dir/manifest.txt"
}

# Run one command, capturing the exact argv and full output. Plan §0.5 item 6.
run() {
  local dir="$1"; shift
  local name="$1"; shift
  {
    printf '%s\n' "argv: $*"
    echo "---"
  } > "$dir/$name.cmd"
  ( cd "$dir" && "$@" ) > "$dir/$name.out" 2> "$dir/$name.err"
  local rc=$?
  echo "== $name rc=$rc -> $dir/$name.{out,err}"
  return $rc
}

# llamacpp baseline: KVarN + exact tail. Speculation is disabled (plan §0.5 item 1):
# llama-perplexity scores without speculation; --spec-type none is for the
# server/cli speed runs. KVarN target-cache use may require a model-backed
# speculative mode on this fork -- OPEN ITEM, confirm on first real run.
cell_llamacpp() {
  local dir; dir="$(new_run_dir llamacpp-$KVARN-tail$TAIL_N)"
  need_file "$LLAMACPP_MODEL"
  need_file "$CORPUS_TEXT"
  run "$dir" baseline "$LLAMACPP_DIR/llama-perplexity.exe" \
    -m "$LLAMACPP_MODEL" \
    -f "$CORPUS_TEXT" \
    --cache-type-k "$KVARN" --cache-type-v "$KVARN" \
    --kv-tail-tokens "$TAIL_N" \
    -c "$CTX" --chunks 4
}

# ninfer tail curves: the three DoD §7.1 bodies, tail 0 vs N (plan matrix).
# Needs our build (the product ships no perplexity binary). No speculation:
# the offline evaluator does not use it (plan §0.5 item 2).
cell_ninfer_tiers() {
  need_file "$NINFER_MODEL"
  need_file "$CORPUS_MANIFEST"
  [ -x "$NINFER_PPL" ] || die "ninfer-perplexity not built yet: $NINFER_PPL"
  for dtype in rk8v4 rk4v4-e8 nvfp4; do
    for tail in 0 "$TAIL_N"; do
      local dir; dir="$(new_run_dir "ninfer-$dtype-tail$tail")"
      run "$dir" ppl "$NINFER_PPL" "$NINFER_MODEL" \
        --corpus "$CORPUS_MANIFEST" --quick \
        --kv-dtype "$dtype" --kv-tail-tokens "$tail" \
        --context "$CTX" --stride "$STRIDE"
    done
  done
}

case "${1:-}" in
  manifest)          record_manifest ;;
  llamacpp-kvarn4)   cell_llamacpp ;;
  ninfer-tiers)      cell_ninfer_tiers ;;
  all)               record_manifest; cell_llamacpp; cell_ninfer_tiers ;;
  *) sed -n '2,18p' "${BASH_SOURCE[0]}"; exit 2 ;;
esac
