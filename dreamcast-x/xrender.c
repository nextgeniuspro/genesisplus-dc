/*
        Hardware accelerated renderer for Genesis Plus/DC
	Stuart Dalton, 10th June 2004
*/

#include "shared.h"
#include <dc/pvr.h>

/* PowerVR direct rendering state */
static pvr_dr_state_t dr_state;

/* The texture containing the cached tiles */
static pvr_ptr_t tile_tex;
static uint32 tile_tex_ptr;

/* Depths */
#define DEPTH_FAR_B	10	/* Base depth for far background B */
#define DEPTH_FAR_A	11	/* Base depth for far background A */
#define DEPTH_FAR_W	12	/* Base depth for dodgy window emulation */
#define DEPTH_FAR_S	100	/* Base depth for far sprites */
#define DEPTH_PRIORITY	100	/* Add to base for high priority */

/* Borders */
pvr_poly_hdr_t hdr_border;

#ifdef HIGH_RES
#	define DELTAXY 16
#else
#	define DELTAXY 8
#endif

/*--------------------------------------------------------------------------*/
/* Init, reset, shutdown routines                                           */
/*--------------------------------------------------------------------------*/
int render_init(void)
{
	pvr_poly_cxt_t cxt;

	/* Put the palettes in ARGB1555 mode */
	pvr_set_pal_format(PVR_PAL_ARGB1555);

	/* Create the main tile cache texture */
	tile_tex = pvr_mem_malloc(0x10000);
	tile_tex_ptr = ((uint32)tile_tex & 0x00fffff8) >> 3;

	/* Precompile the border header */
	pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
	cxt.gen.shading = PVR_SHADE_FLAT;
	pvr_poly_compile(&hdr_border, &cxt);

	return 1;
}

void render_reset(void)
{

}

void render_shutdown(void)
{
	pvr_mem_free(tile_tex);
	tile_tex = 0;
	tile_tex_ptr = 0;
}

/*--------------------------------------------------------------------------*/
/* Palette handling garbage                                                 */
/*--------------------------------------------------------------------------*/
void xrender_deal_with_palette()
{
	int i;
	uint16 src;
	uint32 dst;

	for(i = 0; i < 64; i++)
	{
		/* Do we bother cleaning this one? */
		if(!color_dirty[i])
			continue;
		color_dirty[i] = 0;

		/* Decode the colours, and kick them over to the PVR */
		src = ((cram[i<<1])|(cram[(i<<1)+1]<<8));
		dst = ((src&0x007)<<2)|((src&0x038)<<4)|((src&0x1C0)<<6);
		if( (i & 0x0F) != 0 )
			dst |= 0x8000;
		pvr_set_pal_entry(i, dst);
	}

	/* Colour palette clean - byebye! */
	is_color_dirty = 0;
}

#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

/* FIXME - This is enough to drive a perfectly sane person crazy ;-) */
void twiddle_4bit_md(uint8 *src, pvr_ptr_t *dst)
{
	int x, y;
	uint8 * pixels;
	uint16 * vtex;
	pixels = (uint8 *) src;
	vtex = (uint16*)dst;

	for (y=0; y<8; y += 2)
	{
		int yv1 = y<<3;
		int yv2 = yv1+8;
		for (x=0; x<8; x += 2)
		{
			int sx = x ^ 2;
			vtex[TWIDOUT(x>>1, y>>1)] =
				((pixels[(sx+yv1) >>1]&15)<<8) | ((pixels[(sx+yv2) >>1]&15)<<12) |
				((pixels[(sx+yv1) >>1]>>4)   ) | ((pixels[(sx+yv2) >>1]>>4)<<4);
		}
	}
}

void xrender_copy_patterns()
{
	int i;
	for(i = 0; i < bg_list_index; i++)
	{
		int tile = bg_name_list[i];
		bg_name_dirty[tile] = 0;
		tile <<= 5;
		twiddle_4bit_md(vram+tile, tile_tex+tile);
	}
	bg_list_index = 0;
}

#define PT_CMD 0x84840008
#define PT_MODE1 0x92000000
#define PT_MODE2 0x949004C0
#define PT_MODE3 0x28000000

