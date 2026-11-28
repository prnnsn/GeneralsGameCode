# [Perf][C2/5][R2/5] Skip empty CommandSet slots in AI queries

Design only; no engine changes yet.

## Scope
AIUpdate.cpp: privateCommandButton, privateCommandButtonPosition, privateCommandButtonObject; CommandSet parser.

## Implementation
Track occupied slots from authoritative data. Iterate ascending original slot order; preserve duplicate buttons, action counts, exact identity and slot numbers. Rebuild on parse/copy/override, independent of name index.

## Acceptance tests
Test sparse/empty sets, duplicates, mixed unit teams, scripted commands, upgrades and #33.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
