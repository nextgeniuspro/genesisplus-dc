#include <kos.h>
#include "menu.h"
#include "font.h"

#define FRAME_DELAY		15
#ifdef HIGH_RES
	#define LIST_LENGTH	13
#else
	#define LIST_LENGTH	6
#endif

#define LIST_MAX_PATH		1024
#define LIST_MAX_FILE		32

#define LIST_DEFAULT_PATH	"/cd/"
#define SINGLE_GAME_PATH	"/cd/games/game.zip"

#define LIST_TYPE_FILE		0
#define LIST_TYPE_DIR		1

#define SE_DEFAULT		1
#define FPS_DEFAULT		0

char base_path[LIST_MAX_PATH+1];

char filetype[LIST_MAX_PATH];
char filename[LIST_MAX_PATH][LIST_MAX_FILE+1];

pvr_ptr_t curs_tex; 
unsigned short *curs_buf;

void init_menu();
void files_browser();

/* Configurable options */
options_t dc_options =
{
	0,		/* Frameskip */
	487,	        /* M68k clock speed */
	228,     	/* Z80 clock speed */
  	SE_DEFAULT,	/* sound enabled */
  	FPS_DEFAULT	/* fps display */
};

#if defined(CHANKAST)
extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);
#endif

extern void dc_init();
extern void test_exec(char *filename);

void init_menu() {
	int x, y;

	/* load cursor */
	curs_tex = pvr_mem_malloc(8*8*2); 
	curs_buf = (unsigned short *)malloc(8*8);

	for (y=0; y<8; y++)
		for (x=0; x<8; x++)
			curs_buf[y*8+x] = 0xffff;

	pvr_txr_load_ex(curs_buf, curs_tex, 8, 8, PVR_TXRLOAD_16BPP);

	/* load BG image */
	pvr_set_bg_color(0.1, 0.4, 0.2);
}

void clear_menu() {
	/* free cursor */
	pvr_mem_free(curs_tex);
	free(curs_buf);

	/* free BG image */
}

void draw_cursor(int curs_y) {
	pvr_poly_hdr_t hdr; 
	pvr_vertex_t vert; 
	pvr_poly_cxt_t cxt; 

	float	u1=0, v1=0,
		u2=1, v2=1,
		w=586, h=26,
		x=27, y=94+curs_y*24, z=301;

	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 8, 8, curs_tex, PVR_FILTER_NONE);
	pvr_poly_compile(&hdr, &cxt); 

	pvr_prim(&hdr, sizeof(hdr)); 

	vert.argb = PVR_PACK_COLOR(0.5, 0, 0.5, 0.7);
	vert.oargb = 0; 
	vert.flags = PVR_CMD_VERTEX; 

	vert.x = x; 
	vert.y = y; 
	vert.z = z; 
	vert.u = u1; 
	vert.v = v1; 
	pvr_prim(&vert, sizeof(vert)); 

	vert.x = x+w;
	vert.y = y; 
	vert.z = z; 
	vert.u = u2; 
	vert.v = v1; 
	pvr_prim(&vert, sizeof(vert)); 

	vert.x = x; 
	vert.y = y+h; 
	vert.z = z; 
	vert.u = u1; 
	vert.v = v2; 
	pvr_prim(&vert, sizeof(vert)); 

	vert.x = x+w; 
	vert.y = y+h; 
	vert.z = z; 
	vert.u = u2; 
	vert.v = v2; 
	vert.flags = PVR_CMD_VERTEX_EOL; 
	pvr_prim(&vert, sizeof(vert)); 
}

