#ifndef __MAIN_H
#define __MAIN_H

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>

#include <iopcontrol.h>
#include <iopheap.h>
#include <string.h>
#include <loadfile.h>
#include <stdio.h>
#include <sbv_patches.h>
#include <malloc.h>
#include <hdd-ioctl.h>

#include <elf-loader.h>

#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <fileXio_rpc.h>
#include <io_common.h>
#include <iox_stat.h>

#ifdef __EESIO_DEBUG
#include <sio.h>
#define DPRINTF(args...) sio_printf(args)
#define DINIT()          sio_init(38400, 0, 0, 0, 0)
#define LOG(args...)     sio_printf(args)
#else
#define LOG(args...)
#define DPRINTF(args...)
#define DINIT()
#endif

#define PS2PART_IDMAX        32
#define HDL_GAME_NAME_MAX    64
#define HDL_GAME_DATA_OFFSET 0x100000 /* Sector 0x800 in the user data area. */

typedef struct
{
    char partition_name[PS2PART_IDMAX + 1];
    char name[HDL_GAME_NAME_MAX + 1];
    char startup[8 + 1 + 3 + 1];
    u8 hdl_compat_flags;
    u8 ops2l_compat_flags;
    u8 dma_type;
    u8 dma_mode;
    u32 layer_break;
    int disctype;
    u32 start_sector;
    u32 total_size_in_kb;
    u32 magic;
} hdl_game_info_t;

typedef struct // size = 1024
{
    u32 magic; // HDL uses 0xdeadfeed magic here
    u16 reserved;
    u16 version;
    char gamename[160];
    u8 hdl_compat_flags;
    u8 ops2l_compat_flags;
    u8 dma_type;
    u8 dma_mode;
    char startup[60];
    u32 layer1_start;
    u32 discType;
    int num_partitions;
    struct
    {
        u32 part_offset; // in MB
        u32 data_start;  // in sectors
        u32 part_size;   // in KB
    } part_specs[65];
} hdl_apa_header;

#endif
