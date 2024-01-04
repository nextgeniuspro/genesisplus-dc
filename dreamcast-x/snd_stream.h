/* KallistiOS 1.2.0

   dc/sound/stream.h
   (c)2002 Dan Potter

   $Id: stream.h,v 1.6 2003/03/09 01:26:36 bardtx Exp $

*/

#ifndef __GPDC_SOUND_STREAM_H
#define __GPDC_SOUND_STREAM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* Set "get data" callback */
void gpdc_snd_stream_set_callback(void **(*func)(int samples_requested, int *samples_returned));

/* Clear buffers */
void gpdc_snd_stream_clear();

/* Prefill buffers -- do this before calling start() */
void gpdc_snd_stream_prefill();

/* Initialize stream system */
int gpdc_snd_stream_init(void** (*callback)(int, int *));

/* Shut everything down and free mem */
void gpdc_snd_stream_shutdown();

/* Start streaming */
void gpdc_snd_stream_start(uint32 freq, int st);

/* Stop streaming */
void gpdc_snd_stream_stop();

/* Poll streamer to load more data if neccessary */
int gpdc_snd_stream_poll();

/* Set the volume on the streaming channels */
void gpdc_snd_stream_volume(int vol);

__END_DECLS

#endif	/* __DC_SOUND_STREAM_H */

