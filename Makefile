.SILENT:

FRONTEND_OBJS = obj/main.o

EECORE_OBJS =	obj/ps2dev9.o obj/ps2atad.o obj/ps2fs.o obj/ps2hdd.o \
		obj/iomanx.o obj/filexio.o

EE_BIN = OPL-Launcher.elf
EE_SRC_DIR = src/
EE_OBJS_DIR = obj/
EE_ASM_DIR = asm/
EE_OBJS = $(FRONTEND_OBJS) $(EECORE_OBJS)

EE_LIBS = -lfileXio -lpatches -lelf-loader-nocolour
EE_LIB_DIRS += -L$(PS2SDK)/ee/lib
EE_INCS += -I$(PS2SDK)/ports/include
EE_CFLAGS := -O2 -G8192 -mgpopt -Wno-stringop-truncation

all:
	@mkdir -p $(EE_OBJS_DIR)
	@mkdir -p $(EE_ASM_DIR)

	echo "Building OPL Launcher..."
	$(MAKE) $(EE_BIN)

clean:
	rm -f -r $(EE_OBJS_DIR) $(EE_ASM_DIR) $(EE_BIN)

rebuild: clean all

ps2dev9.s:
	bin2s $(PS2SDK)/iop/irx/ps2dev9.irx asm/ps2dev9.s ps2dev9_irx

ps2atad.s:
	bin2s $(PS2SDK)/iop/irx/ps2atad.irx asm/ps2atad.s ps2atad_irx

ps2fs.s:
	bin2s $(PS2SDK)/iop/irx/ps2fs.irx asm/ps2fs.s ps2fs_irx

ps2hdd.s:
	bin2s $(PS2SDK)/iop/irx/ps2hdd-osd.irx asm/ps2hdd.s ps2hdd_irx

iomanx.s:
	bin2s $(PS2SDK)/iop/irx/iomanX.irx asm/iomanx.s iomanx_irx

filexio.s:
	bin2s $(PS2SDK)/iop/irx/fileXio.irx asm/filexio.s filexio_irx

$(EE_OBJS_DIR)%.o : $(EE_SRC_DIR)%.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

$(EE_OBJS_DIR)%.o : %.s
	$(EE_AS) $(EE_ASFLAGS) $(EE_ASM_DIR)$< -o $@

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
