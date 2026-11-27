# Named radius cursors

Named radius cursors let mods give targeted General Powers and superweapons their own terrain decals. Multiple abilities can share a decal style while keeping the radius defined by each ability.

## INI files

| File | Purpose |
| --- | --- |
| `Data/INI/RadiusCursor.ini` or `Data/INI/RadiusCursor/*.ini` | Defines named decal styles. The engine loads these files before command buttons, including when World Builder initializes its shared ControlBar. |
| `Data/INI/CommandButton.ini` and its fragments | A targeted special power can reference a named style with its existing `RadiusCursorType` field. |
| `Data/INI/CommandSet.ini` and its fragments | Assigns the command button to the faction or unit that uses it. |
| `Data/INI/SpecialPower.ini` and its fragments | The associated power's existing `RadiusCursorRadius` supplies the display radius. |
| `Data/INI/InGameUI.ini` | Continues to define the built-in styles. |

The engine uses the existing `RadiusDecalTemplate` parser and renderer. This affects terrain targeting decals for special powers. Mouse pointer names, weapon-derived radius sources, and replay and save formats use their existing definitions.

## Example

Choose a texture from the mod. This definition uses the Spy Satellite texture from Contra Invasion.

```ini
RadiusCursor CUSTOM_SPY_SATELLITE_RED
    Texture = SCCSpySatellite_USA
    Style = SHADOW_ALPHA_DECAL
    OpacityMin = 50%
    OpacityMax = 100%
    OpacityThrobTime = 1000
    Color = R:255 G:70 B:80 A:255
    OnlyVisibleToOwningPlayer = Yes
End
```

Set `RadiusCursorType = CUSTOM_SPY_SATELLITE_RED` on the relevant command button. The ability's configured radius applies to the new style.

Names are case-insensitive. A later definition of the same name can replace fields of the previous definition. Built-in names are reserved. Unknown names and new definitions with an empty texture name are errors. The parser checks that the name is present. Verify the texture asset by displaying the decal.

Custom IDs start after the built-in range. The engine keeps them as local integers and keeps decal template addresses stable as definitions are added. Map-scoped definitions loaded by the engine are discarded on reset, and global styles are restored. Switching between pending commands refreshes the radius decal even when both commands share a style. A period of zero displays steady opacity at `OpacityMax`.

## Reproduce in a skirmish

1. Prepare a separate working directory for the test installation. Make the INI changes there, then start the fork with `generals.exe -quickstart -setCwd "<test-installation>"`.
2. Add two named styles with distinct colors. Duplicate an existing targeted power's command button and give each copy a different style. Bind the copies to two faction command sets and select those factions in separate skirmishes. Compare the decals while aiming the ability. This setup works with the normal skirmish data before a map is selected.
3. To check radius switching, bind two powers with different values of `RadiusCursorRadius` to the same named style. Select one power and then the other without first cancelling targeting. Each circle should use the selected power's radius.
4. Set one style's `OpacityThrobTime` to `0` and check that the decal stays visible at its maximum configured opacity. Cancel targeting and verify the decal clears.

The Zero Hour Release build and the standalone Contra Invasion profile were used for the Spy Satellite test. The mod maintainer confirmed the test in a match. Both World Builder editions use the shared ControlBar initialization path in source. Editor runtime, VC6 builds, and multiplayer replay and CRC checks remain to be verified separately.

## Restore the previous configuration

Restore the affected command-set slots and each command button's previous built-in `RadiusCursorType`. Remove the added definitions once no buttons reference them. If reverting the executable to a build without named cursors, restore the compatible INIs first. An isolated test installation can be retired by returning to the regular launcher.
