#include "configfsisomanager.h"
#include "util.h"
#include <filesystem>
#include <iostream>
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
      return "";
  }

  for (const auto& entry : fs::directory_iterator(usbGadgetRoot)) {
      if (entry.path().filename().string()[0] == '.') continue;

      fs::path gadget = entry.path();
      fs::path udcFile = gadget / "UDC";

      if (!sysfs_read(udcFile.string()).empty()) {
          return gadget.string();
      }
  }
  return "";
}

std::string get_config_root() {
  std::string gadgetRoot = get_gadget_root();
  if (gadgetRoot.empty()) return "";

  fs::path usbConfigRoot = fs::path(gadgetRoot) / "configs";

  if (!fs::exists(usbConfigRoot) || !fs::is_directory(usbConfigRoot)) return "";

  for (const auto& entry : fs::directory_iterator(usbConfigRoot)) {
       if (entry.path().filename().string()[0] != '.') {
           return entry.path().string();
       }
  }
  return "";
}

void configure_windows_descriptors(const std::string& gadgetRoot) {
  std::cout << "\n=== Configuring Windows-compatible USB descriptors ===" << std::endl;
  
  fs::path root = gadgetRoot;

  // Set vendor/product IDs that Windows recognizes
  // Using IDs commonly associated with CD-ROM/mass storage devices
  sysfs_write((root / "idVendor").string(), "0x058f");  // Alcor Micro Corp
  sysfs_write((root / "idProduct").string(), "0x6387"); // Mass Storage
  
  // Set USB 2.0 specification version
  sysfs_write((root / "bcdUSB").string(), "0x0200");
  
  // Set device version
  sysfs_write((root / "bcdDevice").string(), "0x0100");
  
  // Set device class to 0x00 (defined at interface level)
  sysfs_write((root / "bDeviceClass").string(), "0x00");
  sysfs_write((root / "bDeviceSubClass").string(), "0x00");
  sysfs_write((root / "bDeviceProtocol").string(), "0x00");
  
  // Configure device strings (important for Windows driver binding)
  fs::path stringsPath = root / "strings/0x409";
  
  // Create strings directory if it doesn't exist
  if (!fs::exists(stringsPath)) {
    fs::create_directories(stringsPath);
  }
  
  // Use generic strings that Windows recognizes
  sysfs_write((stringsPath / "manufacturer").string(), "Generic");
  sysfs_write((stringsPath / "product").string(), "USB Mass Storage");
  sysfs_write((stringsPath / "serialnumber").string(), "000000000001");
  
  std::cout << "Windows USB descriptors configured" << std::endl;
}

void configure_windows_mass_storage(const std::string& lunRoot) {
  std::cout << "Configuring Windows mass storage settings..." << std::endl;
  
  fs::path root = lunRoot;

  // Set removable flag (critical for Windows CD-ROM recognition)
  sysfs_write((root / "removable").string(), "1");
  
  // Disable forced unit access for better stability
  fs::path nofuaFile = root / "nofua";
  if (fs::exists(nofuaFile)) {
    sysfs_write(nofuaFile.string(), "1");
  }
  
  // Set inquiry string if supported (helps Windows identify the device)
  fs::path inquiryFile = root / "inquiry_string";
  if (fs::exists(inquiryFile)) {
    sysfs_write(inquiryFile.string(), "Generic  USB CD-ROM       1.00");
  }
  
  std::cout << "Windows mass storage settings configured" << std::endl;
}

void mount_iso(const std::string& iso_path, bool cdrom, bool ro, bool windows_mode) {
  std::string gadgetRoot = get_gadget_root();

  if (gadgetRoot.empty()) {
    std::cerr << "No active gadget found!" << std::endl;
    return;
  }
  std::string configRoot = get_config_root();
  std::string udc = get_udc();

  if (udc.empty()) {
    std::cerr << "Failed to get UDC!" << std::endl;
    return;
  }

  fs::path functionRoot = fs::path(gadgetRoot) / "functions";
  fs::path massStorageRoot = functionRoot / "mass_storage.0";
  fs::path lunRoot = massStorageRoot / "lun.0";

  fs::path stallFile = massStorageRoot / "stall";
  fs::path lunFile = lunRoot / "file";
  fs::path lunCdRom = lunRoot / "cdrom";
  fs::path lunRo = lunRoot / "ro";

  // Disable UDC before making changes
  set_udc("", gadgetRoot);

  // If Windows mode is enabled, configure USB descriptors
  if (windows_mode) {
    std::cout << "\n****************************************" << std::endl;
    std::cout << "*** WINDOWS ISO MODE ENABLED ***" << std::endl;
    std::cout << "****************************************" << std::endl;
    
    configure_windows_descriptors(gadgetRoot);
    
    // Force CD-ROM and read-only mode for Windows ISOs
    cdrom = true;
    ro = true;
    
    std::cout << "Forced CD-ROM mode: enabled" << std::endl;
    std::cout << "Forced read-only: enabled" << std::endl;
  }

  if (!fs::exists(massStorageRoot)) {
    fs::create_directories(massStorageRoot);
  }

  // Disable stall for better Windows compatibility
  sysfs_write(stallFile.string(), "0");

  sysfs_write(lunFile.string(), "");

  if (!iso_path.empty())
  {
    fs::path linkPath = fs::path(configRoot) / "mass_storage.0";
    if (!fs::exists(linkPath)) {
      fs::create_directory_symlink(massStorageRoot, linkPath);
    }
    
    sysfs_write(lunCdRom.string(), cdrom ? "1" : "0");
    sysfs_write(lunRo.string(), ro ? "1" : "0");

    // Apply Windows-specific mass storage settings
    if (windows_mode) {
      configure_windows_mass_storage(lunRoot.string());
    }

    sysfs_write(lunFile.string(), iso_path);

    if (windows_mode) {
      std::cout << "\n****************************************" << std::endl;
      std::cout << "Windows ISO mounted successfully!" << std::endl;
      std::cout << "Device should be recognized by Windows Setup" << std::endl;
      std::cout << "as a bootable CD-ROM drive." << std::endl;
      std::cout << "****************************************\n" << std::endl;
    }
  }
  else
  {
    fs::path linkPath = fs::path(configRoot) / "mass_storage.0";
    if (fs::exists(linkPath)) {
        fs::remove(linkPath);
    }
  }

  set_udc(udc, gadgetRoot);
}

void set_udc(const std::string& udc, const std::string& gadget) {
  fs::path udcFile = fs::path(gadget) / "UDC";
  sysfs_write(udcFile.string(), udc);
}

std::string get_udc() {
  std::string gadget_root = get_gadget_root();
  if (gadget_root.empty()) return "";

  fs::path udcFile = fs::path(gadget_root) / "UDC";
  return sysfs_read(udcFile.string());
}
