# GFX roadmap — DX11 first

Owner decision: 2026-11-27. Follow [AGENTS.md](../AGENTS.md) and the existing contribution policies.
This is a planning snapshot, not a claim that untested features work. Refresh main, PRs and profiles before choosing work.
Baseline main for original snapshot: `f01d1eb38654cdcef3eefbf750f7f1d7ed911d80`; delivery audit: `669bdff725182da2b279bc8f7787a55ddab2f837`.
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
| 09b.1 | Shadow-pass counters | Open #75; paused until critical need; retain baseline |
| 10a | Debris profiling | Open #71; paused until critical need; retain diagnostic work |
| 11–11c | Quad-depth workers, settings and profiling | Merged #18, #27, #30; benefit not established |
| 12a–b | Shared static strip prep and shadow rebuild profiling | Merged #28, #70; still serial |
| 13a.1–4 | Material/draw abstraction, roads/scorch, texture addressing, buffer binding | Merged #31, #72–74 |
| 13a.5, 13a.6a, 13a.7 | Filters, scoped terrain bias, road/scorch bindings | Merged #83–85 |
| 13b.1a–b | Resource contract and reproducible corrected inventory | Merged #86, #87, #91 |
| 13b.2a–c | Buffer descriptors, first non-streaming and streaming consumers | Merged #88–90; remaining consumers pending |
| 13b.3a–b | Checked 2D layout and eligible packed DDS path | Merged #92 |
| 13b.3c | CPU-owned packed DDS staging; render-thread completion (soft 16 MiB pending threshold) | Merged #97 |
| 13b.3d | Ordinary 2D all-mip map/unmap through Renderer/DX8, with rollback | Merged #99; task still owns borrowed native resource |
| 13b.2 documentation | Complete buffer adapter implementation notes | Merged #98 |
| 13b.3e–f | Regular 2D allocation/handoff and mip surface acquisition/description | Merged #101–102 |
| 13b.3g–i | Radar mip clear, shroud mapping and object overlay in a single mapping | Merged #103, #105, #107; 288 tests reported per game |

The reports in #30 and #70 do not justify lowering the default quad worker threshold or parallelizing static shadow strips. #75 identifies small dynamic buffer requests as a candidate for optimization after DX11, not a migration prerequisite.

## Phase A — renderer abstraction (highest priority)

Keep the existing DX8 behavior, state-cache semantics and resource lifetime during each slice. Do not optimize just because a caller is being migrated.

| ID | Scope and acceptance | C/R | Dependencies / status |
| --- | --- | --- | --- |
| 13a.5 | Typed min/mag/mip filters, anisotropy, capability queries; preserve all legacy fallbacks | 3/2 | #73; merged #83 |
| 13a.6a | Scoped terrain depth bias backend operation; preserve capability, cache checks and restoration | 2/2 | #23/#29; merged #85 |
| 13a.7 | Migrate remaining road/scorch buffer binding through existing interface | 2/1 | #72/#74; merged #84 |
| 13a.6b | Typed depth comparison/write/test and blend states; migrate shader consumer; preserve cached vs immediate semantics | 3/3 | #117 merged |
| 13a.6c | Stencil, cull and alpha semantics; both shadow paths; remaining color-mask/global cull follow-ups | 4/3 | Merged #117; partial consumer migration |
| 13a.8 | Fixed-function shader texture combine and arguments, including untracked stage 2; remaining coordinate transforms and other consumers | 4/3 | #118 head already contained in main (compare: 0 ahead); open PR is stale, no duplicate implementation |
| 13a.9 | Indexed draw route/strip API and first consumer batch; remaining raw-device and immediate primitives pending | 3/3 | Merged #119; coordinate remaining Renderer changes |
| 13a.10 | Inventory remaining DX8 dependencies across both titles/editors; ensure an eventual native backend has no hidden direct device access | 3/2 | Continuous; required before full backend selection |

The merged Phase A slices do not complete all draw/state abstractions; retain only residual consumers and the 13a.10 inventory, not already integrated slices.

## Phase B — resource abstraction

