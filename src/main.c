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

static int hddGetHDLGameInfo(const char *partition, hdl_game_info_t *ginfo)
{
    u32 size;
    static char buf[1024] ALIGNED(64);
    int fd, ret;
    iox_stat_t PartStat;
    char *PathToPart;

    PathToPart = malloc(strlen(partition) + 6);
    sprintf(PathToPart, "hdd0:%s", partition);

    if ((fd = open(PathToPart, O_RDONLY, 0666)) >= 0) {
        lseek(fd, HDL_GAME_DATA_OFFSET, SEEK_SET);
        ret = read(fd, buf, 1024);
        close(fd);

        if (ret == 1024) {
            fileXioGetStat(PathToPart, &PartStat);

            hdl_apa_header *hdl_header = (hdl_apa_header *)buf;

            // calculate total size
            size = PartStat.size;

            strncpy(ginfo->partition_name, partition, sizeof(ginfo->partition_name) - 1);
            ginfo->partition_name[sizeof(ginfo->partition_name) - 1] = '\0';
            strncpy(ginfo->name, hdl_header->gamename, sizeof(ginfo->name) - 1);
            ginfo->name[sizeof(ginfo->name) - 1] = '\0';
            strncpy(ginfo->startup, hdl_header->startup, sizeof(ginfo->startup) - 1);
            ginfo->startup[sizeof(ginfo->startup) - 1] = '\0';
            ginfo->hdl_compat_flags = hdl_header->hdl_compat_flags;
            ginfo->ops2l_compat_flags = hdl_header->ops2l_compat_flags;
            ginfo->dma_type = hdl_header->dma_type;
            ginfo->dma_mode = hdl_header->dma_mode;
            ginfo->layer_break = hdl_header->layer1_start;
            ginfo->disctype = hdl_header->discType;
            ginfo->start_sector = PartStat.private_5 + (HDL_GAME_DATA_OFFSET + 4096) / 512; /* Note: The APA specification states that there is a 4KB area used for storing the partition's information, before the extended attribute area. */
            ginfo->total_size_in_kb = size / 2;
        } else
            ret = -1;
    } else
        ret = fd;

    free(PathToPart);

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

static inline void BootError(void)
{
    SifExitRpc();

    char *args[2];
    args[0] = "BootError";
    args[1] = NULL;
    ExecOSD(1, args);
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
        /* Unfortunately, it'll mean that some homebrew loader was most likely used to launch this program... and it might already have IOMANX loaded. That thing can't register devices that are added via IOMAN after it gets loaded. */
        /* Reset the IOP to clear out all these stupid homebrew modules... */
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
            BootError();
    } else {
        if (GetMountParams(argv[0], BlockDevice) != NULL) {
            strncpy(PartitionName, &BlockDevice[5], sizeof(PartitionName) - 1);
            PartitionName[sizeof(PartitionName) - 1] = '\0';
        } else
            BootError();
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
                } else
                    sprintf(name, "+OPL");

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

    BootError();

    return 0;
}
