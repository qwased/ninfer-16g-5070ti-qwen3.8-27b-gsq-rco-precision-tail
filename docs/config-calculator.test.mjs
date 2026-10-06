// Regression tests for the pure arithmetic in config-calculator.html. The page is deliberately a
// single self-contained file (no build step, no CDN, works from an offline checkout), so this
// harness extracts its <script> body, runs it in a sandboxed context against a minimal DOM stub,
// and exercises the resulting top-level functions directly -- rather than restructuring the page
// into importable modules, which would give up the "one file, no tooling" property that is the
// whole point of it.
//
// Run with: node docs/config-calculator.test.mjs

import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import vm from "node:vm";
import assert from "node:assert/strict";

const here = dirname(fileURLToPath(import.meta.url));
const html = readFileSync(join(here, "config-calculator.html"), "utf8");
// The extractor tolerates CRLF as well as LF: the repository blob is LF, but Git for Windows'
// default core.autocrlf=true checks the page out with CRLF, and a bare `<script>\n` anchor then
// fails to match at all.
const scriptMatch = html.match(/<script>\r?\n([\s\S]*?)<\/script>/);
assert.ok(scriptMatch, "could not find the <script> block in config-calculator.html");
const source = scriptMatch[1];

// --- minimal DOM stub -------------------------------------------------------------------------
// Enough for buildControls/buildDepthTable/buildSpecTable/render to run at load without throwing.
// Everything is deliberately permissive (any property can be set) rather than a faithful DOM.
function makeElement() {
  const el = {
    value: "",
    innerHTML: "",
    textContent: "",
    style: {},
    classList: { add() {}, remove() {} },
    firstChild: { nodeValue: "" },
    addEventListener() {},
  };
  // A stable child per selector, not a fresh throwaway each call, so code that queries once to
  // write (e.g. tbody.innerHTML = ...) and a test that queries again to read see the same object.
  const children = new Map();
  el.querySelector = (selector) => {
    if (!children.has(selector)) children.set(selector, makeElement());
    return children.get(selector);
  };
  return el;
}
const elements = new Map();
const document = {
  getElementById(id) {
    if (!elements.has(id)) elements.set(id, makeElement());
    return elements.get(id);
  },
  querySelectorAll() { return []; },
};
// A real <select> auto-selects its first <option> once populated; buildControls() only sets
// innerHTML, relying on that browser behavior to leave "model" on the first model key. Seed it
// explicitly since the stub does not replicate that; "27b" is DATA.models' first key.
document.getElementById("model").value = "27b";
document.getElementById("reserve").value = "1536";

const context = { document, console };
vm.createContext(context);
vm.runInContext(source, context, { filename: "config-calculator.html inline script" });
// Top-level `const`/`let` bindings in a vm context live in that context's script-level lexical
// environment, not as properties of the context object itself -- pull the ones under test out
// with a second expression evaluated in the same context, where they are still in scope.
const { DATA, PAGE_TOKENS, MIB, pageRoundUp, perToken, fixedBytes, graphBytes, decodeAtDepth,
        buildKvTable, tailRingPages, tailBytes } =
  vm.runInContext(
    "({DATA, PAGE_TOKENS, MIB, pageRoundUp, perToken, fixedBytes, graphBytes, decodeAtDepth, buildKvTable, tailRingPages, tailBytes})",
    context);

let failures = 0;
function check(name, fn) {
  try {
    fn();
    console.log("ok   " + name);
  } catch (err) {
    failures++;
    console.log("FAIL " + name + ": " + err.message);
  }
}

// --- pageRoundUp -----------------------------------------------------------------------------
check("pageRoundUp: exact multiple stays put", () => {
  assert.equal(pageRoundUp(8192), 8192);
});
check("pageRoundUp: one token over rounds up a full page", () => {
  assert.equal(pageRoundUp(8193), 8192 + PAGE_TOKENS);
});
check("pageRoundUp: one token under the boundary still rounds up to it", () => {
  assert.equal(pageRoundUp(8191), 8192);
});
check("pageRoundUp: below one page rounds up to one page", () => {
  assert.equal(pageRoundUp(1), PAGE_TOKENS);
});

