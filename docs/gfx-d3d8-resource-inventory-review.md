# GFX 13b.1b — independent inventory review and acceptance gate

Review of the supplied `gfx-d3d8-resource-inventory.tsv` and `gfx-d3d8-resource-inventory.md` against source baseline `3d3ebe7c62d3c7113e0e59f155415818927a879a` and the accepted [resource contract](gfx-resource-contract.md) (#86). Main at review time was `2aa21a1ed608fff2b0b4f468f5fbf9ce0405a77c`; the GitHub comparison contains documentation changes only (AGENTS.md, resource contract, roadmap).

**Disposition: changes requested before declaring 13b.1b exhaustive.** The current inventory is a useful candidate register; retain it and its provenance. This document specifies corrections for the agent maintaining the local generator and inventory; it does not claim that the generator or inventory has been corrected or published.

## Verified

- 6,113 unique path/line pairs across 225 files, with no duplicate pairs.
- Scope counts: 5,911 CMake target; 161 excluded literal #if 0; 27 outside selected targets; 6 unresolved header reachability; 8 build configuration.
- For the 5,911 CMake-target rows only: g_generals 5,020 / 155 files; z_generals 5,034 / 156; g_worldbuilder 5,148 / 162; z_worldbuilder 5,155 / 163. Target membership overlaps. Including excluded rows changes these numbers; label the denominator explicitly.
- Dependency counts: 4,122 native or legacy API, 1,975 related resource operations, 16 Renderer handoffs. These are candidate-line counts, not verified runtime call counts or completion percentages.

## Required corrections

### A. Native method coverage (blocking)

A baseline source spot-check of [W3DMouse.cpp](https://github.com/prnnsn/GeneralsGameCode/blob/3d3ebe7c62d3c7113e0e59f155415818927a879a/Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DMouse.cpp#L390-L520) confirms direct device calls to ShowCursor and SetCursorPosition that are absent from the TSV. SetCursorProperties occurs at 419 and 514, but those rows identify only Peek_D3D_Surface. Detect the method itself, receiver type and containing operation; preserve multi-symbol rows. Cursor resource creation, animation, full-screen position, render/update-thread ownership and reset behavior need an explicit migration disposition (13b.3/4/5 as appropriate, with backend bring-up coordination). Do not call the native backend complete with these escapes unresolved.

Expand coverage beyond the current token list: derive/check native COM methods and aliases from the pinned SDK declarations; inspect calls via typed local/member pointers, getters, typedefs, wrappers and macros, and multiline calls. Independently cross-check creation, device/caps, swap chain, presentation, viewport, gamma, scene, render targets, readback, resource access and shader/state methods. Distinguish calls already registered through other symbols from absent rows. Absence in the register is not proof of source occurrence or absence. Document verified exceptions.

### B. Phase-specific classification (blocking for 13b.2a inputs)

2,220 rows have exactly the combined 13b.2/13b.3 planned_area. FVF and D3DFMT_INDEX16 are examples needing a specific primary owner, not blanket texture classification. Split vertex layout/FVF, index formats, texture formats, depth/stencil, and shared conversion tables; assign primary phase plus optional secondary dependencies. For each buffer consumer identify create/rewrite/bind/map/unmap/draw/release and recreation responsibility; retain the original lines and multiple genuine dependencies. Enumerate actual used vertex layouts, index width, stride/offset semantics and buffer update intent for 13b.2a. Do not change the approved resource contract or expand index limits.

### C. Evidence quality

1,800 rows have blank context, of which 1,157 are .cpp rows. Separate declarations/global scope from unknown/unparsed context, and distinguish declaration, definition, call, macro, build reference and non-graphics false positive. Preserve uncertain entries and reason/evidence rather than silently deleting them. Example: Babylon TransDB.h string locks (232, 305) are not evidence of a D3D buffer lock. Resolve the six unconfirmed header reachability cases using include/dependency evidence or explicitly justify deferral. Maintain separate scope and runtime reachability; a CMake source member is not proof of execution.

### D. Reproducible inventory and regression check

Publish the actual inventory generator (reported local inventory.cjs), versioned match/classification rules, invocation, pinned source and SDK revisions, CMake configuration/target graph provenance, exclusions and output schema. No machine-specific paths or private build artifacts. Regenerate the Markdown and TSV from the pinned baseline, deterministically sort path/line, verify line text against Git blobs, validate unique keys, scope/target/category totals and report-link integrity. Record a before/after delta (new, removed, reclassified, reason). Include a small regression fixture for typed aliases, cursor methods, multiline calls, guards, declarations and false positive locks. Check active configuration guards using build evidence; preserve optional branches for later review rather than guessing runtime coverage.

## Acceptance and scope

- Publish corrected report, TSV and generator together in a reviewable follow-up; link this review, #86 and the roadmap.
- Demonstrate all cursor misses above resolved, buffer/layout/format phase attribution corrected, reproducible regeneration and explicit exclusions/unknowns. Unknowns unrelated to 13b.2a may be deferred with owner, phase and evidence; do not block progress on an impossible claim of perfect runtime coverage.
- 13b.2a may proceed after its input coverage and classification are verified and the remaining issues are tracked; 13b.2b/2c and later resource migration must review their consumers before implementation.
- No game source, SIM, RNG, network, replay, asset, format, build option, or installed EXE changes in this documentation review. No build or runtime test claimed. Full independent source rescan was not performed; the current findings are register validation and a targeted source spot-check.
