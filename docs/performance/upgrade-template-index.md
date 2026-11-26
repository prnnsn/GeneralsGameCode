# [Perf][C2/5][R1/5] Index UpgradeTemplate lookups by name key

Design only; no engine changes yet.

## Scope
Upgrade.cpp: findUpgradeByKey, findNonConstUpgradeByKey, linkUpgrade, unlinkUpgrade; Upgrade.h.

## Implementation
Maintain derived index synchronized with authoritative linked list. Preserve first match, duplicate semantics, template mask allocation, enumeration, reset and teardown.

## Acceptance tests
Test default upgrade, duplicate names, unlink/reset, 512 upgrade bits, World Builder and save/load.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
