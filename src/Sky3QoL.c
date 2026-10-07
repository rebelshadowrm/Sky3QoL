/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Alex Basine
 */
/* Build: zig cc -target x86-windows-gnu -shared -O2 Sky3QoL.c -o Sky3QoL.dll -luser32
 * LB-ARK plugin for the September 2022 Sky the 3rd executables.
 * Runtime hooks only: no synthetic key events and no executable edits.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    DWORD timestamp, image_size, turbo_rva, input_rva, tick_rva, pump_rva;
    DWORD focus_va, timer_va, default_run_va, battle_compare_va;
} Profile;
static const Profile profiles[] = {
    {0x632ca6fb,0x2fd7000,0xb50f0,0xb4440,0x166510,0x1583a0,
     0x629f7e,0x887c90,0x9a24bc,0x407dc6},
    {0x632ca6f8,0x2fdf000,0xe70a0,0xe63f0,0x198710,0x18a5a0,
     0x65eafe,0x8bbbd0,0x9d67dc,0x448c06}
};
/* Exposed for runtime verification, never used to synthesize input. */
__declspec(dllexport) volatile struct {
    DWORD version, turbo, sprint, initialized, updates, foreground;
    DWORD background_patches, last_input_low, last_input_high;
} qol_state = {12};

static BYTE *base;
static const Profile *profile;
static int ctrl_down, shift_down, off_down, sprint_initialized;
static DWORD last_poll;
static volatile DWORD turbo_mask;
static void (__cdecl *original_pump)(void);
static BYTE (__cdecl *original_tick)(void);
static uint64_t (__attribute__((thiscall)) *original_input)(void *);
static void cursor_before_input(DWORD);
static uint64_t cursor_finish_input(void *, uint64_t);

#include "Sky3VoiceVolume.h"

