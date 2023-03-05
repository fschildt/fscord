internal f32
ui_get_textbox_width(Ui *ui, s32 len)
{
    f32 width = 2*ui->textbox_text_xoff + len*ui->font->x_advance;
    return width; }

internal f32
ui_get_textbox_height(Ui *ui)
{
    f32 height = 2*ui->textbox_text_yoff + ui->font->y_advance;
    return height;
}

internal f32
ui_get_max_text_width(Ui *ui, Ui_Text *text)
{
    Font *font = ui->font;
    f32 width = font->x_advance * (text->size - 1);
    return width;
}

internal f32
ui_get_text_width(Ui *ui, s32 len)
{
    Font *font = ui->font;
    f32 width = font->x_advance * len;
    return width;
}

internal s32
ui_max_textlen_within_width(Ui *ui, s32 len)
{
    s32 result = len / ui->font->x_advance;
    return result;
}

internal f32
ui_get_text_height(Ui *ui, Ui_Text *text)
{
    Font *font = ui->font;
    f32 height = font->y_advance;
    return height;
}

internal V2
ui_get_absolute_pos(Ui_Entity_Header *e)
{
    V2 pos = e->pos;

    Ui_Entity_Header *parent = e->parent;
    while (parent)
    {
        pos = v2_add(pos, parent->pos);
        parent = parent->parent;
    }

    return pos;
}

internal void
ui_draw_textbox(Ui *ui, Sys_Offscreen_Buffer *screen, Ui_Textbox *textbox, V2 pos)
{
    textbox->header.pos = pos;

    V2 absolute_pos = ui_get_absolute_pos(&textbox->header);

    f32 width = ui_get_textbox_width(ui, textbox->size-1);
    f32 height = ui_get_textbox_height(ui);
    V2 dim = v2(width, height);

    V2 text_off = v2(ui->textbox_text_xoff, ui->textbox_text_yoff);
    V2 text_pos = v2_add(absolute_pos, text_off);

    V3 border_color;
    if (ui->active == &textbox->header)
    {
        border_color = v3(0.16, 0.32, 0.08);

        f32 xoff = ui_get_text_width(ui, textbox->cursor);
        V2 cursor_pos = v2(text_pos.x + xoff, text_pos.y);
        V2 cursor_dim = v2(ui->font->x_advance / 4, ui->font->y_advance);
        V3 cursor_col = v3(0, 0, 0);
        render_rectangle(screen, cursor_pos, cursor_dim, cursor_col);
    }
    else
    {
        border_color = v3(0, 0, 0);
    }
    render_border(screen, absolute_pos, dim, ui->textbox_border_size, border_color);
    render_text(screen, ui->font, textbox->buffer, textbox->len, text_pos);

    textbox->header.dim = dim;
}

internal void
ui_draw_text(Ui *ui, Sys_Offscreen_Buffer *screen, Ui_Text *text, V2 pos)
{
    text->header.pos = pos;

    V2 absolute_pos = ui_get_absolute_pos(&text->header);
    f32 width = ui_get_max_text_width(ui, text);
    f32 height = ui_get_text_height(ui, text);
    V2 dim = v2(width, height);

    render_text(screen, ui->font, text->buffer, text->len, absolute_pos);

    text->header.dim = dim;
}

internal void
ui_draw_message(Ui *ui, Sys_Offscreen_Buffer *screen, Message *msg, V2 pos)
{
    Font *font = ui->font;

    pos.x += font->y_advance / 4;
    f32 dx = font->y_advance / 2;


    // draw time
    int time_str_len = 5;
    char time_str[time_str_len+1];
    int time_str_width = ui_get_text_width(ui, time_str_len);

	struct tm *tm_local = localtime(&msg->epoch_time);
    sprintf(time_str, "%.2d:%.2d", tm_local->tm_hour, tm_local->tm_min);

    render_text(screen, font, time_str, time_str_len, pos);
    pos.x += time_str_width + dx;


    // draw name
    int c_width = ui_get_text_width(ui, 1);

    char lt = '<';
    render_text(screen, font, &lt, 1, pos);
    pos.x += c_width;

    int username_width = ui_get_text_width(ui, msg->username_len);
    render_text(screen, font, msg->username, msg->username_len, pos);
    pos.x += username_width;

    char gt = '>';
    render_text(screen, font, &gt, 1, pos);
    pos.x += c_width + dx;


    // draw content
    render_text(screen, font, msg->content, msg->content_len, pos);
}

