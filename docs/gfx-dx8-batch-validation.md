# GFX DX8 batch validation

This batch combines #115 through #119 on the Zero Hour and Generals DX8 backend. It exposes checked resource descriptions and typed rendering operations so future backends can reuse the call sites. Existing game files, INI syntax, archives, replay format, and renderer selection are unchanged.

| PR | Scope | Main source files |
| --- | --- | --- |
| #115 | CPU layouts for cube faces and volume mip levels | `RenderTextureSubresources.h` and its GoogleTest cases |
| #116 | CPU color, depth, stencil, and viewport descriptors | `RenderTargetDesc.h` and its GoogleTest cases |
| #117–#118 | DX8 shader and volume-shadow states, including texture-stage combine | `Renderer.h`, `DX8Backend.cpp`, `shader.cpp`, and both `W3DVolumetricShadow.cpp` files |
| #119 | DX8 submission of indexed draws | `Renderer.h`, `DX8Backend.cpp`, and the mesh, particle, 2D, and water draw sites |

The layouts and descriptors in #115 and #116 describe logical CPU data. They do not allocate native textures or render targets. In the active Contra test tree, existing shadow profiling was kept around the migrated state calls. The fork branch starts from the shared main branch.

## Reproduce the checks

1. In the Win32 Release configuration, enable `RTS_BUILD_GENERALS`, `RTS_BUILD_GENERALS_TOOLS`, `RTS_BUILD_ZEROHOUR_TOOLS`, and `RTS_BUILD_OPTION_TESTS`. Build `g_generals`, `z_generals`, `g_worldbuilder`, `z_worldbuilder`, `g_googletest`, and `z_googletest`. Run CTest in that build directory with `-C Release --output-on-failure`. Both suites passed all 294 tests on Windows.
2. Launch the Zero Hour test executable in the mod. Play a skirmish and replay on [RANK] Final Crossroad ZH v1. With volume shadows enabled, move a dozer past the command center at several zoom levels and inspect shadows, shoreline blending, smoke, explosions, projectile trails, and the HUD. The tester reported that the packaged build worked in game.
3. Keep #94 and #95 separate. #94 still needs a terrain-atlas failure path that prevents a zero texture height from reaching UV calculations. #95 is a standalone DX11 experiment and still needs stronger resize and failure-path checks.

To roll back a local test, restore the executable saved before installation. If the batch is merged through its merge commit, revert that commit with `git revert -m 1 <merge-commit>` and rebuild the affected targets.
