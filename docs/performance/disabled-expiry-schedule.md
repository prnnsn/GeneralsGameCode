# [Perf][C4/5][R3/5] Schedule disabled-status expiration checks

Design only; no engine changes yet.

## Scope
Object.cpp: disabled timer mutations, checkDisabledStatus; GameLogic.cpp: end-of-frame cleanup.

## Implementation
Investigate earliest expiry per object or deterministic queue. Preserve frame phase, object/type order, FOREVER/NEVER and reentrant callbacks. Invalidate on every change, load/reset/destruction. Benchmark separately from #51 (disabled membership); no new event order.

## Acceptance tests
Test expiry boundaries, extension/reduction, forever, simultaneous expiry, reentrancy, wraparound and differential CRC/replay.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
