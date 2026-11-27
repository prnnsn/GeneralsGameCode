# GFX 13b.2a — neutral buffer descriptors and checked layout/range conversions

Implementation brief for the fork's next GFX stage. Dependencies: #86 (resource contract), #87 (inventory review), #82 (DX11-first policy). Baseline: `f63bbf35960142efe05ca20756d247515b2acaf9`. Owner confirms the local review of the inventory corrections. At this baseline the corrected TSV/report/generator are not visible in main; attach or publish their revision and verify 13b.2a input coverage before implementing. This document is a design handoff, not a code implementation or a test result.

## Scope

Introduce small backend-neutral value descriptors and pure checked helpers in shared WW3D2 renderer code. Determine final names and file locations from existing renderer interfaces and build conventions. No Direct3D headers, flags or native handles in neutral types. Keep existing W3D classes, CPU sorting pages, reference counts, buffer construction, locks, binds and draw paths unchanged; no adapter or consumer migration until 13b.2b. No global resource manager. No SIM/RNG/CRC, network/replay, INI, asset or file-format changes. No index-width or capacity expansion.

## Descriptor and validation requirements

- Buffer role (vertex/index), element count, stride in bytes, checked byte size, vertex layout or UInt16 index format, update intent (immutable, rewritable, streaming) and explicit recreation/source policy. Separate CPU-only sorting storage from GPU resource intent; do not implement ownership or upload APIs yet. Reject invalid combinations and narrowing; retain existing 65,535-element limits, distinguish allocation capacity from index value and draw count. Do not use a zero byte range as an implicit whole-buffer lock.
- Canonical vertex layout: explicit byte stride, semantic and semantic index, storage type and byte offset; include packed legacy diffuse/specular representation (byte order/normalization explicit), position vs pretransformed position, normals, point size, blend data and texture coordinate widths where used. Check duplicate semantic/index, bounds, overlap, unsupported encodings and arithmetic overflow. No implicit padding assumptions. Inventory active and optional layouts; do not silently accept unverified mappings.
- Put legacy FVF interpretation in a DX8-specific conversion seam, not in the neutral descriptor. Validate against actual producer structure offsets, sizes and existing FVF behavior; do not change FVFInfoClass or live callers in this PR. Where legacy source and canonical interpretation disagree, record disposition and keep that consumer on DX8 until separately reviewed.
- Use checked wide intermediates before multiplication/addition/narrowing, including Win32 size limits. Convert a nonempty element range to byte offset/length; reject out-of-bounds ranges, arithmetic overflow and zero stride. Define empty logical range explicitly and ensure it cannot turn into DX8 whole-buffer mapping later.
- Separate indexed draw address calculation from map ranges. Existing base vertex = vba_offset + index_base_offset; first index = iba_offset + start_index. Offsets here are elements, not bytes. Preserve backend signedness and range limits at the conversion boundary. Validate index spans using actual primitive topology and referenced vertex range using min vertex/count where known; do not assume all topology is triangle list or that every index value equals its position in the index buffer. Preserve legacy vertex_count fallback (<3) in the caller; do not change live Draw behavior. Validate offset additions separately from access/ownership in 13b.2c.

## Source review notes / regression risks

