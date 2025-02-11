#include <client/fscord.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <basic/string32.h>
#include <os/os.h>
#include <messages/messages.h>
#include <client/login.h>
#include <client/session.h>
#include <client/string32_handles.h>
#include <client/server_connection.h>

typedef struct Fscord Fscord;

internal_fn b32
fscord_update(Fscord *fscord)
{
    OSOffscreenBuffer *offscreen_buffer = fscord->offscreen_buffer;
    Login *login = fscord->login;
    Session *session = fscord->session;


    // handle special case for login attempt
    if (login->is_trying_to_login) {
        login_update_login_attempt(login);
    }


    // handle network events
    ServerConnectionStatus status = server_connection_get_status();
    if (status == SERVER_CONNECTION_ESTABLISHED) {
        b32 handled = server_connection_handle_events();
        if (!handled) {
            server_connection_terminate();
            fscord->is_logged_in = false;
        }
    }


    // handle window events
    OSEvent event;
    while (os_window_get_event(fscord->window, &event)) {
        if (event.type == OS_EVENT_WINDOW_RESIZE) {
            os_offscreen_buffer_resize(offscreen_buffer, event.width, event.height);
            continue;
        }
        if (event.type == OS_EVENT_WINDOW_DESTROYED) {
            return false;
        }

        if (fscord->is_logged_in) {
            session_process_event(session, &event);
        }
        else {
            login_process_event(login, &event);
        }
    }

    if (fscord->is_logged_in) {
        session_draw(session);
    }
    else {
        login_draw(login);
    }

#if 0
    OSSoundBuffer *sound_buffer = os_sound_player_get_buffer(fscord->sound_player);
    play_sound_update(&fscord->ps_user_connected, sound_buffer);
    play_sound_update(&fscord->ps_user_disconnected, sound_buffer);
#endif

    mem_arena_reset(&fscord->trans_arena);

    return true;
}


internal_fn void
fscord_main(Fscord *fscord)
{
    b32 running = true;
    while (running) {
        running = fscord_update(fscord);

        os_window_swap_buffers(fscord->window, fscord->offscreen_buffer);
        //os_sound_player_play(fscord->sound_player);
    }
}


internal_fn Fscord *
fscord_create(void *memory, size_t memory_size)
{
    Fscord *fscord = memory;
    memory += sizeof(*fscord);
    memory_size -= sizeof(*fscord);

    size_t arena_size = MEBIBYTES(10) - sizeof(*fscord);

    MemArena *arena = &fscord->arena;
    mem_arena_init(arena, memory, arena_size);
    memory += arena_size;
    memory_size -= arena_size;

    MemArena *trans_arena = &fscord->trans_arena;
    mem_arena_init(&fscord->trans_arena, memory, memory_size); // take the rest of memory


    string32_handles_create(arena);
    string32_handles_load_language();

    fscord->window = os_window_create("fscord", 1024, 720);
    if (!fscord->window) {
        return 0;
    }

    fscord->offscreen_buffer = os_offscreen_buffer_create(1024, 720);
    if (!fscord->offscreen_buffer) {
        return 0;
    }

    fscord->sound_player = os_sound_player_create(arena, 44100);
    if (!fscord->sound_player) {
        return 0;
    }


    fscord->font = asset_manager_load_font();
    fscord->sound_user_connected = asset_manager_load_sound(0);
    fscord->sound_user_disconnected = asset_manager_load_sound(1);

    os_net_secure_streams_init(arena, 1);
    server_connection_create(arena, fscord);

    fscord->is_logged_in = false;
    fscord->login = login_create(arena, fscord);
    fscord->session = session_create(arena, fscord);

    return fscord;
}


int
main(void)
{
    OSMemory memory;
    if (!os_memory_allocate(&memory, MEBIBYTES(20))) {
        return 0;
    }

    Fscord *fscord = fscord_create(memory.p, memory.size);
    if (!fscord) {
        return 0;
    }

    fscord_main(fscord);
    return 0;
}