// --- exact KV tail -----------------------------------------------------------------------------
// The engine sizes the tail per sequence as a ring of page_count(N) + 1 pages, where
// page_count(N) = ceil(N / 64) and the extra page is the rollback page
// (src/models/qwen3_5/program/planning/startup.cpp): `tail_ring_pages = page_count(kv_tail_tokens)
// + 1`. Pin the +1 and the round-up, because dropping either silently understates the pool.
check("tailRingPages: 0 tokens disables the tail (no pages, no cost)", () => {
  assert.equal(tailRingPages(0), 0);
  assert.equal(tailRingPages(-1), 0);
});
check("tailRingPages: round_up(N/64) pages plus the rollback page", () => {
  const codeRing = (n) => 1 + Math.ceil(n / PAGE_TOKENS); // page_count() + rollback page
  for (const n of [1, 63, 64, 65, 127, 128, 129, 2048, 4096, 262144]) {
    assert.equal(tailRingPages(n), codeRing(n), "ring pages wrong for N=" + n);
  }
  // Concrete anchors so the shape is visible, not just the formula agreeing with itself.
  assert.equal(tailRingPages(1), 2);
  assert.equal(tailRingPages(64), 2);   // exactly one page round-up + rollback
  assert.equal(tailRingPages(65), 3);
  assert.equal(tailRingPages(2048), 33);
});
check("tailBytes: N = 0 is exactly zero on both models (previous numbers unchanged)", () => {
  for (const key of Object.keys(DATA.models)) {
    assert.equal(tailBytes(DATA.models[key], 0), 0);
  }
});
// One page of one head-major BF16 plane is 64 * kv_heads * head_dim * 2 bytes; times two planes
// (K, V) and the full-attention layer count, that is exactly one 64-token page of the BF16 body
// cache. model.kv.bf16 is the measured per-token BF16 body cost, so the tail is
// (ceil(N/64) + 1) * 64 * model.kv.bf16 -- derived from a measured quantity and the layout.
check("tailBytes: (ceil(N/64)+1) pages of the measured BF16 body cache, per sequence", () => {
  for (const key of Object.keys(DATA.models)) {
    const model = DATA.models[key];
    for (const n of [1, 64, 65, 2048, 40960]) {
      const ring = Math.ceil(n / PAGE_TOKENS) + 1;
      assert.equal(tailBytes(model, n), ring * PAGE_TOKENS * model.kv.bf16,
        `${key} tail for N=${n}`);
    }
  }
  // 27B: one 64-token BF16 cache page is 64 * 65536 = 4 MiB; two of them (one page + rollback)
  // for N <= 64.
  assert.equal(tailBytes(DATA.models["27b"], 64), 2 * 64 * 65536);
  assert.equal(tailBytes(DATA.models["27b"], 64), 8 * MIB);
  // A per-sequence constant: independent of context, and different per model (the BF16 body cost
  // differs), which is why it is priced from each model's own measured row.
  assert.notEqual(tailBytes(DATA.models["27b"], 2048), tailBytes(DATA.models["35b"], 2048));
});
check("tailBytes: the tail is BF16 whatever the body format is", () => {
  // KvCacheStorage::BFloat16 is hard-coded for the tail pool (decoder_state.cpp), so the cost must
  // follow model.kv.bf16 and never the selected body format. A quantized body makes the tail a
  // *larger* share of the cache, not a smaller one.
  const model = DATA.models["27b"];
  for (const kv of ["int8", "nvfp4", "rk4v4"]) {
    assert.notEqual(tailBytes(model, 2048), (33 * PAGE_TOKENS) * model.kv[kv]);
  }
});
check("tailBytes: N = 0 leaves the golden engine reservation untouched", () => {
  // The zero-cost identity, stated against the engine cross-check below: disabling the tail adds
  // exactly nothing to the reservation the engine reports.
  const model = DATA.models["27b"];
  const ctx = 262144;
  const none = model.spec.none;
  const kvBytes = (model.kv.int8 * none.kvRatio + none.kvExtraPerToken) * pageRoundUp(ctx);
  const ovhBytes = none.seqFixedBytes + none.seqPerToken * ctx;
  const reservation = kvBytes + ovhBytes + model.workspaceBytes + graphBytes(none, ctx);
  assert.equal(reservation + tailBytes(model, 0), 9197389568);
});
check("tailBytes: a tail consumes context headroom, page-consistently", () => {
  // Constant term -> the tokens it displaces are the ring's own footprint divided by the per-token
  // cost, so a tail of k pages costs k*64 tokens of context plus whatever the rollback page holds.
  const model = DATA.models["27b"];
  const pt = perToken(model, "int8", "none");
  const tail = tailBytes(model, 4096);
  const displaced = tail / pt;
  assert.ok(displaced > 0 && displaced < 4096 * 2,
    "tail should displace O(N) cache tokens, not an unbounded amount (" + displaced + ")");
});

