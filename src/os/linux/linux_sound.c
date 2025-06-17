// Todo: Use a high-priority thread for playing the sound buffer.
// Todo: refactor SND_CALL and other snd_... calls to deal with errors/state better.
// Note: Currently, the snd-api's buffer is large enough to write everything at once

#ifndef _POXIS_C_SOURCE
#define _POSIX_C_SOURCE 200809L // enable POSIX.1-2017
#endif

#include <os/os.h>
#include <basic/basic.h>
#include <basic/mem_arena.h>
#include <alsa/asoundlib.h>


struct OSSoundPlayer {
    snd_pcm_t *pcm;
    u32 frames_per_period;
    u32 samples_per_second;
    i32 channel_count;
    i32 max_sample_count;
    OSSoundBuffer sound_buffer;
};


#define SND_CALL(snd_call) \
{ \
    int error = snd_call; \
    if (error < 0) { \
        const char *error_str = snd_strerror(error); \
        printf(#snd_call" err: %s\n", error_str); \
        snd_pcm_close(pcm); \
        return false; \
    } \
}

internal_fn void
os_list_sound_devices(void)
{
   int status;
   int card = -1;
   char* longname  = NULL;
   char* shortname = NULL;

   if ((status = snd_card_next(&card)) < 0) {
      printf("cannot determine card number: %s", snd_strerror(status));
      return;
   }
   if (card < 0) {
      printf("no sound cards found");
      return;
   }
   while (card >= 0) {
      printf("Card %d:", card);
      if ((status = snd_card_get_name(card, &shortname)) < 0) {
         printf("cannot determine card shortname: %s", snd_strerror(status));
         break;
      }
      if ((status = snd_card_get_longname(card, &longname)) < 0) {
         printf("cannot determine card longname: %s", snd_strerror(status));
         break;
      }
      printf("\tLONG NAME:  %s\n", longname);
      printf("\tSHORT NAME: %s\n", shortname);


      char cardname[128];
      //sprintf(cardname, "default");
      sprintf(cardname, "hw:%d", card);
      printf("cardname = %s\n", cardname);

      snd_ctl_t *ctl;
      int mode = SND_PCM_ASYNC; // SND_PCM_NONBLOCK or SND_PCM_ASYNC
      if (snd_ctl_open(&ctl, cardname, mode) >= 0) {
          printf("cannot snd_stl_open\n");
          int device = -1;
          if (snd_ctl_pcm_next_device(ctl, &device) < 0) {
              printf("no devices found\n");
              device = -1;
          }

          if (device == -1) {
              printf("no device\n");
          }
          while (device >= 0) {
              char devicename[256];
              sprintf(devicename, "%s,%d", cardname, device);
              printf("devicename = %s\n", devicename);

              snd_pcm_t *pcm_handle;
              snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK; // SND_PCM_STREAM_PLAYBACK or SND_PCM_STREAM_CAPTURE
              if (snd_pcm_open(&pcm_handle, devicename, stream, mode) >= 0) {
                  printf("snd_pcm_open success\n");
                  snd_pcm_close(pcm_handle);
              }
              else {
                  printf("snd_pcm_open failed\n");
              }

              if (snd_ctl_pcm_next_device(ctl, &device) < 0) {
                  printf("snd_ctl_pcm_next_device error\n");
                  break;
              }
          }

          snd_ctl_close(ctl);
      }
      else {
          printf("snd_ctl_open failed\n");
      }


      if ((status = snd_card_next(&card)) < 0) {
         printf("cannot determine card number: %s", snd_strerror(status));
         break;
      }
   } 
}

internal_fn b32
os_sound_player_recover(snd_pcm_t *pcm, i64 err)
{
    if (err == -EPIPE) {
        err = snd_pcm_prepare(pcm);
        if (err < 0) {
            printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
            return false;
        }
        return true;
    }
    else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(pcm)) == -EAGAIN) {
            struct timespec ts = {0, 100};
            nanosleep(&ts, 0);
        }
        if (err < 0) {
            err = snd_pcm_prepare(pcm);
            if (err < 0) {
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


void
os_sound_player_play(OSSoundPlayer *player)
{
    OSSoundBuffer *buffer = &player->sound_buffer;

    i16 *samples = player->sound_buffer.samples;
    u32 frames_to_write = buffer->sample_count / player->channel_count;
    while (frames_to_write) {
        snd_pcm_sframes_t frames_written = snd_pcm_writei(player->pcm, samples, frames_to_write);
        if (frames_written < 0) {
            if (!os_sound_player_recover(player->pcm, frames_written)) {
                return;
            }
            continue;
        }
        samples += frames_written * player->channel_count;
        frames_to_write -= frames_written;
    }

    int state = snd_pcm_state(player->pcm);
    if (state != SND_PCM_STATE_RUNNING) {
        snd_pcm_t *pcm = player->pcm;
        snd_pcm_start(pcm);
    }
}

void
os_sound_player_close(OSSoundPlayer *player)
{
    snd_pcm_close(player->pcm);
    free(player->sound_buffer.samples);
}

OSSoundBuffer *
platform_sound_player_get_buffer(OSSoundPlayer *player)
{
    return &player->sound_buffer;
}

OSSoundBuffer *
os_sound_player_get_buffer(OSSoundPlayer *player)
{
    i64 frames_avail = snd_pcm_avail(player->pcm);
    if (frames_avail < 0) {
        if (os_sound_player_recover(player->pcm, frames_avail)) {
            frames_avail = 0;
        }
        frames_avail = snd_pcm_avail(player->pcm);
        if (frames_avail < 0) {
            frames_avail = 0;
        }
    }

    OSSoundBuffer *buffer = &player->sound_buffer;
    buffer->sample_count = frames_avail*2;
    memset(buffer->samples, 0, buffer->sample_count * sizeof(i16));
    return buffer;
}

OSSoundPlayer *
os_sound_player_create(MemArena *arena, i32 samples_per_second)
{
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


    // create player
    OSSoundPlayer *player = mem_arena_push(arena, sizeof(OSSoundPlayer));
    memset(player, 0, sizeof(*player));
    player->pcm = pcm;
    player->frames_per_period = frames_per_period;
    player->samples_per_second = sample_rate;
    player->channel_count = channels;

    // Todo: Redesign this player & buffer interaction. 
    // create buffer
    OSSoundBuffer *sound_buffer = &player->sound_buffer;
    sound_buffer->sample_count = 0;
    sound_buffer->max_sample_count = sample_count;
    sound_buffer->samples = mem_arena_push(arena, sample_count * sizeof(i16));

    return player;
}

