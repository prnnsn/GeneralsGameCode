# [Perf][C2/5][R2/5] Iterate only collision-capable modules

Design only; no engine changes yet.

## Scope
Object.cpp: onCollide, isSalvageCrate.

## Implementation
Use ordered derived collision interface list. Preserve per-callback OBJECT_STATUS_NO_COLLISIONS checks and original callbacks/order, including state mutation. No physics changes. Coordinate shared infrastructure; avoid duplicated caches.

## Acceptance tests
Test projectiles, mines, salvage, collision disabled by earlier callback, multiple collide modules and replay CRC.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
