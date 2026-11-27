# feat(gfx): [GFX 13b.4a] Add checked render target and depth descriptors

13b.4a: neutral single-sample target/depth description; compressed/palette and multisample targets are not accepted in this initial contract. Logical CPU footprint is not physical GPU allocation. Legacy depth format emulation and target creation require native capabilities and separate review. Tests registered; Win32 build pending.

## Host validation

GCC C++17, -Wall -Wextra -Werror, undefined/bounds sanitizers. Hash-verified exact new header (5750750383ef930f88b197eebc0856391992de22) and main RenderTexture.h/.cpp, minimal WW3D format declarations and stub size/alpha for L8/A8R8G8B8. Seven legacy depth formats and footprint, overflow, stencil capacity, dimension, viewport boundary and NaN rejection checks passed. Not a game build, native depth-format capability or registered Google Test execution.
