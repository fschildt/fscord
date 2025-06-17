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
internal_var Fscord *s_fscord;

void
login_draw(Login *login)
{
    OSOffscreenBuffer *offscreen = s_fscord->offscreen_buffer;
    MemArena *trans_arena = &s_fscord->trans_arena;


    // draw background color

    RectF32 bg_rect = rectf32(0, 0, offscreen->width, offscreen->height);
    V3F32 bg_col = v3f32(0.4, 0.2, 0.2);
    draw_rectf32(offscreen, bg_rect, bg_col);



    // draw widgets background

    f32 zoom = 1.f;
    f32 font_size = s_fscord->font->y_advance;
    V2F32 widgets_bg_size = v2f32(zoom*font_size*28, zoom*font_size*20);
    V2F32 widgets_bg_pos = v2f32_center(widgets_bg_size, v2f32(offscreen->width, offscreen->height));
    RectF32 widgets_bg_rect = rectf32(widgets_bg_pos.x,
                                      widgets_bg_pos.y,
                                      widgets_bg_pos.x + widgets_bg_size.x,
                                      widgets_bg_pos.y + widgets_bg_size.y);
    draw_rectf32(offscreen, widgets_bg_rect, v3f32(0.8, 0.6, 0.4));



    // draw widgets

    Font *font = s_fscord->font;

    f32 font_height = font_get_height_from_string32(font);
    V2F32 widgets_size = v2f32(font_get_width_from_string32_len(font, 32), font_height*8);
    V2F32 widgets_gap = v2f32((widgets_bg_size.x - widgets_size.x) / 2,
                              (widgets_bg_size.y - widgets_size.y) / 2);
    V2F32 widgets_pos = v2f32(widgets_bg_pos.x + widgets_gap.x,
                              widgets_bg_pos.y + widgets_gap.y);


    String32 *trans_servername = string32_buffer_to_string32(trans_arena, login->servername);
    String32 *trans_username = string32_buffer_to_string32(trans_arena, login->username);


    V2F32 curr_pos = widgets_pos;
    f32 cursor_y; // Todo: draw UiInputText or something or draw_string32_buffer straight

    draw_string32(offscreen, curr_pos, string32_value(login->warning), font);
    curr_pos.y += font_height * 3;

    draw_string32(offscreen, curr_pos, trans_username, font);
    if (login->is_username_active) {
        cursor_y = curr_pos.y;
    }
    curr_pos.y += font_height * 1.4;

    draw_string32(offscreen, curr_pos, string32_value(SH_LOGIN_USERNAME_HINT), font);
    curr_pos.y += font_height * 2.4;

    draw_string32(offscreen, curr_pos, trans_servername, font);
    if (!login->is_username_active) {
        cursor_y = curr_pos.y;
    }
    curr_pos.y += font_height * 1.4;

    draw_string32(offscreen, curr_pos, string32_value(SH_LOGIN_SERVERNAME_HINT), font);


    // draw cursor
    V2F32 cursor_pos = v2f32(curr_pos.x, cursor_y);
    if (login->is_username_active) {
        cursor_pos.x += font_get_width_from_string32_len(font, login->username->cursor);
    }
    else {
        cursor_pos.x += font_get_width_from_string32_len(font, login->servername->cursor);
    }
    RectF32 cursor_rect = rectf32(cursor_pos.x, cursor_pos.y, cursor_pos.x + font->x_advance/4.f, cursor_pos.y + font->y_advance);
    V3F32 cursor_col = v3f32(0, 0, 0);
    draw_rectf32(offscreen, cursor_rect, cursor_col);
}


internal_fn b32
parse_servername(String32Buffer *servername, char *address, size_t address_size, u16 *port)
{
    u32 *l = servername->codepoints;
    u32 *r = servername->codepoints;
    u32 *end = servername->codepoints + servername->len - 1;


    // address

    while (r <= end && *r != ':') {
        r++;
    }

    size_t address_len = r - l;
    if (address_len > 0 && address_len < address_size) {
        for (size_t i = 0; i < address_len; i++) {
            address[i] = l[i];
        }
        address[address_len] = '\0';
    } else {
        return false;
    }


    // port

    l = r + 1;
    r = end;
    if (l > r) {
        return false;
    }

    size_t port_len = r - l + 1;
    char port_cstr[6];
    if (port_len >= 6) {
        return false;
    }
    for (size_t i = 0; i < port_len; i++) {
        port_cstr[i] = l[i];
    }
    port_cstr[port_len] = '\0';

    *port = atoi(port_cstr);
    if (*port == 0) {
        return false;
    }


    return true;
}

