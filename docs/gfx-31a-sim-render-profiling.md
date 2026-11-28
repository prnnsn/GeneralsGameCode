# GFX 31a — SIM and presentation timing baseline

Base: 669bdff725182da2b279bc8f7787a55ddab2f837. The existing GameLogic::update and GameClient::update already have Tracy zones; this patch adds named call-site scopes to identify their boundaries, not duplicate frame marks. Both games use the same scopes. No graphics API, timing, RNG, CRC, network readiness, message order, scheduler, frame limiter or simulation changes.

- GFX31.EngineUpdate is the whole engine update, excluding the subsequent FramePacer wait.
- GFX31.LogicUpdate measures the actual logic update only when the existing readiness check admits it. preUpdate and readiness checks are outside this scope.
- GFX31.ClientUpdate includes display update/draw and other client work; do not add its nested DisplayUpdate/DisplayDraw timings to its total. DisplayDraw includes actual rendering/present and early return cases, not pure GPU time.
- RadarUpdate, AudioUpdate, MessagePropagation, NetworkUpdate and ClientStep report separate phases. VERIFY_CRC stays in its original scope; no changes to call order.
- Use Tracy with call stacks disabled for the legacy mod dbghelp when required. Capture identical offline replay scenes, normal/heavy battle, 30/60/120 FPS caps if supported, plus multiplayer stalls and minimized window. Compare counts and per-call/per-frame distributions (median/P95/P99); report logic calls per wall second and render frames separately; distinguish profiler overhead and GPU limits. Baseline should be obtained with #120 plus these zones before performance work. No FPS benefit claimed.
- Build both Win32 games and World Builders with profiling on/off, run both CTest suites, compare replay CRC and tick counts against the uninstrumented baseline. No local Win32 build or replay execution performed by this author; static source check only.
- Keep GFX 13 primary. This instrumentation has no DX8/DX11 dependency and remains useful after migration.
