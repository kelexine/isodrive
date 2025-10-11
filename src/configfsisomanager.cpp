#include <sys/stat.h>
#include <sys/types.h>

#include "configfsisomanager.h"
#include "util.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool supported() { return fs_mount_point((char *)"configfs") != nullptr; }

char *get_gadget_root() {
  char *configFsRoot = fs_mount_point((char *)"configfs");
  char *usbGadgetRoot = strjin(configFsRoot, (char *)"/usb_gadget/");
  char *gadgetRoot = nullptr;

  struct dirent *entry = nullptr;
  DIR *dp = nullptr;

  dp = opendir(usbGadgetRoot);

  if (dp != nullptr) {
    while ((entry = readdir(dp))) {
      if (entry->d_name[0] != '.') {
        char *gadget = strjin(usbGadgetRoot, entry->d_name);

        if (sysfs_read(strjin(gadget, (char *)"/UDC")) != nullptr) {
          gadgetRoot = gadget;
          break;
        }
      }
    }
  }
  return gadgetRoot;
}

char *get_config_root() {
  char *gadgetRoot = get_gadget_root();

  if (gadgetRoot == nullptr) {
    return nullptr;
  }
  char *usbConfigRoot = strjin(gadgetRoot, (char *)"/configs/");
  char *configRoot = nullptr;

  struct dirent *entry = nullptr;
  DIR *dp = nullptr;

  dp = opendir(usbConfigRoot);

  if (dp != nullptr) {
    while ((entry = readdir(dp))) {
      if (entry->d_name[0] != '.') {
        configRoot = strjin(usbConfigRoot, entry->d_name);
        break;
      }
    }
  }
  return configRoot;
}

