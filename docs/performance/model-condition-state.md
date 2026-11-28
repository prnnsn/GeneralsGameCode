# [Perf][C3/5][R2/5] Measure ModelConditionState lookup and redundant transitions

Design only; no engine changes yet.

## Scope
Core W3DModelDraw.cpp: findBestInfo and condition transitions; SparseMatchFinder.

## Implementation
Audit existing m_conditionStateMap and measure cache hit rates and transition costs in opt-in profiling before adding cache. Preserve animation re-entry side effects, model changes, animation resets, damage transitions. Keep client-only changes isolated.

## Acceptance tests
Test construction, damage, firing, movement, upgrades, disguise, model rebuild, camera/UI and CPU frame times.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
