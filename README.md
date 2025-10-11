# isodrive (configfs)

## Building

* `sudo apt install build-essential`

* `git clone https://github.com/kelexine/isodrive`

* `cd isodrive`

* `make`

* `sudo make install` (optional)

## Usage
```bash
Usage:
isodrive [FILE]... [OPTION]...
Mounts the given FILE as a bootable device using configfs.
Run without any arguments to unmount any mounted files and display this help message.

Optional arguments:
-rw		Mounts the file in read write mode.
-cdrom		Mounts the file as a cdrom.
-windows	Enables Windows ISO mode (auto-enables CD-ROM, read-only, and Windows-compatible USB descriptors).
-configfs	Forces the app to use configfs.
-usbgadget	Forces the app to use sysfs.
```

### Examples

mount iso as rw
```bash
isodrive /path/to/file.iso -rw
```

mount iso as cdrom
```bash
isodrive /path/to/file.iso -cdrom
```

mount Windows ISO (with Windows-compatible USB descriptors)
```bash
isodrive /path/to/Windows10.iso -windows
```

## Linux
* Has been only tested on Halium based mobile linux, but should work on mainline devices too.

## Android

* On Android can be compiled in termux, using clang++
* On Android you might manually need to mount configfs by running: `mount -t configfs configfs /sys/kernel/config`
* A magisk module is also available for download [HERE](https://github.com/nitanmarcel/isodrive-magisk/releases/latest)

## Os Support
* Should support almost every bootable OS images, but for those who don't work or need extra steps, are documented in the [WIKI](https://github.com/nitanmarcel/isodrive/wiki)

## Windows ISO Support
* The `-windows` flag configures the USB gadget with Windows-compatible descriptors for improved boot compatibility
* Windows ISOs should be mounted using the `-windows` flag for best results
* This feature automatically sets CD-ROM mode and read-only as required for Windows boot
* Uses appropriate USB vendor/product IDs and device strings that Windows recognizes during setup

## Credits

Inspired by https://github.com/fredldotme/ISODriveUT
Original Tool at https://github.com/nitanmarcel/isodrive
