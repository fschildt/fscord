#define WIN32_LEAN_AND_MEAN

// disable annoying warnings
#pragma warning(disable: 4820) // padding in structs
#pragma warning(disable: 5045) // ???
#define _WINSOCK_DEPRECATED_NO_WARNINGS // inet_addr

#include <common/fscord_defs.h>
#include <sys/sys.h>

#include <windows.h>
#include <winsock2.h>
#include <malloc.h>

#include "Win32Window.h"
#include "Win32WindowService.h"
#include "Win32Tcp.h"

typedef struct
{
    Sys_Offscreen_Buffer sys_ob;
} Win32_Offscreen_Buffer;

typedef struct
{
    HWND handle;
    HDC dc;
    Win32_Offscreen_Buffer offscreen_buffer;
} Win32_Window;

typedef struct
{
    HWND service_window;
    PIXELFORMATDESCRIPTOR pfd;
} Win32Data;

static Win32Data g_win32_data;
static Win32WindowService g_win32_window_service;

#include "Win32Window.cpp"
#include "Win32WindowService.cpp"
#include "Win32Tcp.cpp"

void *
sys_allocate_memory(size_t size)
{
    LPVOID addr = VirtualAlloc(0, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!addr)
    {
        // GetLastError...
        printf("VirtualAlloc failed\n");
    }
    return addr;
}

void
sys_free_memory(void *address, size_t size)
{
    BOOL freed = VirtualFree(address, 0, MEM_RELEASE);
    if (!freed)
    {
        printf("VirtualFree failed\n");
    }
}

Sys_Time
sys_get_time()
{
    // @Incomplete
    Sys_Time time = {};
    return time;
}

Sys_Window *
sys_create_window(const char *name, i32 width, i32 height)
{
    static int counter = 0;
    assert(counter++ == 0);
    if (!g_win32_window_service.Init())
    {
        return 0;
    }

    Win32Window *win32_window = g_win32_window_service.CreateWin32Window(name, width, height);

    Sys_Window *sys_window = (Sys_Window*)win32_window;
    return sys_window;
}

void
sys_destroy_window(Sys_Window *window)
{
    Win32Window *win32_window = (Win32Window*)window;
    g_win32_window_service.DestroyWin32Window(win32_window);
}

i32
sys_get_window_events(Sys_Window *sys_window, Sys_Event *events, i32 max_event_count)
{
    Win32Window *win32_window = (Win32Window*)sys_window;
    i32 event_count = g_win32_window_service.GetWindowEvents(win32_window, events, max_event_count);
    return event_count;
}

Sys_Offscreen_Buffer *
sys_get_offscreen_buffer(Sys_Window *sys_window)
{
    Win32Window *window = (Win32Window*)sys_window;
    Sys_Offscreen_Buffer *screen = window->GetOffscreenBuffer();
    return screen;
}

void
sys_show_offscreen_buffer(Sys_Window *sys_window)
{
    Win32Window *window = (Win32Window*)sys_window;
    window->ShowOffscreenBuffer();
}

Sys_Sound_Device *
sys_open_sound_device()
{
    return 0;
}

void
sys_close_sound_device(Sys_Sound_Device *device)
{
}

Sys_Sound_Buffer *
sys_get_sound_buffer(Sys_Sound_Device *device)
{
    static Sys_Sound_Buffer buff = {};
    return &buff;
}

void
sys_play_sound_buffer(Sys_Sound_Device *device)
{
}

Sys_Tcp *
sys_connect(const char *address, u16 port)
{
    Win32Tcp *tcp = new Win32Tcp();
    if (tcp->Connect(address, port)) {
        return (Sys_Tcp*)tcp;
    } else {
        return 0;
    }
}

void
sys_disconnect(Sys_Tcp *sys_tcp)
{
    Win32Tcp *win32_tcp = (Win32Tcp*)sys_tcp;
    delete win32_tcp;
}

bool
sys_send(Sys_Tcp *sys_tcp, void *buffer, i64 size)
{
    assert(size <= S32_MAX);

    Win32Tcp *win32_tcp = (Win32Tcp*)sys_tcp;
    bool sent = win32_tcp->Send(buffer, (i32)size);
    return sent;
}

i64
sys_recv(Sys_Tcp *sys_tcp, void *buffer, i64 size)
{
    assert(size <= S32_MAX);

    Win32Tcp *win32_tcp = (Win32Tcp*)sys_tcp;
    int recvd = win32_tcp->Recv(buffer, (i32)size);
    return recvd;
}