/* Draws a single tile at the specified coordinates */
/* TODO - Speed this up, because it effectively IS the VDP emulator */
void xrender_draw_tile(float x, float y, float zbase, uint16 attr)
{
	pvr_poly_hdr_t *hdr;
	pvr_vertex_t *vert;
	uint32 vertex_colour = 0xffffffff;
	float tc_left, tc_right, tc_top, tc_bum;

	#ifdef HIGH_RES
	x *= 2;
	y *= 2;
	#endif

	/* Construct polygon header
	   This is done manually to avoid calling pvr_poly_compile every frame
	   and so we don't have to cache 32768 of the sodding things */
	hdr = (pvr_poly_hdr_t *)pvr_dr_target(dr_state);
	hdr->cmd = PT_CMD;
	hdr->mode1 = PT_MODE1;

	/* H and V flipping */
	/* Naturally, the flags are in the opposite order to the way we want */
	hdr->mode2 = PT_MODE2;
//	hdr->mode2 |= ((attr & 0x1000) << 5); // V
//	hdr->mode2 |= ((attr & 0x0800) << 7); // H

	/* Colour palette */
	/* TODO - Eliminate some bit shifting */
	hdr->mode3 = PT_MODE3 | PVR_TXRFMT_4BPP_PAL((attr >> 13) & 0x03);

	/* Tile base address */
	hdr->mode3 |= (tile_tex_ptr + ((attr & 0x7ff) << 2));

	/* Fill out junk data, and send to PVR */
	pvr_dr_commit(hdr);

	/* Check priority bit, change paramaters around if necessary */
	if(attr & 0x8000)
	{
		zbase += DEPTH_PRIORITY;
//		vertex_colour = 0xffffffff;
	}

	/* Check for horizontal and vertical flipping */
	if(attr & 0x1000)
	{
		tc_top = 1.0f;
		tc_bum = 0.0f;
	}
	else
	{
		tc_top = 0.0f;
		tc_bum = 1.0f;
	}

	if(attr & 0x0800)
	{
		tc_left = 1.0f;
		tc_right = 0.0f;
	}
	else
	{
		tc_left = 0.0f;
		tc_right = 1.0f;
	}

	/* Send the upper-left vertex */
	vert = pvr_dr_target(dr_state);
	vert->flags = PVR_CMD_VERTEX;
	vert->x = x;
	vert->y = y;
	vert->z = zbase;
	vert->u = tc_left;
	vert->v = tc_top;
	vert->argb = vertex_colour;
	vert->oargb = 0;
	pvr_dr_commit(vert);

	/* Send the upper-right vertex */
	vert = pvr_dr_target(dr_state);
	vert->flags = PVR_CMD_VERTEX;
	vert->x = x+DELTAXY;
	vert->y = y;
	vert->z = zbase;
	vert->u = tc_right;
	vert->v = tc_top;
	vert->argb = vertex_colour;
	vert->oargb = 0;
	pvr_dr_commit(vert);

	/* Send the lower-left vertex */
	vert = pvr_dr_target(dr_state);
	vert->y = y+DELTAXY;
	vert->v = tc_bum;
	pvr_dr_commit(vert);

	/* Send the lower-right vertex */
	vert = pvr_dr_target(dr_state);
	vert->flags = PVR_CMD_VERTEX_EOL;
	vert->y = y+DELTAXY;
	vert->v = tc_bum;
	pvr_dr_commit(vert);

	/* Nothing more to see here ;-) */
}

