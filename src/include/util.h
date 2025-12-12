#ifndef UTIL_H
#define UTIL_H

#include <string>

std::string fs_mount_point(const std::string& filesystem_type);
bool isdir(const std::string& path);
bool isfile(const std::string& path);
bool is_hybrid_iso(const std::string& path);
std::string sysfs_read(const std::string& path);
void sysfs_write(const std::string& path, const std::string& content);

#endif // ifndef UTIL_H