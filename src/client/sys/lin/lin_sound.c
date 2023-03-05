#define SND_CALL(snd_call) \
{ \
    int error = snd_call; \
    if (error < 0) \
    { \
        const char *error_str = snd_strerror(error); \
        printf(#snd_call" err: %s\n", error_str); \
        snd_pcm_close(pcm); \
        return false; \
    } \
}


internal b32
lin_recover_sound_device(snd_pcm_t *pcm, s64 err)
{
    if (err == -EPIPE)
    {
        err = snd_pcm_prepare(pcm);
        if (err < 0)
        {
            printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
            return false;
        }
        return true;
    }
    else if (err == -ESTRPIPE)
    {
        while ((err = snd_pcm_resume(pcm)) == -EAGAIN)
        {
            struct timespec ts = {0, 100};
            nanosleep(&ts, 0);
        }
        if (err < 0)
        {
            err = snd_pcm_prepare(pcm);
            if (err < 0)
            {
                printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
                return false;
            }
            return true;
        }
        return true;
    }
    printf("Can't recover: %s\n", snd_strerror(err));
    return false;
}
                                
internal bool
lin_play_sound_buffer(Lin_Sound_Device *device)
{
    Lin_Sound_Buffer *lin_sb = &device->sound_buffer;
    Sys_Sound_Buffer *sys_sb = (Sys_Sound_Buffer*)lin_sb;

    s16 *samples = sys_sb->samples;
    u32 frames_to_write = sys_sb->sample_count / device->channel_count;
    while (frames_to_write)
    {
        snd_pcm_sframes_t frames_written = snd_pcm_writei(device->pcm, samples, frames_to_write);
        if (frames_written < 0)
        {
            if (!lin_recover_sound_device(device->pcm, frames_written))
            {
                // Todo: don't exit
                printf("Exiting\n");
                exit(EXIT_SUCCESS);
            }
            continue;
        }

        samples += frames_written * device->channel_count;
        frames_to_write -= frames_written;
    }

    int state = snd_pcm_state(device->pcm);
    if (state != SND_PCM_STATE_RUNNING)
    {
        snd_pcm_t *pcm = device->pcm;
        SND_CALL(snd_pcm_start(pcm));
    }

    return true;
}

internal Lin_Sound_Buffer *
lin_get_sound_buffer(Lin_Sound_Device *device)
{
    s64 frames_avail = snd_pcm_avail(device->pcm);
    if (frames_avail < 0)
    {
        if (lin_recover_sound_device(device->pcm, frames_avail))
        {
            frames_avail = 0;
        }
        frames_avail = snd_pcm_avail(device->pcm);
        if (frames_avail < 0)
        {
            frames_avail = 0;
        }
    }

    printf("");

    Lin_Sound_Buffer *lin_sb = &device->sound_buffer;
    Sys_Sound_Buffer *sys_sb = (Sys_Sound_Buffer*)lin_sb;
    sys_sb->samples_per_second = device->samples_per_second;
    sys_sb->sample_count = frames_avail*2;
    sys_sb->samples = lin_sb->samples;
    assert(sys_sb->sample_count <= lin_sb->max_sample_count);
    memset(sys_sb->samples, 0, sys_sb->sample_count *sizeof(s16));

    return lin_sb;
}

internal void
lin_close_sound_device(Lin_Sound_Device *device)
{
    snd_pcm_close(device->pcm);
    free(device->sound_buffer.samples);
}

internal Lin_Sound_Device *
lin_open_sound_device()
{
    static Lin_Sound_Device device;
    assert(!device.pcm);


    // open pcm
    snd_pcm_t *pcm = 0;
    const char *device_name = "default";
    int stream = SND_PCM_STREAM_PLAYBACK;
    int mode = SND_PCM_ASYNC;

    SND_CALL(snd_pcm_open(&pcm, device_name, stream, mode));


    // set hw params
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);

    SND_CALL(snd_pcm_hw_params_any(pcm, params)); // get full configuration space

    int access = SND_PCM_ACCESS_RW_INTERLEAVED;
    SND_CALL(snd_pcm_hw_params_set_access(pcm, params, access));

    int format = SND_PCM_FORMAT_S16_LE;
    SND_CALL(snd_pcm_hw_params_set_format(pcm, params, format));

    u32 channels = 2;
    SND_CALL(snd_pcm_hw_params_set_channels(pcm, params, channels));

    u32 sample_rate = 44100;
    SND_CALL(snd_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, 0));

    u32 period_count = 3;
    u32 sample_count = period_count * (sample_rate / 30);
    u32 frame_count  = sample_count / channels;
    SND_CALL(snd_pcm_hw_params_set_buffer_size(pcm, params, frame_count));

    u32 frames_per_period = frame_count / period_count;
    SND_CALL(snd_pcm_hw_params_set_period_size(pcm, params, frames_per_period, 0));

    SND_CALL(snd_pcm_hw_params(pcm, params));

    device.pcm = pcm;
    device.frames_per_period = frames_per_period;
    device.samples_per_second = sample_rate;
    device.channel_count = channels;

    // setup sound buffer
    Lin_Sound_Buffer *lin_sb = &device.sound_buffer;
    lin_sb->max_sample_count = sample_count;
    lin_sb->samples = malloc(sample_count * sizeof(s16));
    if (!lin_sb->samples)
    {
        printf("malloc failed\n");
        snd_pcm_close(pcm);
    }

    return &device;
}

