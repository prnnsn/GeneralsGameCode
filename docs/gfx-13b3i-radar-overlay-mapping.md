# GFX 13b.3i — rebuild the radar object overlay in one mapping

The radar acquires one checked 2D mip mapping, clears its useful row bytes,
and writes the common object list followed by the local object list. The
mapping closes before the overlay is used for drawing. The refresh is
skipped if the mip cannot be mapped. The terrain and shroud keep their
separate update paths.

Blips still cover the same 2x2 pixels and use the current native format,
object visibility rules and stealth alpha. At the edge, the raster is
clipped to the radar dimensions and the actual mapped surface. The same
row pitch is used for clearing and drawing.

## Validation

Build both games and both World Builder targets as Win32 Release and run
both Google Test suites. Compare the old per-pixel boundary check with the
clipped rectangle at every in-bounds point and just outside each border.

In the mod, move two friendly units to different parts of the minimap,
including the map edge. Confirm their blips move, retain their color and
leave no trail. Reveal an opponent near the edge of your vision and then
withdraw. Check that visible opponent markers disappear when the targets
go back into fog. Change maps in the same session and check the new overlay.

To roll back, revert the GFX 13b.3i patch and rebuild, or restore the
backed-up GFX 13b.3h executable with the game closed.