static int focused(void) {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}
static void update_toggles(void) {
    DWORD now = GetTickCount();
    /* The original message pump is also called inside busy frame waits. */
    if ((DWORD)(now - last_poll) < 2) return;
    last_poll = now;
    int ctrl = !!(GetAsyncKeyState(VK_CONTROL) & 0x8000);
    int shift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
    int off = !!(GetAsyncKeyState(VK_F9) & 0x8000);
    int active = focused();
    if (!sprint_initialized) {
        qol_state.sprint = !!*(BYTE *)(uintptr_t)profile->default_run_va;
        sprint_initialized = 1;
    }
    if (active) {
        if (ctrl && !ctrl_down) qol_state.turbo = !qol_state.turbo;
        if (shift && !shift_down) qol_state.sprint = !qol_state.sprint;
        if (off && !off_down) qol_state.turbo = 0;
    }
    ctrl_down = ctrl;
    shift_down = shift;
    off_down = off;
    turbo_mask = qol_state.turbo ? 2 : 0;
    qol_state.foreground = active;
    ++qol_state.updates;
    voice_sync();
}
static void __cdecl pump_hook(void) {
    update_toggles();
    original_pump();
}
static BYTE __cdecl tick_hook(void) {
    update_toggles();
    return original_tick();
}
static BYTE __cdecl turbo_hook(void) {
    /* Keep this integer-only: original callers retain live XMM registers. */
    return (BYTE)qol_state.turbo;
}
static uint64_t __attribute__((thiscall)) input_hook(void *context) {
    update_toggles();
    cursor_before_input((DWORD)((BYTE *)__builtin_return_address(0)-base));
    uint64_t input = cursor_finish_input(context, original_input(context));
    if (!focused()) input = 0;
    else {
        /* DefaultRun flips the interpretation of the game's walk/run bit. */
        int default_run = !!*(BYTE *)(uintptr_t)profile->default_run_va;
        input &= ~UINT64_C(0x400000);
        if ((int)qol_state.sprint != default_run) input |= UINT64_C(0x400000);
    }
    /* Some game paths inline the turbo query; feed their internal bit too. */
    input &= ~UINT64_C(0x200000000);
    if (qol_state.turbo) input |= UINT64_C(0x200000000);
    qol_state.last_input_low = (DWORD)input;
    qol_state.last_input_high = (DWORD)(input >> 32);
    return input;
}
static int write_code(void *address, const void *bytes, SIZE_T size) {
    DWORD old, ignored;
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old)) return 0;
    memcpy(address, bytes, size);
    FlushInstructionCache(GetCurrentProcess(), address, size);
    VirtualProtect(address, size, old, &ignored);
    return 1;
}
static void jump_bytes(BYTE *out, const void *from, const void *to) {
    out[0] = 0xe9;
    *(int32_t *)(out + 1) = (const BYTE *)to - (const BYTE *)from - 5;
}
static void *trampoline(BYTE *target, SIZE_T length) {
    /* Verified prologues have only pushes and absolute-address instructions. */
    BYTE *copy = VirtualAlloc(NULL, length + 5, MEM_COMMIT|MEM_RESERVE,
                             PAGE_EXECUTE_READWRITE);
    if (!copy) return NULL;
    memcpy(copy, target, length);
    jump_bytes(copy + length, copy + length, target + length);
    return copy;
}
static int detour(BYTE *target, const void *replacement, SIZE_T length) {
    BYTE bytes[16];
    if (length < 5 || length > sizeof bytes) return 0;
    memset(bytes, 0x90, length);
    jump_bytes(bytes, target, replacement);
    return write_code(target, bytes, length);
}
static int focus_comparison(BYTE *p) {
    return p[0] == 0x80 && p[1] == 0x3d &&
           *(DWORD *)(p + 2) == profile->focus_va && p[6] == 0;
}
static int clock_comparison(BYTE *p) {
    return p[0] == 0x80 && p[1] == 0x3d &&
           *(DWORD *)(p + 2) == profile->timer_va + 0x30 && p[6] == 0;
}
static int simulation_comparison(BYTE *p) {
    if (!focus_comparison(p)) return 0;
    if (p[7] == 0x74 && clock_comparison(p + 9)) return 1;
    if (p[7] == 0x0f && p[8] == 0x84 && clock_comparison(p + 13)) return 1;
    return (uintptr_t)p == profile->battle_compare_va &&
           p[7] == 0xf3 && p[8] == 0x0f && p[9] == 0x11 &&
           p[15] == 0x0f && p[16] == 0x84 && clock_comparison(p + 21);
}
static void status(const char *message) {
    FILE *f = fopen("data/plugins/Sky3QoL.log", "w");
    if (f) {
        fprintf(f, "%s\nBackground simulation checks patched: %lu\n",
                message, (unsigned long)qol_state.background_patches);
        fclose(f);
    }
}
#include "Sky3TurboHUD.h"
#include "Sky3Cursor.h"
__declspec(dllexport) void __cdecl lb_init(void) {
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sections;
    BYTE *turbo, *input, *tick, *pump;
    BYTE *checks[32];
    unsigned count = 0, i, j;
    BYTE force_active = 2;
    base = (BYTE *)GetModuleHandleW(NULL);
    dos = (IMAGE_DOS_HEADER *)base;
    nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    if ((uintptr_t)base != 0x400000) {
        status("Unsupported executable base; no changes applied."); return;
    }
    for (i = 0; i < sizeof profiles/sizeof profiles[0]; ++i)
        if (profiles[i].timestamp == nt->FileHeader.TimeDateStamp &&
            profiles[i].image_size == nt->OptionalHeader.SizeOfImage)
            profile = profiles + i;
    if (!profile) { status("Unsupported game build; no changes applied."); return; }
    turbo = base + profile->turbo_rva;
    input = base + profile->input_rva;
    tick = base + profile->tick_rva;
    pump = base + profile->pump_rva;
    if (memcmp(turbo, "\x8b\x0d", 2) ||
        memcmp(turbo + 6, "\x33\xc0\x83\xe1\x02\x0b\xc1\x75\x10", 9) ||
        memcmp(input, "\x83\xec\x2c\x8b\x15", 5) ||
        memcmp(tick, "\x6a\x01\x6a\x01\xb9", 5) ||
        *(DWORD *)(tick + 5) != profile->timer_va ||
        memcmp(pump, "\x51\x56\x6a\x00\xff\x15", 6)) {
        status("Unexpected hook signatures; no changes applied."); return;
    }
    sections = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (!(sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        BYTE *section = base + sections[i].VirtualAddress;
        DWORD size = sections[i].Misc.VirtualSize;
        for (j = 0; j + 32 < size; ++j) {
            if (simulation_comparison(section + j)) {
                if (count >= 32) { status("Too many pause checks; no changes applied."); return; }
                checks[count++] = section + j;
            }
        }
    }
    if (count != 14) { status("Unexpected pause check count; no changes applied."); return; }
    original_tick = (void *)trampoline(tick, 9);
    original_input = (void *)trampoline(input, 9);
    original_pump = (void *)trampoline(pump, 10);
    if (!original_tick || !original_input || !original_pump) {
        status("Unable to allocate hook trampolines; no changes applied."); return;
    }
    for (i = 0; i < count; ++i) {
        if (!write_code(checks[i] + 6, &force_active, 1)) {
            status("Unable to patch a pause check."); return;
        }
        ++qol_state.background_patches;
    }
    /* Route the compiler's two inline copies of the turbo query to our latch. */
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (!(sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        BYTE *section = base + sections[i].VirtualAddress;
        DWORD size = sections[i].Misc.VirtualSize;
        DWORD current_high = *(DWORD *)(turbo + 2);
        DWORD previous_high = *(DWORD *)(turbo + 17);
        DWORD latch = (DWORD)(uintptr_t)&turbo_mask;
        for (j = 0; j + 32 < size; ++j) {
            BYTE *p = section + j;
            if (p == turbo) continue;
            if (p[0] == 0x8b && p[1] == 0x0d &&
                *(DWORD *)(p + 2) == current_high &&
                !memcmp(p + 6, "\x33\xc0\x83\xe1\x02\x0b\xc1\x75", 8) &&
                p[15] == 0x8b && p[16] == 0x0d &&
                *(DWORD *)(p + 17) == previous_high &&
                !memcmp(p + 21, "\x83\xe1\x02\x0b\xc1", 5)) {
                write_code(p + 2, &latch, 4);
                write_code(p + 17, &latch, 4);
            }
        }
    }
    if (!detour(turbo, turbo_hook, 6) || !detour(input, input_hook, 9) ||
        !detour(tick, tick_hook, 9) || !detour(pump, pump_hook, 10)) {
        status("Unable to install toggle hooks."); return;
    }
    qol_state.initialized = 1;
    voice_config();
    hud_install(nt);
    cursor_install(nt);
    status("Sky3QoL active: Ctrl turbo toggle, Shift sprint toggle, F9 turbo off. No synthetic key events.");
}
