#include <basic/mem_arena.h>
#include <client/sound.h>
#include <os/os.h>

void play_sound_update(PlaySound *ps, OSSoundBuffer *dest_buffer) {
    if (!dest_buffer) {
        return;
    }

    i32 samples_readable = ps->sound->sample_count - ps->play_cursor;
    i32 samples_writable = dest_buffer->sample_count;
    i32 samples_to_play;
    if (samples_readable > samples_writable) {
        samples_to_play = samples_writable;
    } else {
        samples_to_play = samples_readable;
    }

    i16 *src = &ps->sound->samples[ps->play_cursor];
    i16 *dest = dest_buffer->samples;
    for (i32 i = 0; i < samples_to_play; i++) {
        dest[i] = src[i];
    }
    
    ps->play_cursor += samples_to_play;
}


void play_sound_init(PlaySound *ps, Sound *sound) {
    ps->play_cursor = 0;
    ps->sound = sound;
}

