typedef struct
{
    s64 seconds;
    s64 nanoseconds;
} Sys_Time;


typedef struct
{
    u8 red_shift;
    u8 green_shift;
    u8 blue_shift;
    u8 alpha_shift;

    s32 width;
    s32 height;
    u32 *pixels;
} Sys_Offscreen_Buffer;

typedef struct
{
    // @Todo: do i even use this?
    s32 width;
    s32 height;
} Sys_Window;


typedef enum : u32
{
    SYS_EVENT_KEY,
} Sys_Event_Type;

enum
{
    SYS_KEY_LEFT = -1,
    SYS_KEY_UP = -2,
    SYS_KEY_RIGHT = -3,
    SYS_KEY_DOWN = -4
};


typedef struct
{
    char c;
} Sys_Event_Key;

typedef struct
{
    Sys_Event_Type type;

    union {
        Sys_Event_Key key;
    };
} Sys_Event;


typedef struct
{
    s32 samples_per_second;
    s32 sample_count;
    s16 *samples;
} Sys_Sound_Buffer;

typedef struct
{
} Sys_Sound_Device;


typedef struct
{
} Sys_Tcp;


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void *
sys_allocate_memory(size_t size);

void
sys_free_memory(void *address, size_t size);


Sys_Time
sys_get_time();


Sys_Window *
sys_create_window(const char *name, s32 width, s32 height);

void
sys_destroy_window(Sys_Window *window);

s32
sys_get_window_events(Sys_Window *window, Sys_Event *events, s32 max_event_count);

Sys_Offscreen_Buffer *
sys_get_offscreen_buffer(Sys_Window *window);

void
sys_show_offscreen_buffer(Sys_Window *window);


Sys_Sound_Device *
sys_open_sound_device();

void
sys_close_sound_device(Sys_Sound_Device *device);

Sys_Sound_Buffer *
sys_get_sound_buffer(Sys_Sound_Device *device);

void
sys_play_sound_buffer(Sys_Sound_Device *device);


Sys_Tcp *
sys_connect(const char *address, u16 port);

void
sys_disconnect(Sys_Tcp *tcp);

bool
sys_send(Sys_Tcp *tcp, void *buffer, s64 size);

s64
sys_recv(Sys_Tcp *tcp, void *buffer, s64 size);


#if defined(__cplusplus) || defined(c_plusplus)
}
#endif



