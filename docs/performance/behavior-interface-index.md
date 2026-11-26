# [Perf][C3/5][R2/5] Design shared ordered Behavior interface indexes

Design only; no engine changes yet.

## Scope
Object.cpp: construction, initialization, teardown; Object.h.

## Implementation
Design compact derived interface indexes shared by collision, upgrade and SpecialPower consumers. Conditional helper layout may differ per object; support multiple interfaces per module, preserve order and partial construction/destruction fallback. Benchmark per-object memory. No consumer migration in infrastructure change.

## Acceptance tests
Test conditional helpers, multiple interfaces, object destruction, overrides, save/load and memory.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
