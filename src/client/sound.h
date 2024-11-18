#ifndef SOUND_H
#define SOUND_H


#include <basic/mem_arena.h>
#include <os/os.h>
#include <basic/basic.h>

typedef struct {
    i32 samples_per_second;
    i32 sample_count;
    i16 *samples;
} Sound;

typedef struct {
    i32 play_cursor;
    Sound *sound;
} PlaySound;

void play_sound_init(PlaySound *ps, Sound *sound);
void play_sound_update(PlaySound *ps, OSSoundBuffer *dest_buffer);


#endif // SOUND_H
