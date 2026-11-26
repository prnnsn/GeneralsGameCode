# [Perf][C2/5][R2/5] Index CommandSet and CommandButton names

Design only; no engine changes yet.

## Scope
Core ControlBar.cpp: findNonConstCommandSet, findNonConstCommandButton, new/override/reset; ControlBar header.

## Implementation
Maintain exact-name base lookup alongside linked lists; preserve final override resolution and insertion order. Invalidate on deletion/reset; no slot numbering changes.

## Acceptance tests
Test duplicates, override/reset, AI and scripts, command upgrades, World Builder, multi-selection.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