/* Main sprite rendering loop */
void xrender_drawsprites()
{
	int count;
	int total = (reg[12] & 1) ? 80 : 64;
	uint8 *p, *q, link = 0;

	for(count = 0; count < total; count ++)
	{
		int width, height, ch;
		int x, y, dx, dy, cy;
		uint16 attr;

		/* Set up pointers to the internal SAT and VRAM */
		q = &sat[link << 3];
		p = &vram[satb + (link << 3)];

		/* Grab coordinates and attributes */
		y = *(uint16 *)&q[0] - 120;
		x = *(uint16 *)&p[6] - 128;
		attr = *(uint16 *)&p[4];
		height = q[3] & 3;
		width = ((q[3] & 0x0f) >> 2) & 3;

		/* 32-cell mode */
		if(!(reg[0x0C] & 0x01))
			x += 32;

		/* Work out flipping stuff */
		if(attr & 0x0800)
		{
			/* Draw from right to left */
			x += width<<3;
			dx = -8;
		}
		else
			/* Draw from left to right */
			dx = 8;

		if(attr & 0x1000)
		{
			/* Draw from bottom to top */
			y += height<<3;
			dy = -8;
		}
		else
			/* Draw from top to bottom */
			dy = 8;

		/* Draw tile */
		width++;
		height++;
		while(width--)
		{
			cy = y;
			ch = height;

			while(ch--)
			{
				xrender_draw_tile(x, cy, DEPTH_FAR_S - count, attr++);
				cy += dy;
			}
			x += dx;
		}

		// Follow the link field
		if (!(link = q[2] & 0x7F))
			break;
	}
}

void get_hscroll(int line, uint16 *scrolla, uint16 *scrollb)
{
    switch(reg[11] & 3)
    {
        case 0: /* Full-screen */
            *scrolla = *(uint16 *)&vram[hscb + 0];
            *scrollb = *(uint16 *)&vram[hscb + 2];
            break;

        case 1: /* First 8 lines */
            *scrolla = *(uint16 *)&vram[hscb + ((line & 7) << 2) + 0];
            *scrollb = *(uint16 *)&vram[hscb + ((line & 7) << 2) + 2];
            break;

        case 2: /* Every 8 lines */
            *scrolla = *(uint16 *)&vram[hscb + ((line & ~7) << 2) + 0];
            *scrollb = *(uint16 *)&vram[hscb + ((line & ~7) << 2) + 2];
            break;

        case 3: /* Every line */
            *scrolla = *(uint16 *)&vram[hscb + (line << 2) + 0];
            *scrollb = *(uint16 *)&vram[hscb + (line << 2) + 2];
            break;
    }

    *scrolla &= 0x03FF;
    *scrollb &= 0x03FF;
}

/* FIXME - this ignores almost all video modes, windowing, half the scrolling
   paramaters, and god knows what else... */
void xrender_ntx(int which)
{
	uint16 table;
	int cell_y, pixel_y;
	float depth;
	int ofs_x, end_x;

	/* Name table base address */
	table = (which) ? ntbb : ntab;
	depth = (which) ? DEPTH_FAR_B : DEPTH_FAR_A;

	{
		/* Vertical scrolling paramaters */
		uint16 y_scroll;
    		int vsr_shift = (which) ? 16 : 0;
		uint32 *vs = (uint32 *)&vsram[0];
		y_scroll = (vs[0] >> vsr_shift);

		/* Set cell and pixel coordinates */
		cell_y = ((y_scroll & 0x3F8) >> 3) & playfield_row_mask;
		pixel_y = -(y_scroll & 0x07);
	}

	/* 40- or 32-cell mode? */
	if(reg[0x0C] & 1)
	{
		ofs_x = 0;
		end_x = 320;
	}
	else
	{
		ofs_x = 32;
		end_x = 288;
	}

	while(pixel_y < 224)
	{
		int cell_x, pixel_x;
		uint16 *ptr;

		{
			/* Horizontal scrolling paramaters */
			uint16 x_scroll;
			uint16 xascroll, xbscroll;
			if(pixel_y < 0)
				get_hscroll(0, &xascroll, &xbscroll);
			else
				get_hscroll(pixel_y, &xascroll, &xbscroll);
			x_scroll = (which) ? -xbscroll : -xascroll;

			/* Set cell and pixel coordinates */
			cell_x = ((x_scroll & 0x3F8) >> 3) & playfield_col_mask;
			pixel_x = -(x_scroll & 0x07) + ofs_x;
		}

		ptr = (uint16 *)&vram[table + (cell_y << playfield_shift)];
		while(pixel_x < end_x)
		{
			xrender_draw_tile(pixel_x, pixel_y + 8, depth, ptr[cell_x]);
			cell_x = (cell_x + 1) & playfield_col_mask;
			pixel_x += 8;
		}

		cell_y = (cell_y + 1) & playfield_row_mask;
		pixel_y += 8;
	}
}

