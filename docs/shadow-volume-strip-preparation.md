# GFX 12a static shadow-volume strip preparation

Generals and Zero Hour now use a shared CPU routine to order silhouette edges into strips and calculate the vertex and triangle counts for static volume shadows. The existing prepass now has explicit inputs and outputs and can be verified independently of D3D8. It prepares the shadow silhouette before either video buffer is allocated or locked.

`Core/GameEngineDevice/Include/W3DDevice/GameClient/W3DShadowVolumeStrips.h` contains the strip preparation. Both games call it from `W3DVolumetricShadow::constructVolumeVB`. The vertex and index writes, stencil drawing, and dynamic volume path use their existing code. The pass is serial. Game INI, assets, saves, replays, and network formats are unchanged.

## Test in Zero Hour

1. Build and run the Win32 Release target `z_generals` with volume shadows enabled. Use a skirmish or replay with both static buildings and moving vehicles. Compare the building shadows with the previous build at the same game time and camera angle.
2. Pan and rotate around the structures, zoom out to overview and back, and watch for missing, flipped, or flickering silhouettes. Keep an animated or airborne unit in view to cover the existing dynamic path.
3. Switch away from the game and return in the same match, then start another match. Check that shadows appear as the camera returns to the scene.

## Profile volume shadow rebuilds (GFX 12b)

Both versions of `W3DVolumetricShadow.cpp` record Tracy CPU zones inside the existing shadow pass. This measures how much frame time goes into rebuilding visible shadow meshes and how much of that time is spent on preparation before any static D3D8 buffer work. The same geometry, cache decisions, and submission order apply to both builds. Game data and replay formats are unaffected.

| Tracy zone | What it measures |
| --- | --- |
| `W3DVolumetricShadowManager::RenderShadows` | Elapsed render-thread time for the volume shadow pass, including D3D8 calls and stencil composition. |
| `W3DVolumetricShadow::RebuildVisibleMesh` | A visible mesh rebuild after the light or transform invalidates its volume. |
| `W3DVolumetricShadow::BuildSilhouette` | CPU silhouette generation within a rebuild. |
| `W3DVolumetricShadow::ConstructDynamicVolume` | Dynamic geometry generation before later D3D8 upload. |
| `W3DVolumetricShadow::ConstructStaticVolume` | Static volume construction, including strip preparation, buffer allocation, and writes. |
| `W3DVolumetricShadow::PrepareStaticStrips` | CPU ordering and sizing of static strips. This zone ends before the buffer slots are allocated or locked. |

Build a Win32 Release profile as described in [GFX 11b](render-quad-depth-workers.md#profile-worker-participation-gfx-11b). Capture 60–90 seconds of a normal-speed replay with 3D volume shadows enabled, including newly visible buildings and moving or turning vehicles. Include a busy stretch at a fixed height and camera angle to compare per-frame costs. Pan to bring a new group into view and check the rendered shadows at both normal and overview zoom.

Use the count and total time of `RebuildVisibleMesh` zones per frame to judge how often cached shadows need work. Compare its busiest frames with the whole pass. The silhouette and construction zones are nested inside rebuilds, and `PrepareStaticStrips` is nested inside static construction. A frame can display cached shadows without recording any rebuilds. These timings measure CPU elapsed time and may include driver waits.

## Profile shadow pass requests (GFX 09b.1)

`Core/GameEngineDevice/Include/W3DDevice/GameClient/W3DShadowPassProfile.h` adds shared Tracy counters to both `W3DVolumetricShadow.cpp` variants. This builds on GFX 12b to measure the draw, state, binding and dynamic buffer activity in the rest of the shadow pass. The patch preserves the original D3D8 call order, geometry, stencil operations and game formats.

With Tracy connected, each invocation publishes nine `ShadowPass/` samples after `RenderShadows` ends. Draw requests include indexed shadow geometry and the two-triangle stencil overlay when submitted. State and binding counts cover the instrumented calls. The dynamic buffer plots count requests by lock flag, failures, and requested bytes of successful locks. Use these measurements as CPU-side workload counts. GPU completion and bandwidth require GPU measurements.

For the default capture, connect Tracy 0.13.1 and record a normal-speed replay with volume shadows and a fixed camera over buildings and moving units. Check that the `ShadowPass/` plots update as casters enter view. Set `GENERALS_PROFILE_SHADOW_CALLS=1` before launching again to add the `ShadowPass::DrawCall`, `ShadowPass::DynamicBufferLock` and `ShadowPass::DynamicBufferUnlock` timing zones. Capture a short interval in this mode. Compare CPU times between captures taken with the same instrumentation and replay conditions.

Clear `GENERALS_PROFILE_SHADOW_CALLS` and restart to return to the per-pass counters. Build with `RTS_BUILD_OPTION_PROFILE_TRACY=OFF` to remove the instrumentation, or restore the previous executable for a full rollback.

## Roll back

Restore the previous executable, or revert the GFX 12a commit and rebuild.

For GFX 12b alone, build with `RTS_BUILD_OPTION_PROFILE_TRACY=OFF` to disable its profiling zones, or revert the GFX 12b commit.
