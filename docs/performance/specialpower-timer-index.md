# [Perf][C2/5][R2/5] Index shared SpecialPower cooldown timers

Design only; no engine changes yet.

## Scope
Player.cpp: addNewSharedSpecialPowerTimer, resetOrStartSpecialPowerReadyFrame, expressSpecialPowerReadyFrame, getOrStartSpecialPowerReadyFrame, Xfer.

## Implementation
Derived ID-to-position index alongside authoritative sequence. Preserve insertion order, exact readyFrame and missing-ID initialization, reset/copy/load and serialization. Avoid invalidated iterators; no iteration over index for serialization.

## Acceptance tests
Test shared/individual cooldowns, missing timer, repeat reset, multiple players, save/load, map reset, replay.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
