# OPL-Launcher

[![CI](https://github.com/ps2homebrew/OPL-Launcher/workflows/CI/badge.svg)](https://github.com/ps2homebrew/OPL-Launcher/actions?query=workflow%3ACI)

OPL-Launcher reads `hdd0:/__common/OPL/conf_hdd.cfg` to launch `$OPL/OPNPS2LD.ELF`.

You can inject OPL-Launcher into APA using i.e HDL Dump:
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
3.  Standrd PS2 game icon. Just take one from game save and rename it to `list.ico`.

4.  `icon.sys` file, which is not binary like in Memory Card case but fully text one (replace title0/1 by target game):
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
## Credits

modified from miniOPL/diskload, credit to sp193 & l_oliveira
