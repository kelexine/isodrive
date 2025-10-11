#ifndef CONFIGFSISOMANAGER_H
#define CONFIGFSISOMANAGER_H

bool supported();
char *get_gadget_root();
char *get_config_root();

// Updated to include windows_mode parameter
void mount_iso(char *iso_path, char *cdrom, char *ro, char *windows_mode);
void umount_iso();
void set_udc(char *udc, char *gadget);
char *get_udc();

// New helper functions for Windows compatibility
void configure_windows_descriptors(char *gadgetRoot);
void configure_windows_mass_storage(char *lunRoot);

#endif // ifndef CONFIGFSISOMANAGER_H
