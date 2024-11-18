#include <basic/string.h>
#include <os/os.h>
#include <client/fscord.h>
#include <client/session.h>
#include <client/draw.h>
#include <client/font.h>
#include <messages/messages.h>

typedef struct Fscord Fscord;
internal_var Fscord *s_fscord;

internal_fn void
session_draw_chat_message(ChatMessage *message, V2F32 pos)
{
    OSOffscreenBuffer *screen = s_fscord->offscreen_buffer;
    Font *font = s_fscord->font;
    MemArena *trans_arena = &s_fscord->trans_arena;

    String32 *str;
    f32 str_width;
    f32 dx = font->y_advance / 2.f;


    pos.x += font->y_advance;


    // draw local time
    time_t local_time_src = message->creation_time.seconds;
	struct tm *local_time = localtime(&local_time_src);
    char tmp_time_len = 5;
    char tmp_time[tmp_time_len+1];
    sprintf(tmp_time, "%.2d:%.2d", local_time->tm_hour, local_time->tm_min);

    str = string32_create(trans_arena, 5);
    string32_copy_ascii_cstr(str, tmp_time);

    draw_string32(screen, pos, str, font);
    str_width = font_string32_width(font, str);
    pos.x += str_width + dx;


    // draw sender_name
    str = string32_create(trans_arena, message->sender_name->len + 2);
    string32_append_ascii_cstr(str, "<");
    string32_append_string32(str, message->sender_name);
    string32_append_ascii_cstr(str, ">");

    draw_string32(screen, pos, str, font);
    str_width = font_string32_width(font, str);
    pos.x += str_width + dx;


    // draw content
    draw_string32(screen, pos, message->content, font);
}

internal_fn void
session_draw_chat(Session *session, RectF32 rect)
{
    Font *font = s_fscord->font;
    OSOffscreenBuffer *screen = s_fscord->offscreen_buffer;


    // draw border
    RectF32 border_rect = rect;
    f32 border_size = font->y_advance / 8.f;
    V3F32 border_color = v3f32(0, 0, 0);
    draw_border(screen, border_rect, border_size, border_color);


    // draw messages
    V2F32 pos = v2f32(rect.x0 + border_size*4, rect.y0 + border_size*4);
    f32 dy = font->y_advance + border_size * 4;

    size_t cnt1 = session->cur_message_count - session->message0;
    size_t cnt2 = session->cur_message_count - cnt1;
    for (size_t i = session->message0; i < cnt1; i++) {
        if (pos.y >= pos.y + dy) {
            break;
        }
        ChatMessage *message = &session->messages[i];
        session_draw_chat_message(message, pos);
        pos.y += dy;
    }
    for (size_t i = 0; i < session->message0; i++) {
        if (pos.y >= pos.y + dy) {
            break;
        }
        ChatMessage *message = &session->messages[i];
        session_draw_chat_message(message, pos);
        pos.y += dy;
    }
}

internal_fn void
session_draw_prompt(Session *session, RectF32 rect)
{
    OSOffscreenBuffer *screen = s_fscord->offscreen_buffer;
    Font *font = s_fscord->font;


    // draw border
    RectF32 border_rect = rect;
    f32 border_size = font->y_advance / 8.f;
    V3F32 border_color = v3f32(0.16, 0.32, 0.08);
    draw_border(screen, border_rect, border_size, border_color);


    // draw text
    f32 xmargin = border_size * 4;
    f32 ymargin = border_size * 4;
    V2F32 pos = v2f32(rect.x0 + xmargin, rect.y0 + ymargin);
    draw_string32(screen, pos, session->prompt, font);


    // draw cursor
    pos.x += font_string32_width_via_len(font, session->prompt_cursor);
    RectF32 cursor_rect = rectf32(pos.x, pos.y, pos.x + font->x_advance/4.f, pos.y + font->y_advance);
    V3F32 cursor_col = v3f32(0, 0, 0);
    draw_rectf32(screen, cursor_rect, cursor_col);
}

internal_fn void
session_draw_users(Session *session, RectF32 rect)
{
    OSOffscreenBuffer *screen = s_fscord->offscreen_buffer;
    Font *font = s_fscord->font;
    MemArena *trans_arena = &s_fscord->trans_arena;


    // draw border
    f32 border_size = font->y_advance / 8.f;
    V3F32 border_color = v3f32(0, 0, 0);
    draw_border(screen, rect, border_size, border_color);


    // draw users
    f32 xmargin = border_size * 2;
    f32 ymargin = border_size * 2;
    f32 dy = font->y_advance;
    V2F32 pos = v2f32(rect.x0 + xmargin, rect.y1 - ymargin - dy);

    for (size_t i = 0; i < session->cur_user_count; i++) {
        if (pos.y < ymargin - font->y_advance) {
            break;
        }

        f32 width_remain = rect.x1 - pos.x;
        User *user = &session->users[i];

        // Todo: the render should draw partial characters based on a rect
        size_t name_len_desired = user->name->len;
        size_t name_len_avail = font_string32_len_via_width(font, width_remain);
        size_t name_len = name_len_desired <= name_len_avail ? name_len_desired : name_len_avail;
        String32 *str = string32_create(trans_arena, name_len);
        string32_copy_string32_with_count(str, user->name, name_len);
        draw_string32(screen, pos, str, font);
        pos.y -= dy;
    }
}