// --- decodeAtDepth -----------------------------------------------------------------------------
const model27b = DATA.models["27b"];
check("decodeAtDepth: exact at a measured depth is not floored/interpolated/extrapolated", () => {
  const d = decodeAtDepth(model27b, "int8", model27b.depths[0]);
  assert.equal(d.exact, true);
  assert.equal(d.floored, false);
  assert.equal(d.extrapolated, false);
  assert.equal(d.value, model27b.speed.int8[0]);
});
check("decodeAtDepth: below the first measured depth is floored, not interpolated", () => {
  const d = decodeAtDepth(model27b, "int8", model27b.depths[0] - 1);
  assert.equal(d.floored, true);
  assert.equal(d.exact, false);
  assert.equal(d.extrapolated, false);
  // Held at the shallowest measurement -- not some fabricated interpolated number.
  assert.equal(d.value, model27b.speed.int8[0]);
});
check("decodeAtDepth: strictly between two measured depths interpolates", () => {
  const [d0, d1] = model27b.depths;
  const mid = Math.round((d0 + d1) / 2);
  const d = decodeAtDepth(model27b, "int8", mid);
  assert.equal(d.exact, false);
  assert.equal(d.floored, undefined);
  assert.equal(d.extrapolated, false);
  const [s0, s1] = model27b.speed.int8;
  assert.ok(d.value < Math.max(s0, s1) + 1e-9 && d.value > Math.min(s0, s1) - 1e-9,
    "interpolated value should lie between the two bracketing measurements");
});
check("decodeAtDepth: past the deepest measured point extrapolates", () => {
  const deepest = model27b.depths[model27b.depths.length - 1];
  const d = decodeAtDepth(model27b, "int8", deepest * 4);
  assert.equal(d.extrapolated, true);
  assert.equal(d.exact, false);
});

// --- the PR's own verified example --------------------------------------------------------------
// From the PR description: requesting a 262,144-token INT8 context on the 27B with no speculation
// is refused by the real engine with "minimum Engine runtime reservation requires 9197389568
// bytes" -- an *incremental* reservation on top of already-resident weights, not the full device
// footprint. The page's own terms (KV + fixed sequence state + per-token remainder + workspace +
// graph allowance, excluding weights) are stated to sum to exactly that. Reproducing it here pins
// that agreement as a regression instead of a one-off manual check.
check("golden: 262144-token int8, no speculation, 27B matches the engine's own refusal figure", () => {
  const model = model27b;
  const ctx = 262144;
  const none = model.spec.none;
  const kvBytes = (model.kv.int8 * none.kvRatio + none.kvExtraPerToken) * pageRoundUp(ctx);
  const ovhBytes = none.seqFixedBytes + none.seqPerToken * ctx;
  const reservation = kvBytes + ovhBytes + model.workspaceBytes + graphBytes(none, ctx);
  assert.equal(reservation, 9197389568);
});

