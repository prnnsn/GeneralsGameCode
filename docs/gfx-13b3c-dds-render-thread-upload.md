# GFX 13b.3c — finish packed 2D DDS uploads on the render thread

The GFX 13b.3b loader already reads only the selected range of a matching
untinted DXT texture. GFX 13b.3c keeps those bytes and their relative mip
offsets in the load task until it reaches the foreground queue. The packed DDS
reader takes no native pointers. The load task checks all source ranges and
destination pitches before accepting the payload. At `End_Load` the render
thread copies the block rows, releases the CPU payload, unlocks the surfaces
and publishes the texture. A foreground demand load uses the same completion
path.

Surface allocation and lock timing are unchanged. The eligibility checks and
failure fallbacks from 13b.3b still apply. Texture conversion, tint, cube and
volume textures, and immediate surface loads use their existing paths.
Neither the DDS format nor INI settings change.

A 16 MiB threshold pauses new background reads while staged DDS results are
waiting for the render thread. One in-flight read may cross the threshold.
The existing thread queues carry the task and synchronize its handoff.
The threshold never prevents a synchronous foreground demand load from
finishing. The task releases its staged bytes on completion or detachment.

## Validation

- Build both games and both World Builder targets for Win32 Release. Run
  both Google Test suites. The new test covers a two-mip packed payload,
  an invalid second pitch, a short source, a missing destination, and row
  padding in a successful copy.
- In a daytime skirmish with the USA Air Force General, build the Levitation
  Pad and inspect its intact and severely damaged models at close and distant
  camera positions. The models use `ABAFSW.dds` and `ABAFSW_E.dds`.
  Compare the first appearance and the result after returning to the menu
  and loading another match. Repeat after restarting the game at the minimum
  texture quality. Look for missing or corrupted textures or a loading stall.

To restore the previous behavior, revert the GFX 13b.3c patch and rebuild,
or restore the backed-up GFX 13b.3b executable with the game closed.
