# feat(gfx): [GFX 13b.3k] Add checked cube and volume texture layouts

13b.3k: checked canonical CPU subresource layouts only; no actual DDS parser, cube/volume loader or native allocation changes. Verify cube face ordering and supported formats against local assets before migrating. Tests registered; Win32 build pending.
