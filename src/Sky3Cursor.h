/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Alex Basine
 */
/* Keep menu navigation's virtual cursor separate from the physical pointer.
 * Game-local USER32 imports only; cursor rendering uses physical coordinates.
 */
__declspec(dllexport) volatile struct {
    DWORD initialized, enabled, blocked, repairs, last_caller_rva;
    LONG client_x, client_y;
} cursor_state;
static BOOL (WINAPI *cursor_original)(int,int);
static BOOL (WINAPI *cursor_get_original)(LPPOINT);
static void (__cdecl *cursor_render_original)(void);
static POINT cursor_physical, cursor_virtual;
static int cursor_virtual_active, cursor_had_virtual, cursor_force_physical;
static int cursor_sticky, cursor_block_click, cursor_clear_bar_selection, cursor_field_poll;
static void (__cdecl *cursor_close_bar)(void) = (void *)(uintptr_t)0x569ea0;
static int cursor_bar_open(void) {
    WORD flags = *(WORD *)(uintptr_t)0x8d8b1c;
    if (!flags || (flags & 8) || ((flags & 2) && !(flags & 4))) return 0;
    for (DWORD a=0x9db430;a<0x9db4b0;a+=4)
        if (*(DWORD *)(uintptr_t)a == 0x8d2a60) return 1;
    return 0;
}
static int cursor_mouse_down(void) {
    if (*(DWORD *)(uintptr_t)0x992f90)
        return !!((*(BYTE *)(uintptr_t)0x992ffc | *(BYTE *)(uintptr_t)0x992ffd) & 0x80);
    return !!(*(DWORD *)(uintptr_t)0x9bd454 | *(DWORD *)(uintptr_t)0x9bd458);
}
static void __attribute__((naked)) cursor_bar_leave(void) {
    __asm__ volatile(
        "cmpl %eax, 0x995264\n\t"
        "jle 1f\n\t"
        "testl $0x08000000, %edi\n\t"
        "jz 1f\n\t"
        "pushl $0x55fd98\n\tret\n\t"
        "1: pushl $0x55fd85\n\tret\n\t");
}