/* FIXME: This overlaps plane A, but doesn't overwrite it... */
void xrender_ntw()
{
	uint16 *attr = (uint16 *)&vram[ntwb];

	/* Window positions and flipping */
	int hp = (reg[17] & 0x1F);
	int hf = (reg[17] >> 7) & 1;
	int vp = (reg[18] & 0x1F);
	int vf = (reg[18] >> 7) & 1;

	/* Display size  */
	int sw = (reg[12] & 1) ? 20 : 16;
	int pitch = (reg[12] & 1) ? 64 : 32;

	/* Number of rows to draw, and pixel y position */
	float pixel_y = 8.0f;
	int rows = 0;

	while(rows < 28)
	{
		int n_cols;
		float pixel_x;
		uint16 *battr;

		if(vf == (rows >= vp))
		{
			/* Window takes up entire line */
			n_cols = sw;
			pixel_x = 0;
			battr = attr;
		}
		else
		{
			/* Window does not take entire line */
			if(hf)
			{
				n_cols = sw - hp;
				pixel_x = 8.0 * hp;
				battr += hp;
			}
			else
			{
				n_cols = hp;
				pixel_x = 0;
				battr = attr;
			}

		}

		/* Fix for 32-cell mode */
		if(sw == 16)
			pixel_x = 32;

		while(n_cols--)
		{
			xrender_draw_tile(pixel_x, pixel_y, DEPTH_FAR_W, *battr++);
			pixel_x += 8.0f;
			xrender_draw_tile(pixel_x, pixel_y, DEPTH_FAR_W, *battr++);
			pixel_x += 8.0f;
		}

		/* Next row */
		pixel_y += 8.0f;
		rows++;
		attr += pitch;
	}
}

/*--------------------------------------------------------------------------*/
/* Main frame rendering function                                            */
/*--------------------------------------------------------------------------*/
static void draw_rect(float x1, float y1, float x2, float y2)
{
	pvr_vertex_t vert;
	pvr_prim(&hdr_border, sizeof(hdr_border));

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x1;
	vert.y = y1;
	vert.z = 250;
	vert.argb = 0;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x2;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x1;
	vert.y = y2;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x2;
	pvr_prim(&vert, sizeof(vert));
}

void xrender_drawframe_op()
{
	pvr_list_begin(PVR_LIST_OP_POLY);

	draw_rect(0, 0, 640, 16);
	draw_rect(0, 464, 640, 480);

	if(!(reg[0x0C] & 0x01))
	{
		draw_rect(0, 0, 64, 480);
		draw_rect(576, 0, 640, 480);
	}

	pvr_list_finish();
}

void xrender_drawframe()
{
	/* Deal with changes to the colour palette */
	if(is_color_dirty)
		xrender_deal_with_palette();

	/* Copy pattern data */
	if(bg_list_index)
		xrender_copy_patterns();

	/* Change the background colour */
	/* FIXME - THIS IS REALLY CRAPPY CODE */
	{
		int i;
		uint16 cval;
		uint8 r, g, b;
		i = reg[0x07] & 0x3F;
		cval = ((cram[i<<1])|(cram[(i<<1)+1]<<8));
		r = ((cval & 0x1C0) >> 1);
		g = ((cval & 0x038) << 2);
		b = ((cval & 0x007) << 5);
		pvr_set_bg_color(r/224.0f, g/224.0f, b/224.0f);
	}

	/* Punch-through display list - almost everything goes here */
	pvr_list_begin(PVR_LIST_PT_POLY);

	/* Set up the direct rendering state */
	pvr_dr_init(dr_state);

	/* Only bother if the display is enabled */
	if(reg[0x01] & 0x40)
	{
		/* Draw sprites, plane A, plane B, window */
		xrender_drawsprites();
		xrender_ntx(0);
		xrender_ntx(1);
		xrender_ntw();
	}

	/* All done for the PT display list */
	pvr_list_finish();
}

void xrender_endframe()
{

}
