# GFX 13b.1b — Direct3D 8 resource inventory

Source baseline **3d3ebe7c62d3c7113e0e59f155415818927a879a**. The DX8 SDK checkout is pinned at **7bddff8c01f5fb931c3cb73d4aa8e66d303d97bc**. The [resource contract](gfx-resource-contract.md) and [review](gfx-d3d8-resource-inventory-review.md) explain the migration and its acceptance criteria.

Examined **4000** Git blobs. The [TSV](gfx-d3d8-resource-inventory.tsv) records **6468** unique path/line pairs in **243** files. Every source field preserves the baseline line, with tabs escaped. Each row has one primary phase and can name secondary phases.

| Scope | Rows |
| --- | ---: |
| CMake target | 6226 |
| build configuration | 8 |
| excluded #if 0 | 173 |
| header outside target lists | 17 |
| outside selected targets | 44 |

| Win32 Release target | Files | Rows with CMake target scope |
| --- | ---: | ---: |
| g_generals | 163 | 5249 |
| z_generals | 164 | 5264 |
| g_worldbuilder | 171 | 5395 |
| z_worldbuilder | 172 | 5403 |

Target rows overlap through shared libraries. The Release CMake codemodel supplies file and target ownership. A listed source can contain optional or inactive code. The guards column preserves those conditions and labels literal #if 0. CMake configuration files and linked SDK declarations supply build evidence.

The C++ compile groups for all four executable targets define BUILD_WITH_D3D8 and RTS_RELEASE. The two Zero Hour targets define RTS_ZEROHOUR=1. The Generals executable targets omit RTS_ZEROHOUR. These flags identify the audited configuration. Conditions on optional features remain visible for review.

| Primary phase | Candidate lines |
| --- | ---: |
| 13b.2a | 492 |
| 13b.2b | 654 |
| 13b.2c | 294 |
| 13b.3 | 1778 |
| 13b.4 | 363 |
| 13b.5 | 873 |
| 13c.1 | 21 |
| 13d | 1967 |
| outside graphics | 26 |

## Native calls and exceptions

- The scanner reads interfaces, aliases and method names from the pinned SDK. Call detection covers typed local declarations, member names, direct getters, COM macros and multiline calls. Type resolution is lexical. Entries with unresolved receiver types remain explicit candidates for source review.
- [W3DMouse.cpp](../Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DMouse.cpp) has seven confirmed device call sites for ShowCursor, SetCursorPosition and SetCursorProperties. They go to 13b.4 with dependencies on texture surfaces, native-handle audit and backend bring-up. Cursor creation, hotspot, animation, fullscreen updates, thread ownership and reset need separate checks before migrating this path.
- The header scope outside the four target lists retains WWAudio buffer references, Babylon string locks, W3DView texture references and WOLAPI declarations that match generic SDK method names. WWAudio and Babylon entries classified as non-graphics are excluded from migration. W3DView and WOL utilities require a target-graph check if brought into scope. CMake source membership and the lexical scanner do not establish runtime reachability.
- The device and SDK blobs are inputs to the scan. Generated code and third-party SDK implementation are outside the first-party source inventory. The register captures optional clauses and unresolved context for follow-up.

## Buffer inputs for 13b.2a

| Consumers | Producer and ownership | Phase |
| --- | --- | --- |
| [Scorch](../Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DScorch.cpp) | VertexFormatXYZDUV1, 24-byte stride, position at 0, packed color at 12, UV at 16. UInt16 indices. Own buffers, rewrite them and retain the existing invalidation and draw order. | 13b.2a layout, 13b.2b non-streaming adapter |
| Roads in [Generals](../Generals/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DRoadBuffer.cpp) and [Zero Hour](../GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DRoadBuffer.cpp) | Same vertex layout and UInt16 index width as scorch. Each road owner allocates, locks, binds and releases its buffers. Generals uses dynamic usage for its road vertex buffer, and Zero Hour selects usage per road path. | 13b.2b and 13b.2c according to usage |
| [Terrain particles](../Core/GameEngineDevice/Source/W3DDevice/GameClient/W3DTerrainParticles.cpp) and [sorting buffers](../Core/Libraries/Source/WWVegas/WW3D2/dx8vertexbuffer.h) | VertexFormatXYZNDUV2, 44-byte stride, position 0, normal 12, packed color 24, UVs 28 and 36. Dynamic GPU reservations and CPU sorting pages need distinct lifetime checks. | 13b.2a layout, 13b.2c mapping and lifetime |
| [General FVF layouts](../Core/Libraries/Source/WWVegas/WW3D2/dx8fvf.h) and [DX8 draw binding](../Core/Libraries/Source/WWVegas/WW3D2/dx8wrapper.cpp) | Validate stride and field offsets against each producer. Byte offsets are for maps. Base vertex is vba_offset + index_base_offset, first index is iba_offset + start_index. TG3 and DMAP require separate compatibility decisions. | 13b.2a |

The Win32 validation for #88 compared 11 ordinary named layouts to the actual structures. The TG3 structure matches the native declaration and differs from legacy metadata. DMAP occupies 48 bytes in the structure and 52 bytes in the native declaration. Keep shader-specific, transformed or mutable legacy metadata behind caller-specific verification before migration.

## Reproduce

Use a Win32 CMake Release configuration of the pinned baseline or a code-equivalent checkout with both games and both World Builders enabled. Request codemodel-v2 under the build directory’s .cmake/api/v1/query, then run the configure step. Use the pinned SDK checkout referenced by cmake/dx8.cmake.

From the source checkout run node docs/tools/gfx-d3d8-inventory.cjs . <CMake-reply-directory> <SDK-checkout> <output-directory>. Run node docs/tools/gfx-d3d8-inventory.cjs --self-test for the scanner regression fixtures. An optional fifth argument supplies the previous TSV for a before/after delta written to standard output.

The schema records path and line, source text, scope, CMake targets and owners, inferred context, kind, dependency, SDK receiver evidence, guards, categories, resource operations, primary and secondary phase, and symbols.
