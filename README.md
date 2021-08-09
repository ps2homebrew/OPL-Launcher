# OPL-Launcher

[![CI](https://github.com/ps2homebrew/OPL-Launcher/workflows/CI/badge.svg)](https://github.com/ps2homebrew/OPL-Launcher/actions?query=workflow%3ACI)

OPL-Launcher reads `hdd0:/__common/OPL/conf_hdd.cfg` to launch `$OPL/OPNPS2LD.ELF`.

You can inject OPL-Launcher into APA using, i.e. [HDL Dump](https://github.com/ps2homebrew/hdl-dump):

```cmd
hdl_dump.exe modify_header hdd<Disk Number>: <PP. Partition with PS2 game>
```

To do so, You must also prepare few files for the injection process:

1.  Signed executable which You can make by [this application](https://www.psx-place.com/resources/kelftool-fmcb-compatible-fork.1104/)

```cmd
        kelftool encrypt mbr OPL-Launcher.elf boot.kelf
```

2.  `system.cnf` file that contains:

```ini
        BOOT2 = PATINFO
        VER = 1.00
        VMODE = NTSC
        HDDUNITPOWER = NICHDD
```

3.  Standard PS2 game icon. Just take one from game save and rename it to `list.ico`.

4.  `icon.sys` file, which is not binary like in Memory Card case but fully text one. Example file, `title0` replaced by game name, `title1` replaced by Game ID and region (information from <http://redump.org/>):

```ini
        PS2X
        title0=ICO
        title1=SCUS-97113 (NTSC-U)
        bgcola=0
        bgcol0=0,0,0
        bgcol1=0,0,0
        bgcol2=0,0,0
        bgcol3=0,0,0
        lightdir0=1.0,-1.0,1.0
        lightdir1=-1.0,1.0,-1.0
        lightdir2=0.0,0.0,0.0
        lightcolamb=64,64,64
        lightcol0=64,64,64
        lightcol1=16,16,16
        lightcol2=0,0,0
        uninstallmes0=
        uninstallmes1=
        uninstallmes2=
```

5.  You can use a combination of 2 partitions: `<PP. Partition with PS2 game resources>` and `<__. Partition with PS2 game>`. Partition names should only differ in the first two characters.
    This will add compatibility with BB Navigator / PSX DESR XMB users. `<PP. Partition with PS2 game resources>` will contain game resources, `<__. Partition with PS2 game>` is the game in HDL format.

<details>
        <summary> <b> Detailed guide for installing on PSX DESR / BB Navigator </b> </summary>
<p>

1.  Install the game with [HDL Dump](https://github.com/ps2homebrew/hdl-dump) by using the switch `-hide`. If you already installed the game without that switch, rename the partition and change the first three characters to `__.`.

2.  Create PFS partition `<PP. Partition with PS2 game resources>`. The name should match the installed game name in the previous step, except that the first three characters should be `PP.`.

3.  Prepare signed executable (for example, by [this app](https://www.psx-place.com/resources/kelftool-fmcb-compatible-fork.1104/))

```cmd
        kelftool encrypt mbr OPL-Launcher.elf boot.kelf
```

4.  Put signed executable into partition from step 2. For example, you can place it in `<PP. Partition with PS2 game resources>/EXECUTE.KELF`

5.  Create text file `system.cnf`, BOOT2 should be the same path as in step 4, only replace `<PP. Partition with PS2 game resources>` with `pfs:`:

```ini
        BOOT2 = pfs:/EXECUTE.KELF
        VER = 1.00
        VMODE = NTSC
        HDDUNITPOWER = NICHDD
```

6.  `list.ico` and `icon.sys` are the same as in the first part of this Readme. These files are not used in BB Navigator / PSX DESR XMB, but you can keep them if you plan to use HDD OSD.

7.  You can now inject all files into PP. with that command:

```cmd
        hdl_dump.exe modify_header hdd<Disk Number>: <PP. Partition with PS2 game>
```

8.  Create folder `res` in `<PP. Partition with PS2 game resources>`. This folder will contain all resources.

9.  Create inside `res` folder text file `info.sys`. Example file, `title` replaced by game name, `title_id` replaced by Game ID and region (information from <http://redump.org/>)

```ini
        title = ICO
        title_id = SCUS-97113 (NTSC-U)
        title_sub_id = 0
        release_date =
        developer_id =
        publisher_id =
        note =
        content_web =
        image_topviewflag = 0
        image_type = 0
        image_count = 1
        image_viewsec = 600
        copyright_viewflag = 0
        copyright_imgcount = 0
        genre =
        parental_lock = 1
        effective_date = 0
        expire_date = 0
        area = J
        violence_flag = 0
        content_type = 255
        content_subtype = 0
```

10. Place `jkt_001.png` and `jkt_002.png` in `res` folder. These two pictures will be used as mini-thumbnails for the game. It can be the same picture as used in OPL art. `jkt_001.png` used in BB Navigator, `jkt_002.png` used in PSX XMB.

</p>
</details>

## Credits

Modified from miniOPL/diskload, credit to [sp193](https://github.com/sp193) & [l_oliveira](https://github.com/7l-oliveira)