| ID | Scope and acceptance | C/R |
| --- | --- | --- |
| 13b.1 | Inventory resource ownership, refcounts, formats, usage flags, pools, lock/unlock, reset and native-handle escape paths; design neutral resource descriptors and lifetime contract. Contract #86 and corrected inventory #91 merged; continue tracking unresolved native dependencies. | 3/2 |
| 13b.2 | Vertex/index resource creation and update API; DX8 reference adapter; retain CPU sorting pages and 16-bit indexing | 4/3 |
| 13b.3 | Texture/surface creation, mip levels, upload, DDS formats and readback; retain legacy fallback behavior | 4/3 |
| 13b.4 | Render targets, depth/stencil, viewport, copies, screenshots and device-reset/resource recreation | 4/4 |
| 13b.5 | Migrate remaining resource consumers in both titles and editors; compile/reference tests for ownership and recovery | 4/3 |

Implement resource interfaces only after lifetime and format semantics are reviewed. Existing W3D resources are still DX8-backed; forwarding methods alone are not enough.

### 13b.3 — canonical completed numbering and next code slices

| ID | Scope | C/R | Dependency / state |
| --- | --- | --- | --- |
| 13b.3a–b | Checked 2D layout and eligible packed DDS | 3/3 | Merged #92 |
| 13b.3c | CPU-owned packed DDS staging; render-thread completion | 3/3 | Merged #97 |
| 13b.3d | Ordinary all-mip map/unmap and failure rollback | 3/3 | Merged #99 |
| 13b.3e | Regular 2D texture task allocation and handoff | 4/3 | Merged #101 |
| 13b.3f | Retained mip surface query and descriptions | 3/2 | Merged #102 |
| 13b.3g | Radar mip clear | 3/2 | Merged #103 |
| 13b.3h | Single-mip checked map; radar shroud writes | 3/3 | Merged #105 |
| 13b.3i | Single-mapping radar object overlay | 3/3 | Merged #107 |
| 13b.3j | Checked pitch-aware terrain atlas | 3/3 | #94 code draft, depends on #105 |
| 13b.3k | Checked cube face and volume slice CPU layout | 3/2 | Merged #115 |
| 13b.3l | Remaining 2D CPU staging/ownership and SurfaceClass lock/access paths | 4/3 | Planned; inspect residuals, do not duplicate #101–105 |
| 13b.3m | Cube/volume actual loaders and native ownership, subresource/face/slice staging | 4/4 | Planned, after #115 |
| 13b.3n | Remaining 2D/surface consumers, partial rects, format and native escape audit | 4/3 | Planned; coordinate 13b.4/5 |
| 13b.4a | Logical color/depth/viewport descriptors and checked single-sample footprint | 3/2 | Merged #116 |

Previous documentation briefs #93 (retained surface access) and #100 (ordinary 2D creation/ownership) overlap the merged #101/#102/#105. Do not implement them as originally written; keep only residual requirements in 13b.3l/n. #94 now contains implementation 13b.3j. Preserve canonical numbering of merged PRs. New drafts are AI-assisted internal fork candidates awaiting local review and Win32 validation.

### Execution lanes and review gates

- Device: #95 implements an isolated DX11 smoke executable, independent of the game. Review/build/run on Windows hardware and WARP; do not select it in gameplay. After acceptance, 13c.2 buffer slice can use #88 contracts, then texture/target slices after ownership contracts.
- Resources: #94 uses #105; #115/#116 descriptor-only code is integrated. Continue actual resource migrations after local review. Preserve DX8 baseline, W3D identity and all unconverted paths. Split 13b.4 into targets/depth, copy/readback/editor previews, reset/recreation/cursor; then 13b.5 remaining VB/IB, textures and escape audit.
- States: #117/#119 merged; #118 head is already contained in main. Reconcile only remaining Renderer header/backend consumers. Preserve original call order and cache invalidation. No new visual effects.
- Host checks: #94 passed 80 isolated atlas combinations; #115 passed 6,285 volume and 198 cube cases; #116 seven depth formats and boundary cases. These are hash-verified host harnesses, descriptor harness uses minimal format stubs. Not game builds or native GPU tests. #117–119 original static validation superseded by the combined #120 integration evidence (four Win32 Release builds, 294 tests per game, reported mod gameplay checks); #95 Windows build/runtime pending. Main #107 reported four successful builds, 288 tests per game and game checks, not integrated-candidate results or CRC.
- Refresh current main before applying; merge nothing automatically. #71/#75 remain deferred.

## Phase C — actual DX11 backend

