#include "configfsisomanager.h"
#include "logger.h"
#include "util.h"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

bool supported() {
    return !fs_mount_point("configfs").empty();
}

std::string get_gadget_root() {
  std::string configFsRoot = fs_mount_point("configfs");
  if (configFsRoot.empty()) return "";

  fs::path usbGadgetRoot = fs::path(configFsRoot) / "usb_gadget";

  if (!fs::exists(usbGadgetRoot) || !fs::is_directory(usbGadgetRoot)) {
      log_debug("usb_gadget directory not found at " + usbGadgetRoot.string());
      return "";
  }

  for (const auto& entry : fs::directory_iterator(usbGadgetRoot)) {
      if (entry.path().filename().string()[0] == '.') continue;

      fs::path gadget = entry.path();
      fs::path udcFile = gadget / "UDC";

      if (!sysfs_read(udcFile.string()).empty()) {
          log_debug("Found active gadget: " + gadget.string());
          return gadget.string();
      }
  }
  log_debug("No active gadget found in " + usbGadgetRoot.string());
  return "";
}

std::string get_config_root() {
  std::string gadgetRoot = get_gadget_root();
  if (gadgetRoot.empty()) return "";

  fs::path usbConfigRoot = fs::path(gadgetRoot) / "configs";

  if (!fs::exists(usbConfigRoot) || !fs::is_directory(usbConfigRoot)) {
    log_debug("configs directory not found at " + usbConfigRoot.string());
    return "";
  }

  for (const auto& entry : fs::directory_iterator(usbConfigRoot)) {
       if (entry.path().filename().string()[0] != '.') {
           return entry.path().string();
       }
  }
  log_debug("No config found in " + usbConfigRoot.string());
  return "";
}

static bool configure_windows_descriptors(const std::string& gadgetRoot, const WindowsMountOptions& win_opts) {
  log_info("");
  log_info("=== Configuring Windows-compatible USB descriptors ===");
  
  fs::path root = gadgetRoot;
  bool success = true;

  // Set vendor/product IDs that Windows recognizes
  // Using IDs commonly associated with CD-ROM/mass storage devices
  success &= sysfs_write((root / "idVendor").string(), "0x058f");  // Alcor Micro Corp
  success &= sysfs_write((root / "idProduct").string(), "0x6387"); // Mass Storage
  
  // Set USB version based on options
  if (win_opts.use_usb3) {
    log_info("Using USB 3.0 descriptors");
    success &= sysfs_write((root / "bcdUSB").string(), "0x0300");
  } else {
    success &= sysfs_write((root / "bcdUSB").string(), "0x0200");
  }
  
  // Set device version
  success &= sysfs_write((root / "bcdDevice").string(), "0x0100");
  
  // Set device class to 0x00 (defined at interface level)
  success &= sysfs_write((root / "bDeviceClass").string(), "0x00");
  success &= sysfs_write((root / "bDeviceSubClass").string(), "0x00");
  success &= sysfs_write((root / "bDeviceProtocol").string(), "0x00");
  
  // Set max power (important for USB 3.0)
  fs::path maxPowerFile = root / "configs/c.1/MaxPower";
  if (fs::exists(maxPowerFile.parent_path())) {
    if (win_opts.use_usb3) {
      success &= sysfs_write(maxPowerFile.string(), "896");  // 896mA for USB 3.0
    } else {
      success &= sysfs_write(maxPowerFile.string(), "500");  // 500mA for USB 2.0
    }
  }
  
  // Configure device strings (important for Windows driver binding)
  fs::path stringsPath = root / "strings/0x409";
  
  // Create strings directory if it doesn't exist
  if (!fs::exists(stringsPath)) {
    std::error_code ec;
    fs::create_directories(stringsPath, ec);
    if (ec) {
      log_error("Failed to create strings directory: " + ec.message());
      return false;
    }
  }
  
  // Set product string based on Windows version
  std::string product_string = "USB Mass Storage";
  if (win_opts.version == WindowsVersion::WIN11) {
    product_string = "USB CD-ROM Drive";
  } else if (win_opts.version == WindowsVersion::WIN10) {
    product_string = "USB CD-ROM Drive";
  }
  
  success &= sysfs_write((stringsPath / "manufacturer").string(), "Generic");
  success &= sysfs_write((stringsPath / "product").string(), product_string);
  success &= sysfs_write((stringsPath / "serialnumber").string(), "000000000001");
  
  if (success) {
    log_info("Windows USB descriptors configured");
  } else {
    log_warn("Some Windows USB descriptor writes failed");
  }
  
  return success;
}

static bool configure_windows_mass_storage(const std::string& lunRoot, const WindowsMountOptions& win_opts) {
  log_info("Configuring Windows mass storage settings...");
  
  fs::path root = lunRoot;
  bool success = true;

  // Set removable flag (critical for Windows CD-ROM recognition)
  success &= sysfs_write((root / "removable").string(), "1");
  
  // Disable forced unit access for better stability
  fs::path nofuaFile = root / "nofua";
  if (fs::exists(nofuaFile)) {
    success &= sysfs_write(nofuaFile.string(), "1");
  }
  
  // Set inquiry string based on Windows version
  fs::path inquiryFile = root / "inquiry_string";
  if (fs::exists(inquiryFile)) {
    std::string inquiry;
    if (win_opts.version == WindowsVersion::WIN11) {
      inquiry = "Generic  USB CD-ROM       1.00";
    } else if (win_opts.version == WindowsVersion::WIN10) {
      inquiry = "Generic  USB CD-ROM       1.00";
    } else {
      inquiry = "Generic  USB CD-ROM       1.00";
    }
    success &= sysfs_write(inquiryFile.string(), inquiry);
  }
  
  if (success) {
    log_info("Windows mass storage settings configured");
  } else {
    log_warn("Some Windows mass storage settings failed");
  }
  
  return success;
}

