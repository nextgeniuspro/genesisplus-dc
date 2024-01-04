/* --------------------------------------------------------
   dreamcast/main.c
   Main loop/emulation code for Genesis Plus / DC

   Dreamcast code by BlackAura (obsidianglow@hotpop.com)
   Genesis Plus by Charles MacDonald (cgfm2@hotmail.com)
   -------------------------------------------------------- */

#include <kos.h>
#include "shared.h"
#include "menu.h"
#include "font.h"

/* FPS counter */
static int return_to_menu = 0;

pvr_init_params_t pvr_params = {
	{ PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 },
	512 * 1024
};

maple_device_t *devs[2];

/* Initialise the Dreamast-specific code */
void dc_init()
{
	devs[0] = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	devs[1] = maple_enum_type(1, MAPLE_FUNC_CONTROLLER);

	/* Initialise the PowerVR hardware */
	#ifdef HIGH_RES
	vid_set_mode(DM_640x480, PM_RGB565);
	#else
	vid_set_mode(DM_320x240, PM_RGB565);
	#endif
  
	pvr_init(&pvr_params);

	/* Set up the font system, load fonts */
	gpfont_init();

	/* Initialise the error handler */
	error_init();
}

/* Maple scanning code */
static __inline__ int update_input_player(int p)
{
	cont_state_t	*state;

	/* Find a controller */
	if(!devs[p])
		return 0;

	/* Get controller state */
	state = (cont_state_t *)maple_dev_status(devs[p]);
	if(!state)
		return 0;

	/* Exit combo - Both triggers + start */
	if((state->ltrig > 0) && (state->rtrig > 0) && (state->buttons & CONT_START))
		return_to_menu=1;

  input.pad[p] = state->buttons;

	/* Exit combo was not pressed */
	return 0;
}

static __inline__ int update_input()
{
	int p1res;
	memset(&input, 0, sizeof(t_input));
	p1res = update_input_player(0);
	update_input_player(1);
	return p1res;
}

static __inline__ uint32 getTimer()
{
	uint32 sec, msec;
	uint32 time;
	timer_ms_gettime(&sec, &msec);
	time = (sec * 1000) + msec;
	return time;
}

pvr_stats_t pvrstats;

/* Main emulator loop */
/* TODO - Split out into separate parts for the menu. Again */
void test_exec(char *filename)
{
  return_to_menu = 0;

	/* Load the ROM */
	int r = load_rom(filename);
	if (r == 0) return;

	/* Initialise the Genesis */
	system_init();
	system_reset();

	/* Start jammin' */
	if (dc_options.sound_enabled)
		audio_init(12000);

	/* Activate two controllers */
	input.dev[0] = DEVICE_3BUTTON;
	input.dev[1] = DEVICE_3BUTTON;
	io_rescan();

	while(!return_to_menu)
	{
		/* Update controllers */
		update_input();

		/* Emulate one frame */
		if(!system_frame(1))
			system_reset();

		/* Begin drawing a frame */
		pvr_wait_ready();
		pvr_scene_begin();

		/* Draw the borders */
		xrender_drawframe_op();

		/* Draw the on-screen display */
		if (dc_options.fps_display)
		{
			pvr_list_begin(PVR_LIST_TR_POLY);
			/* On-screen display goes in the translucent display list */
#ifdef HIGH_RES
			gpfont_printf(&bios_font, 24, 24, "RST: %i/%i/%i FPS: %f\n",
#else
			gpfont_printf(&bios_font, 12, 12, "RST: %i/%i/%i FPS: %f\n",
#endif
				pvrstats.reg_last_time, pvrstats.rnd_last_time,
				pvrstats.frame_last_time, pvrstats.frame_rate);
			pvr_list_finish();
		}

		/* Call the hardware renderer */
		xrender_drawframe();

		/* Finish drawing a frame, and send it off to the PVR */
		pvr_scene_finish();

		/* Shove the tiles off to the PVR, and hope it's late enough */
		xrender_endframe();

		/* Grab the stats for the previous frame */
		if (dc_options.fps_display)
			pvr_get_stats(&pvrstats);
	}

	if (dc_options.sound_enabled)
		gpdc_snd_stream_shutdown();
}


