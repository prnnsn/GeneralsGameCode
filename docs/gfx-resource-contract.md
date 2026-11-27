# GFX 13b.1 — DX11 resource audit and proposed contract

The original audit used `00567a9de9c005d4d073e34a86d2e05d9261ee53` (main after #74). This review checks the contract against `93c875f5c` (main after #83) and the local GFX 13a.7 build with #84 and #85. The DX11-first roadmap is in pending #82. This stage defines the resource contract. Existing DX8 performance work stays deferred.

## 1. Confirmed resource boundaries

Source links refer to the fork paths. The cited implementations were checked in the current checkout.

| Area | Current source and observed behavior | Migration boundary |
| --- | --- | --- |
| Vertex buffers | [dx8vertexbuffer.h](../Core/Libraries/Source/WWVegas/WW3D2/dx8vertexbuffer.h): VertexBufferClass derives RefCountClass, stores FVF info, unsigned-short count and separate engine_refs; DX8VertexBufferClass exposes IDirect3DVertexBuffer8; SortingVertexBufferClass is distinct. | Retain W3D object identity and CPU storage; move GPU allocation/lock/binding behind backend. |
| Index buffers | [dx8indexbuffer.h](../Core/Libraries/Source/WWVegas/WW3D2/dx8indexbuffer.h), [dx8indexbuffer.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8indexbuffer.cpp): index count unsigned short, D3DFMT_INDEX16, CPU sorting buffer and dynamic access classes. | Keep 16-bit indices and legacy offset units during compatibility migration. |
| GPU allocation | [dx8vertexbuffer.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8vertexbuffer.cpp) (Create_Vertex_Buffer), dx8indexbuffer.cpp: WRITEONLY, optional DYNAMIC, NPATCHES and SOFTWAREPROCESSING; dynamic uses DEFAULT pool, otherwise MANAGED; allocation retry/fallback paths exist. | Preserve legacy intent and failure behavior in DX8 adapter; do not map D3D8 flags numerically into D3D11. |
| Dynamic access | `DynamicVBAccessClass` and `DynamicIBAccessClass` retain the backing buffer and store the offset and count in elements. Lock converts these ranges to bytes. Offset zero uses DISCARD and the other ranges use NOOVERWRITE. Vertex locks also use NOSYSLOCK. | Track the reservation separately from a successful map. Release a map only after a successful lock. Keep the buffer and its range valid for draws that still reference it. |
| CPU sorting | Both buffer .cpp files retain 65535-element CPU sorting pages and bounded retired-page reuse (GFX 06). | CPU storage remains CPU storage; not a GPU dynamic buffer or D3D11 staging resource. Preserve reference/queue ordering. |
| Vertex layout | [dx8fvf.h](../Core/Libraries/Source/WWVegas/WW3D2/dx8fvf.h): FVFInfoClass and legacy flags cover position, normal, packed diffuse and variable UV layouts including multi-coordinate variants. | Introduce a canonical layout descriptor with stride, byte offsets, semantic/index and type, independent of D3DFVF. Validate byte-for-byte legacy mapping before using DX11 input layouts. |
| Textures | [texture.h](../Core/Libraries/Source/WWVegas/WW3D2/texture.h), [texture.cpp](../Core/Libraries/Source/WWVegas/WW3D2/texture.cpp): TextureBaseClass is refcounted, owns IDirect3DBaseTexture8, exposes borrowed Peek_D3D_* pointers. Set_D3D_Base_Texture releases old and AddRefs new, updates LastAccessed; Peek also updates LastAccessed. 2D, cube, volume and Z textures have distinct paths. | Preserve access timestamp, reference ownership, mip counts, invalidation, format and replacement semantics. No raw COM pointer in public neutral contract. |
| Loader | [textureloader.h](../Core/Libraries/Source/WWVegas/WW3D2/textureloader.h), [textureloader.cpp](../Core/Libraries/Source/WWVegas/WW3D2/textureloader.cpp): Begin_Load and End_Load assert DX8 thread; begin allocates/locks, Load copies compressed/uncompressed mip data, end unlocks and applies. Cube faces and volume row/slice pitch are separately represented. | Preserve threading and publication ordering; proposed DX11 CPU-owned staging payloads and render-thread upload, not arbitrary worker access to immediate context. |
| Surfaces | [surfaceclass.cpp](../Core/Libraries/Source/WWVegas/WW3D2/surfaceclass.cpp) retains the native surface in `Attach` and releases it in `Detach`. In [texture.cpp](../Core/Libraries/Source/WWVegas/WW3D2/texture.cpp), `Get_Surface_Level` returns a refcounted wrapper and `Get_D3D_Surface_Level` returns a native reference the caller must release. `Peek_D3D_Surface` borrows from the wrapper. | Specify lifetime for each returned view. Preserve full and partial rectangle access, row pitch and readback. |
| Formats | [ww3dformat.h](../Core/Libraries/Source/WWVegas/WW3D2/ww3dformat.h): WW3DFormat and WW3DZFormat include legacy RGB, packed 16-bit, luminance, paletted, bump and DXT1–5 plus multiple depth/stencil formats. | Preserve existing format enum values. Explicit conversion table with unsupported-format fallback; account for channel swizzles, DXT2/4 premultiplied alpha, compressed block pitch and stencil capacity. |
| Targets and recovery | [dx8wrapper.h](../Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper.h), [dx8wrapper.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper.cpp), [dx8texman.h](../Core/Libraries/Source/WWVegas/WW3D2/dx8texman.h): native surface targets/backbuffers/additional swap chains; trackers release and recreate DEFAULT-pool textures. Reset_Device invalidates textures, unbinds VB/IB, invokes cleanup hook, deinitializes dynamic buffers, releases tracked textures, resets device, conditionally recreates resources, invalidates state and reinstates defaults. | Separate swap-chain resize from full device removal/recreation; preserve reload_assets=false path and existing cleanup/reacquire obligations. No stale native resources or cached state after generation changes. |

## 2. Native-handle escape paths already confirmed

These are representative source-inspected call sites, not an exhaustive reference count. A whole-tree inventory must be completed before replacing native resource interfaces.

| Consumer | Confirmed dependency | Required follow-up |
| --- | --- | --- |
| Shared W3DScorch / terrain particles / tracks | Direct legacy construction and locking remain after migration of binding and drawing. [W3DScorch.cpp](../Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DScorch.cpp) uses non-streaming vertex and index buffers and rewrites them in `updateScorches`. | 13b.2/3. Treat the scorch buffers as updateable. Migrate their lock seam together with creation if the consumer needs a neutral buffer type. |
| GeneralsMD W3DProjectedShadow.cpp | CreateIndexBuffer/CreateVertexBuffer with DEFAULT pool (around lines 283–307), direct device access, shadow render target. | 13b.2/4, separate projected-shadow migration; keep GFX 03 staging. |
| GeneralsMD W3DVolumetricShadow.cpp | Native vertex/index getters for static slots, raw dynamic buffer creation, direct stencil draw path (around 1345–1405, 3727–3750). | Migrate ownership and native bindings, not deferred GFX 09b optimization. |
| Shared W3DWater.cpp | Raw native textures, device pixel-shader calls, U8V8 bump maps, managed/default VB/IB allocations (around 228–278, 456–732, 874–966). | 13b.2–4 plus 13d shader translation. |
| Both World Builder wbview3d.cpp | Native device access and SetCleanupHook (Generals around 527,2189,2291; ZH around 545,2282,2385). | Include both editors in resource/recovery tests. |
| [Generals ObjectPreview.cpp](../Generals/Code/Tools/WorldBuilder/src/ObjectPreview.cpp) and [Zero Hour ObjectPreview.cpp](../GeneralsMD/Code/Tools/WorldBuilder/src/ObjectPreview.cpp) | Both render into a color target with depth, restore the main backbuffer, copy the preview to a CPU-readable surface and lock it for readback. Each releases the retained surface wrapper after the copy. | 13b.4. Exercise target restoration and pitch-aware pixel readback in both editors. |

## 3. Proposed resource contract (review before coding)

Do not implement a new global resource manager, replace refcounting, change file formats, or change SIM in this stage.

- **Buffer descriptor.** Specify role, element count and stride, checked byte size, vertex layout or index format, update intent, source data and recreation policy. Keep the existing 16-bit element limits and use checked wider arithmetic for bytes. Select update intent from the consumer's actual writes over its lifetime.
- **Resource ownership:** existing W3D RefCountClass remains the public object owner; native object references live inside backend implementation. Engine_Refs is a separate draw/engine-use counter, not COM ownership. Resource views and access ranges are non-owning only when the parent lifetime is retained. Release exactly once; no raw backend object escapes from new public APIs. Preserve existing borrowed-versus-retained semantics while migrating call sites.
- **Access contract.** Return a scoped result for each write, append or read. Expose a pointer only for a successful map and only until unmap. A failed reservation or map leaves the geometry unpublished. The current dynamic DX8 lock constructors pass the HRESULT to `DX8_ErrorCode` and their destructors call `Unlock` without a success flag. Cover both VB and IB failures before migrating these callers. Preserve CPU sorting arrays and retained pages. Choose GPU map intent from the range and outstanding draws.
- **Texture descriptor:** dimension, extent, array/cube faces, mip count, logical WW3D format, usage intent, sample count, subresource pitches and recreation source. Keep logical format separate from native storage/view format. Define CPU staging payloads with explicit row/slice pitch and compressed blocks; do not change DDS/TGA fallback or loader publication ordering.
- **Render-target descriptor:** color/depth views, dimensions, sample count, ownership, swap-chain association; explicit set/restore, copy/readback and resolve as applicable. Preserve stencil and color/alpha semantics and editor preview behavior.
- **Device lifecycle:** backend owns creation/release of native handles, render thread owns immediate-context operations, resize releases affected views, device removal invalidates all GPU handles and replays recreation policy, retained CPU data or reload from assets when appropriate. Invalidate caches after transitions. Do not assume DX11 has DX8 MANAGED-pool behavior. Keep DX8 operational and separately testable.
- **Compatibility:** no changes to W3D/INI/map assets, SIM tick count, CRC/RNG or replay/network formats. No assumption of equivalent FPS; test with identical replay scenes and uncapped render timing once backend is playable.

The offset conversion in [dx8wrapper.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper.cpp) needs a separate check. For a draw, `vba_offset + index_base_offset` is the base vertex and `iba_offset + start_index` is the first index. Lock offsets are in bytes from the start of the corresponding allocation. Keep these computations separate and test a draw with all four terms nonzero to catch omitted or double-applied offsets.

The reset path in the same file releases resources before attempting the reset. On failure it returns before recreation or default-state setup. On success with `reload_assets=false`, it reinstalls default states and shaders without reacquiring tracked textures or cleanup-hook resources. Test those three lifecycle outcomes separately when the adapter is implemented.

## 4. Split into reviewable implementation steps

| Step | Scope | C/R | Gate |
| --- | --- | --- | --- |
| 13b.1a | This source-based audit and contract | 3/2 | Review ownership, ranges, formats and recovery. |
| 13b.1b | Exhaustive local source inventory: raw D3D handles/calls, creation/lock/reset, format and FVF usage, both game and editor targets | 2/1 | Record path/line, owner, caller, phase, planned replacement; exclude third party and dead code only with evidence. |
| 13b.2a | Neutral buffer descriptors, checked range/layout conversions and unit tests | 3/2 | Verify legacy counts, stride, both offset units, base vertex and first index before changing GPU code. |
| 13b.2b | DX8 adapter for non-streaming VB/IB creation and release, with the write path required by one small consumer | 3/3 | Build four targets and verify reference lifetime, buffer rewrites, recreation and visual parity. |
| 13b.2c | Scoped dynamic access and lock intent adapter, preserve sorting pages | 4/3 | Ring wrap, failed VB and IB maps, retained ranges, reset and queued sorting. |
| 13b.3 | Texture/surface descriptor, formats, loader staging and native-handle migration in slices | 4/3 | DDS/TGA, mip, pitch, cube/volume, alpha, texture restoration. |
| 13b.4 | Targets/depth, copy/readback, resize and recovery | 4/4 | Editor object preview, screenshot, projected shadows, switching away/back, device failure. |
| 13b.5 | Complete remaining resource consumers and audit native escapes | 4/3 | No unapproved direct native handle usage outside backend when building native DX11 path. |

The standalone DX11 device smoke test (13c.1) can be designed in parallel once requirements are reviewed, but does not replace resource contracts or constitute a working game backend. No DX8-specific shadow batching or debris optimization as part of this migration.

## 5. Validation / limitations

The initial review and the current source check cover representative buffer types, loading, reset, native consumers, and preview readback in both World Builders. The next audit step inventories remaining native handles, creation fallbacks, per-format usage, cleanup hooks and build configurations. Validate the original Generals and Zero Hour counterparts during that inventory.

This documentation review uses source checks. Implementation acceptance requires Win32 Release builds of both games and both World Builders, the relevant Tracy configurations, device reset, CPU sorting and dynamic range checks, lifetime checks, and matched replay CRC and rendering checks.
