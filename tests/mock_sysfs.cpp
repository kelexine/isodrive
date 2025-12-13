#include "mock_sysfs.h"
#include <unordered_map>
#include <functional>

namespace mock_sysfs {

namespace {
    std::unordered_map<std::string, std::string> g_mock_fs;
    std::function<void(const std::string&, const std::string&)> g_write_callback;
}

void init() {
    g_mock_fs.clear();
    g_write_callback = nullptr;
}

void cleanup() {
    g_mock_fs.clear();
    g_write_callback = nullptr;
}

void set(const std::string& path, const std::string& value) {
    g_mock_fs[path] = value;
}

std::string get(const std::string& path) {
    auto it = g_mock_fs.find(path);
    if (it != g_mock_fs.end()) {
        return it->second;
    }
    return "";
}

bool exists(const std::string& path) {
    return g_mock_fs.find(path) != g_mock_fs.end();
}

const std::unordered_map<std::string, std::string>& get_all() {
    return g_mock_fs;
}

void clear() {
    g_mock_fs.clear();
}

void set_write_callback(std::function<void(const std::string&, const std::string&)> callback) {
    g_write_callback = callback;
}

void clear_write_callback() {
    g_write_callback = nullptr;
}

// These are used internally when mock mode is enabled
void mock_write(const std::string& path, const std::string& value) {
    g_mock_fs[path] = value;
    if (g_write_callback) {
        g_write_callback(path, value);
    }
}

std::string mock_read(const std::string& path) {
    return get(path);
}

} // namespace mock_sysfs
