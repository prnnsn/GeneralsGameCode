# GFX 13b.2c — checked streaming writes for terrain particles

The first streaming consumer is the terrain-conforming particle batch. It still uses the existing dynamic vertex layout, UInt16 indices and W3D reservation objects. The resource contract and source inventory are in #86 and #91. This stage builds on the descriptor checks in #88 and the DX8 write handling in #89.

## Reservation and submission

- The checked constructors reject an empty request, an unavailable device or an occupied reservation. Native allocation keeps the existing flags, capacity growth and resource eviction retry. A failed allocation releases its wrapper and leaves the allocator available.
- GPU writes validate byte ranges from element offsets. Offset zero uses DISCARD, a nonzero offset uses NOOVERWRITE, and vertex locks also use NOSYSLOCK. The index write holds its existing temporary reference. A failed native write retires the backing from the ring. Current reservations and queued render state retain their own references.
- The particle batch uploads vertices, then indices, and checks the resulting triangle span before binding. The base vertex and first index are checked in elements. Profiling and draw submission occur after both writes and binding succeed. CPU-side batch counts reset on every exit.
- The shared CPU sorting allocation, retirement and reuse paths remain as before. Terrain tracks, older dynamic callers and raw shadow buffers retain their existing paths for caller-specific migration.

## Verification

Build both Win32 Release games, both World Builders and both Google Test programs. The streaming suite checks native flags and byte counts, range limits, failed maps and unmaps, and unavailable reservations. The x86 mock regression exercises the actual ring allocation logic for capacity limits, wrap, growth, a retained older buffer, frame reset and retry. It also exercises the terrain batch ordering and absence of a draw after a failed vertex upload, index upload or bind.

For the visual test use the established conforming-shockwave launcher for Contra Invasion. Select GLA - Assault Division on `[RANK] Final Crossroad ZH v1.map (2)`. Build a Barracks and an Arms Dealer, order several Rebel bikers and destroy them in quick succession on flat and sloped ground. Watch the particle wave during zoom and after minimizing and restoring. Repeat the same passage in a replay using the same test profile.

The launcher restores the particle INI at exit. Restore the GFX 13b.2b executable if the visual check needs rollback. To roll back source changes, revert this streaming adapter and return the terrain batch to the earlier dynamic write calls.
