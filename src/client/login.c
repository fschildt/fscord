#include "basic/string.h"
#include <client/login.h>
#include <client/fscord.h>
#include <client/string_handles.h>
#include <client/draw.h>
#include <client/font.h>

#include <basic/basic.h>
#include <basic/math.h>
#include <os/os.h>

#include <stdlib.h>

typedef struct Fscord Fscord;

void
login_draw(Fscord *fscord)
{
    Login *login = fscord->login;
    OSOffscreenBuffer *offscreen = fscord->offscreen_buffer;

    // draw background color
    RectF32 bg_rect = rectf32(0, 0, offscreen->width, offscreen->height);
    V3F32 bg_col = v3f32(0.4, 0.2, 0.2);
    draw_rectf32(offscreen, bg_rect, bg_col);

    // draw widgets background
    f32 zoom = 1.f;
    f32 font_size = fscord->font->y_advance;
    V2F32 widgets_bg_size = v2f32(zoom*font_size*28, zoom*font_size*20);
    V2F32 widgets_bg_pos = v2f32_center(widgets_bg_size, v2f32(offscreen->width, offscreen->height));
    RectF32 widgets_bg_rect = rectf32(widgets_bg_pos.x,
                                      widgets_bg_pos.y,
                                      widgets_bg_pos.x + widgets_bg_size.x,
                                      widgets_bg_pos.y + widgets_bg_size.y);
    draw_rectf32(offscreen, widgets_bg_rect, v3f32(0.8, 0.6, 0.4));

    // draw textboxes
}


internal_fn b32
parse_servername(String32 *servername, char *address, size_t address_size, u16 *port)
{
    u32 *p0 = servername->p;
    u32 *p1 = servername->p;
    u32 *pmax = servername->p + servername->len - 1;


    // address

    while (p1 <= pmax && *p1 != ':') {
        p1++;
    }

    size_t address_len = p1 - p0;
    if (address_len > 0 && address_len < address_size) {
        for (size_t i = 0; i < address_len; i++) {
            address[i] = p0[i];
        }
        address[address_len] = '\0';
    } else {
        return false;
    }


    // port

    p0 = p1 + 1;
    p1 = pmax;
    if (p0 > pmax) {
        return false;
    }

    size_t port_len = p1 - p0 + 1;
    char port_cstr[6];
    if (port_len >= 6) {
        return false;
    }
    for (size_t i = 0; i < port_len; i++) {
        port_cstr[i] = p0[i];
    }
    port_cstr[port_len] = '\0';

    *port = atoi(port_cstr);
    if (*port == 0) {
        return false;
    }


    return true;
}

internal_fn void
login_process_special_key_press(Fscord *state, OSKeyPress key_press)
{
    Login *view = state->login;

    size_t *cursor;
    String32 *str;
    if (view->is_username_active) {
        cursor = &view->username_cursor;
        str = view->username;
    } else {
        cursor = &view->servername_cursor;
        str = view->servername;
    }

    switch (key_press.code) {
        case OS_KEYCODE_LEFT: {
            string32_move_cursor_left(str, cursor);
            if (*cursor > 0) (*cursor)--;
        } break;

        case OS_KEYCODE_RIGHT: {
        } break;

        default:;
    }
}

internal_fn void
login_process_unicode_key_press(Fscord *fscord, OSKeyPress key_press)
{
    Login *login = fscord->login;
    switch (key_press.code) {
        case '\t': {
            login->is_username_active = !login->is_username_active;
        } break;

        case '\r': {
            OSNetSecureStreamStatus status = os_net_secure_stream_get_status(fscord->secure_stream);
            if (status == OS_NET_SECURE_STREAM_HANDSHAKING) {
                break;
            }

            if (login->username->len <= 0) {
                login->warning = g_login_warning_username_invalid;
                break;
            }

            size_t max_address_size = 64;
            char address[max_address_size];
            u16 port;
            if (!parse_servername(login->servername, address, max_address_size, &port)) {
                break;
            }
            printf("address = %s, port = %d\n", address, port);

            // Todo: call this from another thread to avoid stalling the program
            fscord->secure_stream = os_net_secure_stream_connect(address, port, fscord->server_pub_rsa);

            login->warning = g_login_warning_connecting;
        } break;

        default: {
            String32 *str;
            size_t *cursor;
            if (login->is_username_active) {
                str = login->username;
                cursor = &login->username_cursor;
            } else {
                str = login->servername;
                cursor = &login->servername_cursor;
            }
            string32_edit(str, key_press, cursor);
        }
    }
}

internal_fn void
login_process_key_press(Fscord *fscord, OSKeyPress key_press)
{
    Login *login = fscord->login;
    login->warning = g_empty_string32;

    if (key_press.is_unicode) {
        login_process_unicode_key_press(fscord, key_press);
    } else {
        login_process_special_key_press(fscord, key_press);
    }
}
 
void
login_process_event(Fscord *fscord, OSEvent *event)
{
    if (event->type == OS_EVENT_KEY_PRESS) {
        login_process_key_press(fscord, event->key_press);
    }
}

Login *
login_create(MemArena *arena)
{
    Login *login = mem_arena_push(arena, sizeof(Login));
    login->username = string32_create(arena, 32);
    login->username_cursor = 0;
    login->servername = string32_create(arena, 32);
    login->servername_cursor = 0;
    login->warning = g_empty_string32;
    return login;
}

