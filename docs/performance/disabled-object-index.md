# [Perf][C3/5][R3/5] Avoid global scan for temporarily disabled objects

Design only; no engine changes yet.

## Scope
GameLogic.cpp: end-of-frame disabled scan; Object.cpp: setDisabledUntil, clearDisabled, checkDisabledStatus, Xfer and destruction.

## Implementation
Derived disabled-object membership, preserving original m_objList traversal order and additions/removals caused by earlier callbacks in same frame. No unordered iteration. Rebuild on load/reset, lifetime safe. Keep original scan for differential testing. Independent of #37.

## Acceptance tests
Test thousands of objects, none disabled, overlapping effects, callbacks modifying other objects, destruction/recreation, ownership, save/load and per-frame CRC.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
