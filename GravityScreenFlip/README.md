# Gravity Screen Flip (Geode mod)

Target: Geometry Dash 2.2081, Windows, Geode v5.x.

## Before you build
1. Edit `mod.json`: change `"id"` (`yourname.gravity-screen-flip`) and `"developer"` to your own. The id must be `developer.mod-name`, lowercase.
2. Make sure `"geode"` in `mod.json` is not newer than the Geode loader you have installed.

## Prerequisites (Windows)
- Geometry Dash 2.2081 (Steam) with Geode installed
- Visual Studio 2022 (or Build Tools) with the "Desktop development with C++" workload
- CMake 3.21+ and Git
- Geode CLI (https://docs.geode-sdk.org/getting-started/geode-cli)

## Build to a `.geode` file
```bat
geode sdk install            :: once, skip if you already have the SDK
geode sdk update             :: make sure the SDK is current
geode sdk install-binaries   :: once per SDK version, downloads Geode binaries

cd GravityScreenFlip
geode build
```
The package is written to `build\yourname.gravity-screen-flip.geode`.

If you prefer plain CMake (with `GEODE_SDK` set to your SDK folder; use the generator/architecture your Geode setup docs specify):
```bat
cmake -B build
cmake --build build --config RelWithDebInfo
```
The `.geode` file is produced inside `build\`.

## Install
Copy the `.geode` file to `Geometry Dash\geode\mods\` and start the game (or use `geode package install <file>`).

## Settings
| Setting | Default | Meaning |
|---|---|---|
| Enabled | on | Master switch |
| Rotation Duration | 0.3 s | 0 = instant flip |
| Keep Spinning Same Direction | off | Off: 0 to 180 and back. On: always +180 clockwise |
| Zoom to Hide Edges | on | Zooms during the spin so corners never show empty space |
| Keep HUD Upright | on | Counter-rotates the UI layer if it is a child of PlayLayer |

## How it works
`PlayLayer::update` is hooked. After the original runs, the mod reads `m_player1->m_isUpsideDown`, eases an angle toward 0 or 180, and applies rotation, scale and position to the PlayLayer node so the rotation pivot is the screen centre. `PlayLayer::resetLevel` is hooked to snap to the right orientation on respawn (no spin when you die). Menus, the pause menu and the editor are separate nodes and are not touched.

## Limitations
- **The view rotates; inputs do not.** In normal levels this does not matter (one button). In platformer levels, left/right now appear reversed while flipped, because the player's +X still runs toward the screen's left. The mod does not remap keys.
- **Horizontal movement is mirrored on screen.** The player still moves forward in level coordinates, but while flipped it travels right-to-left on screen and the level scrolls the opposite way. Look-ahead distance is the same (the player's screen position is mirrored with the view). Reading rhythm-based sections can feel very different.
- **Camera controls are unchanged.** Camera triggers, shake, zoom, offsets, static and edge cameras all operate in level space and are rotated with everything else, so e.g. a "move up" camera trigger appears as downward while flipped. Mods that read screen coordinates (cursor-based tools, some overlays) are not rotation-aware.
- **HUD handling is best-effort.** If the UI layer is a child of PlayLayer it is counter-rotated. Elements that live elsewhere inside PlayLayer (depending on game version, e.g. progress bar/percentage) may rotate with the view.
- **Dual mode** follows player 1 only.
- **Mid-rotation edges.** Without "Zoom to Hide Edges" the 16:9 view exposes empty corners around 90 degrees. The zoom option prevents that at the cost of a brief zoom.
- **Other mods** that set PlayLayer's own position/rotation/scale (screen-shake or zoom mods) will conflict.
- Pausing freezes an in-progress rotation; it resumes on unpause.
- Not compiled or tested in the environment that generated it (no network access). If a future Geode release renames a binding, the compiler will point at the line.
