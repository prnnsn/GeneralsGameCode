# [Perf][C2/5][R2/5] Optimize Object::findModule name-key lookup

Design only; no engine changes yet.

## Scope
Object.cpp: findModule, construction/destruction; Object.h.

## Implementation
Use derived key-to-index lookup only while stable. Preserve first match and INTENSE_DEBUG duplicate diagnostics. Retain original scan during partial construction/teardown, including null sentinel behavior. No pointer ordering or serialized index.

## Acceptance tests
Test missing/duplicate keys, conditional helpers, destruction/reentrant lookup, map overrides and memory/call cost.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