int scan_dir(char *dir_path, int index) {
	file_t f, d;
	dirent_t *de;
	char tmp_path[LIST_MAX_PATH];
	int i;

	for (i=0; i<LIST_LENGTH; i++) {
		filetype[i] = -1;
		memset(filename[i], 0, LIST_MAX_FILE);
	}

	i = 0;

	d = fs_open(dir_path, O_RDONLY | O_DIR);

	if (!d) {
		printf("Can't open %s for reading.\n", dir_path);
		return -1;
	}

//	printf("Opened %s for scanning.\n", dir_path);

	while ((de = fs_readdir(d))) {
		if (!strcmp(de->name+strlen(de->name)-4, ".zip") ||
			!strcmp(de->name+strlen(de->name)-4, ".ZIP") ||
			!strcmp(de->name+strlen(de->name)-3, ".gz") ||
			!strcmp(de->name+strlen(de->name)-3, ".GZ") ||
			!strcmp(de->name+strlen(de->name)-4, ".bin") ||
			!strcmp(de->name+strlen(de->name)-4, ".BIN") ||
			!strcmp(de->name+strlen(de->name)-4, ".smd") ||
			!strcmp(de->name+strlen(de->name)-4, ".SMD")) {

			if (i<index || (i-index)>=LIST_LENGTH) {
//				printf("File #%d %s skipped.\n", i, de->name);
				i ++;
				continue;
			}

			strcpy(filename[i-index], de->name);

			filetype[i-index] = LIST_TYPE_FILE;

			i ++;

		} else {

			sprintf(tmp_path, "%s%s", dir_path, de->name);

			f = fs_open(tmp_path, O_RDONLY | O_DIR);

			if (f) {
				fs_close(f);

				if (i<index || (i-index)>=LIST_LENGTH) {
//					printf("Dir #%d %s skipped.\n", i, de->name);
					i ++;
					continue;
				}

				strcpy(filename[i-index], de->name);

				filetype[i-index] = LIST_TYPE_DIR;

				i ++;
			}

		}
	}

	fs_close(d);

	return i;
}

