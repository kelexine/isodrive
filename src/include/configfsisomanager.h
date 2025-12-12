#ifndef CONFIGFSISOMANAGER_H
#define CONFIGFSISOMANAGER_H

#include <string>

bool supported();
std::string get_gadget_root();
std::string get_config_root();

void mount_iso(const std::string& iso_path, bool cdrom, bool ro, bool windows_mode);
void set_udc(const std::string& udc, const std::string& gadget);
std::string get_udc();

#endif // ifndef CONFIGFSISOMANAGER_H