internal void
ui_draw_history(Ui *ui, Sys_Offscreen_Buffer *screen, V2 pos, V2 dim)
{
    Ui_History *ui_hist = ui->session->history;
    Message_History *hist = ui_hist->history;
    Font *font = ui->font;

    f32 border_size = font->y_advance / 8;
    V3 border_color = v3(0, 0, 0);
    render_border(screen, pos, dim, border_size, border_color);

    pos.x += border_size * 4;
    pos.y += border_size * 4;
    
    f32 dy = font->y_advance + border_size * 4;

    int base = hist->base;
    int cnt_ge = hist->cnt - base;
    int cnt_lt = hist->cnt - cnt_ge;

    for (int i = cnt_lt-1; i >= 0; i--)
    {
        Message *message = hist->messages + i;
        if (pos.y >= pos.y + dim.y) break;
        ui_draw_message(ui, screen, message, pos);
        pos.y += dy;
    }

    for (int i = base+cnt_ge-1; i >= base; i--)
    {
        Message *message = hist->messages + i;
        if (pos.y >= pos.y + dim.y) break;
        ui_draw_message(ui, screen, message, pos);
        pos.y += dy;
    }
}

internal void
ui_draw_prompt(Ui *ui, Sys_Offscreen_Buffer *screen, V2 pos, V2 dim)
{
    Ui_Textbox *prompt = ui->session->prompt;
    Font *font = ui->font;

    f32 border_size = font->y_advance / 8;
    V3 border_color = v3(0.16, 0.32, 0.08);
    render_border(screen, pos, dim, border_size, border_color);

    f32 xmargin = border_size * 4;
    f32 ymargin = border_size * 4;

    // text
    V2 text_pos = v2(pos.x + xmargin, pos.y + ymargin);
    render_text(screen, ui->font, prompt->buffer, prompt->len, text_pos);

    // cursor
    f32 xoff = ui_get_text_width(ui, prompt->cursor);
    V2 cursor_pos = v2(text_pos.x + xoff, text_pos.y);
    V2 cursor_dim = v2(ui->font->x_advance / 4, ui->font->y_advance);
    V3 cursor_col = v3(0, 0, 0);
    render_rectangle(screen, cursor_pos, cursor_dim, cursor_col);
}

internal void
ui_draw_userlist(Ui *ui, Sys_Offscreen_Buffer *screen, V2 pos, V2 dim)
{
    Ui_Userlist *userlist = ui->session->userlist;
    Font *font = ui->font;

    f32 border_size = font->y_advance / 8;
    V3 border_color = v3(0, 0, 0);
    render_border(screen, pos, dim, border_size, border_color);

    f32 xmargin = border_size * 2;
    f32 ymargin = border_size * 2;
    f32 xmax = pos.x + dim.x;
    f32 dy = font->y_advance;
    V2 pos_curr = v2(pos.x + xmargin, pos.y + dim.y - ymargin - dy);

    s32 cnt_users = userlist->userlist->cnt_curr;
    for (s32 i = 0; i < cnt_users; i++)
    {
        if (pos_curr.y < ymargin - font->y_advance)
        {
            break;
        }

        f32 width_remain = xmax - pos_curr.x;


        User *user = userlist->userlist->users + i;

        s32 name_len = user->name_len;
        s32 name_len_avail = ui_max_textlen_within_width(ui, width_remain);
        if (name_len > name_len_avail)
        {
            name_len = name_len_avail;
        }

        render_text(screen, font, user->name, name_len, pos_curr);
        pos_curr.y -= dy;
    }
}

internal void
ui_draw_session(Ui *ui, Sys_Offscreen_Buffer *screen, State *state)
{
    V2 pos = v2(0, 0);
    V2 dim = v2(screen->width, screen->height);
    V3 col = v3(0.8, 0.6, 0.4);
    render_rectangle(screen, pos, dim, col);


    f32 scale = ui->font->y_advance;


    V2 userlist_pos = v2(0, 0);
    V2 userlist_dim = v2(scale * 8, screen->height);

    V2 prompt_pos = v2(userlist_dim.x, 0);
    V2 prompt_dim = v2(screen->width - userlist_dim.x, scale*2);

    V2 history_pos = v2(prompt_pos.x, prompt_pos.y + prompt_dim.y);
    V2 history_dim = v2(prompt_dim.x, screen->height - prompt_dim.y);

    ui_draw_userlist(ui, screen, userlist_pos, userlist_dim);
    ui_draw_prompt(ui, screen, prompt_pos, prompt_dim);
    ui_draw_history(ui, screen, history_pos, history_dim);
}

