#include "androidusbisomanager.h"
#include "configfsisomanager.h"
#include "logger.h"
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
            << "-hdd\t\t Forces the file to be mounted as a hard disk (disables auto-detect).\n\n"
            << "Windows ISO options:\n"
            << "-windows\t Enables Windows ISO mode (auto-detects if not specified).\n"
            << "-win10\t\t Forces Windows 10 mode.\n"
            << "-win11\t\t Forces Windows 11 mode.\n"
            << "-usb3\t\t Uses USB 3.0 (SuperSpeed) descriptors.\n\n"
            << "Backend options:\n"
            << "-configfs\t Forces the app to use configfs.\n"
            << "-usbgadget\t Forces the app to use sysfs.\n\n"
            << "Output options:\n"
            << "-v, -verbose\t Enables verbose/debug output.\n"
            << "-q, -quiet\t Suppresses all output except errors.\n\n";
}

bool configs(const std::string& iso_target, bool cdrom, bool ro, const WindowsMountOptions& win_opts) {
  log_info("Using configfs!");

  if (!supported())
  {
    log_error("usb_gadget is not supported!");
    return false;
  }
  
  return mount_iso(iso_target, cdrom, ro, win_opts);
}

bool usb(const std::string& iso_target, bool cdrom, bool ro) {
  log_info("Using sysfs!");
  if (!usb_supported())
  {
    log_error("usb_gadget is not supported!");
    return false;
  }
  if (cdrom || !ro)
  {
    log_warn("cdrom/ro flags ignored. (this is expected for sysfs backend)");
  }
  if (iso_target.empty())
    return usb_reset_iso();
  else
    return usb_mount_iso(iso_target);
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
  
  // Windows options
  bool windows_mode = false;
  bool force_win10 = false;
  bool force_win11 = false;
  bool use_usb3 = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-rw") {
      ro = false;
    } else if (arg == "-cdrom") {
      cdrom = true;
    } else if (arg == "-windows") {
      windows_mode = true;
    } else if (arg == "-win10") {
      windows_mode = true;
      force_win10 = true;
    } else if (arg == "-win11") {
      windows_mode = true;
      force_win11 = true;
    } else if (arg == "-usb3") {
      use_usb3 = true;
    } else if (arg == "-hdd") {
      force_hdd = true;
    } else if (arg == "-configfs") {
      force_configfs = true;
    } else if (arg == "-usbgadget") {
      force_usbgadget = true;
    } else if (arg == "-v" || arg == "-verbose") {
      log_set_level(LogLevel::DEBUG);
    } else if (arg == "-q" || arg == "-quiet") {
      log_set_level(LogLevel::ERROR);
    } else if (iso_target.empty() && arg[0] != '-') {
      iso_target = arg;
    }
  }

  if (argc == 1) {
    print_help();
  }

  // Check for incompatible flags
  if (cdrom && !ro && !windows_mode) {
    log_error("Incompatible arguments -cdrom and -rw");
    return 1;
  }

  if (cdrom && force_hdd) {
    log_error("Incompatible arguments -cdrom and -hdd");
    return 1;
  }

  if (force_win10 && force_win11) {
    log_error("Incompatible arguments -win10 and -win11");
    return 1;
  }

  if (!iso_target.empty() && !isfile(iso_target)) {
    log_error("File not found: " + iso_target);
    return 1;
  }

  // Build Windows mount options
  WindowsMountOptions win_opts = {};
  win_opts.enabled = false;
  win_opts.version = WindowsVersion::NONE;
  win_opts.use_usb3 = use_usb3;
  win_opts.has_uefi = false;
  win_opts.has_legacy = false;

  // Auto-detect Windows ISO if not forcing HDD mode
  if (!iso_target.empty() && !force_hdd) {
    // Check if it's a Windows ISO
    WindowsIsoInfo iso_info = get_windows_iso_info(iso_target);
    
    if (iso_info.is_windows || windows_mode) {
      win_opts.enabled = true;
      
      // Use detected info unless overridden
      if (force_win11) {
        win_opts.version = WindowsVersion::WIN11;
      } else if (force_win10) {
        win_opts.version = WindowsVersion::WIN10;
      } else if (iso_info.is_windows) {
        win_opts.version = iso_info.version;
      } else {
        win_opts.version = WindowsVersion::WIN_UNKNOWN;
      }
      
      win_opts.has_uefi = iso_info.has_uefi;
      win_opts.has_legacy = iso_info.has_legacy;
      
      // If we detected Windows, show info
      if (iso_info.is_windows && !windows_mode) {
        log_info("Windows ISO detected: " + iso_info.volume_label);
        log_info("Auto-enabling Windows mode.");
      }
    } else if (!is_hybrid_iso(iso_target) && !cdrom) {
      // Non-hybrid, non-Windows ISO - still use CD-ROM mode
      log_info("Non-hybrid ISO detected. Mounting as CD-ROM.");
      cdrom = true;
    }
  }

  bool success = false;

  if (force_configfs) {
    success = configs(iso_target, cdrom, ro, win_opts);
  }
  else if (force_usbgadget) {
    if (win_opts.enabled) {
       log_warn("Windows mode is only supported with configfs backend");
    }
    success = usb(iso_target, cdrom, ro);
  }
  else if (supported()) {
    success = configs(iso_target, cdrom, ro, win_opts);
  }
  else if (usb_supported()) {
    if (win_opts.enabled) {
       log_warn("Windows mode is only supported with configfs backend");
    }
    success = usb(iso_target, cdrom, ro);
  }
  else {
    log_error("Device does not support isodrive");
    return 1;
  }

  return success ? 0 : 1;
}