static void print_windows_info(const WindowsMountOptions& win_opts) {
  log_info("");
  log_info("****************************************");
  log_info("***    WINDOWS ISO MODE ENABLED     ***");
  log_info("****************************************");
  log_info("");
  
  // Show detected version
  log_info("Detected: " + windows_version_to_string(win_opts.version));
  
  // Show boot mode info
  if (win_opts.has_uefi && win_opts.has_legacy) {
    log_info("Boot Mode: UEFI + Legacy BIOS (dual boot)");
  } else if (win_opts.has_uefi) {
    log_info("Boot Mode: UEFI only");
  } else if (win_opts.has_legacy) {
    log_info("Boot Mode: Legacy BIOS only");
  } else {
    log_info("Boot Mode: Unknown");
  }
  
  // Show USB mode
  if (win_opts.use_usb3) {
    log_info("USB Mode: USB 3.0 (SuperSpeed)");
  } else {
    log_info("USB Mode: USB 2.0 (High Speed)");
  }
  
  log_info("");
}

bool mount_iso(const std::string& iso_path, bool cdrom, bool ro, const WindowsMountOptions& win_opts) {
  std::string gadgetRoot = get_gadget_root();

  if (gadgetRoot.empty()) {
    log_error("No active gadget found!");
    return false;
  }
  std::string configRoot = get_config_root();
  std::string udc = get_udc();

  if (udc.empty()) {
    log_error("Failed to get UDC!");
    return false;
  }

  fs::path functionRoot = fs::path(gadgetRoot) / "functions";
  fs::path massStorageRoot = functionRoot / "mass_storage.0";
  fs::path lunRoot = massStorageRoot / "lun.0";

  fs::path stallFile = massStorageRoot / "stall";
  fs::path lunFile = lunRoot / "file";
  fs::path lunCdRom = lunRoot / "cdrom";
  fs::path lunRo = lunRoot / "ro";

  bool success = true;

  // Disable UDC before making changes
  if (!set_udc("", gadgetRoot)) {
    log_warn("Failed to disable UDC before configuration");
  }

  // If Windows mode is enabled, configure USB descriptors
  if (win_opts.enabled) {
    print_windows_info(win_opts);
    
    if (!configure_windows_descriptors(gadgetRoot, win_opts)) {
      log_warn("Windows descriptor configuration had errors");
    }
    
    // Force CD-ROM and read-only mode for Windows ISOs
    cdrom = true;
    ro = true;
    
    log_info("Forced CD-ROM mode: enabled");
    log_info("Forced read-only: enabled");
  }

  if (!fs::exists(massStorageRoot)) {
    std::error_code ec;
    fs::create_directories(massStorageRoot, ec);
    if (ec) {
      log_error("Failed to create mass_storage function: " + ec.message());
      set_udc(udc, gadgetRoot);
      return false;
    }
  }

  // Disable stall for better Windows compatibility
  success &= sysfs_write(stallFile.string(), "0");

  success &= sysfs_write(lunFile.string(), "");

  if (!iso_path.empty())
  {
    fs::path linkPath = fs::path(configRoot) / "mass_storage.0";
    if (!fs::exists(linkPath)) {
      std::error_code ec;
      fs::create_directory_symlink(massStorageRoot, linkPath, ec);
      if (ec) {
        log_error("Failed to create symlink: " + ec.message());
        set_udc(udc, gadgetRoot);
        return false;
      }
    }
    
    success &= sysfs_write(lunCdRom.string(), cdrom ? "1" : "0");
    success &= sysfs_write(lunRo.string(), ro ? "1" : "0");

    // Apply Windows-specific mass storage settings
    if (win_opts.enabled) {
      if (!configure_windows_mass_storage(lunRoot.string(), win_opts)) {
        log_warn("Windows mass storage configuration had errors");
      }
    }

    success &= sysfs_write(lunFile.string(), iso_path);

    if (win_opts.enabled) {
      log_info("");
      log_info("****************************************");
      log_info("Windows ISO mounted successfully!");
      log_info("");
      log_info("The device should be recognized by Windows");
      log_info("Setup as a bootable CD-ROM drive.");
      
      if (win_opts.has_uefi) {
        log_info("");
        log_info("For UEFI boot:");
        log_info("  - Select UEFI boot from your boot menu");
        log_info("  - Secure Boot may need to be disabled");
      }
      
      if (win_opts.has_legacy) {
        log_info("");
        log_info("For Legacy BIOS boot:");
        log_info("  - Select USB-CDROM from your boot menu");
      }
      
      log_info("****************************************");
      log_info("");
    }
  }
  else
  {
    fs::path linkPath = fs::path(configRoot) / "mass_storage.0";
    if (fs::exists(linkPath)) {
      std::error_code ec;
      fs::remove(linkPath, ec);
      if (ec) {
        log_warn("Failed to remove symlink: " + ec.message());
      }
    }
  }

  if (!set_udc(udc, gadgetRoot)) {
    log_error("Failed to re-enable UDC");
    return false;
  }

  return success;
}

bool set_udc(const std::string& udc, const std::string& gadget) {
  fs::path udcFile = fs::path(gadget) / "UDC";
  return sysfs_write(udcFile.string(), udc);
}

std::string get_udc() {
  std::string gadget_root = get_gadget_root();
  if (gadget_root.empty()) return "";

  fs::path udcFile = fs::path(gadget_root) / "UDC";
  return sysfs_read(udcFile.string());
}
