internal void
playing_sound_update(Playing_Sound *playing_sound, Sys_Sound_Buffer *out)
{
    if (!out) return;

    s32 samples_to_output = out->sample_count;

    s32 play_cursor = playing_sound->play_cursor;
    Sound *sound = playing_sound->sound;

    if (play_cursor == -1) return;

    s32 samples_to_play = sound->sample_count - play_cursor;
    if (samples_to_play > samples_to_output)
    {
        samples_to_play = samples_to_output;
    }

    s16 *src = sound->samples + play_cursor;
    s16 *dest = out->samples;
    for (s32 j = 0; j < samples_to_play; j++)
    {
        dest[j] = src[j];
    }

    play_cursor += samples_to_play;
    if (play_cursor == sound->sample_count)
    {
        play_cursor = -1;
    }
    playing_sound->play_cursor = play_cursor;
}

internal void
playing_sound_start(Playing_Sound *playing_sound)
{
    playing_sound->play_cursor = 0;
}

internal void
playing_sound_init(Playing_Sound *playing_sound, Sound *sound)
{
    playing_sound->play_cursor = -1;
    playing_sound->sound = sound;
}

