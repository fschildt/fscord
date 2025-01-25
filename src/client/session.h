#ifndef SESSION_H
#define SESSION_H

#include <basic/mem_arena.h>
#include <basic/string32.h>
#include <basic/time.h>
#include <os/os.h>

struct Fscord;

typedef struct {
    String32Buffer *name;
} User;

typedef struct {
    Time creation_time;
    String32Buffer *sender_name;
    String32Buffer *content;
} ChatMessage;

typedef struct {
    size_t cur_user_count;
    size_t max_user_count;
    User *users;

    size_t message0;
    size_t cur_message_count;
    size_t max_message_count;
    ChatMessage *messages;

    String32Buffer *prompt;
    size_t prompt_cursor;
} Session;

Session *session_create(MemArena *arena, struct Fscord *fscord);
void session_reset(Session *session);
void session_process_event(Session *session, OSEvent *event);
void session_draw(Session *session);

#endif // SESSION_H
