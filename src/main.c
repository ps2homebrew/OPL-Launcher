#include "include/main.h"

extern unsigned char ps2dev9_irx[];
extern unsigned int size_ps2dev9_irx;

extern unsigned char ps2atad_irx[];
extern unsigned int size_ps2atad_irx;

extern unsigned char ps2hdd_irx[];
extern unsigned int size_ps2hdd_irx;

extern unsigned char iomanx_irx[];
extern unsigned int size_iomanx_irx;

extern unsigned char filexio_irx[];
extern unsigned int size_filexio_irx;

extern unsigned char ps2fs_irx[];
extern unsigned int size_ps2fs_irx;

static unsigned char IOBuffer[1024] ALIGNED(64);

struct GameDataEntry
{
    u32 lba, size;
    char id[PS2PART_IDMAX];
};

static int hddReadSectors(u32 lba, u32 nsectors, void *buf)
{
    hddAtaTransfer_t *args = (hddAtaTransfer_t *)IOBuffer;

    args->lba = lba;
    args->size = nsectors;

    if (fileXioDevctl("hdd0:", HDIOC_READSECTOR, args, sizeof(hddAtaTransfer_t), buf, nsectors * 512) != 0)
        return -1;

    return 0;
}

static int hddReadHDLGameInfo(struct GameDataEntry *game, hdl_game_info_t *ginfo)
{
    int ret;

    ret = hddReadSectors(game->lba, 2, IOBuffer);
    if (ret == 0) {
        hdl_apa_header *hdl_header = (hdl_apa_header *)IOBuffer;

        strncpy(ginfo->partition_name, game->id, PS2PART_IDMAX);
        ginfo->partition_name[PS2PART_IDMAX] = '\0';
        strncpy(ginfo->name, hdl_header->gamename, HDL_GAME_NAME_MAX);
        strncpy(ginfo->startup, hdl_header->startup, sizeof(ginfo->startup) - 1);
        ginfo->hdl_compat_flags = hdl_header->hdl_compat_flags;
        ginfo->ops2l_compat_flags = hdl_header->ops2l_compat_flags;
        ginfo->dma_type = hdl_header->dma_type;
        ginfo->dma_mode = hdl_header->dma_mode;
        ginfo->layer_break = hdl_header->layer1_start;
        ginfo->disctype = (u8)hdl_header->discType;
        ginfo->start_sector = game->lba;
        ginfo->total_size_in_kb = game->size * 2; // size * 2048 / 1024 = 2x
    } else
        ret = -1;

    return ret;
}

static int hddGetHDLGameInfo(const char *partition, hdl_game_info_t *ginfo)
{
    int fd, ret = 0;

    if ((fd = fileXioDopen("hdd0:")) >= 0) {
        struct GameDataEntry *pGameEntry = NULL;
        iox_dirent_t dirent;

        while (fileXioDread(fd, &dirent) > 0) {
            if (strcmp(dirent.name, partition))
                continue;

            if (dirent.stat.mode == HDL_FS_MAGIC) {
                pGameEntry = malloc(sizeof(struct GameDataEntry));

                if (pGameEntry == NULL)
                    return -1;

                strncpy(pGameEntry->id, dirent.name, APA_IDMAX);
                pGameEntry->size = 0;
                pGameEntry->lba = 0;

                if (!(dirent.stat.attr & APA_FLAG_SUB)) {
                    // Note: The APA specification states that there is a 4KB area used for storing the partition's information, before the extended attribute area.
                    pGameEntry->lba = dirent.stat.private_5 + (HDL_GAME_DATA_OFFSET + 4096) / 512;
                }

                pGameEntry->size += (dirent.stat.size / 4); // size in HDD sectors * (512 / 2048) = 0.25x
            }

            break;
        }

        fileXioDclose(fd);

        ret = hddReadHDLGameInfo(pGameEntry, ginfo);
        free(pGameEntry);
    } else
        ret = fd;

    return ret;
}

static inline const char *GetMountParams(const char *command, char *BlockDevice)
{
    const char *MountPath;
    int BlockDeviceNameLen;

    if ((MountPath = strchr(&command[5], ':')) != NULL) {
        BlockDeviceNameLen = (unsigned int)MountPath - (unsigned int)command;
        strncpy(BlockDevice, command, BlockDeviceNameLen);
        BlockDevice[BlockDeviceNameLen] = '\0';

        MountPath++; //This is the location of the mount path;
    }

    return MountPath;
}

