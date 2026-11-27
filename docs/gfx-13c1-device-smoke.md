# GFX 13c.1 — standalone D3D11 device smoke test

Independent of 13b.3 resource migration. Follow #82, #86 and roadmap. Baseline ed9ae439. Implementation brief only. This is not a playable renderer or a change to the shipping executable.

## Scope
Create an opt-in Windows-only standalone test executable/target using Windows SDK D3D11/DXGI, isolated CMake switch default OFF, with no dependency from the four shipping game/editor targets. Keep legacy Win32 D3D8 SDK/build unchanged. Check existing CMake options, supported compiler and SDK; report toolchain prerequisites. Use explicit device/context and windowed swap chain creation, hardware feature-level negotiation and explicit WARP test mode; document chosen feature-level minimum and depth/stencil format with capability checks (initial D24_UNORM_S8_UINT candidate). Handle unsupported hardware clearly, no silent game fallback. Debug layer optional and gracefully absent; report selected adapter, feature level and HRESULT errors.

Create RTV/DSV, set viewport, clear known color and depth/stencil, present. Verify resize releases/unbinds all back-buffer references before ResizeBuffers, recreates views, handles zero/minimized size without spinning, and clean shutdown. Check failure paths for partial initialization, resize and present/device removal; log removal reason; do not claim full game resource recovery. Separate deterministic WARP/headless-capable readback test from interactive window/present smoke where practical. Use scoped COM ownership, render-thread context, no legacy API mixing or native pointer escapes into game interfaces. No game Renderer selection, shader translation, SIM or mod changes in this slice.

## Acceptance
- Default OFF: normal Win32 Release builds of both games and editors remain unchanged, no new D3D11 runtime dependency on them; check link/imports.
- ON: compile and run isolated x86 executable on hardware and WARP where available; record adapter/feature level, clear/readback color, depth/stencil creation, resize/minimize/restore and repeated init/shutdown. Debug layer clean where installed. Explicitly mark unavailable device/debug/runtime cases; do not simulate success.
- Exercise injected init/resize/present failures or test seam and verify cleanup. Document run commands and actual results. Do not merge automatically. Subsequent 13c.2 implements neutral GPU resources only after relevant resource contracts are stable.
