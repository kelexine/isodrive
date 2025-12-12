#include "androidusbisomanager.h"
#include "configfsisomanager.h"
#include "util.h"
#include <iostream>
#include <string>
#include <unistd.h>

void print_help() {
  std::cout << "Usage:\n"
            << "isodrive [FILE]... [OPTION]...\n"
            << "Mounts the given FILE as a bootable device using configfs.\n"
            << "Run without any arguments to unmount any mounted files and display "
               "this help message.\n\n"
            << "Optional arguments:\n"
            << "-rw\t\t Mounts the file in read write mode.\n"
            << "-cdrom\t\t Mounts the file as a cdrom.\n"
            << "-windows\t Enables Windows ISO mode (auto-enables CD-ROM, read-only, and Windows-compatible USB descriptors).\n"
            << "-hdd\t\t Forces the file to be mounted as a hard disk (disables auto-detect).\n"
            << "-configfs\t Forces the app to use configfs.\n"
            << "-usbgadget\t Forces the app to use sysfs.\n\n";
}

void configs(const std::string& iso_target, bool cdrom, bool ro, bool windows_mode) {
  std::cout << "Using configfs!" << std::endl;

  if (!supported())
  {
    std::cerr << "usb_gadget is not supported!" << std::endl;
    return;
  }
  
  mount_iso(iso_target, cdrom, ro, windows_mode);
}

void usb(const std::string& iso_target, bool cdrom, bool ro) {
  std::cout << "Using sysfs!" << std::endl;
  if (!usb_supported())
  {
    std::cerr << "usb_gadget is not supported!" << std::endl;
    return;
  }
  if (cdrom || !ro)
  {
    std::cout << "cdrom/ro flags ignored. (this is expected)" << std::endl;
  }
  if (iso_target.empty())
    usb_reset_iso();
  else
    usb_mount_iso(iso_target);
}

int main(int argc, char *argv[]) {
  if (getuid() != 0) {
    std::cerr << "Permission denied" << std::endl;
    return 1;
  }

  std::string iso_target = "";
  bool cdrom = false;
  bool ro = true;
  bool force_configfs = false;
  bool force_usbgadget = false;
  bool force_hdd = false;
  bool windows_mode = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-rw") {
      ro = false;
    } else if (arg == "-cdrom") {
      cdrom = true;
    } else if (arg == "-windows") {
      windows_mode = true;
    } else if (arg == "-hdd") {
      force_hdd = true;
    } else if (arg == "-configfs") {
      force_configfs = true;
    } else if (arg == "-usbgadget") {
      force_usbgadget = true;
    } else if (iso_target.empty()) {
      iso_target = arg;
    }
  }

  if (argc == 1) {
    print_help();
  }

  // Check for incompatible flags
  if (cdrom && !ro && !windows_mode) {
    std::cerr << "Incompatible arguments -cdrom and -rw" << std::endl;
    return 1;
  }

  if (cdrom && force_hdd) {
    std::cerr << "Incompatible arguments -cdrom and -hdd" << std::endl;
    return 1;
  }

  if (!iso_target.empty() && !isfile(iso_target)) {
    std::cerr << "Error: File not found: " << iso_target << std::endl;
    return 1;
  }

  // Auto-detect Windows/Non-hybrid ISOs
  if (!iso_target.empty() && !cdrom && ro && !force_hdd && !windows_mode) {
    if (!is_hybrid_iso(iso_target)) {
      std::cout << "Non-hybrid ISO detected (e.g. Windows). Auto-enabling Windows Mode." << std::endl;
      windows_mode = true;
      // Windows mode implies CD-ROM, which configfsisomanager handles.
    }
  }

  if (force_configfs)
    configs(iso_target, cdrom, ro, windows_mode);
  else if (force_usbgadget) {
    if (windows_mode) {
       std::cout << "Warning: Windows mode is only supported with configfs backend" << std::endl;
    }
    usb(iso_target, cdrom, ro);
  }
  else if (supported())
    configs(iso_target, cdrom, ro, windows_mode);
  else if (usb_supported()) {
    if (windows_mode) {
       std::cout << "Warning: Windows mode is only supported with configfs backend" << std::endl;
    }
    usb(iso_target, cdrom, ro);
  }
  else
    std::cerr << "Device does not support isodrive" << std::endl;
  return 0;
}
