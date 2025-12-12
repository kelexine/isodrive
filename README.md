# isodrive (configfs) - Fork by kelexine

## Building

### Prerequisites
* **Linux:** `build-essential`, `cmake`
* **Android (Termux):** `clang`, `cmake`, `make`, `zip`

### Build Instructions

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/kelexine/isodrive
    cd isodrive
    ```

2.  **Build:**
    ```bash
    make
    ```
    This automatically creates the `build` directory and compiles the project.

3.  **Install (Optional):**
    ```bash
    sudo make install
    ```

4.  **Build Magisk Module:**
    ```bash
    make magisk
    ```
    This will generate `isodrive-magisk.zip` in the project root.

5.  **Run Tests:**
    ```bash
    make test
    ```

## Usage
```bash
Usage:
isodrive [FILE]... [OPTION]...
Mounts the given FILE as a bootable device using configfs.
Run without any arguments to unmount any mounted files and display this help message.

Optional arguments:
-rw		Mounts the file in read write mode.
-cdrom		Mounts the file as a cdrom.
-hdd		Forces the file to be mounted as a hard disk (disables auto-detect).
-configfs	Forces the app to use configfs.
-usbgadget	Forces the app to use sysfs.
```

### Examples

Mount ISO as Read-Write:
```bash
sudo isodrive /path/to/file.iso -rw
```

Mount ISO as CD-ROM:
```bash
sudo isodrive /path/to/file.iso -cdrom
```

## Linux
* Has been only tested on Halium based mobile linux, but should work on mainline devices too.

## Android

* On Android can be compiled in Termux, using `clang` and `cmake`.
* On Android you might manually need to mount configfs by running: `mount -t configfs configfs /sys/kernel/config`
* You can build a Magisk module directly by running `make magisk`.

## OS Support
* **Windows ISOs:** Automatically detected and mounted as CD-ROM for better compatibility.
* Should support almost every bootable OS images. Issues or extra steps are documented in the [WIKI](https://github.com/kelexine/isodrive/wiki)

## Credits

* Fork maintained by **kelexine**
* Original project by **nitanmarcel**
* Inspired by https://github.com/fredldotme/ISODriveUT