static POINT cursor_client(POINT point) {
    if (!*(BYTE *)(uintptr_t)0x629f7b) {
        point.x -= *(LONG *)(uintptr_t)0x9bd464;
        point.y -= *(LONG *)(uintptr_t)0x9bd468;
    }
    return point;
}
static void cursor_clear_open_selection(void) {
    if (cursor_sticky && cursor_clear_bar_selection && cursor_state.last_caller_rva == 0x169e76) {
        cursor_virtual_active = 0;
        *(LONG *)(uintptr_t)0x8d8aec = -1;
        *(BYTE *)(uintptr_t)0x629f79 = 1;
        *(BYTE *)(uintptr_t)0x665118 = 1;
    }
}
static BOOL WINAPI cursor_no_snap(int x, int y) {
    ++cursor_state.blocked;
    cursor_state.last_caller_rva = (DWORD)((BYTE *)__builtin_return_address(0)-base);
    POINT point;
    if (focused() && cursor_get_original(&point)) {
        cursor_physical = point;
        cursor_virtual.x = x;
        cursor_virtual.y = y;
        cursor_virtual_active = 1;
        point = cursor_client(point);
        cursor_state.client_x = point.x;
        cursor_state.client_y = point.y;
        cursor_clear_open_selection();
    }
    return TRUE;
}
static BOOL WINAPI cursor_get(LPPOINT point) {
    if (!cursor_get_original(point)) return FALSE;
    if (cursor_virtual_active || cursor_force_physical) {
        /* A click must always hit the visible, physical cursor location. */
        if (cursor_force_physical || !focused() || point->x != cursor_physical.x || point->y != cursor_physical.y ||
            (cursor_mouse_down() || ((GetAsyncKeyState(VK_LBUTTON)|GetAsyncKeyState(VK_RBUTTON)|
              GetAsyncKeyState(VK_MBUTTON)) & 0x8000))) {
            cursor_virtual_active = 0;
            /* Prevent a keyboard warp from looking like physical mouse motion. */
            POINT client = cursor_client(*point);
            *(LONG *)(uintptr_t)0x995260 = client.x;
            *(LONG *)(uintptr_t)0x995264 = client.y;
            /* Genuine physical movement must reactivate mouse navigation. */
            *(BYTE *)(uintptr_t)0x629f79 = 1;
            *(BYTE *)(uintptr_t)0x665118 = 1;
            ++cursor_state.repairs;
        } else *point = cursor_virtual;
    }
    return TRUE;
}
static void cursor_before_input(DWORD caller_rva) {
    cursor_had_virtual = cursor_virtual_active;
    /* Only the field loop owns the dismissible top bar. Camp/modal menus
     * borrow that same parent widget and must control its transitions. */
    cursor_field_poll = caller_rva == 0x15f803 || caller_rva == 0x15f81f;
    if (!cursor_field_poll) cursor_block_click = 0;
}
static uint64_t cursor_finish_input(void *context, uint64_t input) {
    /* DirectInput's click is authoritative even when USER32 reports no button.
     * Re-query the game's context at the physical position before it converts
     * that first click into a command for the old keyboard selection.
     * This poll reads current device state, not buffered/consumed events.
     */
    if (cursor_state.enabled && cursor_had_virtual && (input & UINT64_C(0x28000000)) && focused()) {
        cursor_force_physical = 1;
        input = original_input(context);
        cursor_force_physical = 0;
    }
    if (cursor_state.enabled && cursor_sticky && cursor_field_poll && focused()) {
        int left = !!(input & UINT64_C(0x28000000));
        if (!left) cursor_block_click = 0;
        POINT point;
        if (left && !cursor_block_click && cursor_bar_open() && cursor_get_original(&point)) {
            point = cursor_client(point);
            LONG bottom = (LONG)((int64_t)*(LONG *)(uintptr_t)0x62c474 * 46 / 480);
            LONG width = *(LONG *)(uintptr_t)0x62c468;
            if (point.y > bottom || point.y < 0 || point.x < 0 || point.x >= width) {
                cursor_close_bar();
                cursor_virtual_active = 0;
                cursor_block_click = 1;
            }
        }
        /* Consume this click through release, before a widget can activate the
         * selected Status entry or the field can interpret it as movement. */
        if (cursor_block_click) input &= ~UINT64_C(0x28000020);
    }
    return input;
}
static void __cdecl cursor_render(void) {
    POINT point;
    if (!cursor_get_original(&point)) { cursor_render_original(); return; }
    point = cursor_client(point);
    const DWORD addresses[] = {0x665110,0x665114,0x995260,0x995264};
    LONG saved[4];
    for (unsigned i=0;i<4;++i) {
        saved[i] = *(LONG *)(uintptr_t)addresses[i];
        *(LONG *)(uintptr_t)addresses[i] = (i&1)?point.y:point.x;
    }
    cursor_render_original();
    for (unsigned i=0;i<4;++i) *(LONG *)(uintptr_t)addresses[i] = saved[i];
}
static void cursor_install(IMAGE_NT_HEADERS *nt) {
    int enabled = GetPrivateProfileIntW(L"Mouse",L"DisableCursorSnap",1,
                                      L".\\data\\plugins\\Sky3QoL.ini");
    if (!enabled || profile->timestamp != 0x632ca6fb) return;
    const DWORD iat_va = 0x5e326c, get_va = 0x5e3268;
    BYTE signature[6] = {0xff,0x15,0,0,0,0};
    *(DWORD *)(signature+2) = iat_va;
    unsigned count = 0;
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);
    for (unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
        if (!(sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        BYTE *section = base+sections[i].VirtualAddress;
        DWORD size = sections[i].Misc.VirtualSize;
        for (DWORD j=0;j+sizeof signature<=size;++j)
            if (!memcmp(section+j,signature,sizeof signature)) ++count;
    }
    const BYTE render_signature[] = {0x55,0x8b,0xec,0x83,0xe4,0xf0};
    BYTE *render = (BYTE *)(uintptr_t)0x4b5420;
    const BYTE bounds_signature[] = {0x8b,0x1d,0x1c,0x51,0x66,0,0x85,0xdb};
    const BYTE leave_signature[] = {0x39,0x05,0x64,0x52,0x99,0,0x7f,0x13};
    BYTE *bounds = (BYTE *)(uintptr_t)0x4b519b;
    BYTE *leave = (BYTE *)(uintptr_t)0x55fd7d;
    int sticky = GetPrivateProfileIntW(L"Mouse",L"StickyTopBar",1,
                                      L".\\data\\plugins\\Sky3QoL.ini");
    if (count != 16 || memcmp(render,render_signature,sizeof render_signature) ||
        memcmp(bounds,bounds_signature,sizeof bounds_signature) ||
        (sticky && memcmp(leave,leave_signature,sizeof leave_signature))) return;
    cursor_original = *(void **)(uintptr_t)iat_va;
    cursor_get_original = *(void **)(uintptr_t)get_va;
    HMODULE user = GetModuleHandleW(L"user32.dll");
    if ((void *)cursor_original != (void *)GetProcAddress(user,"SetCursorPos") ||
        (void *)cursor_get_original != (void *)GetProcAddress(user,"GetCursorPos")) return;
    cursor_render_original = (void *)trampoline(render,6);
    if (!cursor_render_original) return;
    void *replacement = cursor_no_snap, *get_replacement = cursor_get;
    if (!detour(render,cursor_render,6)) return;
    if (!write_code((void *)(uintptr_t)get_va,&get_replacement,sizeof get_replacement)) {
        write_code(render,render_signature,sizeof render_signature); return;
    }
    if (!write_code((void *)(uintptr_t)iat_va,&replacement,sizeof replacement)) {
        write_code((void *)(uintptr_t)get_va,&cursor_get_original,sizeof cursor_get_original);
        write_code(render,render_signature,sizeof render_signature); return;
    }
    /* Physical clicks must not be clamped into the currently active panel. */
    if (!detour(bounds,(void *)(uintptr_t)0x4b5405,sizeof bounds_signature) ||
        (sticky && !detour(leave,cursor_bar_leave,sizeof leave_signature))) {
        write_code(bounds,bounds_signature,sizeof bounds_signature);
        write_code((void *)(uintptr_t)iat_va,&cursor_original,sizeof cursor_original);
        write_code((void *)(uintptr_t)get_va,&cursor_get_original,sizeof cursor_get_original);
        write_code(render,render_signature,sizeof render_signature); return;
    }
    cursor_sticky = sticky;
    cursor_clear_bar_selection = GetPrivateProfileIntW(L"Mouse",L"ClearTopBarSelection",1,
                                      L".\\data\\plugins\\Sky3QoL.ini");
    cursor_state.initialized = 1;
    cursor_state.enabled = 1;
}
