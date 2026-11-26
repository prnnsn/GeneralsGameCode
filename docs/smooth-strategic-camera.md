# Smooth strategic wheel camera (follow-up to #21)

Manual overview wheel zoom only. Logarithmic target, critically damped log-height spring, 66.7 ms elapsed-time cap, retained velocity on retarget. Valid map hit uses a fixed view-space anchor; solve XY at the same interpolated height, project onto map bounds once, and discard the anchor at the final height. Background or off-map terrain uses the current pivot. Full overview still converges to map center. Blend legacy and overview bounds near the old maximum. Terrain reference Z follows smoothly without the extra legacy XY displacement; terrain-only Z settling and necessary bound corrections may remain.

No new UI, simulation, protocol, RNG, replay format, graphics or terrain rendering changes. Legacy camera remains available via `-noCameraOverview`. Pan/rotation smoothing deferred. Risk: medium for camera geometry and constraints, low for isolated math.

The implementation is in `W3DView.cpp` and `W3DView.h`, with the wheel math in `StrategicCameraZoom.h`. It uses the base branch's eased transition between the standard and overview bounds. The same calculation handles the bounds during both normal and strategic camera updates. Strategic wheel input retains its log-height spring and clears the fixed anchor when the cursor moves over invalid terrain or leaves the viewport.

## Review and validation
- GoogleTest cases added for time subdivision, retarget, settling, reciprocal steps, bounds and stall cap. Native build and gameplay tests are not claimed.
- Test on flat and mountainous maps: single/burst/reversed wheel, stop at both limits, four edges/corners, cursor over background, crossing between map and background mid-zoom. Check no stale-anchor pull or independent XY tail.
- Test square/rectangular maps, 16:9/21:9/32:9; check full overview centering and legacy height threshold.
- Compare 30/60/120 FPS, pause and stalled frame. Verify picking, construction, radar, fog, camera lock, shake, scripted cameras, old replays and two-client CRC.
- Review after #21; merge only after native build and in-game testing.

## Rollback

To remove the follow-up after integration, revert the commit that introduces PR #22 and rebuild `z_generals`. For a merge commit, use `git revert -m 1 <merge-commit>` on the destination branch.
