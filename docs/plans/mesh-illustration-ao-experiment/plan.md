+++
type = "plan"
id = "mesh-illustration-ao-experiment"
status = "active"
created = "2026-09-03"

[[steps]]
id = "reference-baselines"
title = "Capture no-AO reference images, geometry characteristics, and output metrics for the governed fixture matrix"
status = "done"

[[steps]]
id = "bounded-ao-kernel"
title = "Prototype deterministic whole-assembly BVH ambient-visibility sampling with explicit work, memory, cancellation, and cache bounds"
status = "done"
depends_on = ["reference-baselines"]

[[steps]]
id = "vector-shading-integration"
title = "Apply quantized AO only to ambient illumination while preserving materials, linework, fusion safety, and SVG/Canvas parity"
status = "done"
depends_on = ["bounded-ao-kernel"]

[[steps]]
id = "demo-controls"
title = "Add opt-in experimental AO strength, radius, quality, and band controls without exposing a stable public contract"
status = "done"
depends_on = ["vector-shading-integration"]

[[steps]]
id = "evidence-matrix"
title = "Measure visual quality, deterministic behavior, preparation cost, caching, fusion impact, and SVG size across the experiment matrix"
status = "done"
depends_on = ["demo-controls"]

[[steps]]
id = "design-doc-intent-audit"
title = "Confirm the disposable spike has not changed durable production intent or record only accepted handoff implications"
status = "done"
depends_on = ["evidence-matrix"]

[[steps]]
id = "test-runtime-impact-audit"
title = "Confirm experiment checks are bounded and do not impose lasting suite cost if the spike is discarded"
status = "done"
depends_on = ["evidence-matrix"]

[[steps]]
id = "external-review"
title = "Obtain a focused independent check of the evidence and discard-or-handoff recommendation"
status = "done"
depends_on = ["evidence-matrix", "design-doc-intent-audit", "test-runtime-impact-audit"]

[[steps]]
id = "user-decision"
title = "Present side-by-side packaged demo results and record discard or integration-handoff decision"
status = "pending"
depends_on = ["external-review"]

[[steps]]
id = "branch-handoff"
title = "Discard the failed spike or package the accepted experiment and evidence for the integration agent"
status = "pending"
depends_on = ["user-decision"]

[[exit_criteria]]
id = "baseline-unchanged"
title = "AO-off SVG and Canvas output remains identical to the accepted illustration baseline"
status = "pending"

[[exit_criteria]]
id = "bounded-evidence"
title = "The fixture matrix records visual results, deterministic behavior, runtime, caching, fusion impact, and SVG growth"
status = "pending"

[[exit_criteria]]
id = "user-decision"
title = "The user explicitly chooses discard or integration handoff from side-by-side demo evidence"
status = "pending"

[[exit_criteria]]
id = "clean-handoff"
title = "Failed experimental code is removed, or accepted commits and evidence are ready for the integration agent without merge or release work on this branch"
status = "pending"

[[exit_criteria]]
id = "design-doc-intent-audit"
title = "The experiment leaves durable production intent unchanged unless its accepted implications are explicitly handed off"
status = "pending"

[[exit_criteria]]
id = "test-runtime-impact-audit"
title = "Experiment coverage and runtime are bounded, and disposable checks are removed with a rejected spike"
status = "pending"

[[exit_criteria]]
id = "external-review"
title = "A focused independent review finds the evidence sufficient for the user decision"
status = "pending"
+++

# Mesh Illustration Ambient Occlusion Experiment

## Purpose

Evaluate whether deterministic mesh-space ambient occlusion materially improves Geometer's technical illustration output before adding it to any supported interface. The experiment must produce comparable SVG and Canvas output and must not restore the retired GPU raster-HLR path.

## Boundary

- Fast vector HLR and mesh-shadow linework remain unchanged.
- AO is computed from the complete tessellated assembly so separate components can create contact occlusion.
- The spike remains opt-in, defaults off, and has no stable serialized or package contract until explicit promotion.
- The spike is time-boxed to the fixture matrix. If it does not clearly improve the illustrations, remove it and move on without further hardening.
- AO preparation is CPU/worker based, deterministic, bounded, cancellable, and cached independently of camera and style-only changes.
- The renderer attenuates only the ambient lighting term; key light, rim light, material identity, HLR lines, and alpha behavior remain independently controlled.
- SVG and Canvas consume the same prepared AO values and tonal policy.

## Experiment matrix

Use SOT-223, Cap_SMT_Aluminum_F, ABM8, BGA90, and one synthesized multi-part PCB-style assembly. Capture no-AO baselines and a bounded matrix of 8/16/32 hemisphere samples, 2/5/10 percent bounding-box-diagonal radius, 0.15/0.35/0.55 strength, and 3/5/8 tonal bands. Include concave, convex, close-contact, separated-part, smooth curved, and coplanar-marking cases.

## Measurements

Record AO preparation time, peak working memory where observable, cache reuse, SVG byte and draw-count growth, fusion loss, SVG/Canvas parity, deterministic repeatability, and visible artifacts such as self-shadow acne, tessellation print-through, band noise, detached halos, or lost source colors.

## Bounded smoke evidence

The automated browser evaluation uses 8 samples and a 5 percent scene-diagonal radius. It verifies AO-off byte-for-byte SVG restoration, cached reproduction, shared SVG/Canvas render-command counts, and bounded preparation across all six bundled fixtures. Observed worker times ranged from 13 ms for SOT-23/SOT-223 to 1.70 s for the 24,060-triangle BGA90. AO increased SVG draw counts by 0.3 to 106 percent depending on how strongly tonal bands split otherwise fusible regions.

This is sufficient to present the experiment for visual user evaluation, but it is not exhaustive qualification of every sample/radius/strength/band combination, pixel-exact Canvas output, peak memory, or a synthesized PCB assembly. Independent package review approved the experimental boundary and implementation after cache invalidation and finite-input fixes; the user-decision gate remains pending.

## Decision gate

The experiment ends with one of two explicit outcomes:

1. delete the spike because the visual value or cost is unacceptable;
2. package the accepted implementation, measurements, and limitations for the integration agent to evaluate and promote.

Promotion requires user approval of side-by-side demo output. Experimental code must not silently become part of the current Fast HLR production contract. This branch does not own integration or release.
