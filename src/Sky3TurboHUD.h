/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Alex Basine
 */
/* Native DX9 HUD. Geometric artwork: no fonts, textures, or overlay windows.
 * Save/restore: https://learn.microsoft.com/en-us/windows/win32/direct3d9/state-blocks-save-and-restore-state
 */
#define COBJMACROS
#include <d3d9.h>

__declspec(dllexport) volatile struct {
    DWORD hooks, frames, draws, width, height;
    HRESULT last_result;
} hud_state;
static int hud_bottom, hud_white, hud_enabled = 1;
typedef struct { float x, y, z, rhw; DWORD color; } HudVertex;
static HudVertex hud_vertices[2048];
static unsigned hud_count;

static void hud_triangle(float ax, float ay, float bx, float by,
                         float cx, float cy, DWORD color) {
    if (hud_count + 3 > sizeof hud_vertices / sizeof hud_vertices[0]) return;
    hud_vertices[hud_count++] = (HudVertex){ax-.5f, ay-.5f, 0, 1, color};
    hud_vertices[hud_count++] = (HudVertex){bx-.5f, by-.5f, 0, 1, color};
    hud_vertices[hud_count++] = (HudVertex){cx-.5f, cy-.5f, 0, 1, color};
}
static void hud_rect(float x, float y, float w, float h, DWORD color) {
    hud_triangle(x,y,x+w,y,x+w,y+h,color);
    hud_triangle(x,y,x+w,y+h,x,y+h,color);
}
static void hud_panel(float x, float y, float w, float h, float c, DWORD color) {
    float points[8][2] = {{x+c,y},{x+w-c,y},{x+w,y+c},{x+w,y+h-c},
                         {x+w-c,y+h},{x+c,y+h},{x,y+h-c},{x,y+c}};
    for (int i=0; i<8; ++i)
        hud_triangle(x+w/2,y+h/2,points[i][0],points[i][1],
                     points[(i+1)%8][0],points[(i+1)%8][1],color);
}
static void hud_chevron(float x, float y, float s, DWORD color) {
    hud_triangle(x,y,x+5*s,y,x+12*s,y+7*s,color);
    hud_triangle(x,y,x+12*s,y+7*s,x+7*s,y+7*s,color);
    hud_triangle(x+7*s,y+7*s,x+12*s,y+7*s,x+5*s,y+14*s,color);
    hud_triangle(x+7*s,y+7*s,x+5*s,y+14*s,x,y+14*s,color);
}
static void hud_artwork(UINT width, UINT height) {
    /* Five 5x7 glyphs: TURBO. Integer pixels stay crisp at small sizes. */
    static const BYTE letters[5][7] = {
        {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
        {30,17,17,30,20,18,17}, {30,17,17,30,17,17,30},
        {14,17,17,17,17,17,14}
    };
    float s = height >= 1080 ? 1.5f : 1.0f;
    float w = (hud_white ? 36 : 112)*s, h = 26*s;
    float margin = 14*s, x = width-w-margin;
    float y = hud_bottom ? height-h-margin : margin;
    DWORD gold = hud_white ? 0xfff5f5f5 : 0xffffd981;
    hud_count = 0;
    hud_panel(x,y,w,h,4*s,0xc0c6a665);
    hud_panel(x+s,y+s,w-2*s,h-2*s,3*s,0xe0182338);
    hud_chevron(x+8*s,y+6*s,s,gold);
    hud_chevron(x+18*s,y+6*s,s,gold);
    if (!hud_white)
        for (int glyph=0; glyph<5; ++glyph)
            for (int row=0; row<7; ++row)
                for (int col=0; col<5; ++col)
                    if (letters[glyph][row] & (1 << (4-col)))
                        hud_rect(x+(40+glyph*12+col*2)*s,
                                 y+(6+row*2)*s,2*s,2*s,gold);
}
static void hud_draw(IDirect3DDevice9 *device) {
    IDirect3DSurface9 *target = NULL, *back = NULL;
    IDirect3DStateBlock9 *saved = NULL;
    D3DSURFACE_DESC desc;
    D3DVIEWPORT9 viewport;
    ++hud_state.frames;
    if (!hud_enabled || !qol_state.turbo) return;
    /* Never stamp the badge onto intermediate effects or movie textures. */
    if (FAILED(IDirect3DDevice9_GetRenderTarget(device,0,&target))) return;
    HRESULT got_back = IDirect3DDevice9_GetBackBuffer(device,0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    int visible = SUCCEEDED(got_back) && target == back;
    if (back) IDirect3DSurface9_Release(back);
    if (!visible || FAILED(IDirect3DSurface9_GetDesc(target,&desc))) {
        IDirect3DSurface9_Release(target); return;
    }
    IDirect3DSurface9_Release(target);
    if (FAILED(IDirect3DDevice9_CreateStateBlock(device,D3DSBT_ALL,&saved))) return;
    viewport = (D3DVIEWPORT9){0,0,desc.Width,desc.Height,0,1};
    IDirect3DDevice9_SetViewport(device,&viewport);
    IDirect3DDevice9_SetVertexShader(device,NULL);
    IDirect3DDevice9_SetPixelShader(device,NULL);
    IDirect3DDevice9_SetTexture(device,0,NULL);
    IDirect3DDevice9_SetFVF(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_ZENABLE,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_ZWRITEENABLE,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_STENCILENABLE,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_CULLMODE,D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_LIGHTING,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_FOGENABLE,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_SCISSORTESTENABLE,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHATESTENABLE,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_ALPHABLENDENABLE,TRUE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    IDirect3DDevice9_SetRenderState(device,D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(device,D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(device,D3DRS_BLENDOP,D3DBLENDOP_ADD);
    IDirect3DDevice9_SetRenderState(device,D3DRS_COLORWRITEENABLE,15);
    IDirect3DDevice9_SetRenderState(device,D3DRS_FILLMODE,D3DFILL_SOLID);
    IDirect3DDevice9_SetRenderState(device,D3DRS_CLIPPLANEENABLE,0);
    IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    IDirect3DDevice9_SetTextureStageState(device,0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    IDirect3DDevice9_SetTextureStageState(device,1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    hud_artwork(desc.Width,desc.Height);
    hud_state.last_result = IDirect3DDevice9_DrawPrimitiveUP(device,D3DPT_TRIANGLELIST,
                                 hud_count/3,hud_vertices,sizeof(HudVertex));
    if (SUCCEEDED(hud_state.last_result)) ++hud_state.draws;
    hud_state.width = desc.Width; hud_state.height = desc.Height;
    IDirect3DStateBlock9_Apply(saved);
    IDirect3DStateBlock9_Release(saved);
}
static HRESULT WINAPI hud_end_scene(IDirect3DDevice9 *device) {
    hud_draw(device);
    return IDirect3DDevice9_EndScene(device);
}
static void hud_install(IMAGE_NT_HEADERS *nt) {
    WCHAR setting[32];
    hud_enabled = GetPrivateProfileIntW(L"HUD",L"Enabled",1,L".\\data\\plugins\\Sky3QoL.ini");
    GetPrivateProfileStringW(L"HUD",L"Position",L"top-right",setting,32,L".\\data\\plugins\\Sky3QoL.ini");
    hud_bottom = !lstrcmpiW(setting,L"bottom-right");
    GetPrivateProfileStringW(L"HUD",L"Style",L"gold",setting,32,L".\\data\\plugins\\Sky3QoL.ini");
    hud_white = !lstrcmpiW(setting,L"white");
    if (!hud_enabled || profile->timestamp != 0x632ca6fb) return;
    static const BYTE call[] = {0xa1,0xfc,0x19,0x99,0,0x50,0x8b,0x08,0xff,0x91,0xa8,0,0,0};
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(nt);
    BYTE *sites[17]; unsigned found = 0;
    for (unsigned i=0; i<nt->FileHeader.NumberOfSections; ++i) {
        if (!(sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        BYTE *section = base + sections[i].VirtualAddress;
        DWORD size = sections[i].Misc.VirtualSize;
        for (DWORD j=0; j+sizeof call<=size; ++j)
            if (!memcmp(section+j,call,sizeof call)) {
                if (found == 17) return;
                sites[found++] = section+j+8;
            }
    }
    if (found != 17) return;
    for (unsigned i=0; i<found; ++i) {
        BYTE patch[6] = {0xe8,0,0,0,0,0x90};
        *(int32_t *)(patch+1) = (BYTE *)hud_end_scene-sites[i]-5;
        if (write_code(sites[i],patch,sizeof patch)) ++hud_state.hooks;
    }
}