// --- and the same cross-check for a *speculative* configuration ---------------------------------
// This is the half docs/archive/TODO.md section 2b flagged as untested, and it was untested because the page's
// speculative model was known to be wrong. With per-mode terms measured by
// scripts/sweeps/speculative-memory-terms.ps1, the engine can be asked the same question:
//
//   ninfer qwen3_8_27b.ninfer --max-context 262144 --kv-dtype int8 --spec mtp --draft-tokens 3 //       --lm-head-draft
//   error: requested Engine runtime reservation requires 9838038272 bytes
//
// The page's terms reproduce that to 0.044%. The remaining 4.3 MB on 9.84 GB is page-rounding in
// the engine's own allocator, which this page models at KV-page granularity only.
//
// For scale, the model this page used before carrying per-mode terms came out 611 MiB *short* of
// the same figure -- section 2b's "roughly 170 MiB optimistic" was itself measured at a 40,960
// context, where the per-token terms contribute far less than they do here.
check("golden: 262144-token int8 with MTP3 + draft head is within 0.1% of the engine", () => {
  const model = model27b;
  const ctx = 262144;
  const spec = model.spec["mtp3+head"];
  const kvBytes = (model.kv.int8 * spec.kvRatio + spec.kvExtraPerToken) * pageRoundUp(ctx);
  const ovhBytes = spec.seqFixedBytes + spec.seqPerToken * ctx;
  const reservation = kvBytes + ovhBytes + model.workspaceBytes + graphBytes(spec, ctx);
  const engine = 9838038272;
  const error = Math.abs(reservation - engine) / engine;
  assert.ok(error < 0.001, `reservation ${reservation} vs engine ${engine} (${(error * 100).toFixed(3)}%)`);
});

// --- DFlash v1 on the 35B, which the model reproduces exactly ----------------------------------
// The strongest cross-check available, and the one that settles how DFlash should be modelled:
//
//   ninfer qwen3_6_35b_a3b.ninfer --max-context 262144 --kv-dtype int8 --spec dflash --draft-tokens 3
//   error: requested Engine runtime reservation requires 4312377600 bytes
//
// The page's terms reproduce that figure *to the byte*. They only do so because DFlash's extra KV
// is modelled as a fixed 4,096 bytes per token rather than as a multiplier: measured, the extra is
// 32 MiB at 8,192 tokens, 64 MiB at 16,384 and 128 MiB at 32,768, exactly linear in context and
// identical for int8 and nvfp4. As a ratio it would have to be 1.388 for int8 and 1.711 for nvfp4,
// which is the same quantity wearing the wrong shape.
//
// For scale: the model this page used before per-mode terms came out 1,289 MiB short here, a 31%
// undercount. Section 2b's "roughly 170 MiB" was measured at a 40,960 context on MTP3, which is
// the mildest combination of mode and depth.
check("golden: 262144-token int8 with DFlash-3 on the 35B matches the engine exactly", () => {
  const model = DATA.models["35b"];
  const ctx = 262144;
  const spec = model.spec["dflash-3"];
  const kvBytes = (model.kv.int8 * spec.kvRatio + spec.kvExtraPerToken) * pageRoundUp(ctx);
  const ovhBytes = spec.seqFixedBytes + spec.seqPerToken * ctx;
  const reservation = kvBytes + ovhBytes + model.workspaceBytes + graphBytes(spec, ctx);
  assert.equal(reservation, 4312377600);
});

// --- DFlash's extra KV is per token, not per format --------------------------------------------
// If someone re-derives this as a ratio, these two assertions fail. That is the point.
check("DFlash's extra KV is a fixed 4096 bytes per token across formats", () => {
  const model = DATA.models["35b"];
  const spec = model.spec["dflash-3"];
  assert.equal(spec.kvExtraPerToken, 4096);
  assert.equal(spec.kvRatio, 1);
  for (const kv of ["int8", "nvfp4", "bf16"]) {
    const delta = perToken(model, kv, "dflash-3") - perToken(model, kv, "none");
    // 4096 of extra cache plus the doubled non-KV per-token term (0.0625 -> 0.125).
    assert.equal(delta, 4096 + 0.0625);
  }
});

