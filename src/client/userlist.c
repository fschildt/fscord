#include <client/userlist.h>
#include <basic/mem_arena.h>

#include <string.h>
#include <stdio.h>

void userlist_add_user(Userlist *userlist, String32 *username) {
    if (userlist->user_count == userlist->user_count_max) {
        printf("\tcannot add another user, since list is full, server should prevent this\n");
        return;
    }
    User *user = &userlist->users[userlist->user_count];
    string32_copy(username, user->name);

}

void userlist_remove_user(Userlist *userlist, String32 *username) {
    for (i32 i = 0; i < userlist->user_count; i++) {
        User *user = &userlist->users[i];
        if (string32_equal(username, user->name)) {
            i32 users_behind_count = userlist->user_count - i - 1;
            memmove(user, user+1, users_behind_count*sizeof(User));
            userlist->user_count -= 1;
        }
    }
}

Userlist *userlist_create_and_init(MemArena *arena, i32 user_count_max) {
    Userlist *userlist = mem_arena_push(arena, Userlist);
    userlist->user_count = 0;
    userlist->user_count_max = user_count_max;
    userlist->users = mem_arena_push(arena, User, user_count_max);
    return userlist;
}
