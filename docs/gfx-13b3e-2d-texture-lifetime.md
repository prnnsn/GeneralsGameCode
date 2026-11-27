# GFX 13b.3e — ordinary 2D texture creation and handoff

Regular 2D texture tasks acquire one backend reference from
`Renderer::Create_Load_Texture_2D`. They keep that reference while mips are
mapped and initialized. The DX8 backend applies the completed texture to
the existing W3D texture object and releases the task reference. W3D
takes its own reference in `Apply_New_Surface`. A map or unmap failure
releases the unpublished reference before using the existing
missing-texture path.

DDS and TGA branches pass the same dimensions, format and mip request
to the original DX8 allocator. The zero mip count still requests the
full chain. The supported DX8 load allocation stays in the managed pool,
with the same native format selection and memory-pressure retry.
Cube, volume, thumbnail and immediate surface loads retain their
existing creation and lifetime paths. Texture and INI file formats
are unchanged.

## Validation and rollback

Build Win32 Release for Generals, Zero Hour and both World Builder
targets. Run both Google Test suites, including the existing DDS
staging and DX8 mip mapping failure tests.

In the mod, start a daytime skirmish with the USA Air Force General.
Build the Levitation Pad at maximum texture quality. Inspect its intact
and damaged models and save the battle. Return to the main menu, switch
to minimum texture quality and reload the same save without restarting.
Repeat from the main menu at maximum quality. Changing the setting
invalidates the existing textures. Check that the correct materials
reappear each time and that the game can return to the battle.

To roll back, revert the GFX 13b.3e patch and rebuild, or restore the
backed-up GFX 13b.3d executable with the game closed.
