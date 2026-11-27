# GFX 13b.3a — checked 2D texture layout

This slice establishes CPU-side size and row calculations before migrating a texture or surface consumer. The descriptor uses the existing logical WW3D format and an explicit stored mip count. The next slice can use the validated footprint when translating a native surface or passing a staging payload.

## Layout rules

- Uncompressed layouts use the existing WW3D byte sizes. Their byte order and color interpretation remain attached to the logical format. Palette-dependent layouts require additional state and are outside this slice.
- DXT data uses 4×4 blocks. DXT1 consumes eight bytes per block and can carry one-bit alpha. DXT2–5 consume sixteen bytes per block. DXT2 and DXT4 retain their premultiplied alpha identity. [Microsoft block compression reference](https://learn.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11)
- Mip dimensions reduce independently down to 1×1. Block counts round up. The resulting footprint describes CPU data. Native texture creation still validates device dimensions, formats and capabilities.
- Row bytes describe useful data. Source pitch can include padding between rows. The required span ends at the last useful byte on the final row. The helpers reject malformed counts, sizes beyond the Win32 byte domain and insufficient storage. The caller checks signed native pitch and resolves automatic mip counts before conversion.

## Legacy paths reviewed

[Zero Hour DDS loading](../GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2/ddsfile.cpp) and [Generals DDS loading](../Generals/Code/Libraries/Source/WWVegas/WW3D2/ddsfile.cpp) own reduction, level offsets and format conversion. The [shared texture loader](../Core/Libraries/Source/WWVegas/WW3D2/textureloader.cpp) owns DDS fallback and the begin, copy and end phases. [Surface locks](../Core/Libraries/Source/WWVegas/WW3D2/surfaceclass.cpp) supply native row pitch. The helpers in this slice are exercised by tests. Those consumers still use their current paths.

Both DDS loaders divide the previous level's size by four while it exceeds 16 bytes. Their dimension accessors clamp compressed extents to four texels. A block footprint instead rounds each logical axis up separately. For a 512×256 DXT3 image, the old table gives eight bytes at level 7, where the logical 4×2 image occupies a 16-byte block. A local header survey found differences in 407 of 3,856 loose 2D DXT files at zero reduction, after applying the loader's rule for dropping the two lowest levels. These are size comparisons from file headers. Visual impact requires a separate test. Keep the legacy offset and reduction policy explicit during loader migration.

## Verify and roll back

Build Generals, Zero Hour, both World Builders and both Google Test targets in Win32 Release. Run CTest. The added tests cover legacy sizes, DXT packing and alpha, odd mip sizes, small compressed levels, overflow, invalid mip counts and padded rows.

Reverting the helper, registration and tests removes this slice. Later adoption in the texture loader or editors needs its own checks against source textures and native pitches.
