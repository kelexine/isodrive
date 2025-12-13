#include "simple_test.h"
#include "mock_sysfs.h"
#include "../src/include/androidusbisomanager.h"
#include "../src/include/logger.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// Test helper: create a temp directory structure for Android sysfs
class AndroidSysfsSim {
public:
    std::string base_path;
    
    AndroidSysfsSim() : base_path("/tmp/isodrive_android_test") {
        fs::create_directories(base_path + "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun");
    }
    
    ~AndroidSysfsSim() {
        fs::remove_all(base_path);
    }
    
    std::string create_enable_file(const std::string& value = "0") {
        std::string path = base_path + "/sys/devices/virtual/android_usb/android0/enable";
        std::ofstream f(path);
        f << value;
        f.close();
        return path;
    }
    
    std::string create_functions_file(const std::string& value = "mtp") {
        std::string path = base_path + "/sys/devices/virtual/android_usb/android0/functions";
        std::ofstream f(path);
        f << value;
        f.close();
        return path;
    }
    
    std::string create_img_file(const std::string& value = "") {
        std::string path = base_path + "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun/file";
        std::ofstream f(path);
        f << value;
        f.close();
        return path;
    }
    
    std::string read_file(const std::string& path) {
        std::ifstream f(path);
        std::string value;
        f >> value;
        return value;
    }
};

// ============================================================================
// Tests for mock sysfs with Android paths
// ============================================================================

TEST(test_android_sysfs_mock_enable) {
    mock_sysfs::init();
    
    mock_sysfs::set(ANDROID0_SYSFS_ENABLE, "0");
    ASSERT_EQ(std::string("0"), mock_sysfs::get(ANDROID0_SYSFS_ENABLE));
    
    mock_sysfs::set(ANDROID0_SYSFS_ENABLE, "1");
    ASSERT_EQ(std::string("1"), mock_sysfs::get(ANDROID0_SYSFS_ENABLE));
    
    mock_sysfs::cleanup();
    return true;
}

TEST(test_android_sysfs_mock_functions) {
    mock_sysfs::init();
    
    mock_sysfs::set(ANDROID0_SYSFS_FEATURES, "mtp");
    ASSERT_EQ(std::string("mtp"), mock_sysfs::get(ANDROID0_SYSFS_FEATURES));
    
    mock_sysfs::set(ANDROID0_SYSFS_FEATURES, "mass_storage");
    ASSERT_EQ(std::string("mass_storage"), mock_sysfs::get(ANDROID0_SYSFS_FEATURES));
    
    mock_sysfs::cleanup();
    return true;
}

TEST(test_android_sysfs_mock_img_file) {
    mock_sysfs::init();
    
    std::string test_iso = "/data/local/tmp/test.iso";
    mock_sysfs::set(ANDROID0_SYSFS_IMG_FILE, "");
    ASSERT_EQ(std::string(""), mock_sysfs::get(ANDROID0_SYSFS_IMG_FILE));
    
    mock_sysfs::set(ANDROID0_SYSFS_IMG_FILE, test_iso);
    ASSERT_EQ(test_iso, mock_sysfs::get(ANDROID0_SYSFS_IMG_FILE));
    
    mock_sysfs::cleanup();
    return true;
}

// ============================================================================
// Tests for simulated filesystem operations
// ============================================================================

TEST(test_android_sysfs_sim_structure) {
    AndroidSysfsSim sim;
    
    std::string enable_path = sim.create_enable_file("0");
    std::string functions_path = sim.create_functions_file("mtp");
    std::string img_path = sim.create_img_file("");
    
    ASSERT_TRUE(fs::exists(enable_path));
    ASSERT_TRUE(fs::exists(functions_path));
    ASSERT_TRUE(fs::exists(img_path));
    
    ASSERT_EQ(std::string("0"), sim.read_file(enable_path));
    ASSERT_EQ(std::string("mtp"), sim.read_file(functions_path));
    
    return true;
}

TEST(test_android_sysfs_sim_state_changes) {
    AndroidSysfsSim sim;
    
    std::string enable_path = sim.create_enable_file("0");
    
    // Simulate state change: disabled -> enabled
    ASSERT_EQ(std::string("0"), sim.read_file(enable_path));
    
    // Write new value
    std::ofstream f(enable_path);
    f << "1";
    f.close();
    
    ASSERT_EQ(std::string("1"), sim.read_file(enable_path));
    
    return true;
}

TEST(test_android_sysfs_sim_full_workflow) {
    AndroidSysfsSim sim;
    
    // Create all required files
    std::string enable_path = sim.create_enable_file("1");
    std::string functions_path = sim.create_functions_file("mtp");
    std::string img_path = sim.create_img_file("");
    
    // Simulate mount workflow:
    // 1. Disable USB
    {
        std::ofstream f(enable_path);
        f << "0";
    }
    ASSERT_EQ(std::string("0"), sim.read_file(enable_path));
    
    // 2. Set image file
    {
        std::ofstream f(img_path);
        f << "/path/to/test.iso";
    }
    ASSERT_EQ(std::string("/path/to/test.iso"), sim.read_file(img_path));
    
    // 3. Set function to mass_storage
    {
        std::ofstream f(functions_path);
        f << "mass_storage";
    }
    ASSERT_EQ(std::string("mass_storage"), sim.read_file(functions_path));
    
    // 4. Re-enable USB
    {
        std::ofstream f(enable_path);
        f << "1";
    }
    ASSERT_EQ(std::string("1"), sim.read_file(enable_path));
    
    return true;
}

TEST(test_android_sysfs_sim_reset_workflow) {
    AndroidSysfsSim sim;
    
    // Create files in "mounted" state
    std::string enable_path = sim.create_enable_file("1");
    std::string functions_path = sim.create_functions_file("mass_storage");
    std::string img_path = sim.create_img_file("/path/to/mounted.iso");
    
    // Simulate reset workflow:
    // 1. Clear image
    {
        std::ofstream f(img_path);
        f << "";
    }
    
    // 2. Disable USB
    {
        std::ofstream f(enable_path);
        f << "0";
    }
    
    // 3. Restore MTP function
    {
        std::ofstream f(functions_path);
        f << "mtp";
    }
    ASSERT_EQ(std::string("mtp"), sim.read_file(functions_path));
    
    // 4. Re-enable USB
    {
        std::ofstream f(enable_path);
        f << "1";
    }
    ASSERT_EQ(std::string("1"), sim.read_file(enable_path));
    
    return true;
}

int main() {
    // Suppress log output during tests
    log_set_level(LogLevel::SILENT);
    
    return run_tests();
}