internal void
ui_draw_login(Ui *ui, Sys_Offscreen_Buffer *screen)
{
    V2 pos = v2(0, 0);
    V2 dim = v2(screen->width, screen->height);

    // draw background
    V3 bg_col = v3(0.4, 0.2, 0.2);
    render_rectangle(screen, pos, dim, bg_col);


    // draw login panel
    f32 scale = ui->font->y_advance;
    f32 width = scale*28;
    f32 height = scale*20;

    Ui_Login *login = ui->login;
    login->header.dim = v2(width, height);
    login->header.pos = center_v2(login->header.dim, v2(screen->width, screen->height));
    render_rectangle(screen, login->header.pos, login->header.dim, v3(0.8, 0.6, 0.4));

    // draw login children
    s32 max_len = login->servername->size - 1;
    f32 textbox_width = ui_get_textbox_width(ui, max_len);
    f32 textbox_height = ui_get_textbox_height(ui);
    f32 text_height = ui->font->y_advance;

    f32 children_height = textbox_height*2 + text_height*4;
    f32 children_width  = textbox_width;

    // - coordinates relative to login panel
    V2 children_dim = v2(children_width, children_height);
    V2 children_pos = center_v2(children_dim, login->header.dim);

    pos = children_pos;

    ui_draw_text(ui, screen, login->warning, pos);
    pos.y += text_height*2;

    ui_draw_textbox(ui, screen, login->servername, pos);
    pos.y += textbox_height;

    ui_draw_text(ui, screen, login->servername_hint, pos);
    pos.y += text_height;

    ui_draw_textbox(ui, screen, login->username, pos);
    pos.y += textbox_height;

    ui_draw_text(ui, screen, login->username_hint, pos);
    pos.y += text_height;
}


internal void
ui_edit_textbox(Ui_Textbox *textbox, char c)
{
    s32 len = textbox->len;
    s32 size = textbox->size;
    if (c >= 32 && c <= 126 && len < size-1)
    {
        // insert character
        memmove(textbox->buffer + textbox->cursor + 1,
                textbox->buffer + textbox->cursor,
                len - textbox->cursor + 1);
        textbox->buffer[textbox->cursor] = c;
        textbox->cursor += 1;
        textbox->len += 1;
    }
    else if (c == 8 && textbox->cursor > 0)
    {
        // delete character left (8 is ascii backspace)
        memmove(textbox->buffer + textbox->cursor -1,
                textbox->buffer + textbox->cursor,
                len - textbox->cursor + 1);
        textbox->cursor -= 1;
        textbox->len -= 1;
    }
    else if (c == 127 && textbox->cursor < len)
    {
        // delete character right (127 is ascii delete)
        memmove(textbox->buffer + textbox->cursor,
                textbox->buffer + textbox->cursor + 1,
                len - textbox->cursor);
        textbox->len -= 1;
    }
    else if (c == SYS_KEY_LEFT)
    {
        // move cursor left
        if (textbox->cursor > 0)
        {
            textbox->cursor -= 1;
        }
    }
    else if (c == SYS_KEY_RIGHT)
    {
        // move cursor right
        if (textbox->cursor < textbox->len)
        {
            textbox->cursor += 1;
        }
    }
}

internal void
ui_login_set_warning(Ui_Text *warning, const char *str, s32 len)
{
    assert(len < warning->size);

    memcpy(warning->buffer, str, len);
    warning->len = len;
}

internal void
ui_login_set_default_warning(Ui_Text *warning)
{
    const char *msg = "Enter: Login, Tab: change selection";
    s32 msg_len = strlen(msg);
    assert(msg_len < warning->size);

    memcpy(warning->buffer, msg, msg_len);
    warning->len = msg_len;
}

internal bool
ui_parse_servername(char *address, s32 *address_len, u16 *port, char *servername, s32 servername_len, Ui_Text *warning)
{
    char *colon = servername;
    while (*colon != ':' && *colon != '\0')
    {
        colon++;
    }

    if (*colon == ':')
    {
        u16 tmp_port = atoi(colon + 1);
        if (tmp_port != 0)
        {
            size_t address_len = colon-servername;
            memcpy(address, servername, address_len);
            address[address_len] = '\0';
            *port = tmp_port;
            return true;
        }
    }

    char *warning_msg = "Error: invalid servername format";
    s32 warning_msg_len = strlen(warning_msg);
    ui_login_set_warning(warning, warning_msg, warning_msg_len);
    return false;
}


