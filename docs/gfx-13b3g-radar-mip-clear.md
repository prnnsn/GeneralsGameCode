# GFX 13b.3g — clear radar texture mips through the renderer

Radar reset clears the terrain and object overlay textures through the
renderer. The periodic object-overlay refresh uses the same operation
before plotting markers. A failed clear skips that refresh. The shroud
keeps its separate transparency and update rules.

The DX8 backend acquires the native surface, confirms it has an
uncompressed pixel layout, and checks the locked pitch. It clears the
useful bytes in each row while preserving alignment padding. Every successful
lock receives an unlock attempt, including an invalid mapped view. The
native reference is released when the operation finishes. The existing
mip surface accessor shares that reference-acquisition code.

## Validation

Build Win32 Release for both games and both World Builder targets. Run
both Google Test suites. The new tests exercise padded rows without
trailing padding, short pitches and short destination spans.

In the mod, move a starting unit and then an additional unit in different
directions so their radar markers leave the old positions. Check for
stale dots. Return to the main menu and load a different map. The terrain
and marker layers should reflect the new battle as soon as the radar is
available.

To roll back, revert the GFX 13b.3g patch and rebuild, or restore the
backed-up GFX 13b.3f executable with the game closed.
