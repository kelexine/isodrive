#include "util.h"
#include "logger.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mntent.h>
#include <string>

namespace fs = std::filesystem;

// ISO 9660 constants
constexpr int ISO_SECTOR_SIZE = 2048;
constexpr int ISO_PVD_SECTOR = 16;  // Primary Volume Descriptor is at sector 16
constexpr int ISO_PVD_OFFSET = ISO_PVD_SECTOR * ISO_SECTOR_SIZE;  // 32768

std::string fs_mount_point(const std::string& filesystem_type) {
  struct mntent *ent;
  FILE *mounts;
  std::string mount_point;

  mounts = setmntent("/proc/mounts", "r");
  if (!mounts) {
    log_debug("Failed to open /proc/mounts");
    return "";
  }

  while (nullptr != (ent = getmntent(mounts))) {
    if (filesystem_type == ent->mnt_fsname) {
      mount_point = ent->mnt_dir;
      break;
    }
  }
  endmntent(mounts);

  // Alternate search location on Android
  if (mount_point.empty() && filesystem_type == "configfs") {
    if (fs::exists("/config/usb_gadget")) {
      mount_point = "/config";
      log_debug("Found configfs at /config (Android fallback)");
    }
  }

  if (!mount_point.empty()) {
    log_debug("Found " + filesystem_type + " at " + mount_point);
  }

  return mount_point;
}

bool isdir(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool isfile(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

bool is_hybrid_iso(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    log_debug("Cannot open file for hybrid ISO check: " + path);
    return false;
  }

  file.seekg(510);
  if (file.fail()) {
    log_debug("Failed to seek to offset 510 in: " + path);
    return false;
  }

  unsigned char buffer[2];
  file.read(reinterpret_cast<char*>(buffer), 2);

  if (file.gcount() != 2) {
    log_debug("Failed to read 2 bytes at offset 510 from: " + path);
    return false;
  }

  bool is_hybrid = (buffer[0] == 0x55 && buffer[1] == 0xAA);
  log_debug("ISO " + path + " hybrid check: " + (is_hybrid ? "true" : "false"));
  return is_hybrid;
}

// Helper: Read ISO 9660 Primary Volume Descriptor and extract volume label
static std::string read_iso_volume_label(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return "";
  }

  // Seek to Primary Volume Descriptor (sector 16)
  file.seekg(ISO_PVD_OFFSET);
  if (file.fail()) {
    return "";
  }

  // Read the sector
  char buffer[ISO_SECTOR_SIZE];
  file.read(buffer, ISO_SECTOR_SIZE);
  if (file.gcount() != ISO_SECTOR_SIZE) {
    return "";
  }

  // Verify this is a Primary Volume Descriptor
  // Byte 0: Type (1 = PVD)
  // Bytes 1-5: "CD001"
  if (buffer[0] != 1 || std::strncmp(buffer + 1, "CD001", 5) != 0) {
    log_debug("Not a valid ISO 9660 Primary Volume Descriptor");
    return "";
  }

  // Volume Identifier is at offset 40, 32 bytes, space-padded
  std::string volume_id(buffer + 40, 32);
  
  // Trim trailing spaces
  size_t end = volume_id.find_last_not_of(' ');
  if (end != std::string::npos) {
    volume_id = volume_id.substr(0, end + 1);
  } else {
    volume_id.clear();
  }

  log_debug("ISO volume label: " + volume_id);
  return volume_id;
}

// Helper: Check if a specific path exists within the ISO (basic check via volume label patterns)
static bool iso_contains_windows_markers(const std::string& volume_label) {
  std::string upper_label = volume_label;
  std::transform(upper_label.begin(), upper_label.end(), upper_label.begin(), ::toupper);

  // Common Windows ISO volume labels
  if (upper_label.find("WIN") != std::string::npos) return true;
  if (upper_label.find("WINDOWS") != std::string::npos) return true;
  if (upper_label.find("CCCOMA") != std::string::npos) return true;  // Windows Media Creation Tool
  if (upper_label.find("ESD-ISO") != std::string::npos) return true;  // Windows ESD
  if (upper_label.find("J_CCSA") != std::string::npos) return true;   // Some Windows ISOs
  if (upper_label.find("CPBA") != std::string::npos) return true;     // Some Windows ISOs
  
  return false;
}

// Helper: Detect Windows version from volume label
static WindowsVersion detect_version_from_label(const std::string& volume_label) {
  std::string upper_label = volume_label;
  std::transform(upper_label.begin(), upper_label.end(), upper_label.begin(), ::toupper);

  // Windows 11 patterns
  if (upper_label.find("WIN11") != std::string::npos) return WindowsVersion::WIN11;
  if (upper_label.find("WINDOWS 11") != std::string::npos) return WindowsVersion::WIN11;
  if (upper_label.find("W11") != std::string::npos) return WindowsVersion::WIN11;
  
  // Windows 10 patterns
  if (upper_label.find("WIN10") != std::string::npos) return WindowsVersion::WIN10;
  if (upper_label.find("WINDOWS 10") != std::string::npos) return WindowsVersion::WIN10;
  if (upper_label.find("W10") != std::string::npos) return WindowsVersion::WIN10;

  // Recent Windows ISO naming conventions
  // CCCOMA_X64FRE - typically Windows 10/11
  // Check for presence of newer patterns
  if (upper_label.find("CCCOMA") != std::string::npos) {
    // This is a Media Creation Tool ISO, but we can't determine version without more info
    return WindowsVersion::WIN_UNKNOWN;
  }

  return WindowsVersion::WIN_UNKNOWN;
}

