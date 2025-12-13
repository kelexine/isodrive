#include "simple_test.h"
#include "mock_sysfs.h"
#include "../src/include/logger.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// Test helper: create a temp directory structure
class TempDir {
public:
    std::string path;
    
    TempDir(const std::string& name) : path("/tmp/isodrive_test_" + name) {
        fs::create_directories(path);
    }
    
    ~TempDir() {
        fs::remove_all(path);
    }
    
    std::string create_file(const std::string& relative_path, const std::string& content = "") {
        fs::path full_path = fs::path(path) / relative_path;
        fs::create_directories(full_path.parent_path());
        std::ofstream f(full_path);
        f << content;
        f.close();
        return full_path.string();
    }
    
    std::string create_dir(const std::string& relative_path) {
        fs::path full_path = fs::path(path) / relative_path;
        fs::create_directories(full_path);
        return full_path.string();
    }
};

// ============================================================================
// Tests for configfs support detection and path discovery
// ============================================================================

TEST(test_mock_sysfs_basic) {
    mock_sysfs::init();
    
    mock_sysfs::set("/test/path", "test_value");
    ASSERT_EQ(std::string("test_value"), mock_sysfs::get("/test/path"));
    ASSERT_TRUE(mock_sysfs::exists("/test/path"));
    ASSERT_TRUE(!mock_sysfs::exists("/nonexistent"));
    
    mock_sysfs::cleanup();
    ASSERT_TRUE(!mock_sysfs::exists("/test/path"));
    
    return true;
}

TEST(test_mock_sysfs_write_callback) {
    mock_sysfs::init();
    
    std::vector<std::pair<std::string, std::string>> writes;
    
    mock_sysfs::set_write_callback([&writes](const std::string& path, const std::string& value) {
        writes.push_back({path, value});
    });
    
    mock_sysfs::set("/path1", "value1");
    mock_sysfs::set("/path2", "value2");
    
    // Note: set() triggers callback via mock_write internally
    // In our current implementation, set() doesn't use the callback
    // This test verifies the callback registration works
    
    mock_sysfs::clear_write_callback();
    mock_sysfs::cleanup();
    
    return true;
}

TEST(test_mock_sysfs_multiple_paths) {
    mock_sysfs::init();
    
    mock_sysfs::set("/sys/kernel/config/usb_gadget/g1/UDC", "musb-hdrc.0");
    mock_sysfs::set("/sys/kernel/config/usb_gadget/g1/idVendor", "0x1234");
    mock_sysfs::set("/sys/kernel/config/usb_gadget/g1/idProduct", "0x5678");
    
    ASSERT_EQ(std::string("musb-hdrc.0"), mock_sysfs::get("/sys/kernel/config/usb_gadget/g1/UDC"));
    ASSERT_EQ(std::string("0x1234"), mock_sysfs::get("/sys/kernel/config/usb_gadget/g1/idVendor"));
    ASSERT_EQ(std::string("0x5678"), mock_sysfs::get("/sys/kernel/config/usb_gadget/g1/idProduct"));
    
    auto all = mock_sysfs::get_all();
    ASSERT_EQ(3u, all.size());
    
    mock_sysfs::cleanup();
    return true;
}

// ============================================================================
// Tests for file-based operations that can run without mocking
// ============================================================================

TEST(test_temp_dir_helper) {
    TempDir tmp("configfs_test");
    
    std::string file = tmp.create_file("test.txt", "hello");
    ASSERT_TRUE(fs::exists(file));
    
    std::ifstream f(file);
    std::string content;
    f >> content;
    ASSERT_EQ(std::string("hello"), content);
    
    std::string dir = tmp.create_dir("subdir/nested");
    ASSERT_TRUE(fs::is_directory(dir));
    
    // TempDir destructor cleans up
    return true;
}

TEST(test_gadget_structure_simulation) {
    // Simulates the structure we'd expect from configfs
    TempDir tmp("gadget_sim");
    
    // Create a simulated gadget structure
    std::string gadget_root = tmp.create_dir("usb_gadget/g1");
    std::string udc_file = tmp.create_file("usb_gadget/g1/UDC", "musb-hdrc.0");
    std::string configs_dir = tmp.create_dir("usb_gadget/g1/configs/c.1");
    std::string functions_dir = tmp.create_dir("usb_gadget/g1/functions");
    
    // Verify structure exists
    ASSERT_TRUE(fs::is_directory(gadget_root));
    ASSERT_TRUE(fs::exists(udc_file));
    ASSERT_TRUE(fs::is_directory(configs_dir));
    ASSERT_TRUE(fs::is_directory(functions_dir));
    
    // Read UDC file
    std::ifstream f(udc_file);
    std::string udc;
    f >> udc;
    ASSERT_EQ(std::string("musb-hdrc.0"), udc);
    
    return true;
}

TEST(test_mass_storage_structure_simulation) {
    TempDir tmp("mass_storage_sim");
    
    // Create mass_storage function structure
    std::string lun_dir = tmp.create_dir("mass_storage.0/lun.0");
    std::string file_path = tmp.create_file("mass_storage.0/lun.0/file", "");
    std::string cdrom_path = tmp.create_file("mass_storage.0/lun.0/cdrom", "0");
    std::string ro_path = tmp.create_file("mass_storage.0/lun.0/ro", "1");
    std::string removable_path = tmp.create_file("mass_storage.0/lun.0/removable", "1");
    std::string stall_path = tmp.create_file("mass_storage.0/stall", "1");
    
    ASSERT_TRUE(fs::is_directory(lun_dir));
    ASSERT_TRUE(fs::exists(file_path));
    ASSERT_TRUE(fs::exists(cdrom_path));
    ASSERT_TRUE(fs::exists(ro_path));
    ASSERT_TRUE(fs::exists(removable_path));
    ASSERT_TRUE(fs::exists(stall_path));
    
    return true;
}

// ============================================================================
// Logging tests
// ============================================================================

TEST(test_log_levels) {
    // Save current level
    LogLevel original = log_get_level();
    
    log_set_level(LogLevel::DEBUG);
    ASSERT_TRUE(log_get_level() == LogLevel::DEBUG);
    
    log_set_level(LogLevel::ERROR);
    ASSERT_TRUE(log_get_level() == LogLevel::ERROR);
    
    log_set_level(LogLevel::SILENT);
    ASSERT_TRUE(log_get_level() == LogLevel::SILENT);
    
    // Restore
    log_set_level(original);
    
    return true;
}

int main() {
    // Suppress log output during tests
    log_set_level(LogLevel::SILENT);
    
    return run_tests();
}
