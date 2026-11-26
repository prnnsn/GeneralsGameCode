# [Perf][C2/5][R1/5] Index SpecialPower templates by name

Design only; no engine changes yet.

## Scope
SpecialPower.cpp: findSpecialPowerTemplatePrivate, parseSpecialPowerDefinition, reset; SpecialPower.h.

## Implementation
Index base templates by exact AsciiString comparison semantics. Preserve vector order, overrides and IDs. Invalidate before deletion and rebuild after reset. Independent of #35.

## Acceptance tests
Test default, override-only entries, reset, missing names, case behavior, World Builder and #35 independently.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
