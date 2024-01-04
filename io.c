/*
    io.c
    I/O controller chip emulation
*/

#include "shared.h"

port_t port[3];
uint8 io_reg[0x10];

/* SD - Reset the device stuff */
extern void io_rescan()
{
    int i;

    /* SD - Use the devices properly */
    for(i = 0; i < 3; i++)
    {
    	switch(input.dev[i])
	{
		case DEVICE_NONE:
			/* Nothing */
			port[i].data_w = NULL;
			port[i].data_r = NULL;
			break;
		case DEVICE_2BUTTON:
			/* 2-button / SMS pad */
			port[i].data_w = NULL;
			port[i].data_r = pad_2b_r;
			break;
		case DEVICE_3BUTTON:
			/* 3-button Genesis pad */
			port[i].data_w = device_3b_w;
			port[i].data_r = device_3b_r;
			break;
		case DEVICE_6BUTTON:
			/* TODO - 6-button Genesis pad */
			port[i].data_w = NULL;
			port[i].data_r = NULL;
			break;
	}
    }
}

void io_reset(void)
{
    /* I/O register default settings */
    uint8 io_def[0x10] =
    {
        0xA0,
        0x7F, 0x7F, 0x7F,
        0x00, 0x00, 0x00, 
        0xFF, 0x00, 0x00,
        0xFF, 0x00, 0x00,  
        0xFB, 0x00, 0x00,  
    };

    /* Initialize I/O registers */
    memcpy(io_reg, io_def, 0x10);

    /* SD - Re-scan I/O ports */
    io_rescan();
}

/*--------------------------------------------------------------------------*/
/* I/O chip functions                                                       */
/*--------------------------------------------------------------------------*/

void gen_io_w(int offset, int value)
{
    switch(offset)
    {
        case 0x01: /* Port A Data */
            value = ((value & 0x80) | (value & io_reg[offset+3]));
            io_reg[offset] = value;
            if(port[0].data_w) port[0].data_w(0, value);
            return;

        case 0x02: /* Port B Data */
            value = ((value & 0x80) | (value & io_reg[offset+3]));
            io_reg[offset] = value;
            if(port[1].data_w) port[1].data_w(1, value);
            return;

        case 0x03: /* Port C Data */
            value = ((value & 0x80) | (value & io_reg[offset+3]));
            io_reg[offset] = value;
            if(port[2].data_w) port[2].data_w(2, value);
            return;  

        case 0x04: /* Port A Ctrl */
        case 0x05: /* Port B Ctrl */
        case 0x06: /* Port C Ctrl */
            io_reg[offset] = value & 0xFF;
            break;

        case 0x07: /* Port A TxData */
        case 0x0A: /* Port B TxData */
        case 0x0D: /* Port C TxData */
            io_reg[offset] = value;
            break;

        case 0x09: /* Port A S-Ctrl */
        case 0x0C: /* Port B S-Ctrl */
        case 0x0F: /* Port C S-Ctrl */
            io_reg[offset] = (value & 0xF8);
            break;
    }
}

#ifdef MODE_PAL /* Europe, USA, Japan, Default (USA) */
static uint8 mode_priorities[4] = {0xC0, 0x80, 0x00, 0x80};
#elif defined MODE_JAP /* Japan, USA, Europe, Default (USA) */
static uint8 mode_priorities[4] = {0x00, 0x80, 0xC0, 0x80};
#else /* USA, Europe, Japan, Default (USA) */
static uint8 mode_priorities[4] = {0x80, 0xC0, 0x00, 0x80};
#endif

int gen_io_r(int offset)
{
    int findit;
    int mode_ofs;
    uint8 prev_best = 3;
    uint8 temp;
    uint8 has_scd = 0x20; /* No Sega CD unit attached */
    uint8 gen_ver = 0x00; /* Version 0 hardware */

    switch(offset)
    {
        case 0x00: /* Version */
	    for(mode_ofs = 0; mode_ofs < 3; mode_ofs++)
	    {
		switch(READ_BYTE(cart_rom, 0x0001F0 + mode_ofs))
		{
			case 'J':
				temp = 0x00;
				break;
			case 'U':
				temp = 0x80;
				break;
			case 'E':
			case 'A':
			case 'B':
			case '4':
				temp = 0xC0;
				break;
			default:
				temp = 0x80;
				break;
		}
		for(findit = 0; findit < 3; findit++)
		{
		    if(mode_priorities[findit] == temp)
		    {
		        if(findit < prev_best)
			{
			    prev_best = findit;
          #if !defined(_arch_dreamcast)
			    printf("Mode read - Testing 0x%02X (priority %d)\n", mode_priorities[prev_best], prev_best);
          #endif
			}
		    }
		}
	    }
      #if !defined(_arch_dreamcast)
	    printf("Mode read - Priority %d (0x%02X)\n", prev_best, mode_priorities[prev_best]);
      #endif
            return (mode_priorities[prev_best] | has_scd | gen_ver);
            break;

        case 0x01: /* Port A Data */
            if(port[0].data_r) return ((io_reg[offset] & 0x80) | port[0].data_r(0));
            return (io_reg[offset] | ((~io_reg[offset+3]) & 0x7F));

        case 0x02: /* Port B Data */
            if(port[1].data_r) return ((io_reg[offset] & 0x80) | port[1].data_r(1));
            return (io_reg[offset] | ((~io_reg[offset+3]) & 0x7F));

        case 0x03: /* Port C Data */
            if(port[2].data_r) return ((io_reg[offset] & 0x80) | port[2].data_r(2));
            return (io_reg[offset] | ((~io_reg[offset+3]) & 0x7F));
    }

    return (io_reg[offset]);
}

/*--------------------------------------------------------------------------*/
/* Input callbacks                                                          */
/*--------------------------------------------------------------------------*/

uint8 pad_2b_r(int pad)
{
    uint8 temp = 0x3F;
    if(input.pad[pad] & INPUT_UP)    temp &= ~0x01;
    if(input.pad[pad] & INPUT_DOWN)  temp &= ~0x02;
    if(input.pad[pad] & INPUT_LEFT)  temp &= ~0x04;
    if(input.pad[pad] & INPUT_RIGHT) temp &= ~0x08;
    if(input.pad[pad] & INPUT_B)     temp &= ~0x10;
    if(input.pad[pad] & INPUT_C)     temp &= ~0x20;
    return (temp);
}

static int pad_latches[3] = {0,0,0};

uint8 device_3b_r(int pad)
{
    uint8 temp = 0x3F;

    if(pad_latches[pad])
    {
        temp = 0x3f;
        if(input.pad[pad] & INPUT_UP)    temp &= ~0x01;
        if(input.pad[pad] & INPUT_DOWN)  temp &= ~0x02;
        if(input.pad[pad] & INPUT_LEFT)  temp &= ~0x04;
        if(input.pad[pad] & INPUT_RIGHT) temp &= ~0x08;
        if(input.pad[pad] & INPUT_B)     temp &= ~0x10;
        if(input.pad[pad] & INPUT_C)     temp &= ~0x20;
        return (temp | 0x40);
    }
    else
    {
        temp = 0x33;
        if(input.pad[pad] & INPUT_UP)    temp &= ~0x01;
        if(input.pad[pad] & INPUT_DOWN)  temp &= ~0x02;
        if(input.pad[pad] & INPUT_A)     temp &= ~0x10;
        if(input.pad[pad] & INPUT_START) temp &= ~0x20;
        return (temp);
    }
}

void device_3b_w(int pad, uint8 data)
{
    pad_latches[pad] = (data & 0x40);
}
