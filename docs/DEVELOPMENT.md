# Development

## Layout

- `src/Sky3QoL.c`: build profiles, turbo/sprint latches, game input and timing hooks, LB-ARK entry point.
- `src/Sky3TurboHUD.h`: geometric DX9 badge with Direct3D state save/restore and intermediate-target filtering.
- `src/Sky3VoiceVolume.h`: optional verified SoraVoice player-volume integration.
- `src/Sky3Cursor.h`: game-local cursor imports, physical cursor rendering, virtual menu positions, and field-only outside-click dismissal.
- `tests/cursor-regression.c`: isolated regression fixture using mapped scratch addresses and fake device/UI state.

The source is independently authored plugin code. It interoperates with LB-ARK and SoraVoice; it is not a fork of either project, and no upstream source/binary is distributed here.

## Build

`build.ps1` invokes Zig 0.14.1 as an x86 Windows GNU-target C compiler and packages only this project's DLL, INI, installer, documentation, and MIT license. No game installation is needed to build or run the isolated tests.

`Test.ps1` generates a fixture header from the production cursor header, redirecting fixed game addresses into scratch memory. It compiles with assertions enabled and tests field vs modal input routing, click consumption through release, keyboard/physical position separation, rendering without trails, and selection reset.

## Runtime behavior

LB-ARK calls exported `lb_init`. Core hooks preserve original native input and replace only the relevant turbo/sprint interpretation. The plugin polls physical Ctrl/Shift/F9 states and does not import SendInput or keybd_event.

The voice integration calls the supported SoraVoice player's existing thread-safe volume method; it does not patch SoraVoice code. Unknown voice DLL layouts are not used.

The cursor code hooks only the game's own SetCursorPos/GetCursorPos import entries. Its draw wrapper temporarily uses physical coordinates and restores navigation state afterward. The top bar shares a parent widget with camp/modal menus: custom dismissal must remain limited to native field return RVAs `0x15f803` and `0x15f81f`, never Equipment's `0x16bc84` or other modal callers.

Original game files are not modified by the plugin. Installed settings are read at startup. Diagnostic state is exported through `qol_state`, `hud_state`, `voice_state`, and `cursor_state`; startup status goes to `data/plugins/Sky3QoL.log`.

## Reporting and porting

Include binary hashes and exact repro steps with reports. Verify a new profile against the actual executable and test its UI transitions before adding support. Do not repair live modal state by clearing transition flags: a prior development recovery attempt on an already corrupted menu was followed by a crash.

## References

- [LB-ARK plugin loading](https://github.com/Aureole-Suite/LB-ARK)
- [SoraVoice source](https://github.com/ZhenjianYang/SoraVoice)
- [SetCursorPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursorpos)
- [Direct3D9 state blocks](https://learn.microsoft.com/en-us/windows/win32/direct3d9/state-blocks-save-and-restore-state)
