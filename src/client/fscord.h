#ifndef FSCORD_H
#define FSCORD_H

#include <os/os.h>
#include <basic/time.h>
#include <client/userlist.h>
#include <client/chat.h>
#include <client/sound.h>
#include <client/asset_manager.h>
#include <client/login.h>
#include <client/session.h>

struct Fscord {
    MemArena arena;
    MemArena trans_arena;

    OSWindow *window;
    OSOffscreenBuffer *offscreen_buffer;
    OSSoundPlayer *sound_player;
    OSSoundBuffer *sound_buffer;

    EVP_PKEY *server_pub_rsa;
    OSNetSecureStream *secure_stream;

    Font *font;
    f32 zoom;

    Sound *sound_user_connected;
    Sound *sound_user_disconnected;
    PlaySound ps_user_connected;
    PlaySound ps_user_disconnected;

    b32 is_login;
    Login *login;
    Session *session;
};

#endif // FSCORD_H
