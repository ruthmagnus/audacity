/*
  Open Sound System (oss) implementation of snd
  by Dominic Mazzoni
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/soundcard.h>


/* snd includes */

#include "snd.h"

typedef struct {
  int audio_fd;
} oss_info_struct, *oss_info;


oss_info get_oss_info(snd_type snd)
{
  return (oss_info) snd->u.audio.descriptor;
}


int audio_reset(snd_type snd);


long audio_poll(snd_type snd)
{
  oss_info dp = get_oss_info(snd);

  audio_buf_info info;
  ioctl(dp->audio_fd, SNDCTL_DSP_GETOSPACE, &info);

  /* Note that this returns frames while audio_write
	 returns bytes */

  return info.bytes / snd_bytes_per_frame(snd);
}


long audio_read(snd_type snd, void *buffer, long length)
{
  /* Not implemented */
  return !SND_SUCCESS;
}


long audio_write(snd_type snd, void *buffer, long length_in_bytes)
{
  oss_info dp = get_oss_info(snd);

  int rval = write(dp->audio_fd, buffer, length_in_bytes);

  return rval;
}


int audio_open(snd_type snd, long *flags)
{
  int format;
  int channels;
  int rate;
  oss_info dp;

  snd->u.audio.descriptor = (oss_info) malloc(sizeof(oss_info_struct));
  dp = get_oss_info(snd);

  /* Open /dev/dspW */
  dp->audio_fd = open("/dev/dspW", O_WRONLY, 0);

  if (dp->audio_fd == -1)
	return !SND_SUCCESS;

  /* Set format to signed 16-bit little-endian */
  format = AFMT_S16_LE;
  if (ioctl(dp->audio_fd, SNDCTL_DSP_SETFMT, &format) == -1)
	return !SND_SUCCESS;
  if (format != AFMT_S16_LE) /* this format is not supported */
	return !SND_SUCCESS;
  
  /* Set number of channels */
  channels = snd->format.channels;
  if (ioctl(dp->audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
	return !SND_SUCCESS;
  if (channels != snd->format.channels)
	return !SND_SUCCESS;

  /* Set sampling rate.  Must set sampling rate AFTER setting
	 number of channels. */
  rate = (int)(snd->format.srate + 0.5);
  if (ioctl(dp->audio_fd, SNDCTL_DSP_SPEED, &rate) == -1)
	return !SND_SUCCESS;
  if (rate - (int)(snd->format.srate + 0.5) > 100 ||
	  rate - (int)(snd->format.srate + 0.5) < -100)
	return !SND_SUCCESS;

  return SND_SUCCESS;
}


int audio_close(snd_type snd)
{
  int dummy;
  oss_info dp = get_oss_info(snd);

  /* Stop playing immediately */
  dummy = 0;
  ioctl(dp->audio_fd, SNDCTL_DSP_RESET, &dummy);

  close(dp->audio_fd);

  free((void *)snd->u.audio.descriptor);

  return SND_SUCCESS;
}


/* audio_flush -- finish audio output */
int audio_flush(snd_type snd)
{
  int dummy;
  oss_info dp = get_oss_info(snd);

  dummy = 0;
  if (ioctl(dp->audio_fd, SNDCTL_DSP_SYNC, &dummy) == -1)
	return !SND_SUCCESS;

  return SND_SUCCESS;
}


int audio_reset(snd_type snd)
{
  int dummy;
  oss_info dp = get_oss_info(snd);

  /* Stop playing immediately */
  dummy = 0;
  ioctl(dp->audio_fd, SNDCTL_DSP_RESET, &dummy);

  return SND_SUCCESS;
}

snd_fns_node osssystem_dictionary = { audio_poll, audio_read, audio_write, 
									  audio_open, audio_close, audio_reset,
									  audio_flush };

void snd_init()
{
  snd_add_device("oss", "default", &osssystem_dictionary);
}