- [dx8fvf.h](../Core/Libraries/Source/WWVegas/WW3D2/dx8fvf.h) defines ordinary XYZ/normal/diffuse/UV variants and TG3, DMAP and cube-map variants. [dx8fvf.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8fvf.cpp) has independent size and offset logic. In the offset constructor, the SIZE2 test precedes SIZE3/SIZE4; verify the SDK zero-valued SIZE2 encoding and compare nonstandard coordinate offsets with actual producers. DMAP flag expression declares texture coordinate widths 1/4/2 while the named structure visibly contains six texture floats (T1x..T1w, T2x..T2y). Verify actual usage, sizeof/offsetof and shader declaration before supporting this mapping. These are source-level discrepancy candidates, not confirmed gameplay bugs. Do not fix them as incidental refactoring.
- [dx8vertexbuffer.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8vertexbuffer.cpp#L881) and [dx8indexbuffer.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8indexbuffer.cpp#L440) convert reservation offsets to bytes; GPU dynamic locks and CPU sorting accesses differ. Do not conflate them.
- [dx8wrapper.cpp](../Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper.cpp#L2050) uses first-index addition at DrawIndexedPrimitive and a legacy vertex_count fallback; the base-vertex addition is applied in index binding. Check both locations and CPU sorting equivalents.

## Test matrix

| Case | Expected |
| --- | --- |
| VB stride 32, offset 7, count 11, capacity 64 | Byte offset 224; byte length 352 |
| IB UInt16, offset 13, count 9, capacity 64 | Byte offset 26; byte length 18 |
| vba_offset 7, index_base_offset 5, iba_offset 13, start_index 3 | Base vertex 12; first index 16; neither byte-scaled |
| Above offsets, three triangle-list primitives | Nine indices at [16,25); validate against IB capacity and independently validate vertex span |
| First/last element; range ending exactly at capacity | Accept valid nonempty range |
| One past end, overflowing addition/multiplication, zero stride, invalid index stride/format, 65,536 elements | Reject without wrap or truncation |
| Empty range, including offset == capacity | Explicit empty result or rejection; never native whole-buffer lock |
| Ordinary FVF layouts | Match sizeof/offsetof and legacy stride for each actual supported producer |
| UV widths 1/2/3/4, multiple UV indices, packed colors, pretransformed/blend variants if used | Match verified storage and semantic offsets; unsupported variants fail explicitly |
| TG3, DMAP, cube-map and legacy mutable FVF/size overrides | Document actual usage and discrepancies; test supported cases and explicit rejection of unsupported cases |
| Invalid layouts: overlap, duplicate semantic/index, out-of-stride field, unsupported type | Reject explicitly |

Use unit tests independent of device creation, including boundary/negative cases. Verify on Win32 Release and the available project test configuration; build g_generals, z_generals, g_worldbuilder, z_worldbuilder. Run relevant DX8 smoke checks (roads, scorch, terrain particles, sorting-heavy scene and both editor previews where available), record exact build/test results. Matched replay/CRC and rendering comparisons are required when live adapters or consumers are migrated; do not claim them from descriptor-only tests.

## Exit gate and next stages

Publish reviewable implementation and tests separately from this brief, with corrected inventory provenance, supported layouts and any deferred/unverified legacy variants. No live graphics changes should be needed in 13b.2a. After approval, 13b.2b adds DX8 create/release/write adapter and one non-streaming consumer (scorch candidate); 13b.2c handles scoped dynamic maps, failure publication, ring wrap and CPU sorting lifetime. Keep DX8 performance optimizations deferred. No automatic merge.

## Implementation review snapshot

Implementation commit `6d03a87801b4845e4d21179812c0699e63ee995a`: shared `RenderBuffer.h`, isolated `Backend/DX8VertexLayout.h`, both Google Test sources, and explicit CMake registration. All live rendering callers are unchanged. This is a draft candidate; the corrected local inventory revision must still be attached and checked before acceptance.

- Native decoder interprets XYZ/XYZRHW and explicit UV widths; rejects blend/shader-specific declarations, unknown bits and producer stride mismatches. The stricter legacy conversion also checks the offsets and size in FVFInfoClass; unsupported RHW/point-size metadata, TG3 mismatch, DMAP stride mismatch and mutable overrides stay deferred until caller-specific verification. Neutral layout can describe blend storage, but no legacy blend converter is claimed.
- Ordinary named FVF producer sizes and the shared dynamic layout offsets have Google Test cases; verify all actual producer offsets and optional paths locally before using them. No live adapter or allocation is introduced.
- Standalone neutral smoke check in the Linux analysis environment: source copied from the connector and verified using git hash-object, blob `59872b1b7a249851a1a21e69240fea91c4f1cf58`; GCC C++11, `-Wall -Wextra -Werror -pedantic -fsanitize=undefined -O2`. Passed 2,151,680 byte-range combinations (capacity 0..79, first/count 0..81, stride 0/2/32/UINT32_MAX), including 1,981,039 expected rejections and unchanged failure outputs. Additional descriptor/layout and draw-address assertions passed. This standalone harness is not the registered Google Test suite. SDK-dependent tests, MSVC Win32 builds, game/editor smoke and replay checks remain unrun.
- Follow-up implementation briefs: #89 (13b.2b), #90 (13b.2c). Both blocked on accepted code and local validation.