// --- the DFlash2 graph allowance is a step in context -------------------------------------------
// Measured 288 MiB at 8,192 and 384 MiB at 16,384 and 32,768. A flat value would understate the
// deeper contexts by 96 MiB, and using the deeper value everywhere would overstate 8,192 by the
// same. Pin both sides so a future edit cannot quietly flatten it.
check("graphBytes: DFlash2's allowance steps with context, other modes are flat", () => {
  const spec = model27b.spec["dflash2-7"];
  assert.equal(graphBytes(spec, 8192), 288 * MIB);
  assert.equal(graphBytes(spec, 16384), 384 * MIB);
  assert.equal(graphBytes(spec, 262144), 384 * MIB);
  const mtp = model27b.spec["mtp3+head"];
  assert.equal(graphBytes(mtp, 8192), 86 * MIB);
  assert.equal(graphBytes(mtp, 262144), 86 * MIB);
  assert.equal(graphBytes(model27b.spec.none, 262144), 12 * MIB);
});

// --- speculation must never look cheaper than no speculation ------------------------------------
// The failure mode the old model had: every speculative row understated memory, so the page could
// report a speculative configuration fitting a context that the plain one did not. Assert the
// ordering directly, for every mode on both models.
check("every speculative mode costs at least as much as none, per token and fixed", () => {
  for (const key of Object.keys(DATA.models)) {
    const model = DATA.models[key];
    const noneFixed = fixedBytes(model, "none", 262144);
    const nonePt = perToken(model, "int8", "none");
    for (const specKey of Object.keys(model.spec)) {
      if (specKey === "none") continue;
      assert.ok(fixedBytes(model, specKey, 262144) >= noneFixed,
        `${key}/${specKey} fixed bytes below none`);
      assert.ok(perToken(model, "int8", specKey) >= nonePt,
        `${key}/${specKey} per-token below none`);
    }
  }
});

// --- max-context page alignment -----------------------------------------------------------------
check("max-context formula (mirrored here) is always a page multiple and fits", () => {
  const model = model27b;
  const pt = perToken(model, "int8", "none");
  const fixed = fixedBytes(model, "none", model.nativeMaxContext); // weights + workspace + graph + seqFixed, once
  for (const reserveMiB of [0, 307, 1536, 2560]) {
    const usable = (DATA.cardMiB - reserveMiB) * 1024 * 1024;
    const room = usable - fixed;
    const maxCtx = Math.max(0, PAGE_TOKENS * Math.floor(room / (PAGE_TOKENS * pt)));
    assert.equal(maxCtx % PAGE_TOKENS, 0, "reserve=" + reserveMiB + "MiB gave a non-page-aligned max");
    // Reconstructed the same way render()'s `total` is: seqFixedBytes counted once (inside
    // ovhBytes), not twice (fixedBytes() already carries its own copy for the room calculation).
    const kvBytes = model.kv.int8 * pageRoundUp(maxCtx);
    const perTokenOverhead = model.spec.none.seqPerToken * maxCtx;
    assert.ok(fixed + kvBytes + perTokenOverhead <= usable + 1e-6,
      "reserve=" + reserveMiB + "MiB: reported max-context must not exceed usable memory");
  }
});

// buildKvTable() duplicated the same unrounded formula independently of render()'s max-context
// box; this exercises that second copy end to end, not just the shared math above.
check("buildKvTable: every format's Largest-Context figure is page-aligned", () => {
  document.getElementById("kv").value = "int8";
  document.getElementById("spec").value = "none";
  buildKvTable();
  const html = document.getElementById("kv-table").querySelector("tbody").innerHTML;
  const cells = [...html.matchAll(/<tr data-kv="(\w+)">.*?<td class="[^"]*">([\d,]+|—)( \(max\))?<\/td>/gs)];
  assert.ok(cells.length > 0, "expected at least one KV-format row to have rendered");
  for (const [, kv, numberText] of cells) {
    if (numberText === "—") continue; // an unmeasured format for this model
    const ctx = Number(numberText.replace(/,/g, ""));
    assert.equal(ctx % PAGE_TOKENS, 0, kv + "'s reported largest context is not page-aligned: " + ctx);
  }
});

console.log(failures === 0 ? "\nPASS" : "\nFAIL (" + failures + ")");
process.exit(failures === 0 ? 0 : 1);
