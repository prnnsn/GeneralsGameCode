# GFX 13b.2c — dynamic buffer reservations, maps and CPU sorting lifetime

Status: implementation task for local agent; blocked on accepted #88 and the non-streaming 13b.2b adapter. Start from then-current main after both are approved. Verify corrected #87 inventory and phase-specific consumers before implementation. Related #86, #82. No automatic merge.

## Source evidence and risks
- Core/Libraries/Source/WWVegas/WW3D2/dx8vertexbuffer.cpp Allocate_DX8_Dynamic_Buffer and dynamic WriteLockClass (~760–940); dx8indexbuffer.cpp dynamic WriteLockClass (~440) and allocation. Reservations use element offsets; GPU locks use byte offsets and lengths; offset zero DISCARD, otherwise NOOVERWRITE; VB also NOSYSLOCK. Existing lock constructors forward HRESULT to DX8_ErrorCode and destructors unconditionally call Unlock. IB lock holds an additional reference. Preserve reference balance and thread ownership, not merely pointers.
- Capacity increases replace backing buffers; wrap sets offset to zero. Access destructors advance reservation offset and clear in-use flags. Reset/deinit paths and frame boundaries differ between GPU and CPU sorting.
- CPU sorting pages are 65,535 elements, with retained/retired page reuse and Num_Refs()==1 eligibility; queued render states may hold refs. Preserve GFX 06/07/11 and deterministic ordering; never map these pages as GPU buffers or recycle while a draw still refers to them.
- Both games have raw volume-shadow buffer locks; inspect their usage in inventory, but do not combine GFX 09b batching or profiling optimization with this migration.

## Implementation slices
1. Separate reservation (capacity, backing owner/generation, first/count, element units), successful scoped map (pointer/range/mode), and committed draw range. Convert via the checked helpers. Support explicit empty/no-op without native zero-size lock. Keep render-thread-only GPU operations. Retain old caller behavior on success. Document and test failure policy; a failed reservation, lock or unlock cannot publish geometry or trigger a bogus unlock.
2. Preserve dynamic VB/IB grow/wrap/reset semantics; choose DISCARD/NOOVERWRITE from valid range and outstanding draws. Validate base vertex and first index separately; no byte scaling of draw offsets. Check signedness, max 65,535 count, primitive topology, and referenced vertex span. Preserve old vertex_count fallback at caller boundary.
3. Keep CPU sorting storage separate, retain references until queued draw completion; preserve retirement/reuse/deinit and sorting worker behavior. Avoid lifetime changes or new races. Check both VB and IB and mismatch between successful maps in a paired submission.
4. Migrate the smallest representative streaming caller(s) first; inventory terrain tracks/particles and sorted paths; stage direct raw shadows and other consumers independently if necessary. No unrelated performance tuning, new threads, index expansion, changes to SIM, RNG, assets, replay/network or graphics settings. Keep diagnostics and baseline profiling.

## Acceptance matrix
- Exact success-path native call trace: offset zero and nonzero, byte sizes, DISCARD, NOOVERWRITE, NOSYSLOCK, references and ordering; all four draw-offset terms nonzero.
- Failed allocation, reservation, lock/unlock, partial VB/IB success, null pointers, zero range, overflow, end-at-capacity and out-of-range. Assert no submission/incorrect unlock or leaked references on failure; retry/reset restores valid state.
- Ring wrap, grow, frame reset, device recovery, queued old-page draw after retirement, multiple retired pages, pool full and reuse only after refs retire. Verify paired VB/IB lifetime, debug and release behavior and no changes to CPU sort order.
- Build both games and both editors (Win32 Release), both Google Test executables; compare representative particle/track/scorch and sorted-heavy scenes, camera zoom, Alt+Tab, replay, and profiling trace to prior DX8. If live paths migrated, perform matched replay/CRC and network test; report what was actually exercised. Keep local mod files unchanged; no installed EXE replacement without owner approval.
- Report deferred layout/consumer variants and remaining direct DX8 calls; update inventory/delta when implementation is accepted.

This is a dependency-aware implementation brief only. Do not claim a functioning adapter or runtime tests from this document.
