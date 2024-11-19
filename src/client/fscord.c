#include <client/fscord.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <basic/string.h>
#include <os/os.h>
#include <messages/messages.h>
#include <client/login.h>
#include <client/session.h>
#include <client/string_handles.h>

typedef struct Fscord Fscord;

internal_fn b32
fscord_update(Fscord *fscord)
{
    OSOffscreenBuffer *offscreen_buffer = fscord->offscreen_buffer;
    OSEvent event;
    while (os_window_get_event(fscord->window, &event)) {
        if (event.type == OS_EVENT_WINDOW_RESIZE) {
            os_offscreen_buffer_resize(offscreen_buffer, event.width, event.height);
            continue;
        }
        if (event.type == OS_EVENT_WINDOW_DESTROYED) {
            return false;
        }
        if (fscord->is_login) {
            login_process_event(fscord, &event);
        }
        else {
            session_process_event(fscord->session, &event);
        }
    }

    if (fscord->is_login) {
        login_draw(fscord);
    }
    else {
        session_draw(fscord->session);
    }

#if 0
    OSSoundBuffer *sound_buffer = os_sound_player_get_buffer(fscord->sound_player);
    play_sound_update(&fscord->ps_user_connected, sound_buffer);
    play_sound_update(&fscord->ps_user_disconnected, sound_buffer);
#endif

    mem_arena_reset(&fscord->trans_arena);

    return true;
}

internal_fn b32
fscord_init(Fscord *fscord, void *memory, size_t memory_size)
{
    MemArena *arena = &fscord->arena;
    mem_arena_init(arena, memory, memory_size);

    string_handles_create(arena);
    string_handles_load_language();

    fscord->window = os_window_create("fscord", 1024, 720);
    if (!fscord->window) {
        return false;
    }

    fscord->offscreen_buffer = os_offscreen_buffer_create(1024, 720);
    if (!fscord->offscreen_buffer) {
        return false;
    }

    fscord->sound_player = os_sound_player_create(arena, 44100);
    if (!fscord->sound_player) {
        return false;
    }

    fscord->font = asset_manager_load_font();
    fscord->sound_user_connected = asset_manager_load_sound(0);
    fscord->sound_user_disconnected = asset_manager_load_sound(1);

    os_net_secure_streams_init(arena, 1);
    fscord->server_pub_rsa = rsa_create_via_file(arena, "./server_pubkey.pem", true);

    fscord->login = login_create(arena);
    fscord->is_login = true;

    return true;
}

internal_fn void
fscord_main()
{
    OSMemory memory;
    if (!os_memory_allocate(&memory, MEBIBYTES(20))) {
        return;
    }

    Fscord *fscord = (Fscord*)memory.p;
    if (!fscord_init(fscord, memory.p+sizeof(*fscord), memory.size-sizeof(*fscord))) {
        return;
    }

    b32 running = true;
    while (running) {
        running = fscord_update(fscord);

        os_window_swap_buffers(fscord->window, fscord->offscreen_buffer);
        //os_sound_player_play(fscord->sound_player);
    }
}

int main(void)
{
    fscord_main();
    return 0;
}

