# [Perf][C2/5][R2/5] Index SpecialPower-capable Behavior modules

Design only; no engine changes yet.

## Scope
Object.cpp: pauseAllSpecialPowers, getSpecialPowerModule, findSpecialPowerModuleInterface, findAnyShortcutSpecialPowerModuleInterface.

## Implementation
Use ordered derived interface list. Preserve SPECIAL_INVALID first match, template identity, shortcut flags, pauseCountdown and callback order. Do not cache mutable availability. Coordinate infrastructure #44 and #37.

## Acceptance tests
Test duplicate power types, missing powers, shortcuts, disable, overlapping EMP, ownership changes, recharge and save/load.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
