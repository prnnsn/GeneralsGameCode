# GFX 13b.2b — rewritable DX8 buffers for terrain scorches

This stage uses the buffer descriptors from #88 and the source audit from #91. The shared scorch pass is the first consumer. `Renderer` returns the existing reference-counted W3D buffer identities. The DX8 backend validates the producer layout, then supplies the legacy XYZDUV1 FVF and UInt16 index format to the existing managed-buffer allocation path.

## Allocation and write lifetime

- The consumer uses the actual `sizeof` and `offsetof` of its vertex producer. The current vertex adapter accepts that verified layout. Index capacity and both buffer capacities keep the existing element limits.
- The DX8 constructors retain the original flags, managed pool and retry after resource eviction. The new path can report a failed allocation with diagnostics. If index allocation fails after vertex allocation, the scorch pass releases the vertex buffer. Map and device resource recreation continue through the existing height-map lifecycle.
- Each write validates a nonempty element range. The full vertex write uses the legacy native `Lock(0, 0, ..., 0)`, and the full index write uses `Lock(0, 131070, ..., 0)`. Subranges use explicit checked byte lengths.
- A lock exposes data only on success and holds a W3D reference until cleanup. Scorches lock the index buffer before the vertex buffer and finish in reverse order. Publication of vertex and index counts waits for both finishes. A failed map leaves the rebuild pending. An unmap failure discards and recreates the allocation pair so a later write cannot reuse a potentially locked buffer.
- The existing shader, texture, draw order and terrain depth-bias strength of 2 still govern scorch rendering. Geometry retains reverse insertion order and the existing capacity limit.

## Verification

Build both Win32 Release games, both World Builders and both Google Test executables. Run CTest in Release. The DX8 tests exercise complete and partial ranges, empty and invalid requests, unsuccessful locks, empty native data, failed unlocks and a missing device. A separate x86 differential harness exercises the previous and current source with a fake height map and native writes. It compares vertex and index bytes, lock order, a scene that reaches the buffer limit, and successful retry after either map or unmap fails.

For the in-game check use `[RANK] Final Crossroad ZH v1.map (2)`. Select GLA - Assault Division, build a Barracks and an Arms Dealer, and order Rebel bikers at the Barracks. Destroy several on open terrain and along the road. Check newly added and overlapping marks as the view moves and zooms, including after minimizing and restoring. Reload the match or play the replay to check buffer recreation. The game test uses the existing mod assets.

Restore the preceding GFX 13a.7 executable if the game test needs rollback. To roll back the source, revert the adapter and consumer changes and rebuild with the preceding buffer code.
