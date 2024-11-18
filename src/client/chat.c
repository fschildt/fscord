#include <client/chat.h>
#include <basic/mem_arena.h>

void
chat_add_message(Chat *chat, String32 *sender_name, String32 *content, Time creation_time)
{
    // Todo: is the index correct? Think once more.
    size_t index = (chat->start_index + chat->message_count) % ARRAY_COUNT(chat->messages);

    ChatMessage *message = &chat->messages[index];
    message->creation_time = creation_time;
    string32_copy(message->sender_name, sender_name);
    string32_copy(message->content, content);
}

void
chat_reset(Chat *chat)
{
    chat->start_index = 0;
    chat->message_count = 0;
}

Chat *
chat_create_and_init(MemArena *arena, i32 message_count_max)
{
    Chat *chat = mem_arena_push(arena, Chat);
    chat->start_index = 0;
    chat->message_count = 0;
    return chat;
}

