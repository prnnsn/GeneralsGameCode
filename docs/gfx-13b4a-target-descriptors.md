# feat(gfx): [GFX 13b.4a] Add checked render target and depth descriptors

13b.4a: neutral single-sample target/depth description; compressed/palette and multisample targets are not accepted in this initial contract. Logical CPU footprint is not physical GPU allocation. Legacy depth format emulation and target creation require native capabilities and separate review. Tests registered; Win32 build pending.
