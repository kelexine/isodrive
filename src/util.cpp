#include "util.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mntent.h>
#include <string>

namespace fs = std::filesystem;

std::string fs_mount_point(const std::string& filesystem_type) {
  struct mntent *ent;
  FILE *mounts;
  std::string mount_point;

  mounts = setmntent("/proc/mounts", "r");
  if (!mounts) return "";

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
    }
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
  if (!file) return false;

  file.seekg(510);
  if (file.fail()) return false;

  unsigned char buffer[2];
  file.read(reinterpret_cast<char*>(buffer), 2);

  if (file.gcount() != 2) return false;

  return (buffer[0] == 0x55 && buffer[1] == 0xAA);
}

void sysfs_write(const std::string& path, const std::string& content) {
  std::cout << "Write: " << content << " -> " << path << std::endl;
  std::ofstream sysfsFile(path);
  if (sysfsFile.is_open()) {
      sysfsFile << content << std::endl;
  } else {
      std::cerr << "Failed to open " << path << " for writing." << std::endl;
  }
}

std::string sysfs_read(const std::string& path) {
  std::string value;
  std::ifstream sysfsFile(path);

  if (!sysfsFile.is_open()) {
    return "";
  }
  sysfsFile >> value;
  return value;
}