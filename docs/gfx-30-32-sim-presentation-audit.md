# GFX 30–32 — SIM/presentation boundary, migration-independent first slices

Source baseline: 669bdff725182da2b279bc8f7787a55ddab2f837 (#120). This is a representative source audit (Zero Hour with matching shared systems and both GameEngine update loops checked), not an exhaustive dependency inventory. GFX 13 stays primary; no dependency on this work is added to 13a–13e. The owner permits portable preparatory work in parallel. No speculative DX8-only optimization, scheduler change, separate thread, new renderer API or change of authoritative state in these first slices.

## 30a: existing timing and interpolation

| Source (baseline line) | What exists | Implication |
| --- | --- | --- |
| GeneralsMD/Code/GameEngine/Source/Common/GameEngine.cpp:788–859,868–899 | preUpdate sets pause/freeze; network readiness uses isFrameDataReady; regular mode conditionally uses a time accumulator clamped to one target step. Client updates and draws before network and the conditional logic update; step follows admitted logic if not frozen. | Partial cadence decoupling already exists, not independent threads or guaranteed 30 Hz. Preserve exact ordering including VERIFY_CRC. Do not introduce catch-up loops or change network tick admission in GFX. |
| Core/GameEngine/Include/Common/FramePacer.h; Source/Common/FramePacer.cpp:56–80,229–239 | Wall-time pacing, logic time scaling, getLogicFramePhase in [0,1]; reset after blocking operations. Phase advances by the existing ratio and caps at one; frozen and halted states affect time scale. | Reuse the existing phase; establish the exact previous/current sample convention per consumer. Test variable FPS, pause, stall, time scaling, load/reset and fast replay; avoid extrapolation. |
| GeneralsMD/Code/GameEngine/Source/GameClient/Drawable.cpp:1363–1395 | applyPhysicsXform interpolates previous/current pitch, roll, yaw and Z, handles objects without a previous sync result. | Do not replace this with a second interpolation system; audit world translation/orientation and render-only ownership separately. |
| Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DView.cpp:1581–1610,1646–1775 | stepView advances sharp camera shake on logic steps deliberately; camera lock reads live Object and orientation. | Do not smooth intended shake. Audit follow camera independently, keep target/click logic authoritative. |
| Core/GameEngineDevice/Source/W3DDevice/GameClient/Drawable/Draw/W3DModelDraw.cpp:2420–2425 | reads AI turret angles for model bones. | A visual turret sample could be interpolated only after auditing bone/muzzle queries used by gameplay. Never change simulation aim, muzzle origin or firing calculations. |
| Core/GameEngineDevice/Source/W3DDevice/Common/System/W3DRadar.cpp:65,610–727,1375–1378 | Overlay updates on GameClient::getFrame()%6; that is a SIM counter (GameClient.h:137; GameLogic.cpp:3739 sets it). Rendering reads live Object position and visibility and quantizes to a 128x128 grid. At high FPS it can rebuild multiple times at the same qualifying tick. | #122 separates visible presentation values from texture writes without changing cadence; later measure refresh and pixel quantization before smoothing. Do not delay removal of hidden objects, change the radar hit-test or add native API calls. |

## 32a: thread-boundary blockers

| Boundary | Evidence / risk | Required design before changing |
| --- | --- | --- |
| SIM → client | GameLogic.cpp calls Radar::newMap/refreshTerrain, GameClient::setFrame/destroyDrawable, TacticalView, ParticleSystemManager; object and render object lifecycle are coupled. | Inventory tick-time vs load/reset hooks; queue value events only when order and lifetime are defined; no raw Object pointers in render handoff. |
| client → SIM | GameClient::update uses player/shroud data, messages, input, UI, particle manager, display; Drawable::draw tests live Object state; W3DRadar::canRenderObject reads shroud, relationships, stealth. | Separate presentation-only data from authoritative queries; evaluate visibility on owning thread, retain only allowed data. Never interpolate hidden enemy positions. |
| SIM ↔ model bones | W3DModelDraw accesses AI and bone transforms; existing clientOnly naming is not a proof that all callers are client-only. | Complete call graph of muzzle/weapon/selection/FX/transform reads, including OPT turret/model condition work (#108/#114). Do not change shared simulation matrices or module logic. |
| display ↔ device | W3DDisplay::update:1885–1925 checks DX8 TestCooperativeLevel before updateViews because heightmap refresh needs device; WW3D::Sync follows hasUpdated. | Resolve with GFX 13 resource/ownership migration; don't move this function to a worker now or add a second device abstraction. |
| frame ordering / deterministic state | GameEngine::update VERIFY_CRC block surrounds radar/audio/client/messages/network; logic outside; step after logic. | Retain tick order, CRC/RNG streams, replay formats, INI and asset format. Verify CRC at corresponding ticks, not wall-clock frames. |
| particles and audio | GameClient.cpp:685–739 updates display before bone-attached particles; particle update gated by logic hasUpdated and freeze. | Audit every RNG/timer and gameplay consumer; do not move whole particle update to independent wall time. Audio and input ownership separate tasks. |

## Execution map (not a prerequisite of GFX 13)

| Stage | Work | State / gate |
| --- | --- | --- |
| 30a / 32a | Initial timing and boundary audit | This document; expand call graph before any general separation |
| 31a | Named Tracy scopes in both games | #121 code draft; collect measured baseline, compare counts and CRC |
| 30b.1 / 32b pilot | Value-only radar overlay scratch, same thread | #122 code draft; compare pixels/visibility/lifetime, measure extra CPU work |
| 30b.2 | Radar history and interpolation | After #122 accepted; stable IDs, visible samples, reset/discontinuity, pause/stall, no hidden position retention; test cadence independently |
| 30b.3 | Optional finer visual resolution / batched screen-space marker presentation | Only if profiling and visual evidence warrant; no backend-specific draw path or logical radar coordinate changes |
| 30c–f | World transform, turret/bone, visual projectile and camera follow interpolation | Audit actual existing paths, exclude authoritative matrices; one consumer per review |
| 31b–c | Audit visual timers and remove inappropriate render-rate dependencies | No changes to scheduler, speed, input command timing, network admission or logic RNG |
| 32b–c | Design immutable render-only object snapshots and same-thread consumers | Pilot after boundary audit; ID generations, creation/deletion, visible data, no SIM pointer retention, no serialized format changes; measure memory/time |
| 32d | Actual thread split and handoff | Defer until stable compatible DX11 and a complete ownership/call-order audit; bounded queues, publish/acquire synchronization, lifecycle, backpressure and replay/CRC gates |

Review gates: no API-specific duplicate implementation; retain old graphics behavior for baseline slices; four Win32 build targets, both CTest suites, offline/replay and multiplayer at different FPS caps, pause/resume, load/restart/observer transitions, visibility, CRC and RNG sequence. Avoid mixing unrelated OPT changes during measurement. No runtime or Win32 build performed for this audit. Estimates and future design are proposals, not implementation claims.
