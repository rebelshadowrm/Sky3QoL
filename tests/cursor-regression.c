#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
static BYTE *base=(BYTE *)0x400000;
static struct {DWORD timestamp;} prof={0x632ca6fb}, *profile=&prof;
static int active=1, mouse_down;
static POINT physical={100,200};
static int focused(void){return active;}
static SHORT fake_key(int key){(void)key;return mouse_down?0x8000:0;}
#define GetAsyncKeyState fake_key
static int write_code(void*a,const void*b,SIZE_T n){(void)a;(void)b;(void)n;return 0;}
static void *trampoline(BYTE*a,SIZE_T n){(void)a;(void)n;return NULL;}
static int detour(BYTE*a,const void*b,SIZE_T n){(void)a;(void)b;(void)n;return 0;}
static uint64_t (__attribute__((thiscall)) *original_input)(void *);
#include "cursor-test-header.h"
static BOOL WINAPI get_real(LPPOINT p){*p=physical;return TRUE;}
static void __cdecl draw_real(void){
 assert(*(LONG *)0x20105110==physical.x-10);
 assert(*(LONG *)0x20105114==physical.y-20);
 assert(*(LONG *)0x20205260==physical.x-10);
 assert(*(LONG *)0x20205264==physical.y-20);
}
static uint64_t __attribute__((thiscall)) pick_context(void *context){
 (void)context;POINT p;assert(cursor_get(&p));
 assert(p.x==physical.x && p.y==physical.y);
 return UINT64_C(0x28000000);
}
static int closed_count;
static void __cdecl close_bar(void){++closed_count;*(WORD *)0x20508b1c |= 8;}
static int bar_route(int y,int clicks){
 *(LONG *)0x20205264=y;
 int result;
 __asm__ volatile("call *%2":"=a"(result):"0"(100),"r"(cursor_bar_leave),"D"(clicks):"ecx","edx","memory","cc");
 return result;
}
static LONG WINAPI crash(EXCEPTION_POINTERS *e){fprintf(stderr,"Exception %lx at %p, accessing %lx\n",e->ExceptionRecord->ExceptionCode,e->ExceptionRecord->ExceptionAddress,(unsigned long)e->ExceptionRecord->ExceptionInformation[1]);fflush(stderr);return EXCEPTION_CONTINUE_SEARCH;}
int main(void){
 AddVectoredExceptionHandler(1,crash);
 const DWORD blocks[]={0x20000000,0x20100000,0x20200000,0x20300000,0x21000000,0x20400000,0x20500000};
 for(unsigned i=0;i<7;++i){void *mapped=VirtualAlloc((void *)(uintptr_t)blocks[i],0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!mapped){fprintf(stderr,"map failed: %lx error %lu\n",blocks[i],GetLastError());return 2;}}
 *(LONG *)0x2030d464=10;*(LONG *)0x2030d468=20;
 cursor_get_original=get_real;cursor_render_original=draw_real;
 const BYTE inside[]={0xb8,1,0,0,0,0xc3},outside[]={0xb8,2,0,0,0,0xc3};
 memcpy((void *)0x2100fd85,inside,sizeof inside);memcpy((void *)0x2100fd98,outside,sizeof outside);
 DWORD old;assert(VirtualProtect((void *)0x21000000,0x10000,PAGE_EXECUTE_READWRITE,&old));
 assert(bar_route(300,0)==1);assert(bar_route(300,0x08000000)==2);
 assert(bar_route(50,0x08000000)==1);assert(bar_route(100,0x08000000)==1);
 POINT p;
 assert(cursor_no_snap(500,600));assert(cursor_get(&p));
 assert(p.x==500 && p.y==600);assert(physical.x==100 && physical.y==200);
 *(LONG *)0x20105110=490;*(LONG *)0x20105114=580;
 *(LONG *)0x20205260=480;*(LONG *)0x20205264=570;
 cursor_render();
 assert(*(LONG *)0x20105110==490 && *(LONG *)0x20105114==580);
 assert(*(LONG *)0x20205260==480 && *(LONG *)0x20205264==570);
 physical.x=101;physical.y=201;
 assert(cursor_get(&p));assert(p.x==101 && p.y==201 && !cursor_virtual_active);
 assert(*(BYTE *)0x20009f79==1 && *(BYTE *)0x20105118==1);
 assert(cursor_no_snap(700,800));mouse_down=1;
 assert(cursor_get(&p));assert(p.x==101 && p.y==201 && !cursor_virtual_active);
 mouse_down=0;assert(cursor_no_snap(900,1000));
 *(DWORD *)0x20202f90=1;*(BYTE *)0x20202ffc=0x80;
 assert(cursor_get(&p));assert(p.x==101 && p.y==201 && !cursor_virtual_active);
 *(BYTE *)0x20202ffc=0;assert(cursor_no_snap(900,1000));active=0;
 assert(cursor_get(&p));assert(!cursor_virtual_active);
 active=1;cursor_state.enabled=1;original_input=pick_context;
 assert(cursor_no_snap(500,600));cursor_before_input(0x15f81f);
 assert(cursor_finish_input(NULL,UINT64_C(0x28000020))==UINT64_C(0x28000000));
 assert(!cursor_virtual_active && !cursor_force_physical);
 cursor_sticky=1;cursor_close_bar=close_bar;
 *(LONG *)0x2000c474=1080;*(LONG *)0x2000c468=1920;
 *(DWORD *)0x2040b430=0x20502a60;*(WORD *)0x20508b1c=4;
 cursor_had_virtual=0;
 assert(cursor_finish_input(NULL,UINT64_C(0x28000020))==0);
 assert(closed_count==1 && cursor_block_click);
 assert(cursor_finish_input(NULL,UINT64_C(0x28000020))==0 && closed_count==1);
 assert(cursor_finish_input(NULL,0)==0 && !cursor_block_click);
 *(WORD *)0x20508b1c=4;physical.y=40;
 assert(cursor_finish_input(NULL,UINT64_C(0x28000020))==UINT64_C(0x28000020));
 assert(closed_count==1);
 cursor_sticky=1;cursor_clear_bar_selection=1;
 *(LONG *)0x20508aec=0;
 /* The callback site comparison is covered by original executable signature
  * and runtime caller observation; exercise the matching branch directly. */
 cursor_state.last_caller_rva=0x169e76;
 cursor_virtual_active=1;
 cursor_clear_open_selection();
 assert(!cursor_virtual_active && *(LONG *)0x20508aec==-1);
 physical.y=201;*(WORD *)0x20508b1c=4;
 cursor_before_input(0x16bc84);
 assert(!cursor_field_poll);
 assert(cursor_finish_input(NULL,UINT64_C(0x28000020))==UINT64_C(0x28000020));
 assert(closed_count==1 && *(WORD *)0x20508b1c==4);
 cursor_before_input(0x15f803);assert(cursor_field_poll);
 cursor_had_virtual=0;
 assert(cursor_finish_input(NULL,UINT64_C(0x28000020))==0 && closed_count==2);
 puts("PASS: virtual menu position, physical draw without trails, restored menu state, physical movement/click, DirectInput clicks, loss of focus; top-bar mouse leave stays open, outside click closes, inside/boundary clicks stay open; first stationary click is re-picked physically instead of confirming Status; outside click calls native close and is consumed through release; inside clicks pass; menu-open clears Status focus without a physical warp; Equipment/modal input cannot close the borrowed parent bar, field input still closes it.");
 return 0;
}
