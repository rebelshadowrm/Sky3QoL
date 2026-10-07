/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Alex Basine
 */
/* SoraVoice Lite 20230823's thread-safe Player::SetVolume(int) interface.
 * Layout/signatures verified against the bundled DLL and matching upstream source.
 * Unknown SoraVoice builds are left untouched. No SoraVoice code is patched.
 */
__declspec(dllexport) volatile struct {
    DWORD supported, volume, se_level, changes;
} voice_state;
static int voice_link = 1, voice_gain = 100;
static void *voice_last_player;
static int voice_last_volume = -1;

static int voice_readable(const void *address, SIZE_T length) {
    MEMORY_BASIC_INFORMATION info;
    if (!address || !VirtualQuery(address,&info,sizeof info)) return 0;
    if (info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD|PAGE_NOACCESS))) return 0;
    return (uintptr_t)address + length >= (uintptr_t)address &&
           (uintptr_t)address + length <= (uintptr_t)info.BaseAddress + info.RegionSize;
}
static void voice_sync(void) {
    /* Rounded linear gains matching the game's ten DirectSound volume steps. */
    static const int gains[10] = {2,4,10,18,32,42,56,60,100,100};
    if (!voice_link) return;
    BYTE *module = (BYTE *)GetModuleHandleW(L"ed_voice.dll");
    if (!module) return;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)module;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(module+dos->e_lfanew);
    if (nt->FileHeader.TimeDateStamp != 0x64e5c74c ||
        nt->OptionalHeader.SizeOfImage != 0x128e000 ||
        memcmp(module+0x154a0,"\x55\x8b\xec\x6a\xff\x68",6)) return;
    if (*(DWORD *)(module+0x1282530) != 3) return;
    BYTE *sv = *(BYTE **)(module+0x1282530+76);
    if (!voice_readable(sv,80)) return;
    void *player = *(void **)(sv+76);
    if (!voice_readable(player,4)) return;
    void **methods = *(void ***)player;
    if (!voice_readable(methods,16) || methods[2] != module+0x154a0 ||
        methods[3] != module+0x15540) return;
    int level = *(BYTE *)(uintptr_t)(profile->default_run_va-0x13);
    if (level > 9) level = 9;
    int muted = !!*(BYTE *)(uintptr_t)(profile->default_run_va-0x11);
    int volume = muted ? 0 : (gains[level]*voice_gain+50)/100;
    voice_state.supported = 1;
    voice_state.se_level = level;
    if (player != voice_last_player || volume != voice_last_volume) {
        void (__attribute__((thiscall)) *set_volume)(void *,int) = (void *)methods[2];
        set_volume(player,volume);
        voice_last_player = player;
        voice_last_volume = volume;
        voice_state.volume = volume;
        ++voice_state.changes;
    }
}
static void voice_config(void) {
    voice_link = GetPrivateProfileIntW(L"Audio",L"LinkVoicesToSE",1,L".\\data\\plugins\\Sky3QoL.ini");
    voice_gain = GetPrivateProfileIntW(L"Audio",L"VoiceGainPercent",100,L".\\data\\plugins\\Sky3QoL.ini");
    if (voice_gain < 0) voice_gain = 0;
    if (voice_gain > 100) voice_gain = 100;
}
