# [Perf][C2/5][R2/5] Skip unrelated Behavior modules during upgrades

Design only; no engine changes yet.

## Scope
Object.cpp: updateUpgradeModules, forceRefreshSubObjectUpgradeStatus; Player.cpp: onUpgradeCompleted; UpgradeModule.cpp.

## Implementation
Use ordered upgrade interface index; skip objects with no upgrade module. Preserve player/object mask union, already-upgraded, conflicts/activation, removal before activation, construction/capture/death and disguise refresh. Do not prefilter by newly completed upgrade until dependencies proven. Coordinate infrastructure.

## Acceptance tests
Test global/unit upgrades, AND/ANY, conflicts/removal, capture, construction, disguise, save/load, mass upgrades.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
