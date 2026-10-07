# Compatibility

## Tested target

English Trails in the Sky the 3rd, September 22, 2022 build, Windows DX9:

- Executable: `ed6_win3_DX9.exe`
- SHA256: `6939b938b71b280ad9115e30ace16f97923a5c232414ce71f75f7f326ab9c75a`
- PE timestamp: `0x632ca6fb`
- Image size: `0x02fd7000`
- Preferred/runtime base: `0x00400000`
- LB-ARK version used for local tests: 1.3.2
- Optional SoraVoice version used: Lite 20230823

The installer requires the exact DX9 hash. The plugin checks known PE profiles, expected hook signatures, and pause-check counts before applying its core hooks. HUD, cursor, and voice integration also check their own expected signatures. This is not a promise of transactional rollback for every possible allocation/protection failure or other mod conflict.

Check your executable without uploading it:

```powershell
Get-FileHash .\ed6_win3_DX9.exe -Algorithm SHA256
```

## DX8

The source includes the September 2022 DX8 profile (`ed6_win3.exe`, timestamp `0x632ca6f8`, image size `0x02fdf000`). Its hook signatures and initialization were checked locally, but a complete gameplay pass was not finished. HUD and cursor improvements are DX9-only. Use DX9 for the public release; DX8 is experimental.

## Other releases

Other executables, Sky FC/SC, remakes, Linux/Proton/Wine, and additional mod combinations have not been validated. Different binaries need their own offsets and testing, not just a renamed executable or a disabled check.

## What was validated

- Left/right Ctrl and Shift latch on a press and retain their state after release; F9 disables turbo.
- Walk, sprint, and sprint-plus-turbo movement differed as expected in local field tests.
- The simulation clock advanced while unfocused; specific cutscene playback was not separately observed.
- HUD draw/state restoration and on/off behavior were checked on DX9.
- SoraVoice SE volume steps and mute/unmute were checked with the supported DLL.
- Mouse ghosting, sticky bar, stationary outside dismissal, and initial selection were visually tested.
- After the field/modal scope fix, the user playtested and reported the Equipment softlock appeared fixed. The regression fixture verifies that an Equipment caller cannot invoke custom parent-bar dismissal.

This was a bounded local playtest, not a full playthrough or an exhaustive compatibility sweep.
