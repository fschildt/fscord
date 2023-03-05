#include <common/fscord_defs.h>
#include <common/fscord_net.h>

#include <stdarg.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>


typedef struct
{
    size_t size_used;
    size_t size_total;
    void *memory;
} Memory_Arena;


typedef struct
{
    s32 name_len;
    char name[FSCN_MAX_USERNAME_LEN];
} User;

typedef struct
{
    s32 cnt_curr;
    s32 cnt_max;
    User *users;
} Userlist;

typedef struct
{
    s64 epoch_time;
    s32 username_len;
    char username[FSCN_MAX_USERNAME_LEN];
    s32 content_len;
    char content[FSCN_MAX_MESSAGE_LEN];
} Message;

typedef struct
{
    s32 cnt_max;
    s32 cnt;
    s32 base;
    Message *messages;
} Message_History;


#include "sys/sys.h"
#include "config.h"
#include "assets.h"
#include "sound.h"
#include "math.h"
#include "net.h"
#include "ui.h"

typedef struct
{
    Sys_Time t0;
    Sys_Time t1;
} Timer;


#define TIMER_START(name) \
    static Timer timer_##name; \
    timer_##name.t0 = sys_get_time();

// @Todo: On windows this %ld should be %lld
#define TIMER_END(name) \
    timer_##name.t1 = sys_get_time(); \
    s64 sec_diff = timer_##name.t1.seconds - timer_##name.t0.seconds; \
    s64 nsec0 = timer_##name.t0.nanoseconds; \
    s64 nsec1 = timer_##name.t1.nanoseconds; \
    s64 nsec_total = sec_diff*1000*1000*1000; \
    if (nsec1 >= nsec0) nsec_total += nsec1 - nsec0; \
    else                nsec_total -= nsec1 + nsec0; \
    s64 ms_part = nsec_total / (1000*1000); \
    s64 ns_part = nsec_total % (1000*1000); \
    printf("%-24s: %3ld,%06ld ms\n", #name, ms_part, ns_part);


#define CYCLER_START(name) \
    u64 cycler_##name_t0 = __rdtsc();

// @Todo: On windows this %lu should be %llu
#define CYCLER_END(name) \
    u64 cycler_##name_t1 = __rdtsc(); \
    u64 cycles_counted_##name = cycler_##name_t1 - cycler_##name_t0; \
    printf("cycles elapsed (%s): %lu\n", #name, cycles_counted_##name);


typedef struct
{
    Memory_Arena arena;

    // Todo: does this belong here?
    Sys_Offscreen_Buffer *screen;
    Sys_Sound_Buffer *sound_buffer;

    Userlist userlist;
    Message_History message_history;

    Ui ui;
    Net net;

    // play_cursor = -1 means it is not playing :c
    Playing_Sound ps_user_connected;
    Playing_Sound ps_user_disconnected;
} State;


// @Quality: move the code below to somewhere else.
// Currently the tool/asset_generator uses this as well.

void
memory_arena_pop(Memory_Arena *arena, size_t size)
{
    assert(arena->size_used >= size);
    arena->size_used -= size;
}

void *
memory_arena_push(Memory_Arena *arena, size_t size)
{
    assert(arena->size_used + size <= arena->size_total);

    void *result = arena->memory + arena->size_used;
    arena->size_used += size;

    return result;
}

void
memory_arena_append(Memory_Arena *arena, const void *buff, size_t size)
{
    assert(arena->size_used + size <= arena->size_total);

    void *dest = arena->memory + arena->size_used;
    memcpy(dest, buff, size);
    arena->size_used += size;
}

bool
memory_arena_can_push(Memory_Arena *arena, size_t size)
{
    bool result = arena->size_used + size <= arena->size_total;
    return result;
}

void
memory_arena_make_sub_arena(Memory_Arena *arena, Memory_Arena *subarena, size_t size)
{
    assert(arena->size_used + size <= arena->size_total);

    u8 *sub_mem = memory_arena_push(arena, size);

    subarena->size_used = 0;
    subarena->size_total = size;
    subarena->memory = sub_mem;
}

void
memory_arena_reset(Memory_Arena *arena)
{
    arena->size_used = 0;
}

void
memory_arena_init(Memory_Arena *arena, void *memory, size_t size)
{
    arena->size_used = 0;
    arena->size_total = size;
    arena->memory = memory;
}