#if 0
internal void
ui_draw_session(Ui *ui, PlatformOffscreenBuffer *screen, State *state)
{
    V2F32 pos = v2(0, 0);
    V2F32 dim = v2(screen->width, screen->height);
    V3 col = V3F32(0.8, 0.6, 0.4);
    draw_rectf32(screen, pos, dim, col);


    f32 scale = ui->font->y_advance;


    V2F32 userlist_pos = v2(0, 0);
    V2F32 userlist_dim = v2(scale * 8, screen->height);

    V2F32 prompt_pos = v2(userlist_dim.x, 0);
    V2F32 prompt_dim = v2(screen->width - userlist_dim.x, scale*2);

    V2F32 history_pos = v2(prompt_pos.x, prompt_pos.y + prompt_dim.y);
    V2F32 history_dim = v2(prompt_dim.x, screen->height - prompt_dim.y);

    ui_draw_userlist(ui, screen, userlist_pos, userlist_dim);
    ui_draw_prompt(ui, screen, prompt_pos, prompt_dim);
    ui_draw_history(ui, screen, history_pos, history_dim);
}
#endif
void
session_draw(Session *session)
{
    OSOffscreenBuffer *screen = s_fscord->offscreen_buffer;

    f32 left_width = screen->width * 0.7;
    f32 right_width = screen->width - left_width;
    f32 prompt_height = screen->height * 0.1; // Todo: ui_textbox_get_height(...)

    RectF32 users_rect = rectf32(0, 0, left_width, screen->height);
    RectF32 prompt_rect = rectf32(left_width, 0, screen->width, prompt_height);
    RectF32 chat_rect = rectf32(left_width, prompt_height, screen->width, screen->height);

    session_draw_users(session, users_rect);
    session_draw_prompt(session, prompt_rect);
    session_draw_chat(session, chat_rect);
}

internal_fn void
session_add_chat_message(Session *session, Time creation_time, String32 *sender_name, String32 *content)
{
    size_t i = (session->message0 + session->cur_message_count) % session->max_message_count;

    ChatMessage *message = &session->messages[i];
    message->creation_time = creation_time;
    string32_copy_string32(message->sender_name, sender_name);
    string32_copy_string32(message->content, content);

    if (session->cur_message_count < session->max_message_count) {
        session->cur_message_count++;
    }
}

internal_fn void
session_rm_user(Session *session, String32 *username)
{
    // Todo: use a hashmap
    size_t rm_idx = SIZE_MAX;
    for (size_t i = 0; i < session->cur_user_count; i++) {
        if (string32_equal(session->users[i].name, username)) {
            rm_idx = i;
            break;
        }
    }
    assert(rm_idx != SIZE_MAX);

    size_t last_idx = session->cur_user_count - 1;
    for (size_t i = rm_idx + 1; i <= last_idx; i++) {
        string32_copy_string32(session->users[i-1].name, session->users[i].name);
    }

    session->cur_user_count -= 1;
}

internal_fn void
session_add_user(Session *session, String32 *username)
{
    assert(session->cur_user_count < session->max_user_count);

    User *user = &session->users[session->cur_user_count++];
    string32_copy_string32(user->name, username);
}

void
session_process_event(Session *session, OSEvent *event)
{
    switch (event->type) {
    case OS_EVENT_KEY_PRESS: {
        u32 codepoint = event->key_press.code;
        if (codepoint == '\r') {
            msg_chat_message(session->prompt);
            string32_reset(session->prompt);
            session->prompt_cursor = 0;
        }
        else {
            string32_edit(session->prompt, event->key_press, &session->prompt_cursor);
        }
    }
    break;

    default: {
        printf("ui_udpate_session did not process an event\n");
    }
    }
}

void
session_reset(Session *session)
{
    session->cur_user_count = 0;

    session->message0 = 0;
    session->cur_message_count = 0;

    string32_reset(session->prompt);
}

Session *
session_create(MemArena *arena, struct Fscord *fscord)
{
    Session *session = mem_arena_push(arena, sizeof(Session));

    size_t max_user_count = MESSAGES_MAX_USER_COUNT;
    session->cur_user_count = 0;
    session->max_user_count = max_user_count;
    session->users = mem_arena_push(arena, max_user_count * sizeof(User));
    for (size_t i = 0; i < max_user_count; i++) {
        session->users[i].name = string32_create(arena, MESSAGES_MAX_MESSAGE_LEN);
    }

    size_t max_message_count = 256;
    session->message0 = 0;
    session->cur_message_count = 0;
    session->max_message_count = max_message_count;
    session->messages = mem_arena_push(arena, max_message_count * sizeof(ChatMessage));
    for (size_t i = 0; i < max_message_count; i++) {
        session->messages[i].sender_name = string32_create(arena, MESSAGES_MAX_USERNAME_LEN);
        session->messages[i].content = string32_create(arena, MESSAGES_MAX_MESSAGE_LEN);
    }

    session->prompt = string32_create(arena, 1024);

    s_fscord = fscord;

    return session;
}

