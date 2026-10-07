# Sky3QoL

Native quality-of-life fixes for **Trails in the Sky the 3rd** on Windows: toggle turbo and sprint, an in-game turbo badge, dialogue volume linked to sound effects, and mouse menus that keep your pointer where you put it.

Built for the September 2022 English PC executable. **DX9 is the playtested target.** This does not target Sky FC/SC, the remakes, or arbitrary newer game builds.

## What this plugin adds

| Change | Behavior |
| --- | --- |
| Turbo toggle | Press either **Ctrl** once to toggle turbo; **F9** turns it off. |
| Sprint toggle | Press either **Shift** once to switch between walking and sprinting. |
| Turbo indicator | Gold `>> TURBO` badge at the top right, drawn inside the DX9 game. |
| Background simulation | Turbo stays enabled across Alt-Tab, and simulation can continue without game focus. Ordinary gameplay input is suppressed while unfocused. |
| Linked dialogue volume | With supported SoraVoice installed, dialogue follows the game's **SE volume and mute**. An additional voice gain setting is available. |
| No cursor snapping | Menus leave your physical mouse pointer in place. Keyboard menu coordinates and the rendered pointer are handled separately. |
| Sticky top bar | Moving off the field's top bar leaves it open. **Escape or an outside left click** closes it; that click is consumed until release. |
| No initial Status selection | Opening the top bar starts mouse focus at the actual pointer instead of selecting Status automatically. |

The toggles use native game hooks, not AutoHotkey or generated keypresses. Original game executables and archives are not rewritten. This plugin does not rebalance characters or combat.

## Install

1. Save and close the game.
2. Install [LB-ARK](https://github.com/Aureole-Suite/LB-ARK) from its own project. Its `d3dxof.dll` goes beside `ed6_win3_DX9.exe`. This project was tested with LB-ARK 1.3.2.
3. Download and extract the plugin ZIP from [Releases](https://github.com/rebelshadowrm/Sky3QoL/releases).
4. Run the included installer from PowerShell:

   ```powershell
   .\Install.ps1 -GameDirectory 'C:\Games\Trails in the Sky the 3rd'
   ```

   The installer verifies the tested DX9 executable, backs up any existing plugin files, installs this plugin, and preserves an existing `Sky3QoL.ini`.
5. Start `ed6_win3_DX9.exe` normally. LB-ARK loads the plugin automatically; no separate mod-loader launcher is needed.

For manual installation, copy the extracted ZIP's `data` folder into the game directory. Read [compatibility](docs/COMPATIBILITY.md) first. Manual copying does not perform the installer's hash check.

If Windows blocks local script execution, use `powershell -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1 -GameDirectory 'C:\Games\Trails in the Sky the 3rd'` after inspecting the script.

## Configuration and removal

Settings are in `data/plugins/Sky3QoL.ini`. Restart the game after editing them.

| Setting | Options |
| --- | --- |
| `[HUD] Enabled` | `1` shows the turbo badge; `0` hides it. |
| `[HUD] Position` | `top-right` or `bottom-right`. |
| `[HUD] Style` | `gold` or `white`. |
| `[Audio] LinkVoicesToSE` | `1` links supported SoraVoice dialogue to SE; `0` keeps SoraVoice's independent setting. |
| `[Audio] VoiceGainPercent` | Additional dialogue gain, `0` through `100`. |
| `[Mouse] DisableCursorSnap` | `1` uses the new mouse behavior; `0` restores native cursor handling. |
| `[Mouse] StickyTopBar` | `1` keeps the field bar open until dismissal; `0` restores closing on mouse leave. Requires snapping disabled. |
| `[Mouse] ClearTopBarSelection` | `1` removes the initial Status selection; `0` retains it. Requires sticky bar and snapping disabled. |

Core turbo/sprint/background hooks are installed together. HUD, voice-volume linkage, and mouse changes are optional through the settings above. For controller-oriented cursor navigation, set `DisableCursorSnap=0`.

To remove the plugin, close the game and rename `data/plugins/Sky3QoL.dll` to `Sky3QoL.dll.disabled`, or remove that DLL. Leave other mods' files in place. If replacing an older plugin, the installer keeps a timestamped backup under `Sky3QoL-Backups`.

## Other mods and credits

This repository contains our original plugin, **not forks of the projects below**. We used these projects alongside it and appreciate their authors' work. Download their mods and read their documentation directly:

| Project | Credit and role |
| --- | --- |
| [LB-ARK](https://github.com/Aureole-Suite/LB-ARK) | Kyuuhachi / Aureole Suite and contributors: file redirection and the native plugin loader this project uses. |
| [SoraVoice](https://github.com/ZhenjianYang/SoraVoice) | ZhenjianYang and contributors: the voice playback implementation. Our volume integration targets SoraVoice Lite 20230823. |
| [SkyInstaller](https://github.com/chyyran/skyinstaller) | chyyran and contributors: convenient installation of the separate voice setup. |
| [InstantKiseki](https://github.com/Kyuuhachi/InstantKiseki) | Kyuuhachi and contributors: instant dialogue text. This is a separate plugin, not a feature we wrote. |
| [More Detailed Skill Descriptions](https://www.reddit.com/r/Falcom/comments/1uci4h9/trails_in_the_sky_the_3rd_more_detailed_skill/) | SoftBrilliant: expanded skill numbers and descriptions; obtain it from the author's release thread. |
| [Falcom Mod Compendium](https://github.com/Red-BY/FalcomModCompendium) | Red-BY and contributors: a directory of other Falcom mods, including optional balance and presentation changes. |

Voices, instant text, and detailed descriptions were part of the original local setup, but are **separate downloads**. This repository and its plugin ZIP contain no game executable, game archive, Evolution voice asset, save, or upstream mod binary. Nihon Falcom and XSEED retain ownership of their game assets; upstream projects retain their own licenses. Our MIT license applies to our original source only.

## Controller rotation workaround

One tested DualSense setup exposed triggers on DirectInput X/Y Rotation and its right stick on Z/Z Rotation. This game's camera read X Rotation as a centered camera-stick axis, so a released trigger caused continuous rotation. The local keyboard/mouse setup disabled controller input and right-stick camera control through Config3 (`UseGamaPad=0`, `RightCamera=0`).

This is a documented configuration workaround, **not an axis-remapping feature of this DLL**. Other controllers may behave differently. Check the device with Windows `joy.cpl`; do not assume that every rotation problem is hardware drift.

## Build and contribute

Use [Zig 0.14.1](https://ziglang.org/download/0.14.1/) on Windows:

```powershell
.\build.ps1 -Zig 'C:\Tools\zig\zig.exe'
.\Test.ps1 -Zig 'C:\Tools\zig\zig.exe'
```

The build produces a 32-bit Windows DLL and plugin-only ZIP under `dist`. Tests exercise cursor/navigation logic in an isolated process; they do not launch the game or modify saves. See [development notes](docs/DEVELOPMENT.md), [compatibility](docs/COMPATIBILITY.md), and the [changelog](CHANGELOG.md).

Bug reports should include the executable SHA256, DX9/DX8 selection, plugin version, enabled options, other installed plugins, and exact steps. Please do not upload game executables, voice assets, or saves to this repository.