internal void
ui_update_session(Ui *ui, Sys_Event *event, State *state)
{
    switch (event->type)
    {
        case SYS_EVENT_KEY:
        {
            char c = event->key.c;
            Ui_Textbox *prompt = ui->session->prompt;

            if (c == '\r')
            {
                net_send_message(&state->net, prompt->buffer, prompt->len);
                prompt->len = 0;
                prompt->cursor = 0;
            }
            else
            {
                ui_edit_textbox(prompt, c);
            }
        }
        break;

        default:
        {
            printf("ui_udpate_session did not process an event\n");
        }
    }
}

internal void
ui_update_login(Ui *ui, Sys_Event *event, State *state)
{
    Ui_Login *login = ui->login;

    switch (event->type)
    {
        case SYS_EVENT_KEY:
        {
            ui_login_set_default_warning(ui->login->warning);

            Ui_Textbox *textbox_active;
            Ui_Textbox *textbox_other;
            if (ui->active == (Ui_Entity_Header*)login->username)
            {
                textbox_active = login->username;
                textbox_other = login->servername;
            }
            else if (ui->active == (Ui_Entity_Header*)login->servername)
            {
                textbox_active = login->servername;
                textbox_other = login->username;
            }
            else
            {
                assert(0);
            }

            char c = event->key.c;
            if (c == '\r')
            {
                if (state->net.status == NET_HANDSHAKE_PENDING ||
                    state->net.status == NET_LOGIN_PENDING)
                {
                    break;
                }

                // get username
                const char *warning_username_len = "username has to have a length>0";
                char *username = login->username->buffer;
                s32 username_len = login->username->len;
                if (username_len <= 0)
                {
                    ui_login_set_warning(login->warning, warning_username_len, strlen(warning_username_len));
                    break;
                }

                // parse servername
                s32 address_len = 64;
                char address[address_len];
                u16 port;
                if (!ui_parse_servername(address, &address_len, &port,
                                         login->servername->buffer,
                                         login->servername->len,
                                         login->warning))
                {
                    break;
                }
                printf("address = %s, port = %d\n", address, port);


                // update warning
                const char *warning_message = "connecting...";
                ui_login_set_warning(login->warning, warning_message, strlen(warning_message));

                net_connect_and_login(&state->net, address, port,
                                      username, username_len);
            }
            else if (c == '\t')
            {
                ui->active = (Ui_Entity_Header*)textbox_other;
            }
            else
            {
                ui_edit_textbox(textbox_active, c);
            }
        }
        break;

        default:;
    }
}

internal void
ui_update_and_render(Ui *ui, Sys_Event *events, s32 event_count, State *state)
{
    if (state->net.status == NET_LOGGED_IN)
    {
        for (s32 i = 0; i < event_count; i++)
        {
            Sys_Event *event = events + i;
            ui_update_session(ui, event, state);
        }
    }
    else
    {
        const char *neterror = net_get_last_error(&state->net);
        if (neterror)
        {
            ui_login_set_warning(ui->login->warning, neterror, strlen(neterror));
            ui->session->prompt->len = 0;
            userlist_reset(&state->userlist);
            message_history_reset(&state->message_history);
        }

        for (s32 i = 0; i < event_count; i++)
        {
            Sys_Event *event = events + i;
            ui_update_login(ui, event, state);
        }
    }


    // render
    if (state->net.status == NET_LOGGED_IN)
    {
        ui_draw_session(ui, state->screen, state);
    }
    else
    {
        ui_draw_login(ui, state->screen);
    }
}


