# GFX roadmap — DX11 first

Owner decision: 2026-11-27. Follow [AGENTS.md](../AGENTS.md) and the existing contribution policies.
This is a planning snapshot, not a claim that untested features work. Refresh main, PRs and profiles before choosing work.
Baseline main: `00567a9de9c005d4d073e34a86d2e05d9261ee53`.
C/R = relative engineering complexity / regression risk (1–5), not measurements.

## Completed and ongoing work

| GFX | Scope | State / PR |
| --- | --- | --- |
| 01–02 | Particle texture lookup and vertex colour packing | Merged #9, #11 |
| 03 | Decal streaming | Merged #10 |
| 03b–d | Terrain decal bias and overview depth precision | Merged #23, #24, #29; isolated bias A/B remains open |
| 04–06 | Radix sort, bounded sorting geometry, reusable CPU pages | Merged #12–14 |
| 07–08 | Quad sorting and compatible draw runs | Integrated by #25; count fix #26 |
| 09 | Stencil shadow upload safety | Merged #17; optimization itself incomplete |
| 09b.1 | Shadow-pass counters | Open #75 (main); finish review and retain baseline |
| 10a | Debris profiling | Open #71; finish review and retain baseline |
| 11–11c | Quad-depth workers, settings and profiling | Merged #18, #27, #30; benefit not established |
| 12a–b | Shared static strip prep and shadow rebuild profiling | Merged #28, #70; still serial |
| 13a.1–4 | Material/draw abstraction, roads/scorch, texture addressing, buffer binding | Merged #31, #72–74 |

The reports in #30 and #70 do not justify lowering the default quad worker threshold or parallelizing static shadow strips. #75 identifies small dynamic buffer requests as a candidate for optimization after DX11, not a migration prerequisite.

## Phase A — renderer abstraction (highest priority)

Keep the existing DX8 behavior, state-cache semantics and resource lifetime during each slice. Do not optimize just because a caller is being migrated.

| ID | Scope and acceptance | C/R | Dependencies / status |
| --- | --- | --- | --- |
| 13a.5 | Typed min/mag/mip filters, anisotropy, capability queries; preserve all legacy fallbacks | 3/2 | #73; draft #83 |
| 13a.6a | Scoped terrain depth bias backend operation; preserve capability, cache checks and restoration | 2/2 | #23/#29; draft #85 |
| 13a.7 | Migrate remaining road/scorch buffer binding through existing interface | 2/1 | #72/#74; draft #84 |
| 13a.6b | Typed depth comparison/write/test and blend states; migrate a narrow shader consumer; preserve immediate vs deferred semantics | 3/3 | 13a.6a |
| 13a.6c | Stencil, cull, color write mask and alpha test semantics; distinguish direct device calls from cached wrapper state | 4/3 | 13a.6b |
| 13a.8 | Fixed-function texture-stage combine, arguments, coordinate transforms and sampler restoration | 4/3 | 13a.5, 13a.6b |
| 13a.9 | Draw strip, buffer-type draw overloads, immediate primitives and remaining draw/state consumers, in independent subsystem slices | 3/3 | 13a.6–8; no silent change to ordering |
| 13a.10 | Inventory remaining DX8 dependencies across both titles/editors; ensure an eventual native backend has no hidden direct device access | 3/2 | Continuous; required before full backend selection |

Do not treat the first three draft PRs as a finished abstraction. Their shared Renderer declarations may need rebase reconciliation. Review and compile each independently.

## Phase B — resource abstraction

