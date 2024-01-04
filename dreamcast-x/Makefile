# Genesis Plus - Sega Mega Drive emulator
# (c) 1999, 2000, 2001, 2002, 2003  Charles MacDonald
# Sega Dreamcast port (c) 2004 Stuart Dalton
# Experimental (hardware accelerated) version

TARGET = genplus-x.elf
BIN = genplus-x.bin
FIRSTREAD = 1st_read.bin

OBJS = genesis.o xvdp.o xrender.o system.o unzip.o fileio.o loadrom.o io.o mem68k.o memz80.o membnk.o memvdp.o
OBJS += sound.o fm.o sn76496.o
OBJS += error.o font.o main.o menu.o snd_stream.o
PROBJS = z80.o
PROBJS += m68kcpu.o m68kops.o m68kopac.o m68kopdm.o m68kopnz.o
PROBJS += c68k.o c68kexec.o cpu68k.o
GSRC = ../m68k/m68kops.c ../m68k/m68kopac.c ../m68k/m68kopdm.c ../m68k/m68kopnz.c

CDRECORD_ID = ATAPI:0,0,0
CDRECORD_DEV = /dev/hdc

all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

KOS_LOCAL_CFLAGS = -Os -I. -I.. -I../cpu -I../sound -I../c68k -I ../m68k -DLSB_FIRST -DALIGN_LONG -DXVDP -DHIGH_RES

clean:
	rm -f $(OBJS) $(TARGET) $(BIN) $(FIRSTREAD)

rclean:
	rm -f $(OBJS) $(PROBJS) $(TARGET) $(BIN) $(FIRSTREAD) $(GSRC) ../m68k/m68kmake

rm-elf:
	-rm -f $(TARGET) $(BIN) $(FIRSTREAD)

$(TARGET): $(OBJS) $(PROBJS) $(GSRC) ../m68k/m68kmake
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(OBJS) $(PROBJS) -L$(KOS_BASE)/lib -lm -lz  $(KOS_LIBS)

%.o: %.c
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) -D_arch_$(KOS_ARCH) $(KOS_ALL_INCS) -c $< -o $@

%.o: ../%.c
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) -D_arch_$(KOS_ARCH) $(KOS_ALL_INCS) -c $< -o $@

%.o: ../c68k/%.c
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) -D_arch_$(KOS_ARCH) $(KOS_ALL_INCS) -c $< -o $@

%.o: ../cpu/%.c
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) -D_arch_$(KOS_ARCH) $(KOS_ALL_INCS) -c $< -o $@

%.o: ../sound/%.c
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) -D_arch_$(KOS_ARCH) $(KOS_ALL_INCS) -c $< -o $@

%.o: ../m68k/%.c
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) -D_arch_$(KOS_ARCH) $(KOS_ALL_INCS) -DGENESIS_HACKS="1" -c $< -o $@

$(GSRC): ../m68k/m68kmake
	cd ../m68k && ./m68kmake

../m68k/m68kmake: ../m68k/m68kmake.c
	gcc ../m68k/m68kmake.c -o ../m68k/m68kmake

$(BIN): $(TARGET)
	$(KOS_STRIP) $(TARGET)
	kos-objcopy -O binary -R .stack $(TARGET) $(BIN)

$(FIRSTREAD): $(BIN)
	../utils/scramble $(BIN) $(FIRSTREAD)

dist: $(TARGET) $(BIN) $(FIRSTREAD)

selfboot: $(FIRSTREAD)
	rm -f disc/session1.iso
	mkisofs -C 0,11702 -G disc/IP.bin -joliet -rock -l -x .DS_Store -o disc/session1.iso $(FIRSTREAD) disc/data
	rm -f disc/disc.cdi
	../utils/cdi4dc disc/session1.iso disc/disc.cdi
