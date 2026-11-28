# [Perf][C2/5][R2/5] Iterate occupied TurretAI slots

Design only; no engine changes yet.

## Scope
AIUpdate.cpp: constructor, update, getWhichTurretForWeaponSlot, isWeaponSlotOnTurretAndAimingAtTarget, getWhichTurretForCurWeapon.

## Implementation
Benchmark small MAX_TURRETS first. Use derived occupied indices or mask; preserve original order, slot identities, first-match precedence and serialized state. Support original and eight weapon slots independently.

## Acceptance tests
Test zero/one/multiple turrets, linked turrets, multiple weapon slots, target transfer, disabled/dead, #32 and #33 combinations.

## Invariants
No gameplay/limit changes. Preserve SIM order, RNG, CRC/Xfer layout, saves and replay; derived state rebuilt after load. Benchmark CPU and memory, compare per-frame CRC and RNG on identical Invasion Panel workloads. AI-generated draft for review in this fork, not upstream.
