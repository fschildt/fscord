#ifndef OS_H
#define OS_H

#include <basic/mem_arena.h>
#include <basic/basic.h>
#include <openssl/rsa.h>

typedef struct {
    size_t size;
    void *p;
} OSMemory;

b32  os_memory_allocate(OSMemory *memory, size_t size);
void os_memory_free(OSMemory *memory);


typedef struct {
    i64 seconds;
    i64 nanoseconds;
} OSTime;

OSTime os_time_get_now();


typedef void OSLibrary;
OSLibrary* os_library_open(const char *path);
void       os_library_close(OSLibrary *lib);
void*      os_library_get_proc(OSLibrary *lib, const char *name);


char *os_read_file_as_string(MemArena *arena, char *filepath, size_t *len);


typedef enum {
    OS_EVENT_KEY_PRESS,
    OS_EVENT_KEY_RELEASE,
    OS_EVENT_WINDOW_RESIZE,
    OS_EVENT_WINDOW_DESTROYED,
} OSEventType;

typedef enum {
    OS_KEYCODE_LEFT,
    OS_KEYCODE_RIGHT,
    OS_KEYCODE_UP,
    OS_KEYCODE_DOWN,
} OSKeySpecial;

typedef struct OSKeyPress {
    b32 is_unicode; // else it's "special", see OSKeySpecial
    u32 code;   // unicode character or some other keyboard keys
} OSKeyPress;

typedef struct OSKeyRelease {
    b32 is_special;
    u32 code;   // unicode character or some other keyboard keys
} OSKeyRelease;

typedef struct OSEvent {
    OSEventType type;
    union {
        OSKeyPress key_press;
        OSKeyRelease key_release;

        // resize
        struct {
            i32 width;
            i32 height;
        };
    };
} OSEvent;



typedef struct {
    u8 red_shift;
    u8 green_shift;
    u8 blue_shift;
    u8 alpha_shift;
    i32 width;
    i32 height;
    u32 *pixels;
} OSOffscreenBuffer;

OSOffscreenBuffer *os_offscreen_buffer_create(i32 width, i32 height);
void               os_offscreen_buffer_destroy(OSOffscreenBuffer *offscreen_buffer);
void               os_offscreen_buffer_resize(OSOffscreenBuffer *offscreen_buffer, i32 width, i32 height);



typedef struct OSWindow OSWindow;
OSWindow*          os_window_create(const char *name, i32 width, i32 height);
void               os_window_destroy(OSWindow *window);
b32                os_window_get_event(OSWindow *window, OSEvent *event);
OSOffscreenBuffer* os_window_get_offscreen_buffer(OSWindow *window);
void               os_window_swap_buffers(OSWindow *window, OSOffscreenBuffer *offscreen_buffer);



#define OS_NET_STREAM_ID_INVALID U32_MAX

void  os_net_streams_init(MemArena *arena, size_t max_count);

u32  os_net_stream_listen(u16 port);
u32  os_net_stream_accept(u32 listener_id);
u32  os_net_stream_connect(char *address, u16 port);
void os_net_stream_close(u32 id);

b32  os_net_stream_send(u32 id, void *buffer, size_t size);
b32  os_net_stream_recv(u32 id, void *buffer, size_t size);
int  os_net_stream_get_fd(u32 id);



typedef enum {
    OS_NET_SECURE_STREAM_CONNECTED,
    OS_NET_SECURE_STREAM_DISCONNECTED,
    OS_NET_SECURE_STREAM_HANDSHAKING,
} OSNetSecureStreamStatus;

#define OS_NET_SECURE_STREAM_ID_INVALID U32_MAX

void os_net_secure_streams_init(MemArena *arena, size_t max_count);

u32  os_net_secure_stream_listen(u16 port, EVP_PKEY *server_rsa_pri);
u32  os_net_secure_stream_accept(u32 listener_id);
u32  os_net_secure_stream_connect(char *address, u16 port, EVP_PKEY *server_rsa_pub);
void os_net_secure_stream_close(u32 id);

b32  os_net_secure_stream_send(u32 id, void *buffer, size_t size);
i64  os_net_secure_stream_recv(u32 id, void *buffer, size_t size);
int  os_net_secure_stream_get_fd(u32 id);
OSNetSecureStreamStatus os_net_secure_stream_get_status(u32 id);



typedef void (fn_os_work_proc)(void *data);
typedef struct {
    fn_os_work_proc *proc;
    void *data;
} OSWork;

typedef struct OSWorkQueue OSWorkQueue;

OSWorkQueue *os_work_queue_create(i32 thread_count);
void         os_work_queue_push(OSWorkQueue *queue, OSWork work);
void         os_work_queue_wait_all(OSWorkQueue *queue); // Todo: os_work_queue_wait_one(...)
void         os_work_queue_reset(OSWorkQueue *queue);



typedef struct OSSoundPlayer OSSoundPlayer;

typedef struct {
    i32 play_cursor;
    i32 max_sample_count;
    i32 sample_count;
    i32 samples_per_second;
    i16 *samples;
} OSSoundBuffer;

// Todo: maybe change api by replacing get_buffer with play_buffer
OSSoundPlayer* os_sound_player_create(MemArena *arena, i32 samples_per_second);
OSSoundBuffer* os_sound_player_get_buffer(OSSoundPlayer *player);
void           os_sound_player_close(OSSoundPlayer *player);


#endif // OS_H
