/*
    Copyright (C) 1999, 2000, 2001, 2002, 2003  Charles MacDonald

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "shared.h"

t_bitmap bitmap;
t_input input;
t_snd snd;
static int sound_tbl[262];

#ifdef _arch_dreamcast
#include "menu.h"
#include "dreamcast-x/snd_stream.h"
void **audio_callback(int samples_requested, int *samples_returned);
#endif

#define SND_SIZE (snd.buffer_size * sizeof(int16))

int audio_init(int rate)
{
    int i;

    /* 68000 and YM2612 clock */
    float vclk = 53693175.0 / 7;

    /* Z80 and SN76489 clock */
    float zclk = 3579545.0;

    /* Clear the sound data context */
    memset(&snd, 0, sizeof(snd));

    /* Make sure the requested sample rate is valid */
    if(!rate || ((rate < 8000) | (rate > 44100)))
    {
        return (0);
    }

    /* Calculate the sound buffer size */
#ifdef MODE_PAL
    snd.buffer_size = (rate / 50);
#else
    snd.buffer_size = (rate / 60);
#endif
    snd.sample_rate = rate;

    /* Allocate sound buffers */
#ifdef _arch_dreamcast
	snd.buffer[0] = memalign(32, SND_SIZE);
	snd.buffer[1] = memalign(32, SND_SIZE);
	snd.fm.buffer[0] = memalign(32, SND_SIZE);
	snd.fm.buffer[1] = memalign(32, SND_SIZE);
	snd.psg.buffer = memalign(32, SND_SIZE);
#else // !_arch_dreamcast
	snd.buffer[0] = malloc(snd.buffer_size * sizeof(int16));
	snd.buffer[1] = malloc(snd.buffer_size * sizeof(int16));
	#ifdef GENS_FM
		snd.fm.buffer[0] = malloc(snd.buffer_size * sizeof(int));
		snd.fm.buffer[1] = malloc(snd.buffer_size * sizeof(int));
	#else // !GENS_FM
		snd.fm.buffer[0] = malloc(snd.buffer_size * sizeof(int16));
		snd.fm.buffer[1] = malloc(snd.buffer_size * sizeof(int16));
	#endif // GENS_FM
	snd.psg.buffer = malloc(snd.buffer_size * sizeof(int16));
#endif // _arch_dreamcast

    /* Make sure we could allocate everything */
    if(!snd.buffer[0] || !snd.buffer[1] || !snd.fm.buffer[0] || !snd.fm.buffer[1] || !snd.psg.buffer)
    {
        return (0);
    }

    /* Initialize sound chip emulation */
    SN76496_sh_start(zclk, 100, rate);
#ifdef GENS_FM
    YM2612_Init(vclk, rate, 0);
#else
    YM2612Init(1, vclk, rate, NULL, NULL);
#endif

    /* Set audio enable flag */
    snd.enabled = 1;

    /* Make sound table */
    for (i = 0; i < 262; i++)
    {
        float p = snd.buffer_size * i;
        p = p / 262;
    	sound_tbl[i] = p;
    }    	


#ifdef _arch_dreamcast
	gpdc_snd_stream_init(&audio_callback);
	gpdc_snd_stream_clear();
	gpdc_snd_stream_start(rate, 1);
#endif

    return (1);
}

void audio_shutdown(void)
{
#ifdef _arch_dreamcast
	gpdc_snd_stream_stop();
	gpdc_snd_stream_shutdown();
#endif
}

void system_init(void)
{
    gen_init();
    vdp_init();
    render_init();
}

void system_reset(void)
{
    gen_reset();
    vdp_reset();
    render_reset();

    if(snd.enabled)
    {
#ifdef GENS_FM
        YM2612_Reset();
#else
        YM2612ResetChip(0);
#endif
        memset(snd.buffer[0], 0, SND_SIZE);
        memset(snd.buffer[1], 0, SND_SIZE);
#ifdef GENS_FM
        memset(snd.fm.buffer[0], 0, SND_SIZE*2);
        memset(snd.fm.buffer[1], 0, SND_SIZE*2);
#else
        memset(snd.fm.buffer[0], 0, SND_SIZE);
        memset(snd.fm.buffer[1], 0, SND_SIZE);
#endif
        memset(snd.psg.buffer, 0, SND_SIZE);
    }
}

void system_shutdown(void)
{
    gen_shutdown();
    vdp_shutdown();
    render_shutdown();
}


