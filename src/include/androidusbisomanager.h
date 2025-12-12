#ifndef ANDROIDUSBISOMANAGER_H
#define ANDROIDUSBISOMANAGER_H

#include <string>

#define ANDROID0_SYSFS_ENABLE "/sys/devices/virtual/android_usb/android0/enable"
#define ANDROID0_SYSFS_IMG_FILE                                                \
  "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun/file"
#define ANDROID0_SYSFS_FEATURES                                                \
  "/sys/devices/virtual/android_usb/android0/functions"

bool usb_supported();

void usb_mount_iso(const std::string& iso_path);
void usb_reset_iso();

bool usb_enabled();
void usb_set_enabled(bool enabled);

#endif // ifndef ANDROIDUSBISOMANAGER_H