// @Speed: make a macro instead of a switch-statement
internal void *
ui_make_entity(Ui *ui, Ui_Entity_Type type)
{
    Memory_Arena *arena = &ui->arena;
    Ui_Entity_Header *curr_parent = ui->curr_parent;

    Ui_Entity_Header *header;
    switch (type)
    {
        case UI_TEXT:
        {
            Ui_Text *text = memory_arena_push(arena, sizeof(Ui_Text));
            header = (Ui_Entity_Header*)text;
        }
        break;

        case UI_TEXTBOX:
        {
            Ui_Textbox *textbox = memory_arena_push(arena, sizeof(Ui_Textbox));
            header = (Ui_Entity_Header*)textbox;
        }
        break;


        case UI_LOGIN:
        {
            Ui_Login *login = memory_arena_push(arena, sizeof(Ui_Login));
            header = (Ui_Entity_Header*)login;
        }
        break;

        case UI_USERLIST:
        {
            Ui_Userlist *userlist = memory_arena_push(arena, sizeof(Ui_Userlist));
            header = (Ui_Entity_Header*)userlist;
        }
        break;

        case UI_HISTORY:
        {
            Ui_History *hist = memory_arena_push(arena, sizeof(Ui_History));
            header = (Ui_Entity_Header*)hist;
        }
        break;

        case UI_SESSION:
        {
            Ui_Session *session = memory_arena_push(arena, sizeof(Ui_Session));
            header = (Ui_Entity_Header*)session;
        }
        break;
    }

    header->type = type;
    header->parent = curr_parent;
    header->next = 0;
    header->first_child = 0;
    header->last_child = 0;

    Ui_Entity_Header *parent = curr_parent;
    if (parent)
    {
        if (!parent->first_child)
        {
            parent->first_child = header;
            parent->last_child = header;
        }
        else
        {
            parent->last_child->next = header;
            parent->last_child = header;
        }
    }

    return header;
}

internal void
ui_begin_children(Ui *ui, Ui_Entity_Header *new_parent)
{
    ui->curr_parent = new_parent;
}

internal void
ui_end_children(Ui *ui)
{
    ui->curr_parent = ui->curr_parent->parent;
}

internal Ui_Text *
ui_make_text(Ui *ui, s32 size, char *str)
{
    assert(size > strlen(str));

    Ui_Text *text = (Ui_Text*)ui_make_entity(ui, UI_TEXT);
    text->size = size;
    text->len = strlen(str);
    text->buffer = memory_arena_push(&ui->arena, size);
    strcpy(text->buffer, str);

    return text;
}

internal Ui_Textbox *
ui_make_textbox(Ui *ui, s32 size, const char *str)
{
    s32 len = strlen(str);

    Ui_Textbox *textbox = (Ui_Textbox*)ui_make_entity(ui, UI_TEXTBOX);
    textbox->cursor = len;
    textbox->len = len;
    textbox->size = size;
    textbox->buffer = memory_arena_push(&ui->arena, size);
    assert(len < size);
    memcpy(textbox->buffer, str, len);

    return textbox;
}

internal Ui_History *
ui_make_history(Ui *ui, Message_History *history)
{
    Ui_History *ui_history = (Ui_History*)ui_make_entity(ui, UI_HISTORY);
    ui_history->history = history;
    return ui_history;
}

internal Ui_Userlist *
ui_make_userlist(Ui *ui, Userlist *userlist)
{
    Ui_Userlist *ui_userlist = (Ui_Userlist*)ui_make_entity(ui, UI_USERLIST);
    ui_userlist->userlist = userlist;
    return ui_userlist;
}

internal Ui_Session *
ui_make_session(Ui *ui, State *state)
{
    Ui_Session *session = ui_make_entity(ui, UI_SESSION);
    ui_begin_children(ui, (Ui_Entity_Header*)session);
    {
        session->userlist = ui_make_userlist(ui, &state->userlist);
        session->prompt = ui_make_textbox(ui, 48, "");
        session->history = ui_make_history(ui, &state->message_history);
    }
    ui_end_children(ui);

    return session;
}

internal Ui_Login *
ui_make_login(Ui *ui)
{
    Ui_Login *login = ui_make_entity(ui, UI_LOGIN);
    ui_begin_children(ui, (Ui_Entity_Header*)login);
    {
        login->username_hint = ui_make_text(ui, 16, "username:");
        login->servername_hint = ui_make_text(ui, 16, "servername:");
        login->username = ui_make_textbox(ui, 32, "");
        login->servername = ui_make_textbox(ui, 32, "127.0.0.1:1905");
        login->warning = ui_make_text(ui, 48, "");
    }
    ui_end_children(ui);

    return login;
}

internal bool
ui_init(Ui *ui, State *state, Font *font, Memory_Arena *arena)
{
    ui->is_login = true;
    memory_arena_make_sub_arena(arena, &ui->arena, KIBIBYTES(40));

    ui->font = font;
    ui->textbox_border_size = ui->font->y_advance / 16;
    ui->textbox_text_xoff = ui->font->y_advance / 4;
    ui->textbox_text_yoff = ui->font->y_advance / 4;

    ui->curr_parent = 0;
    ui->login = ui_make_login(ui);
    ui->session = ui_make_session(ui, state);

    ui->active = (Ui_Entity_Header*)ui->login->username;
    ui_login_set_default_warning(ui->login->warning);

    return true;
}