internal_fn void
login_process_special_key_press(Login *login, OSEventKeyPress key_press)
{
    String32Buffer *buffer;
    if (login->is_username_active) {
        buffer = login->username;
    } else {
        buffer = login->servername;
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

void
login_process_login_result(Login *login, u32 result)
{
    assert(login->is_trying_to_login);

    if (result == S2C_LOGIN_SUCCESS) {
        s_fscord->is_logged_in = true;
    }
    else {
        login->warning = SH_LOGIN_WARNING_COULD_NOT_CONNECT; // Todo: "could not login"
    }

    login->is_trying_to_login = false;
    login->is_c2s_login_sent = false;
}

void
login_update_login_attempt(Login *login)
{
    ServerConnectionStatus status = server_connection_get_status();
    if (status == SERVER_CONNECTION_NOT_ESTABLISHED) {
        login->is_trying_to_login = false;
        login->is_c2s_login_sent = false;
        login->warning = SH_LOGIN_WARNING_COULD_NOT_CONNECT;
        return;
    }
    else if (status == SERVER_CONNECTION_ESTABLISHING) {
        return;
    }
    else if (status == SERVER_CONNECTION_ESTABLISHED) {
        if (!login->is_c2s_login_sent) {
            String32 *username = string32_buffer_to_string32(&s_fscord->trans_arena, login->username);
            send_c2s_login(username, string32_value(SH_EMPTY));
            login->is_c2s_login_sent = true;
        }
        return;
    }
    else {
        InvalidCodePath;
    }
}

internal_fn void
login_process_unicode_key_press(Login *login, OSEventKeyPress key_press)
{
    switch (key_press.code) {
        case '\t': {
            login->is_username_active = !login->is_username_active;
        } break;

        case '\r': {
            ServerConnectionStatus status = server_connection_get_status();
            if (status == SERVER_CONNECTION_ESTABLISHED) {
                return;
            } else if (status == SERVER_CONNECTION_ESTABLISHING) {
                return;
            }

            assert(status == SERVER_CONNECTION_NOT_ESTABLISHED);

            if (login->username->len == 0) {
                login->warning = SH_LOGIN_WARNING_USERNAME_INVALID;
                break;
            }

            char address[64];
            u16 port;
            if (!parse_servername(login->servername, address, ARRAY_COUNT(address), &port)) {
                login->warning = SH_LOGIN_WARNING_SERVERNAME_INVALID;
                break;
            }

            persist_var EVP_PKEY *server_rsa_pub = 0;
            if (!server_rsa_pub) {
                server_rsa_pub = rsa_create_via_file(&s_fscord->trans_arena, "./server_rsa_pub.pem", true);
                if (!server_rsa_pub) {
                    break;
                }
            }

            server_connection_establish(address, port, server_rsa_pub);

            login->is_trying_to_login = true;
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
login_process_key_press(Login *login, OSEventKeyPress key_press)
{
    login->warning = SH_EMPTY;

    if (key_press.is_unicode) {
        login_process_unicode_key_press(login, key_press);
    } else {
        login_process_special_key_press(login, key_press);
    }
}

void
login_process_event(Login *login, OSEvent *event)
{
    if (login->is_trying_to_login) {
        return;
    }

    if (event->type == OS_EVENT_KEY_PRESS) {
        login_process_key_press(login, event->ev.key_press);
    }
}

Login *
login_create(MemArena *arena, Fscord *fscord)
{
    s_fscord = fscord;

    Login *login = mem_arena_push(arena, sizeof(Login));
    login->is_username_active = false;
    login->is_trying_to_login = false;
    login->is_c2s_login_sent = false;
    login->username = string32_buffer_create(arena, 32);
    login->servername = string32_buffer_create(arena, 32);
    login->warning = SH_EMPTY;

    #if !defined(NDEBUG)
    string32_buffer_append_ascii_cstr(login->username, "user_a");
    login->username->cursor = login->username->len;
    string32_buffer_append_ascii_cstr(login->servername, "127.0.0.1:1905");
    login->servername->cursor = login->servername->len;
    #endif

    return login;
}

