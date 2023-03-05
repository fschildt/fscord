#ifndef _POXIS_C_SOURCE
#define _POSIX_C_SOURCE 200809L // enable POSIX.1-2017
#endif

#include <common/fscord_defs.h>
#include "../sys.h"

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>

#include <alsa/asoundlib.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <alloca.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "lin_window.h"
#include "lin_sound.h"
#include "lin_net.h"

#include "lin_window.c"
#include "lin_sound.c"
#include "lin_net.c"



void *
sys_allocate_memory(size_t size)
{
    // @Performance: Keep the fd of /dev/zero open for the whole runtime?
    int fd = open("/dev/zero", O_RDWR);
    if (fd == -1)
    {
        printf("open() failed: %s\n", strerror(errno));
        return 0;
    }

    void *mem = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);

    if (mem == MAP_FAILED)
    {
        printf("mmap failed\n");
        return 0;
    }

    return mem;
}

void
sys_free_memory(void *address, size_t size)
{
    munmap(address, size);
}



Sys_Time
sys_get_time()
{
    struct timespec ts;
    s32 err = clock_gettime(CLOCK_REALTIME, &ts);
    assert(err == 0);

    Sys_Time time = {};
    time.seconds = ts.tv_sec;
    time.nanoseconds = ts.tv_nsec;
    return time;
}



Sys_Sound_Device *
sys_open_sound_device()
{
    Sys_Sound_Device *device = (Sys_Sound_Device*)lin_open_sound_device();
    return device;
}

void
sys_close_sound_device(Sys_Sound_Device *_device)
{
    Lin_Sound_Device *device = (Lin_Sound_Device*)_device;
    lin_close_sound_device(device);
}

Sys_Sound_Buffer *
sys_get_sound_buffer(Sys_Sound_Device *_device)
{
    Lin_Sound_Device *device = (Lin_Sound_Device*)_device;
    Lin_Sound_Buffer *buffer = lin_get_sound_buffer(device);
    return (Sys_Sound_Buffer*)buffer;
}

void
sys_play_sound_buffer(Sys_Sound_Device *_device)
{
    Lin_Sound_Device *device = (Lin_Sound_Device *)_device;
    lin_play_sound_buffer(device);
}



Sys_Window *
sys_create_window(const char *name, s32 width, s32 height)
{
    Sys_Window *window = (Sys_Window*)lin_create_window(name, width, height);
    return window;
}

void
sys_destroy_window(Sys_Window *sys_window)
{
    lin_destroy_window((Lin_Window*)sys_window);
}

Sys_Offscreen_Buffer *
sys_get_offscreen_buffer(Sys_Window *sys_window)
{
    Lin_Window *lin_window = (Lin_Window*)sys_window;
    Sys_Offscreen_Buffer *sob = (Sys_Offscreen_Buffer*)&lin_window->offscreen_buffer;
    return sob;
}

void
sys_show_offscreen_buffer(Sys_Window *window)
{
    lin_show_offscreen_buffer((Lin_Window*)window);
}

s32
sys_get_window_events(Sys_Window *window, Sys_Event *events, s32 max_event_count)
{
    Lin_Window *_window = (Lin_Window*)window;
    s32 cnt_events = lin_get_window_events(_window, events, max_event_count);
    return cnt_events;
}



Sys_Tcp *
sys_connect(const char *addr, u16 port)
{
    Lin_Tcp *lin_tcp = lin_connect(addr, port);
    Sys_Tcp *sys_tcp = (Sys_Tcp*)lin_tcp;
    return sys_tcp;
}

void
sys_disconnect(Sys_Tcp *sys_tcp)
{
    Lin_Tcp *lin_tcp = (Lin_Tcp*)sys_tcp;
    lin_disconnect(lin_tcp);
}

s64
sys_recv(Sys_Tcp *sys_tcp, void *buff, s64 size)
{
    Lin_Tcp *lin_tcp = (Lin_Tcp*)sys_tcp;
    s64 recvd = lin_recv(lin_tcp, buff, size);
    return recvd;
}

bool
sys_send(Sys_Tcp *sys_tcp, void *buff, s64 size)
{
    Lin_Tcp *lin_tcp = (Lin_Tcp*)sys_tcp;
    bool result = lin_send(lin_tcp, buff, size);
    return result;
}


