# GFX 13b.3h — scoped writes for radar shroud

The DX8 backend exposes a checked write mapping for one ordinary 2D
mip. It verifies the native format, dimensions, pointer and pitch before
accepting the map. The caller gets a linear pixel view. Finishing or
destroying the mapping releases the lock and the backend surface
reference. The renderer's existing mip clear uses the same mapping.

Radar uses one temporary mapping for a single shroud update, or one
mapping for a full refresh. If a batch cannot map its mip, the radar
skips the batch. Map switches and resource teardown close any mapping
before releasing the radar texture. The existing world-to-radar conversion,
inclusive rectangles, alpha values 0, 127 and 255, and native color packing
still apply.

## Validation

Build both games and both World Builder targets in Win32 Release and run
both Google Test suites. New pixel tests cover inclusive rectangles, the
16-bit color layout, row padding, and rejection of out-of-range or short
views before any writes.

In a skirmish with radar enabled, observe an unexplored area on the
minimap. Move a unit into it, confirm the explored patch opens, and move
away so the area becomes fogged. Save and reload the battle to exercise a
full shroud refresh. Start a new map in the same session and confirm the
shroud follows the new battle.

Revert the GFX 13b.3h patch and rebuild, or restore the backed-up GFX
13b.3g executable with the game closed.
