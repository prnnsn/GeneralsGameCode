# Engine optimization roadmap (OPT)

Planning revision: 2026-11-28; audited main: `ceb024e`.
Existing PRs #32–52 are open drafts as checked for this revision. This document changes planning only; it does not integrate, close, or supersede their implementation branches.

## Stack plan

| Stack | PRs | Delivery and review |
| --- | --- | --- |
| OPT-A | #35, #40, #36, #39 | First combined implementation/integration stack, preliminary R1/5. Apply #35 before #40 (shared SpecialPower store); #36 and #39 independently. Separate commits, individual review and rollback; one combined validation pass. |
| OPT-B | #42, #43 | Conditional thematic stack. Validate name index and INI override lifecycle before slot iteration. Separate commits and individual tests. |
| OPT-C | #44, #45, #46, #48 | Shared Behavior infrastructure first (#44); each consumer remains independently reviewable. Avoid duplicate caches; preserve original callback order and lifetime semantics. |
| OPT-D | #37, #51, #52 | Keep distinct PRs. Finish local mask optimization, then evaluate disabled-object membership, then expiration scheduling. Check per-frame determinism separately at each stage. |
| Independent | #32, #34, #38, #41, #47, #49, #50 | Do not bundle into OPT-A. #32 and #34 have code and standalone harness results; engine tests outstanding. #50 is profiling first. |
| Feature (not optimization) | #33 | Keep eight-slot expansion independent; test both combinations with #32. |

## OPT-A implementation contract

- #35: retain original fallback for missing/non-sequential SpecialPower IDs.
- #40: exact name matching, preserve override chains, base vector order and reset semantics.
- #36: derived weapon-presence flag; audit template copying, inheritance, all mutation sites and map overrides.
- #39: derived UpgradeTemplate name-key index; preserve first match, linked-list enumeration, IDs and mask allocation.

Before assembling the stack, rebase against current main and review the original PR patches. Implement design-only #39/#40; verify #35/#36 code individually. Use one integration branch with separate focused commits and record original PR provenance. Do not mark the original PRs superseded or merge them until owner/reviewer approval; avoid double application. Build both game configurations where shared Core is affected and World Builder as appropriate. Compare per-frame CRC/RNG and replay behavior, save/load, map override/reset, CPU cost and memory against baseline. Reject changes without demonstrated benefit or correctness.

## Global gates

Preserve original SIM order, RNG calls, gameplay, limits, Xfer/CRC format, replay/network compatibility and INI/World Builder semantics. Derived indexes must be rebuilt or invalidated on load/reset, and never control nondeterministic iteration. Keep GFX 13/DX11 priority recorded in AGENTS.md; do not mix engine optimization stacks with renderer migration. All risk estimates are preliminary, not benchmark results. #32, #34–37 contain draft code; #38–52 contain design proposals. No engine build or runtime validation is claimed by this roadmap.

This is an AI-assisted planning document for the owner’s fork, not an upstream submission.
