#include "client/server_connection.h"
#include <basic/string32.h>
#include <client/login.h>
#include <client/fscord.h>
#include <client/string32_handles.h>
#include <client/draw.h>
#include <client/font.h>
#include <crypto/rsa.h>

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
    MemArena *trans_arena = &fscord->trans_arena;



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



    // draw widgets

    Font *font = fscord->font;

    f32 font_height = font_get_height_from_string32(font);
    V2F32 widgets_size = v2f32(font_get_width_from_string32_len(font, 32), font_height*8);
    V2F32 widgets_gap = v2f32((widgets_bg_size.w - widgets_size.w) / 2,
                              (widgets_bg_size.h - widgets_size.h) / 2);
    V2F32 widgets_pos = v2f32(widgets_bg_pos.x + widgets_gap.w,
                              widgets_bg_pos.y + widgets_gap.h);


    String32 *trans_servername = string32_buffer_to_string32(trans_arena, login->servername);
    String32 *trans_username = string32_buffer_to_string32(trans_arena, login->username);


    V2F32 curr_pos = widgets_pos;

    draw_string32(offscreen, curr_pos, trans_username, font);
    curr_pos.y += font_height * 1.4;

    draw_string32(offscreen, curr_pos, string32_value(SH_LOGIN_USERNAME_HINT), font);
    curr_pos.y += font_height * 2.4;

    draw_string32(offscreen, curr_pos, trans_servername, font);
    curr_pos.y += font_height * 1.4;

    draw_string32(offscreen, curr_pos, string32_value(SH_LOGIN_SERVERNAME_HINT), font);
}


internal_fn b32
parse_servername(String32Buffer *servername, char *address, size_t address_size, u16 *port)
{
    u32 *p0 = servername->codepoints;
    u32 *p1 = servername->codepoints;
    u32 *pmax = servername->codepoints + servername->len - 1;


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

    String32Buffer *buffer;
    if (view->is_username_active) {
        buffer = view->username;
    } else {
        buffer = view->servername;
    }

    switch (key_press.code) {
        case OS_KEYCODE_LEFT: {
            string32_buffer_move_cursor_left(buffer);
        } break;

        case OS_KEYCODE_RIGHT: {
            string32_buffer_move_cursor_right(buffer);
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
            ServerConnectionStatus status = server_connection_get_status();
            if (status == SERVER_CONNECTION_ESTABLISHED) {
                InvalidCodePath;
                return;
            }
            assert(status == SERVER_CONNECTION_NOT_ESTABLISHED);


            if (login->username->len <= 0) {
                login->warning = SH_LOGIN_WARNING_USERNAME_INVALID;
                break;
            }


            char address[64];
            u16 port;
            if (!parse_servername(login->servername, address, ARRAY_COUNT(address), &port)) {
                break;
            }
            printf("address = %s, port = %d\n", address, port);

            // Todo: Does this belong here? I simply cut-pasted it from main.
            // Todo: When disconnecting/reconnecting gets implemented this will LEAK!
            EVP_PKEY *server_rsa_pub = rsa_create_via_file(&fscord->trans_arena, "./server_pubkey.pem", true);

            // Todo: call this from another thread to avoid stalling the program
            if (!server_connection_establish(address, port, server_rsa_pub)) {
                rsa_destroy(server_rsa_pub);
                login->warning = SH_EMPTY; // Todo: display proper error
                return;
            }

            login->warning = SH_LOGIN_WARNING_CONNECTING;
        } break;

        default: {
            String32Buffer *buffer;
            if (login->is_username_active) {
                buffer = login->username;
            } else {
                buffer = login->servername;
            }
            string32_buffer_edit(buffer, key_press);
        }
    }
}

internal_fn void
login_process_key_press(Fscord *fscord, OSKeyPress key_press)
{
    Login *login = fscord->login;
    login->warning = SH_EMPTY;

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
    login->username = string32_buffer_create(arena, 32);
    login->servername = string32_buffer_create(arena, 32);
    login->warning = SH_EMPTY;
    return login;
}