// Helper: Search ISO for specific file signatures
static bool search_iso_for_bootloader(std::ifstream& file, bool& has_uefi, bool& has_legacy) {
  has_uefi = false;
  has_legacy = false;

  // Read El Torito Boot Record at sector 17
  file.seekg(17 * ISO_SECTOR_SIZE);
  if (file.fail()) {
    return false;
  }

  char buffer[ISO_SECTOR_SIZE];
  file.read(buffer, ISO_SECTOR_SIZE);
  if (file.gcount() != ISO_SECTOR_SIZE) {
    return false;
  }

  // Check for El Torito signature
  // Byte 0: Type (0 = Boot Record)
  // Bytes 1-5: "CD001"
  // Bytes 7-38: "EL TORITO SPECIFICATION"
  if (buffer[0] == 0 && std::strncmp(buffer + 1, "CD001", 5) == 0) {
    if (std::strncmp(buffer + 7, "EL TORITO SPECIFICATION", 23) == 0) {
      log_debug("Found El Torito boot record");
      has_legacy = true;
    }
  }

  // For UEFI detection, we look for the EFI system partition marker
  // by scanning the volume descriptors for EFI boot catalog entries
  // In practice, we check for common patterns in the first few sectors

  // Simple heuristic: If it's a Windows ISO and has El Torito, 
  // modern Windows ISOs almost always have UEFI support
  // A more thorough check would require parsing the directory structure

  // Check sectors for EFI signatures
  for (int sector = 16; sector < 20; sector++) {
    file.seekg(sector * ISO_SECTOR_SIZE);
    if (file.fail()) break;
    
    file.read(buffer, ISO_SECTOR_SIZE);
    if (file.gcount() != ISO_SECTOR_SIZE) break;

    // Look for "EFI" string in the sector (boot catalog reference)
    std::string sector_str(buffer, ISO_SECTOR_SIZE);
    if (sector_str.find("EFI BOOT") != std::string::npos ||
        sector_str.find("efi") != std::string::npos ||
        sector_str.find("BOOTX64") != std::string::npos) {
      has_uefi = true;
      log_debug("Found UEFI boot markers in ISO");
      break;
    }
  }

  // If we found El Torito and it's a recent ISO, assume UEFI is supported
  // (This is a reasonable heuristic for modern Windows ISOs)
  if (has_legacy && !has_uefi) {
    // Most modern Windows ISOs are dual-boot (UEFI + Legacy)
    // Mark UEFI as likely available
    log_debug("Assuming UEFI support for modern Windows ISO");
    has_uefi = true;
  }

  return true;
}

bool is_windows_iso(const std::string& path) {
  std::string volume_label = read_iso_volume_label(path);
  if (volume_label.empty()) {
    return false;
  }
  return iso_contains_windows_markers(volume_label);
}

WindowsIsoInfo get_windows_iso_info(const std::string& path) {
  WindowsIsoInfo info = {};
  info.is_windows = false;
  info.version = WindowsVersion::NONE;
  info.has_uefi = false;
  info.has_legacy = false;

  // Read volume label
  info.volume_label = read_iso_volume_label(path);
  if (info.volume_label.empty()) {
    log_debug("Could not read volume label from: " + path);
    return info;
  }

  // Check if it's a Windows ISO
  info.is_windows = iso_contains_windows_markers(info.volume_label);
  if (!info.is_windows) {
    return info;
  }

  // Detect version
  info.version = detect_version_from_label(info.volume_label);

  // Check for boot support
  std::ifstream file(path, std::ios::binary);
  if (file) {
    search_iso_for_bootloader(file, info.has_uefi, info.has_legacy);
  }

  log_debug("Windows ISO detected: " + info.volume_label + 
            ", version: " + windows_version_to_string(info.version) +
            ", UEFI: " + (info.has_uefi ? "yes" : "no") +
            ", Legacy: " + (info.has_legacy ? "yes" : "no"));

  return info;
}

WindowsVersion get_windows_version(const std::string& path) {
  WindowsIsoInfo info = get_windows_iso_info(path);
  return info.version;
}

std::string windows_version_to_string(WindowsVersion version) {
  switch (version) {
    case WindowsVersion::WIN10:
      return "Windows 10";
    case WindowsVersion::WIN11:
      return "Windows 11";
    case WindowsVersion::WIN_UNKNOWN:
      return "Windows (unknown version)";
    case WindowsVersion::NONE:
    default:
      return "Not Windows";
  }
}

bool sysfs_write(const std::string& path, const std::string& content) {
  log_debug("Write: " + content + " -> " + path);
  std::ofstream sysfsFile(path);
  if (sysfsFile.is_open()) {
      sysfsFile << content << std::endl;
      return true;
  } else {
      log_error("Failed to open " + path + " for writing.");
      return false;
  }
}

std::string sysfs_read(const std::string& path) {
  std::string value;
  std::ifstream sysfsFile(path);

  if (!sysfsFile.is_open()) {
    log_debug("Cannot open for reading: " + path);
    return "";
  }
  sysfsFile >> value;
  log_debug("Read: " + value + " <- " + path);
  return value;
}