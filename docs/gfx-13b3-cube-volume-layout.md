# feat(gfx): [GFX 13b.3k] Add checked cube and volume texture layouts

13b.3k: checked canonical CPU subresource layouts only; no actual DDS parser, cube/volume loader or native allocation changes. Verify cube face ordering and supported formats against local assets before migrating. Tests registered; Win32 build pending.

## Host validation

GCC C++17, -Wall -Wextra -Werror, undefined/bounds sanitizers. Hash-verified exact new header (7c72245c02e5d19fc6cb37040eae0f9603042589), main RenderTexture.h/.cpp (9e27a8102267c4931a9a7d7dc8e58fcf748ecefb/e5d9f6d4050fedcd19ad88abbce3e0d43c04eea2), minimal WW3D format declarations and stub size/alpha for L8/A8R8G8B8. 6,285 volume mip cases and 198 cube face/mip cases passed independent footprint, offset and padded-span checks. Not a game build, full format matrix, native DDS test or registered Google Test execution.
