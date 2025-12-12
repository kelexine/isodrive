#include "androidusbisomanager.h"
#include "util.h"
#include <string>

bool usb_supported() { return isfile(ANDROID0_SYSFS_ENABLE); }

void usb_mount_iso(const std::string& iso_path) {

  if (usb_enabled())
    usb_set_enabled(false);

  sysfs_write(ANDROID0_SYSFS_IMG_FILE, iso_path);

  sysfs_write(ANDROID0_SYSFS_FEATURES, "mass_storage");

  usb_set_enabled(true);
}

void usb_reset_iso() {
  usb_mount_iso("");

  if (usb_enabled())
    usb_set_enabled(false);

  sysfs_write(ANDROID0_SYSFS_FEATURES, "mtp");

  usb_set_enabled(true);
}

bool usb_enabled() {
  return sysfs_read(ANDROID0_SYSFS_ENABLE) == "1";
}

void usb_set_enabled(bool enabled) {
  sysfs_write(ANDROID0_SYSFS_ENABLE, enabled ? "1" : "0");
}
