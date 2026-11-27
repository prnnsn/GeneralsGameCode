# [Perf][C2/5][R2/5] LocomotorSet lookup optimization

Design only; no engine changes yet.

## Scope
Locomotor.cpp: findLocomotor, addLocomotor, clear, xfer.

## Implementation
Cache exact surface-mask queries, including negative results. Preserve first matching locomotor in vector order, including mixed masks. Invalidate on add/clear/load. Existing serialized aggregate surface mask is not sufficient without auditing legacy saves.

## Acceptance tests
Test empty, ground/air/water, amphibious, overlapping masks, overrides, locomotor upgrades and save/load. Profile before adding memory.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