// NEW FUNCTION: Configure Windows-compatible USB descriptors
void configure_windows_descriptors(char *gadgetRoot) {
  printf("\n=== Configuring Windows-compatible USB descriptors ===\n");
  
  // Set vendor/product IDs that Windows recognizes
  // Using IDs commonly associated with CD-ROM/mass storage devices
  char *idVendorFile = strjin(gadgetRoot, (char *)"/idVendor");
  char *idProductFile = strjin(gadgetRoot, (char *)"/idProduct");
  
  // Option 1: Generic Mass Storage/CD-ROM (recommended)
  sysfs_write(idVendorFile, (char *)"0x058f");  // Alcor Micro Corp
  sysfs_write(idProductFile, (char *)"0x6387"); // Mass Storage
  
  // Alternative options (uncomment if needed):
  // sysfs_write(idVendorFile, (char *)"0x0781");  // SanDisk
  // sysfs_write(idProductFile, (char *)"0x5580"); // Flash Drive
  
  // Set USB 2.0 specification version
  char *bcdUSBFile = strjin(gadgetRoot, (char *)"/bcdUSB");
  sysfs_write(bcdUSBFile, (char *)"0x0200");
  
  // Set device version
  char *bcdDeviceFile = strjin(gadgetRoot, (char *)"/bcdDevice");
  sysfs_write(bcdDeviceFile, (char *)"0x0100");
  
  // Set device class to 0x00 (defined at interface level)
  char *bDeviceClassFile = strjin(gadgetRoot, (char *)"/bDeviceClass");
  char *bDeviceSubClassFile = strjin(gadgetRoot, (char *)"/bDeviceSubClass");
  char *bDeviceProtocolFile = strjin(gadgetRoot, (char *)"/bDeviceProtocol");
  
  sysfs_write(bDeviceClassFile, (char *)"0x00");
  sysfs_write(bDeviceSubClassFile, (char *)"0x00");
  sysfs_write(bDeviceProtocolFile, (char *)"0x00");
  
  // Configure device strings (important for Windows driver binding)
  char *stringsPath = strjin(gadgetRoot, (char *)"/strings/0x409");
  
  // Create strings directory if it doesn't exist
  if (!isdir(stringsPath)) {
    mkdir(stringsPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }
  
  char *manufacturerFile = strjin(stringsPath, (char *)"/manufacturer");
  char *productFile = strjin(stringsPath, (char *)"/product");
  char *serialFile = strjin(stringsPath, (char *)"/serialnumber");
  
  // Use generic strings that Windows recognizes
  sysfs_write(manufacturerFile, (char *)"Generic");
  sysfs_write(productFile, (char *)"USB Mass Storage");
  sysfs_write(serialFile, (char *)"000000000001");
  
  printf("Windows USB descriptors configured\n");
}

// NEW FUNCTION: Configure Windows-specific mass storage settings
void configure_windows_mass_storage(char *lunRoot) {
  printf("Configuring Windows mass storage settings...\n");
  
  // Set removable flag (critical for Windows CD-ROM recognition)
  char *removableFile = strjin(lunRoot, (char *)"/removable");
  sysfs_write(removableFile, (char *)"1");
  
  // Disable forced unit access for better stability
  char *nofuaFile = strjin(lunRoot, (char *)"/nofua");
  if (isfile(nofuaFile)) {
    sysfs_write(nofuaFile, (char *)"1");
  }
  
  // Set inquiry string if supported (helps Windows identify the device)
  char *inquiryFile = strjin(lunRoot, (char *)"/inquiry_string");
  if (isfile(inquiryFile)) {
    sysfs_write(inquiryFile, (char *)"Generic  USB CD-ROM       1.00");
  }
  
  printf("Windows mass storage settings configured\n");
}

// UPDATED FUNCTION: mount_iso now with windows_mode parameter
void mount_iso(char *iso_path, char *cdrom, char *ro, char *windows_mode) {
  char *gadgetRoot = get_gadget_root();

  if (gadgetRoot == nullptr) {
    printf("No active gadget found!\n");
    return;
  }
  
  char *configRoot = get_config_root();
  char *udc = get_udc();
  char *functionRoot = strjin(gadgetRoot, (char *)"/functions");
  char *massStorageRoot = strjin(functionRoot, (char *)"/mass_storage.0");
  char *lunRoot = strjin(massStorageRoot, (char *)"/lun.0");

  char *stallFile = strjin(massStorageRoot, (char *)"/stall");
  char *udcFile = strjin(gadgetRoot, (char *)"/UDC");
  char *lunFile = strjin(lunRoot, (char *)"/file");
  char *lunCdRom = strjin(lunRoot, (char *)"/cdrom");
  char *lunRo = strjin(lunRoot, (char *)"/ro");

  // Disable UDC before making changes
  set_udc((char *)"", gadgetRoot);

  // NEW: If Windows mode is enabled, configure USB descriptors
  if (strcmp(windows_mode, "1") == 0) {
    printf("\n****************************************\n");
    printf("*** WINDOWS ISO MODE ENABLED ***\n");
    printf("****************************************\n");
    
    configure_windows_descriptors(gadgetRoot);
    
    // Force CD-ROM and read-only mode for Windows ISOs
    cdrom = (char *)"1";
    ro = (char *)"1";
    
    printf("Forced CD-ROM mode: enabled\n");
    printf("Forced read-only: enabled\n");
  }

  // Create mass storage function if it doesn't exist
  if (!isdir(massStorageRoot)) {
    mkdir(massStorageRoot, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }

  // Link function to config if not already linked
  if (!isdir(strjin(configRoot, (char *)"/mass_storage.0"))) {
    symlink(massStorageRoot, strjin(configRoot, (char *)"/mass_storage.0"));
  }
  
  // Clear existing LUN file
  sysfs_write(lunFile, (char *)"");
  
  if (strcmp(iso_path, "") != 0) {
    // Set CD-ROM and read-only flags
    sysfs_write(lunCdRom, cdrom);
    sysfs_write(lunRo, ro);
    
    // NEW: Apply Windows-specific mass storage settings
    if (strcmp(windows_mode, "1") == 0) {
      configure_windows_mass_storage(lunRoot);
    }
    
    // Set the ISO file as backing storage
    sysfs_write(lunFile, iso_path);
    
    if (strcmp(windows_mode, "1") == 0) {
      printf("\n****************************************\n");
      printf("Windows ISO mounted successfully!\n");
      printf("Device should be recognized by Windows Setup\n");
      printf("as a bootable CD-ROM drive.\n");
      printf("****************************************\n\n");
    }
  }

  // Re-enable UDC
  set_udc(udc, gadgetRoot);
}

void set_udc(char *udc, char *gadget) {
  char *udcFile = strjin(gadget, (char *)"/UDC");
  sysfs_write(udcFile, udc);
}

char *get_udc() {
  char *gadget_root = get_gadget_root();

  if (gadget_root == nullptr) {
    return nullptr;
  }
  char *udcFile = strjin(gadget_root, (char *)"/UDC");
  return sysfs_read(udcFile);
}