void files_browser() 
{
	maple_device_t	*dev;
	cont_state_t	*state;
	char		game_path[LIST_MAX_PATH];
	int		total, i, curs_y=0, index=0,
			up_pressed=0, down_pressed=0, left_pressed=0, right_pressed=0,
			x_pressed=0, y_pressed=0, a_pressed=0, b_pressed=0;

	total = scan_dir(base_path, index);

	while(1)
	{
		pvr_wait_ready();
		pvr_scene_begin();

		pvr_list_begin(PVR_LIST_TR_POLY);

		for (i=0; i<LIST_LENGTH; i++)
		{
			if (filetype[i] == LIST_TYPE_DIR)
			{
				gpfont_printf(&bios_font, 40, 95+i*24, "[%s]", filename[i]);
			}
			else if (filetype[i] != -1)
			{
				gpfont_printf(&bios_font, 40, 95+i*24, "%s", filename[i]);
			}
		}

		gpfont_printf(&bios_font, 30, 30, "Idx: %3d/%-3d - Counter (X): %-3s - Sound (Y): %-3s",
				index+curs_y+1, total,
				(dc_options.fps_display?"On":"Off"),
				(dc_options.sound_enabled?"On":"Off"));

		gpfont_print(&bios_font, 30, 54, "A: open - B: parent");
		gpfont_printf(&bios_font, 30+25*12, 54, "Z80 speed (L/R): %-3d", dc_options.z80_clock);
		gpfont_print(&bios_font, 30, 85+(LIST_LENGTH+1)*24, "L+R+START: exit game - X+Y+A+B+START: exit emu");

		draw_cursor(curs_y);

		pvr_list_finish();

		pvr_scene_finish();

		/* Find a controller */
		dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

		/* Get controller state */
		state = (cont_state_t *)maple_dev_status(dev);
		if(!state)
			continue;

		/* Exit combo - 4 buttons + start */
		if ((state->buttons & CONT_X) && 
			(state->buttons & CONT_Y) &&
			(state->buttons & CONT_A) &&
			(state->buttons & CONT_B) &&
				(state->buttons & CONT_START))
			break;

		/* D-Pad up pressed */    
		if ((!up_pressed || !--up_pressed) && (state->buttons & CONT_DPAD_UP))
		{
			up_pressed = FRAME_DELAY;

			if (curs_y > 0)
				curs_y --;
			else if (curs_y==0 && (index-LIST_LENGTH)>=0)
			{
				index -= LIST_LENGTH;
				total = scan_dir(base_path, index);
				curs_y = LIST_LENGTH-1;
			}
		}

		/* D-Pad down pressed */
		if ((!down_pressed || !--down_pressed) && (state->buttons & CONT_DPAD_DOWN))
		{
			down_pressed = FRAME_DELAY;

			if (curs_y<(LIST_LENGTH-1) && (index+curs_y)<(total-1))
				curs_y ++;
			else if (curs_y==(LIST_LENGTH-1) && (index+LIST_LENGTH)<total)
			{
				index += LIST_LENGTH;
				total = scan_dir(base_path, index);
				curs_y = 0;
			}
		}

		/* D-Pad left pressed */
		if ((!left_pressed || !--left_pressed) && (state->buttons & CONT_DPAD_LEFT))
		{
			left_pressed = FRAME_DELAY;

			if ((index-LIST_LENGTH) >= 0)
			{
				index -= LIST_LENGTH;
				total = scan_dir(base_path, index);
			}
		}

		/* D-Pad right pressed */
		if ((!right_pressed || !--right_pressed) && (state->buttons & CONT_DPAD_RIGHT))
		{
			right_pressed = FRAME_DELAY;

			if ((index+LIST_LENGTH) < total)
			{
				index += LIST_LENGTH;
				total = scan_dir(base_path, index);
				curs_y = ((index+curs_y) >= total) ? total-index-1 : curs_y;
			}
		}

		/* L trigger pressed */
		if (state->ltrig>128 && dc_options.z80_clock>25) {
			dc_options.z80_clock --;
			usleep(50*1000);
		}

		/* R trigger pressed */
		if (state->rtrig>128 && dc_options.z80_clock<228) {
			dc_options.z80_clock ++;
			usleep(50*1000);
		}

		/* X button pressed */
		if ((state->buttons & CONT_X) && !x_pressed)
		{
			x_pressed = 1;

			dc_options.fps_display = !dc_options.fps_display;
		}

		/* Y button pressed */
		if ((state->buttons & CONT_Y) && !y_pressed)
		{
			y_pressed = 1;

			dc_options.sound_enabled = !dc_options.sound_enabled;

			if (dc_options.sound_enabled)
				dc_options.z80_clock = 228;
			else
				dc_options.z80_clock = 25;
		}

		/* A button pressed */
		if ((state->buttons & CONT_A) && !a_pressed)
		{
			a_pressed = 1;

			if (filetype[curs_y] == LIST_TYPE_DIR)
			{
				sprintf(base_path, "%s%s/", base_path, filename[curs_y]);
				index = 0;
				total = scan_dir(base_path, index);
				curs_y = 0;
			} 
			else if (filetype[curs_y] != -1)
			{
				sprintf(game_path, "%s%s", base_path, filename[curs_y]);
				printf("Loading %s\n", game_path);
				test_exec(game_path);
				pvr_set_bg_color(0.1, 0.4, 0.2);
			}
		}

		/* B button pressed */
		if ((state->buttons & CONT_B) && !b_pressed)
		{
			b_pressed = 1;

			for (i=strlen(base_path)-2; i>0; i--)
				if (base_path[i] == '/')
					break;

			if (i >= 3)
			{
				memcpy(base_path, base_path, i+1);
				base_path[i+1] = 0;
			}

			index = 0;
			total = scan_dir(base_path, index);

			curs_y = 0;
		}

		/* X button released */
		if (!(state->buttons & CONT_X) && x_pressed) {
			x_pressed = 0;
		}

		/* Y button released */
		if (!(state->buttons & CONT_Y) && y_pressed) {
			y_pressed = 0;
		}

		/* A button released */
		if (!(state->buttons & CONT_A) && a_pressed) {
			a_pressed = 0;
		}

		/* B button released */
		if (!(state->buttons & CONT_B) && b_pressed) {
			b_pressed = 0;
		}
	}
}


int main(int argc, char *argv[])
{
	dc_init();

	/* load menu's texture(s) */
	init_menu();

	#if defined(CHANKAST)
		test_exec(SINGLE_GAME_PATH);
	#else
		strcpy(base_path, LIST_DEFAULT_PATH);
		files_browser();
	#endif
  
	clear_menu();
	pvr_shutdown();

	return 0;
}
