#include <kos.h>
#include "font.h"

gpfont_t bios_font;

/* Initialise the font system */
void gpfont_init()
{
	pvr_poly_cxt_t cxt;
	uint16 *vram;
	int x, y;

	bios_font.tex_w = 256;
	bios_font.tex_h = 256;
	bios_font.ofs_x = 16;
	bios_font.ofs_y = 24;
	bios_font.font_w = 12;
	bios_font.font_h = 24;

	bios_font.texture = pvr_mem_malloc(256*256*2);
	vram = (uint16 *)bios_font.texture;

	/* Generate texture from the BIOS font */
	for (y=0; y<8; y++)
	{
		for (x=0; x<16; x++)
		{
			if (x != 0 || y != 0)
				bfont_draw(vram, 256, 0, y*16+x);
			vram += 16;
		}
		vram += 23*256;
	}

	/* Set up a polygon header */
	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_NONTWIDDLED,
		256, 256, bios_font.texture, PVR_FILTER_NONE);
	pvr_poly_compile(&bios_font.header, &cxt);
}

void gpfont_print(gpfont_t *font, int x, int y, char *string)
{
	int ix, iy;
	float u1, u2, v1, v2;
	pvr_vertex_t	vert;

	/* Send the font's polygon header */
	pvr_prim(&font->header, sizeof(font->header));
	vert.argb = PVR_PACK_COLOR(1.0f, 1.0f, 1.0f, 1.0f);
	vert.oargb = 0;

	while(*string)
	{
		/* Generate texture coords */
		ix = (*string % 16) * font->ofs_x;
		iy = (*string / 16) * font->ofs_y;
		u1 = ix * 1.0f / font->tex_w;
		v1 = iy * 1.0f / font->tex_h;
		u2 = (ix+font->font_w) * 1.0f / font->tex_w;
		v2 = (iy+font->font_h) * 1.0f / font->tex_h;

		/* Send the vertices */
		vert.flags = PVR_CMD_VERTEX;
		vert.x = x;
		vert.y = y + 24.0f;
		vert.z = 300.0f;
		vert.u = u1;
		vert.v = v2;
		pvr_prim(&vert, sizeof(vert));

		vert.x = x;
		vert.y = y;
		vert.u = u1;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));

		vert.x = x + 12.0f;
		vert.y = y + 24.0f;
		vert.u = u2;
		vert.v = v2;
		pvr_prim(&vert, sizeof(vert));

		vert.flags = PVR_CMD_VERTEX_EOL;
		vert.x = x + 12.0f;
		vert.y = y;
		vert.u = u2;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));

		/* Next character */
		x += font->font_w;
		string++;
	}
}

void gpfont_cprint(gpfont_t *font, int x, int y, char *string)
{
	x -= strlen(string) * (font->font_w / 2);
	gpfont_print(font, x, y, string);
}

void gpfont_printf(gpfont_t *font, int x, int y, char *string, ...)
{
	va_list argptr;
	char buffer[256];

	va_start(argptr, string);
	vsprintf(buffer, string, argptr);
	va_end(argptr);

	gpfont_print(font, x, y, buffer);
}

void gpfont_cprintf(gpfont_t *font, int x, int y, char *string, ...)
{
	va_list argptr;
	char buffer[256];

	va_start(argptr, string);
	vsprintf(buffer, string, argptr);
	va_end(argptr);

	x -= strlen(string) * (font->font_w / 2);
	gpfont_print(font, x, y, buffer);
}