| ID | Scope and acceptance | C/R |
| --- | --- | --- |
| 13c.1 | Isolated Windows DX11 device/context/swap-chain smoke; hardware/WARP, depth, readback, resize/minimize/restore | 3/3 |
| 13c.2 | Implement neutral resource descriptors in DX11, buffer Map/Unmap and texture/surface lifetime; validate uploads and reset paths | 4/4 |
| 13d.1 | Translate fixed-function vertex formats, transforms, lights, fog, material and alpha/texture-stage semantics into shaders | 5/4 |
| 13d.2 | DX11 depth/stencil, rasterizer, blend and sampler states; cache by full descriptors, preserve legacy pass order | 4/4 |
| 13e.1 | Bring up terrain, meshes, particles, decals, shadows, water, UI, fonts/minimap and editor features incrementally | 5/5 |
| 13e.2 | Explicit backend selection/build integration, fallback or clear startup error, resize/fullscreen, device removal, legacy asset tests | 4/4 |
| 13e.3 | Compatibility and performance gate: both game/editor variants, replay/save/network CRC and RNG, matching screenshots, frame-time/CPU/GPU captures, and stable extended zoom | 4/4 |

#95 contains the standalone 13c.1 code candidate; Windows compilation and device tests are pending. A standalone device test is not a playable backend. Avoid promising DX11 FPS improvements or feature parity before 13e.3. Do not ship a partially migrated backend as the default. DX11 work may use Reforged as a reference, but its D3D9-era semantics and resource paths need explicit adaptation to our DX8 baseline. Check source/license and attribute any borrowed code.

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

Reassess each against matched DX8/DX11 traces. Only advance an item early if it is a demonstrated migration dependency or urgent defect/performance regression. Preserve #71/#75 diagnostics and keep both PRs open, but resume work only for a critical defect, severe regression, demonstrated migration dependency, or explicit owner reprioritization. Do not open DX8 optimization PRs solely from these profiles.

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

## Delivery batching — 2026-11-28

- #115, #116, #117, #119 confirmed merged; #118 is still open against the old #117 branch, but its head is already an ancestor of main (0 commits ahead). Do not rebuild or reapply #118; reconcile its metadata with reviewer separately. #96 is the roadmap review, #93/#100 superseded briefs are references only.
- Phase A residuals: combine inventory/doc-only changes without game compilation; batch small, semantically equivalent state/consumer migrations into one draft per existing renderer interface when all preserve the same call order/cache semantics and share the same trace/visual tests. Keep separate commits per consumer. Do not combine state semantics with resource ownership, reset or backend migration.
- Phase B: group descriptor-only CPU validation with its focused unit tests where no native lifetime changes occur; group proven equivalent ordinary 2D call-site conversions only once mapping/ownership contract is accepted. Keep terrain atlas #94, cube/volume native loader ownership, targets/reset/recreation and native escape changes isolated because failure modes differ. Never rerun completed descriptor PRs as new work.
- DX11 device smoke #95 stays independent; buffer, texture and target/reset milestones remain separate until validated. Do not mix game backend switch with new effects.
- GFX 30–32: #123 is documentary audit; #121 is independent profiling baseline; #122 is same-thread radar value capture. Keep #121 and #122 separate code reviews; reuse a baseline and one integrated comparison session only after capturing the pre-change measurements and passing individual gates. Do not combine snapshot lifetime, interpolation or worker/thread changes into one PR.
- #71/#75 remain paused; no extra work. No new merge into main or test claim. Preserve draft review gates and explicit dependencies; only one full build per safe thematic integration candidate where feasible, tests per slice still mandatory. Split when risk, test isolation or rollback worsens.

## GFX umbrella issues — owner decision 2026-11-28

Track requirements and acceptance separately from delivery PRs. [#124](https://github.com/prnnsn/GeneralsGameCode/issues/124) covers 13a–13b Renderer abstraction; [#125](https://github.com/prnnsn/GeneralsGameCode/issues/125) covers 13c–13e DX11 backend; [#126](https://github.com/prnnsn/GeneralsGameCode/issues/126) covers 01–12 performance (remaining nonurgent work deferred); [#127](https://github.com/prnnsn/GeneralsGameCode/issues/127) covers 14–29 visual/graphics improvements (after stable DX11); [#128](https://github.com/prnnsn/GeneralsGameCode/issues/128) covers 30–32 SIM/presentation separation. #120 integrated #115–119, including #118, and reports four successful Win32 Release builds, 294 CTest cases per game, and a mod gameplay check; #94/#95 remain separate. Link future implementation PRs to the appropriate issue without automatic closing keywords. Keep checkbox completion distinct from runtime acceptance; no duplicate implementation or artificial milestones/deadlines.
