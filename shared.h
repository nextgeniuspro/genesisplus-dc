#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef _arch_dreamcast
#include <zlib/zlib.h>
#else
#include <zlib.h>
#endif

#include "types.h"
#include "macros.h"
#include "cpu68k.h"
#include "m68k.h"
#include "z80.h"
#include "genesis.h"
#ifdef XVDP
#include "xvdp.h"
#include "xrender.h"
#else
#include "vdp.h"
#include "render.h"
#endif
#include "mem68k.h"
#include "memz80.h"
#include "membnk.h"
#include "memvdp.h"
#include "system.h"
#include "unzip.h"
#include "fileio.h"
#include "loadrom.h"
#include "io.h"
#include "sound.h"

#ifdef GENS_FM
#	include "gens_ym2612.h"
#else
#	include "fm.h"
#endif

#include "sn76496.h"
#include "osd.h"

#endif /* _SHARED_H_ */

