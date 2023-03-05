#include "fscord.h"

#if defined(__linux__)
#include <alloca.h>
#elif defined(_WIN64)
#define alloca(size) _alloca(size)
#endif

internal void
playing_sound_start(Playing_Sound *ps);


internal void
add_message(State *state, char *sender, s32 sender_len, char *content, s32 content_len, s64 epoch_time)
{
    s32 index;

    Message_History *hist = &state->message_history;
    if (hist->cnt == 0)
    {
        index = 0;
        hist->cnt += 1;
    }
    else if (hist->cnt < hist->cnt_max)
    {
        index = hist->cnt;
        hist->cnt += 1;
    }
    else
    {
        index = hist->base;
        if (hist->base == hist->cnt_max - 1) hist->base = 0;
        else                                 hist->base += 1;
    }

    Message *message = hist->messages + index;
    message->epoch_time = epoch_time;
    message->username_len = sender_len;
    memcpy(message->username, sender, sender_len);
    message->content_len = content_len;
    memcpy(message->content, content, content_len);
}

internal void
remove_user(State *state, char *name, s32 name_len)
{
    printf("removing user ");
    for (int i = 0; i < name_len; i++) putchar(name[i]);
    putchar('\n');

    Userlist *userlist = &state->userlist;
    for (s32 i = 0; i < userlist->cnt_curr; i++)
    {
        User *user = userlist->users + i;
        if (user->name_len == name_len &&
            memcmp(user->name, name, name_len) == 0)
        {
            s32 cnt_after = userlist->cnt_curr - i - 1;
            memmove(user, user+1, cnt_after * sizeof(User));
            userlist->cnt_curr -= 1;
        }
    }

    playing_sound_start(&state->ps_user_disconnected);
}

internal void
add_user(State *state, char *name, s32 name_len)
{
    printf("adding user ");
    for (int i = 0; i < name_len; i++) putchar(name[i]);
    putchar('\n');

    Userlist *userlist = &state->userlist;
    if (userlist->cnt_curr == userlist->cnt_max)
    {
        printf("\tuserlist maximum reached, server should prevent this\n");
        return;
    }

    User *user = userlist->users + userlist->cnt_curr++;
    user->name_len = name_len;
    memcpy(user->name, name, name_len);

    playing_sound_start(&state->ps_user_connected);
}

internal void
message_history_reset(Message_History *hist)
{
    hist->cnt = 0;
    hist->base = 0;
}

internal void
message_history_init(Message_History *hist, Memory_Arena *arena)
{
    int cnt_max = 512;

    hist->cnt_max = cnt_max;
    hist->cnt = 0;
    hist->base = 0;
    hist->messages = memory_arena_push(arena, sizeof(Message) * cnt_max);
}

internal void
userlist_reset(Userlist *userlist)
{
    userlist->cnt_curr = 0;
}

internal void
userlist_init(Userlist *userlist, Memory_Arena *arena)
{
    int max_users = FSCN_MAX_USERS;

    userlist->cnt_curr = 0;
    userlist->cnt_max = max_users;
    userlist->users = memory_arena_push(arena, sizeof(User) * max_users);
}


#include "generated/asset_font.c"
#include "generated/asset_sound_user_connected.c"
#include "generated/asset_sound_user_disconnected.c"
#include "assets.c"
#include "math.c"
#include "crypt.c"
#include "net.c"
#include "render.c"
#include "sound.c"
#include "ui.c"

internal void
init_state(State *state, void *memory, size_t memory_size)
{
    // locate assets from global memory
    Font *font = locate_font();
    Sound *sound_user_connected = locate_sound_user_connected();
    Sound *sound_user_disconnected = locate_sound_user_disconnected();

    Memory_Arena *arena = &state->arena;
    memory_arena_init(arena,
                      memory + sizeof(State),
                      memory_size - sizeof(State));

    userlist_init(&state->userlist, arena);
    message_history_init(&state->message_history, arena);
    net_init(&state->net, arena);
    ui_init(&state->ui, state, font, arena);
    playing_sound_init(&state->ps_user_connected, sound_user_connected);
    playing_sound_init(&state->ps_user_disconnected, sound_user_disconnected);
}

int
main(void)
{
    TIMER_START(main);
    CYCLER_START(main);
    

    // init state
    size_t memory_size = MEGABYTES(1);
    void *memory = sys_allocate_memory(memory_size);
    if (!memory) return 0;
    State *state = (State*)memory;
    init_state(state, memory, memory_size);


    // open output devices
    Sys_Window *window = sys_create_window("fscord", 1024, 720);
    Sys_Sound_Device *sound_device = sys_open_sound_device();
    if (!window) return 0;


    // main loop
    s32 max_event_count = 32;
    Sys_Event events[max_event_count];
    for (;;)
    {
        net_update(&state->net, state);
        s32 event_count = sys_get_window_events(window, events, max_event_count);
        if (event_count == -1) break;

        state->screen = sys_get_offscreen_buffer(window);
        ui_update_and_render(&state->ui, events, event_count, state);

        Sys_Sound_Buffer *sound_buffer = sys_get_sound_buffer(sound_device);
        playing_sound_update(&state->ps_user_connected, sound_buffer);
        playing_sound_update(&state->ps_user_disconnected, sound_buffer);

        sys_show_offscreen_buffer(window);
        sys_play_sound_buffer(sound_device);
    }


    TIMER_END(main);
    CYCLER_END(main);
}