int system_frame(int do_skip)
{
    int line;

    if(gen_running == 0)
        return 0;

    /* Clear V-Blank flag */
    status &= ~0x0008;

    /* Toggle even/odd flag (IM2 only) */
    if(im2_flag)
        status ^= 0x0010;

    /* Point to start of sound buffer */
    snd.fm.lastStage = snd.fm.curStage = 0;
    snd.psg.lastStage = snd.psg.curStage = 0;

#ifdef GENS_FM
    /* Blank sound buffer */
    memset(snd.fm.buffer[0], 0, SND_SIZE*2);
    memset(snd.fm.buffer[1], 0, SND_SIZE*2);
#endif

#ifndef XVDP
    /* Parse sprites for line 0 (done on line 261) */
    parse_satb(0x80);
#endif

    for(line = 0; line < 262; line += 1)
    {
        /* Used by HV counter */
        v_counter = line;

        /* Run Z80 emulation (if enabled) */
        if(zreset == 1 && zbusreq == 0)
        {
			#ifdef _arch_dreamcast
				z80_execute(dc_options.z80_clock);
			#elif defined MODE_PAL
				z80_execute(274);
			#else
				z80_execute(228);
			#endif
            if(!gen_running) break;
        }

        /* Run 68000 emulation */
		#ifdef _arch_dreamcast
			M68K_Exec(dc_options.m68_clock);
		#elif defined MODE_PAL
			M68K_Exec(585);
		#else
			M68K_Exec(487);
		#endif
        if(!gen_running) break;

        /* If a Z80 interrupt is still pending after a scanline, cancel it */
        if(zirq == 1)
        {
            zirq = 0;
            z80_set_irq_line(0, CLEAR_LINE);
        }
#ifndef XVDP
        /* Render a line of the display */
        if(do_skip == 0)
        {
            if(line <  frame_end   ) render_line(line);
            if(line <  frame_end-1 ) parse_satb(0x81 + line);
        }
#endif

        /* Do H interrupt processing */
        if(line <= frame_end)
        {
            counter -= 1;
            if(counter == -1)
            {
                counter = reg[10];
                hint_pending = 1;
                if(reg[0] & 0x10)
                {
                    M68K_SetIRQ(4);
                }
            }
        }
        else
        {
            counter = reg[10];
        }

        /* Do end of frame processing */
        if(line == frame_end)
        {
            status |= 0x0088;
            vint_pending = 1;

            /* Give enough time to read the interrupt pending flag before
               the interrupt actually occurs. */
            M68K_Exec(16);
            if(!gen_running) break;

            if(reg[1] & 0x20)
            {
                M68K_SetIRQ(6);
            }

            if(zreset == 1 && zbusreq == 0)
            {
                z80_set_irq_line(0, ASSERT_LINE);
                zirq = 1;
            }
        }
 
        fm_update_timers();

        snd.fm.curStage = sound_tbl[line];
        snd.psg.curStage = sound_tbl[line];
    }

    if(snd.enabled)
    {
        audio_update();
    }

    return gen_running;
}


#ifdef GENS_FM
#	define PSG_VOLUME 3
#	define FM_VOLUME 2
#else
#	define PSG_VOLUME 2
#	define FM_VOLUME 1
#endif

void audio_update(void)
{
    int i;
    int16 acc;
#ifdef GENS_FM
    int32 *tempBuffer[2];
#else
    int16 *tempBuffer[2];
#endif

    tempBuffer[0] = snd.fm.buffer[0] + snd.fm.lastStage;
    tempBuffer[1] = snd.fm.buffer[1] + snd.fm.lastStage;

#ifdef GENS_FM
    YM2612_Update((int **)tempBuffer, snd.buffer_size - snd.fm.lastStage);
    YM2612_DacAndTimers_Update((int **)tempBuffer, snd.buffer_size - snd.fm.lastStage);
#else
    YM2612UpdateOne(0, (int16 **)tempBuffer, snd.buffer_size - snd.fm.lastStage);
#endif

    SN76496Update(0, snd.psg.buffer + snd.psg.lastStage, snd.buffer_size - snd.psg.lastStage);

    for(i = 0; i < snd.buffer_size; i += 1)
    {
        int16 psg = snd.psg.buffer[i] / PSG_VOLUME;

        acc = 0;
        acc += snd.fm.buffer[0][i] / FM_VOLUME;
        acc += psg;
        snd.buffer[0][i] = acc;

        acc = 0;
        acc += snd.fm.buffer[1][i] / FM_VOLUME;
        acc += psg;
        snd.buffer[1][i] = acc;
    }

#ifdef _arch_dreamcast
	gpdc_snd_stream_poll();
#endif
}

#ifdef _arch_dreamcast
void **audio_callback(int samples_requested, int *samples_returned)
{
    *samples_returned = snd.buffer_size;
    return (void**)snd.buffer;
}
#endif