| ID | Scope and acceptance | C/R |
| --- | --- | --- |
| 13b.1 | Inventory resource ownership, refcounts, formats, usage flags, pools, lock/unlock, reset and native-handle escape paths; design neutral resource descriptors and lifetime contract. First-pass audit in [draft #86](https://github.com/prnnsn/GeneralsGameCode/pull/86); complete exhaustive inventory before implementation. | 3/2 |
| 13b.2 | Vertex/index resource creation and update API; DX8 reference adapter; retain CPU sorting pages and 16-bit indexing | 4/3 |
| 13b.3 | Texture/surface creation, mip levels, upload, DDS formats and readback; retain legacy fallback behavior | 4/3 |
| 13b.4 | Render targets, depth/stencil, viewport, copies, screenshots and device-reset/resource recreation | 4/4 |
| 13b.5 | Migrate remaining resource consumers in both titles and editors; compile/reference tests for ownership and recovery | 4/3 |

13b.1 can begin while Phase A is in review. Implement resource interfaces only after lifetime and format semantics are reviewed. Existing W3D resources are still DX8-backed; adding forwarding methods alone is not enough.

## Phase C — actual DX11 backend

| ID | Scope and acceptance | C/R |
| --- | --- | --- |
| 13c.1 | Isolated Windows DX11 device/context/swap-chain bring-up and test target; select feature levels, depth format, resize/cleanup/failure handling; keep shipping DX8 untouched | 3/3 |
| 13c.2 | Implement neutral resource descriptors in DX11, buffer Map/Unmap and texture/surface lifetime; validate uploads and reset paths | 4/4 |
| 13d.1 | Translate fixed-function vertex formats, transforms, lights, fog, material and alpha/texture-stage semantics into shaders | 5/4 |
| 13d.2 | DX11 depth/stencil, rasterizer, blend and sampler states; cache by full descriptors, preserve legacy pass order | 4/4 |
| 13e.1 | Bring up terrain, meshes, particles, decals, shadows, water, UI, fonts/minimap and editor features incrementally | 5/5 |
| 13e.2 | Explicit backend selection/build integration, fallback or clear startup error, resize/fullscreen, device removal, legacy asset tests | 4/4 |
| 13e.3 | Compatibility and performance gate: both game/editor variants, replay/save/network CRC and RNG, matching screenshots, frame-time/CPU/GPU captures, and stable extended zoom | 4/4 |

A standalone device test is not a playable backend. Avoid promising DX11 FPS improvements or feature parity before 13e.3. Do not ship a partially migrated backend as the default. DX11 work may use Reforged as a reference, but its D3D9-era semantics and resource paths need explicit adaptation to our DX8 baseline. Check source/license and attribute any borrowed code.

## Deferred optimization backlog — after stable DX11 and new profiles

| ID | Task |
| --- | --- |
| 09b.2 | Batch dynamic shadow geometry uploads |
| 09b.3 | Reuse geometry across stencil passes |
| 09b.4 | Reduce resource binding requests |
| 09b.5 | Group compatible shadow draws |
| 09b.6 | Comparative shadow optimization measurements |
| 10b–c | Avoid redundant debris transforms, batching (client only) |
| 11d | Reduce quad-worker overhead |
| 12c | Static shadow parallel preparation |

Reassess each against matched DX8/DX11 traces. Only advance an item early if it is a demonstrated migration dependency or urgent defect/performance regression. Preserve #71/#75 diagnostics; do not open DX8 optimization PRs solely from these profiles.

## After compatible DX11 — remaining GFX roadmap

| ID | Task | C/R |
| --- | --- | --- |
| 14 | Shader precompile/cache | 3/2 |
| 15 | GPU particles | 4/4 |
| 16 | Instancing | 4/3 |
| 17 | Soft shadows | 4/3 |
| 18 | MSAA/vegetation antialiasing | 3/2 |
| 19 | Dynamic lighting | 4/3 |
| 20 | Bloom | 3/2 |
| 21 | HDR/tone mapping | 3/2 |
| 22 | Ambient occlusion | 4/3 |
| 23 | Improved water | 4/3 |
| 24 | Water foam/trails | 4/3 |
| 25 | Volumetric smoke/toxins | 4/4 |
| 26 | Atmospheric haze | 3/2 |
| 27 | Weather | 4/3 |
| 28 | Visual building destruction | 4/4 |
| 29 | Battle marks | 3/2 |
| 30 | Movement interpolation | 4/4 |
| 31 | Independent render FPS | 5/4 |
| 32 | SIM/render separation | 5/5 |

These are planning candidates, not guarantees of donor feature availability or implementation compatibility. Revalidate scope and risk before starting. GFX 30–32 require a separate determinism design; never change the 30 Hz simulation or network/replay formats incidentally to a graphics migration.

## Review workflow

Use focused PRs with explicit dependencies and real validation status. Start from current main, keep unrelated SIM/INI/network work out, and do not auto-merge. In the initial DX11 compatibility milestone preserve existing rendering, assets, gameplay and editor behavior; add new effects only after the reference backend is stable.
