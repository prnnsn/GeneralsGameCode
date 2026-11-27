# GFX 13b.3d — map ordinary 2D texture mips in the DX8 backend

The regular 2D texture task passes its borrowed resource to the renderer
when it needs to map or unmap mip levels. The DX8 backend obtains the native
level count, checks that it fits the task's existing arrays, and maps all
levels in order. The task retains ownership and receives CPU pointers and
row pitches. DXT uploads from 13b.3c still finish on the render thread. Other
regular 2D uploads also use this mapping and retain their copy paths.
Cube and volume textures keep their existing native mapping paths.

A failed native lock rolls back previous locks. If a successful native lock
returns an invalid pointer or pitch, rollback also releases that lock.
Unmapping attempts every acquired level and clears the published CPU
pointers and pitches even when the driver reports an error. A mapping or
unmapping failure routes through the texture loader's existing missing
texture handling. The worker never gets a partially mapped 2D texture.

The texture allocation, DDS/TGA formats, settings and load order are
unchanged. The renderer interface leaves resource ownership in the task
until the 2D texture lifetime itself is migrated.

## Validation

- Build both games and both World Builder targets for Win32 Release and run
  both Google Test suites. The DX8 mip mapping tests cover level capacity,
  pitch, lock flags and order, rollback, and cleanup after an unlock error.
- Start a daytime skirmish using the USA Air Force General. Build the
  Levitation Pad, then inspect its intact and severely damaged models close
  up and from a distance. Load a second match from the menu to exercise new
  mappings. Repeat at minimum texture quality after restarting the game.
  Watch for placeholders, a crash or a stall on the first appearance.

Revert the GFX 13b.3d patch and rebuild, or restore the backed-up GFX 13b.3c
executable with the game closed.
