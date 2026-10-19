# WIP EDK2 UEFI Implementation for LG G4, LG Nexus 5X, and Huawei Nexus 6P (based on [Lumia950XLPkg](https://github.com/WOA-Project/Lumia950XLPkg))

## Support Status
 - EDK2 boots fully!
 - Display via framebuffer works (1440x2560 QHD on LG G4 & Nexus 6P, 1080x1920 FHD on Nexus 5X)
 - Keypad navigation (Power / Volume buttons)

## Run

Stock bootloader on LG G4 and Nexus 5X is aarch64, so it's possible to test boot via Fastboot:
```bash
fastboot boot uefi_lgg4.img
```
(Or use `uefi_lgg4_2k.img` if using a 2048-byte page size bootloader / recovery environment.)

This implementation reuses PrePi from [SurfaceDuoPkg](https://github.com/WOA-Project/SurfaceDuoPkg) and `BootShim`. The reason for it is that
the stock bootloader loads UEFI at the kernel load address, which isn't suitable. `BootShim` relocates
the payload to the designated UEFI base address (`0x00200000`).

## Compile

Build for LG G4:
```bash
./build.sh -d LGG4
```

Or for release mode:
```bash
./build.sh -d LGG4 --release
```

Last known good edk2 commit is 838c730fe61578847823da1b815e4c9c5ee1bed4 (thx Gus for pinning).

## Windows Notes

Windows boots fine (although with only emmc and display via framebuffer working for now on the 5X/G4, Nexus 6P loads more drivers).

## Linux notes

Mainline linux boots with framebuffer working via efifb.

## Framebuffer notes

The bootloader on Nexus 5X/6P disables refresh, so a function to enable it again had to be implemented

## WSL Build Notes

If you checked out the EDK2 repository under Windows and build it using WSL, you will have trouble locating BaseTools Python classes due to [file case sensitive behavior changes](https://blogs.msdn.microsoft.com/commandline/2018/02/28/per-directory-case-sensitivity-and-wsl/) in WSL. Run the following PowerShell script under EDK2 directory prior to build:

	Get-ChildItem .\BaseTools\ -Directory -Recurse | Foreach-Object { fsutil.exe file setCaseSensitiveInfo $_.FullName }

You only need to run it once.

## Acknowledgements

- [WOA-Project](https://github.com/WOA-Project) for creating Lumia950XLPkg
- Gustave Monce for all the explanation and providing ACPI samples
- Minecrell for explaining how the lk2nd framebuffer refresh code worked
- Imbushuo for all the blog posts about running Windows on ARM 
- [EFIDroid Project](http://efidroid.org)
- Andrei Warkentin and his [RaspberryPiPkg](https://github.com/andreiw/RaspberryPiPkg)
- Sarah Purohit
- [Googulator](https://github.com/Googulator/)

## License

All code except drivers in `GPLDriver` and DT in device-specific directories
are licensed under BSD 2-Clause. GPL Drivers are licensed under GPLv2 license.
