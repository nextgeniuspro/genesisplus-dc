
#ifndef _RENDER_H_
#define _RENDER_H_


/* Look-up pixel table information */
#define LUT_MAX         (5)
#define LUT_SIZE        (0x10000)

/* Clip structure */
typedef struct
{
    uint8 left;
    uint8 right;
    uint8 enable;
}clip_t;

/* Function prototypes */
int render_init(void);
void render_reset(void);
void render_shutdown(void);

/* New stuff for hardware renderer */
void xrender_drawframe_op();
void xrender_drawframe();
void xrender_endframe();

#endif /* _RENDER_H_ */

