# GFX 13b.3j — checked terrain atlas update

Supersedes the former 13b.3f brief after #101–107. Uses #105 single-mip mapping for TerrainTextureClass::update only. All tile and border rectangles are prevalidated; preserves bottom-up source, A1R5G5B5 packing and wrap border order, respects actual row pitch. Unmap before backend box filter/LOD; return 0 on failure. Other terrain texture writers remain unchanged. Review callers' handling of 0, format fallback and partial failure. New tests cover orientation, padding and invalid rectangles. Local agent must build both games/editors and compare real map atlas bytes and mip/LOD, reload, zoom and Alt+Tab. No engine build or game test claimed.

## Host validation

The exact TerrainAtlasWrite.h blob (013a20024a311e693ca64563359b411d1848aeca) was compiled in an isolated GCC C++17 harness with -Wall -Wextra -Werror and undefined/bounds sanitizers. 80 combinations of tile sizes, pitches and origins matched an independent per-pixel wrapped atlas; invalid border, short destination and short source were rejected without mutation. This did not compile TerrainTex.cpp, execute DX8 or test actual mod assets.
