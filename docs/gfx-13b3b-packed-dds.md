# GFX 13b.3b — packed 2D DDS loading

The ordinary 2D texture load task uses the block layout from GFX 13b.3a when an
untinted legacy DDS has the same DXT format as its locked surfaces and the
selected dimensions match the texture allocation. The loader computes every
stored mip offset from the declared dimensions and requires the sum to match
the file payload exactly. It reads just the selected mip range and checks all
destination pitches before writing. Tight rows use a single copy per mip.
Padded surfaces use one copy per block row.

The selected reduction and mip count still come from the existing texture task.
Ineligible DDS files, conversion, tint, synchronous surface loading, cubemaps
and volumes continue through their respective existing paths. A read or copy
failure in the packed path goes through the 2D task's existing failure handling.
This change reads the existing DDS format and introduces no asset or INI format.

## Loose assets checked

The following counts cover structurally plausible loose 2D DXT DDS files.
Each reduction uses the legacy exposed mip window, which omits the two smallest
stored levels when possible. The comparison measures source payload bytes
before device limits, which can further narrow the window.

| Requested reduction | Files with the requested level | Different mip size | Different source offset | Different bytes read |
| --- | ---: | ---: | ---: | ---: |
| 0 | 3,854 | 407 | 94 | 94 |
| 1 | 3,641 | 407 | 95 | 94 |
| 2 | 3,640 | 406 | 94 | 94 |

Eight files at reduction 0 have payload lengths that differ from the
calculated full chain. They retain the existing loader. For example,
`EXGzrLsr.dds` (8 x 64 DXT1) stores mip 3 at payload offset 352 bytes.
The old calculation seeks to 336 bytes. The device's ordinary 2D mip limit
can exclude that level. `ABAFSW_E.dds` (512 x 256 DXT3) has a size
difference at level 7, which is also outside the usual native allocation.
It serves as a game regression case for the normal face and damaged geometry.

## Validation and rollback

Build both games and World Builder targets for Win32 Release and run both Google
Test executables. The new tests exercise offsets after skipped mips and reject
truncated or padded payloads without updating the output. The normal game test
loads a skirmish at texture reduction 0 and repeats at reduction 2 with the
same map and scene. Compare model textures and alpha at close and distant
camera positions. On a daytime map, use the USA Air Force General's Levitation
Pad. Its undamaged and severely damaged models directly reference
`ABAFSW.dds` and `ABAFSW_E.dds`, respectively.

Revert this stage's source patch to restore the previous loader. For a local
mod test installation, restore the backed-up `generals.exe` while the game
is closed, or rebuild the preceding source version.
