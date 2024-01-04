#ifndef _FONT_H_
#define _FONT_H_

typedef struct gpfont_s
{
	pvr_ptr_t texture;
	pvr_poly_hdr_t header;

	int tex_w;		/* Texture width */
	int tex_h;		/* Texture height */
	int ofs_x;		/* Glyph X offset factor */
	int ofs_y;		/* Glyph Y offset factor */
	int font_w;		/* Glyph width */
	int font_h;		/* Glyph height */
} gpfont_t;

extern gpfont_t bios_font;

void gpfont_init();
void gpfont_print(gpfont_t *font, int x, int y, char *string);
void gpfont_cprint(gpfont_t *font, int x, int y, char *string);
void gpfont_printf(gpfont_t *font, int x, int y, char *string, ...);
void gpfont_cprintf(gpfont_t *font, int x, int y, char *string, ...);

#endif // _FONT_H_
