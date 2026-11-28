# GFX 13b.3e — retained single-mip surface access

Follow #92, #97, #99, #86, #91. Baseline f83baba8079d7206527ab313d37c439cc5d6de2e. Renumbered from 13b.3c: that stage is already delivered by #97; 13b.3d is delivered by #99. Design brief for local implementation, not completed code. Keep the DX8 game path and packed DDS eligibility unchanged.

## Scope
#99 already provides Renderer::Map_Texture_2D_Mips / Unmap_Texture_2D_Mips for the ordinary loader, backed by DX8TextureMipWrite. It maps all levels on a borrowed RenderTexture2DHandle (void*), and handles partial failure. Do not replace or duplicate that path. Reuse its checked row/layout and failure conventions where appropriate. This slice is specifically single-mip retained surface access through the existing W3D owner, with the mip/owner lifetime explicit; do not add a new public native pointer or pretend the borrowed handle already owns the texture. No migration of loader allocation in this slice.

Introduce a narrow backend-neutral 2D subresource description and scoped access result, reusing the existing checked texture layout helpers. Describe logical format, extent, mip index, access intent, row bytes, pitch, and checked accessible span. Keep native handles in the DX8 seam. Retain the W3D object identity; distinguish a retained surface reference, a retained wrapper and a borrowed pointer. Do not replace all SurfaceClass callers or invent a global resource manager.

Source evidence: Core/Libraries/Source/WWVegas/WW3D2/texture.cpp Get_Surface_Level obtains a native reference, wraps it and releases the native reference; Get_D3D_Surface_Level transfers a native reference; Peek_D3D_Surface borrows it. Get_Surface_Level currently proceeds after DX8_ErrorCode even if GetSurfaceLevel fails. surfaceclass.cpp Lock passes the result to DX8_ErrorCode and returns pBits/pitch, Unlock is independent. Introduce explicit checked acquisition, GetDesc and LockRect success; retain surface through map lifetime, unlock exactly once only after a successful lock, release exactly once even on errors. Expose unlock failure separately and never publish invalid writes. Preserve LastAccessed updates, render-thread ownership, native lock flags and reference behavior for the eligible path.

Support full mip of a verified uncompressed 2D format first; explicitly reject unsupported cases. Validate mip range, dimensions, logical format, signed pitch, row span, byte overflow before exposing storage. Partial rectangles, compressed alignment, cube/volume and depth views remain tracked follow-ups; do not silently substitute whole surface. No changes to DDS/TGA fallback, mip generation, texture filtering, SIM, RNG, assets or formats.

## Acceptance
- Device-independent tests: retained/borrowed distinctions, GetSurfaceLevel/GetDesc/Lock/Unlock failure injection, invalid mip, null handles, padded pitch, negative/short pitch, overflow, move/scope lifetime and no double release/unlock. Use fake seam for failure tests and actual DX8 for a smoke test.
- Win32 Release builds both games and both World Builders; both unit suites. Verify unchanged existing graphics paths and document exact outcomes.
- 13b.3f (#94) will migrate one terrain texture consumer, not this slice. No automatic merge; no claim of working DX11.
