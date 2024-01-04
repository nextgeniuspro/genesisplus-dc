#ifndef _DC_MENU_H_
#define _DC_MENU_H_

typedef struct options_s
{
	int frameskip;		 /* Frameskip count                 */
	int m68_clock;		 /* 68000 cycles per scanline (487) */
	int z80_clock;		 /* Z80 cycles per scanlins (228)   */
  int sound_enabled; /* 1 or 0                          */
  int fps_display;   /* 1 or 0                          */
} options_t;

extern options_t dc_options;

#endif // _DC_MENU_H_
