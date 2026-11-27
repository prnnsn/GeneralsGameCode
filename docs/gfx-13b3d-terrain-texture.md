# GFX 13b.3d — first rewritten terrain texture consumer

Depends on accepted 13b.3c checked retained surface access. Follow #92 and #91. Baseline ed9ae439. Brief only, no implementation or test result. Start from current main after dependency merges.

## Source evidence
Core/GameEngineDevice/Source/W3DDevice/GameClient/TerrainTex.cpp TerrainTextureClass::update obtains mip 0 with GetSurfaceLevel, GetDesc, LockRect, writes A1R5G5B5 terrain tiles and duplicates four-pixel borders, unlocks/releases, filters through D3DXFilterTexture(D3DX_FILTER_BOX) and applies LOD when reduction is nonzero. The active pixel and border address calculations use surface_desc.Width * pixelBytes rather than locked_rect.Pitch. This is a source-level padded-pitch risk, not proof of visible corruption. Check all active branches of this exact method before migration; other atlas/alpha/light paths remain separate.

## Scope
Migrate one complete update path to the neutral surface access interface. Preserve legacy tile order, orientation, packed color/alpha, borders, allocation, format selection, dimensions, mip filter and LOD semantics on valid data. Use actual validated pitch for every row and border offset. Check bounds for the source tile/class dimensions and all writes before modifying the atlas, including four-pixel borders and arithmetic. Distinguish invalid inputs, failed acquisition/lock/unlock, and failed mip generation; define caller-compatible retry/invalid state, do not mark a failed partial texture valid. Do not change texture filtering algorithm, map data, DDS paths or other consumers. Keep D3DX and SetLOD inside DX8 seam where necessary; do not add direct D3D calls to neutral code.

## Acceptance
Compare old/new bytes on tightly packed representative atlases; padded pitch fixtures must match logical pixels and preserve padding. Cover tile edge and corner, border wrap, minimum size and malformed positions, wrong format, failed acquisition/map/unmap and recovery. Confirm mip generation ordering, texture reduction 0 and 2, normal/overview zoom, multiple terrain tiles on a verified mod map, reload and Alt+Tab; use exact displayed map name chosen from local assets. Build both games/editors and unit suites, report real results and rollback. No SIM, RNG, network/replay or asset format changes. No automatic merge.
