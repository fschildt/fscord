typedef struct
{
    Sys_Sound_Buffer sys_sb;
    s32 max_sample_count;
    s16 *samples;
} Lin_Sound_Buffer;

typedef struct
{
    snd_pcm_t *pcm;
    u32 frames_per_period;
    u32 samples_per_second;
    s32 channel_count;

    Lin_Sound_Buffer sound_buffer;
} Lin_Sound_Device;


// unused functions, but maybe usable in the future

#if 0
internal void
lin_print_sound_card_list(void)
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
      if (snd_ctl_open(&ctl, cardname, mode) >= 0)
      {
          printf("cannot snd_stl_open\n");
          int device = -1;
          if (snd_ctl_pcm_next_device(ctl, &device) < 0)
          {
              printf("no devices found\n");
              device = -1;
          }

          if (device == -1)
          {
              printf("no device\n");
          }
          while (device >= 0)
          {
              char devicename[256];
              sprintf(devicename, "%s,%d", cardname, device);
              printf("devicename = %s\n", devicename);

              snd_pcm_t *pcm_handle;
              snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK; // SND_PCM_STREAM_PLAYBACK or SND_PCM_STREAM_CAPTURE
              if (snd_pcm_open(&pcm_handle, devicename, stream, mode) >= 0)
              {
                  printf("snd_pcm_open success\n");
                  snd_pcm_close(pcm_handle);
              }
              else
              {
                  printf("snd_pcm_open failed\n");
              }

              if (snd_ctl_pcm_next_device(ctl, &device) < 0)
              {
                  printf("snd_ctl_pcm_next_device error\n");
                  break;
              }
          }

          snd_ctl_close(ctl);
      }
      else
      {
          printf("snd_ctl_open failed\n");
      }


      if ((status = snd_card_next(&card)) < 0) {
         printf("cannot determine card number: %s", snd_strerror(status));
         break;
      }
   } 
}
#endif