int main(int argc, char *argv[])
{
    char PartitionName[33], BlockDevice[38];
    int result;
    hdl_game_info_t GameInfo;

    SifInitRpc(0);

    /* Do as many things as possible while the IOP slowly resets itself. */
    if (argc == 2) {
        /* Argument 1 will contain the name of the partition containing the game. */
        /* Unfortunately, it'll mean that some homebrew loader was most likely used to launch this program... and it might already have IOMANX loaded. That thing can't register devices that are added via IOMAN after it gets loaded.
		Reset the IOP to clear out all these stupid homebrew modules... */
        while (!SifIopReset(NULL, 0)) {};

        if (strlen(argv[1]) <= 32) {
            strncpy(PartitionName, argv[1], sizeof(PartitionName) - 1);
            PartitionName[sizeof(PartitionName) - 1] = '\0';
            result = 0;
        } else
            result = -1;

        while (!SifIopSync()) {};

        SifInitRpc(0);

        if (result < 0)
            goto BootError;
    } else {
        if (GetMountParams(argv[0], BlockDevice) != NULL) {
            strncpy(PartitionName, &BlockDevice[5], sizeof(PartitionName) - 1);
            PartitionName[sizeof(PartitionName) - 1] = '\0';
        } else
            goto BootError;
    }

    SifInitIopHeap();
    SifLoadFileInit();

    sbv_patch_enable_lmb();

    SifExecModuleBuffer(ps2dev9_irx, size_ps2dev9_irx, 0, NULL, NULL);

    SifExecModuleBuffer(iomanx_irx, size_iomanx_irx, 0, NULL, NULL);
    SifExecModuleBuffer(filexio_irx, size_filexio_irx, 0, NULL, NULL);

    fileXioInit();

    SifExecModuleBuffer(ps2atad_irx, size_ps2atad_irx, 0, NULL, NULL);
    SifExecModuleBuffer(ps2hdd_irx, size_ps2hdd_irx, 0, NULL, NULL);
    SifExecModuleBuffer(ps2fs_irx, size_ps2fs_irx, 0, NULL, NULL);

    SifLoadFileExit();
    SifExitIopHeap();

    DPRINTF("Retrieving game information...\n");

    result = hddGetHDLGameInfo(PartitionName, &GameInfo);
    if (result < 0) {
        // some users use PP.<Partition> for storing game icons and settings
        // for example BB.Navigator users and PSX DESR 1st gen (which just doesnt support PATINFO)
        // they store game inside hidden partition: __.<Partition>
        PartitionName[0] = '_';
        PartitionName[1] = '_';
        result = hddGetHDLGameInfo(PartitionName, &GameInfo);
    }

    if (result >= 0) {
        char name[128];
        char oplPartition[256];
        char oplFilePath[256];
        char *prefix = "pfs0:";

        fileXioUmount("pfs0:");

        result = fileXioMount("pfs0:", "hdd0:__common", FIO_MT_RDWR);
        if (result == 0) {
            FILE *fd = fopen("pfs0:OPL/conf_hdd.cfg", "rb");
            if (fd != NULL) {
                char *val;
                char line[128];
                if (fgets(line, 128, fd) != NULL) {
                    if ((val = strchr(line, '=')) != NULL)
                        val++;

                    sprintf(name, val);
                    // OPL adds windows CR+LF (0x0D 0x0A) .. remove that shiz from the string.. second check is 'just in case'
                    if ((val = strchr(name, '\r')) != NULL)
                        *val = '\0';

                    if ((val = strchr(name, '\n')) != NULL)
                        *val = '\0';
                }
                fclose(fd);

            } else
                sprintf(name, "+OPL");

            fileXioUmount("pfs0:");
        } else
            sprintf(name, "+OPL");

        snprintf(oplPartition, sizeof(oplPartition), "hdd0:%s", name);

        if (oplPartition[5] != '+')
            prefix = "pfs0:OPL/";

        sprintf(oplFilePath, "%sOPNPS2LD.ELF", prefix);

        result = fileXioMount("pfs0:", oplPartition, FIO_MT_RDWR);
        if (result == 0) {
            char *boot_argv[4];
            char start[128];

            boot_argv[0] = GameInfo.startup;
            sprintf(start, "%u", GameInfo.start_sector);
            boot_argv[1] = start;
            boot_argv[2] = name;
            boot_argv[3] = "mini";

            LoadELFFromFile(oplFilePath, 4, boot_argv); //args will be shifted +1 and oplFilePath will be the new argv0
        }
    }

    DPRINTF("Error loading game: %s, code: %d\n", PartitionName, result);

BootError:
    SifExitRpc();

    char *args[2];
    args[0] = "BootError";
    args[1] = NULL;
    ExecOSD(1, args);

    return 0;
}
