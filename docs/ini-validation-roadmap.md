# INI Validation System — roadmap

Scope: Zero Hour (GeneralsMD) first; shared Core changes must build for both games. Integration order, not PR creation order. C = complexity, R = regression/determinism risk (1–5). All code changes in separate draft PRs for review; no automatic merges.

Baseline: main 1ca0688fb2ba8b04f45720f9fc37ad3e952c4061. Issues #53–68. Initial state: no INI validator implementation confirmed in main. Drafts: #76 (this roadmap), #77 (01a), #78 (01b.1, stacked on #77). Track actual merged commits, draft/closed PRs and test evidence, not merely issue closure.

## Contract

- IIV (EXE_STARTUP): only defects that cannot be repaired by the selected map, or required before map loading. Map-repairable dependencies are DEFERRED, not ERROR. A cached null that is never refreshed is not repairable merely because the target can be declared in map.ini.
- MIV (MATCH_LOADING): validate effective definitions for the selected map after map.ini/solo.ini and all relevant overrides, before first dependent use and simulation. If an early consumer runs during loading, validate its prerequisites before it runs. Validate map script references after deserialization, before execution.
- Every report entry explicitly identifies IIV/MIV; include source file/line, owner, module/tag/field/target, latest effective override if available, map context and session summary. Never invent source information.
- BIG provenance: report SourceType (BIG/LOOSE/UNKNOWN), BIGFile (actual archive name, including extension; relative archive path when available), and File (virtual INI path). Loose source: BIGFile=N/A; unavailable provenance: UNKNOWN. Capture the VFS source at the actual successful open, never infer it by a later lookup. Preserve separate source metadata for the effective erroneous field, its original definition and latest override where known. Missing targets have no invented archive; report the INI containing the reference. No change to VFS priority or mounting order. See #53, #56, #68.
- Diagnostics only until explicit runtime-fix stage. Do not mutate game state, RNG, name keys, IDs, masks, CRC, Xfer or network/replay formats. No per-frame checks. Preserve fatal parsing paths and existing accepted syntax. Do not run game objects/scripts to validate. Clear map metadata on unload/failure.
- Default compatibility behavior until Warn/Strict policy is reviewed. Strict cannot block an IIV DEFERRED reference. No late asymmetric match abort.

| ID | Scope | Issue | C | R | Depends on | Initial status |
|---|---|---|---|---|---|---|
| INI 01a | Diagnostic stage, severity, record formatting and unit tests | #53 | 1 | 1 | — | Draft #77 |
| INI 01b.1 | Isolated session collector, counts, sorted records | #53 | 2 | 1 | 01a | Draft #78 (stacked) |
| INI 01b.2 | Report writer, BIGFile and source metadata, session counters; provenance-aware deduplication | #53 | 2 | 1 | 01b.1,04a | Planned |
| INI 02a | Classify resolution timing and map-repairability per field | #54 | 2 | 1 | 01a | Planned |
| INI 04a | Source location, actual BIG/loose VFS provenance and typed reference registration | #56 | 3 | 2 | 01a | Planned |
| INI 04b | Effective reference tracking, per-field archive provenance, override removal and lifetime | #56 | 3 | 2 | 04a | Planned |
| INI 05a | Unknown fields and fatal parse diagnostic capture, no behavior change | #57 | 2 | 1 | 01b.2 | Planned |
| INI 05b | Numeric/duplicate rules with retail compatibility fixtures | #57 | 3 | 2 | 05a | Planned |
| INI 02b | Startup IIV entry/exit and deferred classification | #54 | 3 | 2 | 01b.2,02a,04a | Planned |
| INI 03a | MIV map-load context and lifecycle (A→B→A) | #55 | 3 | 2 | 01b.2,04b | Planned |
| INI 03b | Effective validation boundary before use/simulation | #55 | 3 | 2 | 02b,03a | Planned |
| INI 06a | GrantUpgradeCreate and explicit Upgrade references | #58 | 2 | 1 | 02b,03b,04b | Planned |
| INI 06b | UpgradeMuxData, TriggerAlt, cameo and other upgrade fields | #58 | 3 | 1 | 06a | Planned |
| INI 07a | CommandButton typed targets and required fields | #59 | 3 | 1 | 06a | Planned |
| INI 07b | CommandSet targets and command semantics | #59 | 3 | 1 | 07a | Planned |
| INI 08 | Object, Prerequisites, PlayerTemplate and Science | #60 | 3 | 1 | 03b,04b | Planned |
| INI 09a | Weapon and WeaponSet (3/8 slots as supported) | #61 | 3 | 1 | 04b,03b | Planned |
| INI 09b | Armor, ArmorSet and DamageFX | #61 | 2 | 1 | 04b,03b | Planned |
| INI 10a | OCL, Locomotor, SpecialPower | #62 | 3 | 2 | 08,09a | Planned |
| INI 10b | Behavior interface dependencies and AIData | #62 | 4 | 2 | 10a | Planned |
| INI 11a | FX/particles/mapped images/audio definitions | #63 | 3 | 1 | 04b,03b | Planned |
| INI 11b | W3D/texture/animation/bone/audio file checks | #63 | 4 | 1 | 11a | Planned |
| INI 12 | Map script static reference validation | #64 | 4 | 2 | 03b,08 | Planned |
| INI 13a | Legacy/Warn and session configuration | #65 | 2 | 1 | 01b.2,02b,03b | Planned |
| INI 13b | Strict / Validate Only, exit codes | #65 | 3 | 2 | 13a,16a | Planned |
| INI 16a | Integration fixtures, BIG/loose priority and map override regression tests | #68 | 3 | 1 | 02b,03b | Planned |
| INI 16b | Retail CRC/RNG/replay/network/save/load comparison | #68 | 3 | 1 | 16a | Planned |
| INI 14 | Separate runtime fail-closed fixes for malformed configurations | #66 | 3 | 3 | 06a,07a,08,16b | Planned |
| INI 15 | Optional measured lookup caching | #67 | 4 | 3 | 04b,06b,16b | Planned |

## Current draft review order

Review #76 (documentation) independently; review #77 before #78. #78 is based on the #77 branch intentionally and must be retargeted after #77 integration. Both code drafts are unbuilt and untested; no engine hooks are active. Continue with 02a and 04a after the core contract review; no automated merge.

## Review gates

1. Compile shared changes for Generals and Zero Hour, including legacy compatible configuration; run unit tests.
2. Verify wrong fields, missing required targets, explicit None, forward references, map-added target, map-replaced/removed module, repeated map, failed load, save/replay. Verify identical virtual paths across BIG archives, loose/BIG precedence, effective field override provenance, unknown origin and map unmount. No invented full coverage.
3. Compare correct data with diagnostics on/off: IDs, CRC, RNG sequence, replay results, network simulation; measure startup/load time and RAM.
4. Record test outcomes and link each PR. Do not claim merged or verified until evidence exists.
