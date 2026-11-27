# GFX 13b.3f — ordinary 2D mip surfaces and descriptions

The existing `TextureClass::Get_Surface_Level` and `Get_Level_Description`
methods use the renderer to obtain the requested 2D mip. DX8 acquires its
native surface and gives the caller an owned W3D `SurfaceClass` reference.
The backend releases the temporary native query reference after the
wrapper has retained its own. Consumers use the same W3D release method.

For description-only requests, DX8 queries the width, height and actual
format directly from the mip. This replaces allocation of a temporary
`SurfaceClass` and the extra description query. A missing or failed mip
returns no surface and keeps the caller's existing description on a
failed metadata query.

This stage leaves native render-target binding, surface locks, texture
upload and asset formats on their existing paths.

## Validation and rollback

Build both games and both World Builder targets as Win32 Release and
run their Google Test suites. World Builder's object preview also uses
`Get_Surface_Level`.

In a skirmish, check that the radar background, shroud, camera outline
and moving unit markers render together. Use the minimap to move the
camera and move a starting unit into unexplored terrain. Return to the
menu and load another map to check that the radar clears the old map
and rebuilds its layers. A map with water also exercises 2D texture
description requests in the water renderer.

To restore the previous behavior, revert the GFX 13b.3f patch and rebuild,
or restore the backed-up GFX 13b.3e executable with the game closed.
