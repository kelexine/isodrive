#include "simple_test.h"
#include "../src/include/util.h"
#include "../src/include/logger.h"
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <cstring>

namespace fs = std::filesystem;

// Helper to create a temp file
std::string create_temp_file(const std::string& content) {
    std::string filename = "temp_test_file.txt";
    std::ofstream f(filename);
    f << content;
    f.close();
    return filename;
}

// Helper to create a temp dummy ISO with MBR signature
std::string create_dummy_iso(bool valid_signature) {
    std::string filename = "temp_test.iso";
    std::ofstream f(filename, std::ios::binary);
    // Write 512 bytes
    char buffer[512] = {0};
    if (valid_signature) {
        buffer[510] = 0x55;
        buffer[511] = (char)0xAA;
    }
    f.write(buffer, 512);
    f.close();
    return filename;
}

// Helper to create a dummy ISO with ISO 9660 structure
std::string create_iso9660_iso(const std::string& volume_label, bool with_eltorito = true) {
    std::string filename = "temp_test_iso9660.iso";
    std::ofstream f(filename, std::ios::binary);
    
    // ISO sector size
    const int SECTOR_SIZE = 2048;
    char sector[SECTOR_SIZE];
    
    // Write 16 empty sectors (system area + reserved)
    memset(sector, 0, SECTOR_SIZE);
    for (int i = 0; i < 16; i++) {
        f.write(sector, SECTOR_SIZE);
    }
    
    // Sector 16: Primary Volume Descriptor
    memset(sector, 0, SECTOR_SIZE);
    sector[0] = 1;  // Type: Primary Volume Descriptor
    memcpy(sector + 1, "CD001", 5);  // Standard Identifier
    sector[6] = 1;  // Version
    
    // Volume Identifier at offset 40, 32 bytes, space-padded
    std::string vol_id = volume_label;
    vol_id.resize(32, ' ');
    memcpy(sector + 40, vol_id.c_str(), 32);
    
    f.write(sector, SECTOR_SIZE);
    
    // Sector 17: Boot Record (El Torito) if requested
    memset(sector, 0, SECTOR_SIZE);
    if (with_eltorito) {
        sector[0] = 0;  // Type: Boot Record
        memcpy(sector + 1, "CD001", 5);
        sector[6] = 1;  // Version
        memcpy(sector + 7, "EL TORITO SPECIFICATION", 23);
    }
    f.write(sector, SECTOR_SIZE);
    
    // Sector 18: Volume Descriptor Set Terminator
    memset(sector, 0, SECTOR_SIZE);
    sector[0] = (char)255;  // Type: Set Terminator
    memcpy(sector + 1, "CD001", 5);
    f.write(sector, SECTOR_SIZE);
    
    f.close();
    return filename;
}

// =============================================================================
// Original tests
// =============================================================================

TEST(test_sysfs_read_write) {
    std::string filename = "test_sysfs.txt";
    sysfs_write(filename, "hello world");
    
    std::string content = sysfs_read(filename);
    
    // cleanup
    fs::remove(filename);
    
    ASSERT_EQ(std::string("hello"), content); // sysfs_read uses >> which reads up to whitespace
    return true;
}

TEST(test_isfile) {
    std::string filename = create_temp_file("test");
    ASSERT_TRUE(isfile(filename));
    ASSERT_TRUE(!isdir(filename));
    fs::remove(filename);
    ASSERT_TRUE(!isfile(filename));
    return true;
}

TEST(test_isdir) {
    std::string dirname = "test_dir_tmp";
    fs::create_directory(dirname);
    ASSERT_TRUE(isdir(dirname));
    ASSERT_TRUE(!isfile(dirname));
    fs::remove(dirname);
    ASSERT_TRUE(!isdir(dirname));
    return true;
}

TEST(test_is_hybrid_iso_valid) {
    std::string filename = create_dummy_iso(true);
    bool result = is_hybrid_iso(filename);
    fs::remove(filename);
    ASSERT_TRUE(result);
    return true;
}

TEST(test_is_hybrid_iso_invalid) {
    std::string filename = create_dummy_iso(false);
    bool result = is_hybrid_iso(filename);
    fs::remove(filename);
    ASSERT_TRUE(!result);
    return true;
}

// =============================================================================
// New Windows ISO detection tests
// =============================================================================

TEST(test_is_windows_iso_win10) {
    std::string filename = create_iso9660_iso("WIN10_X64_EN");
    bool result = is_windows_iso(filename);
    fs::remove(filename);
    ASSERT_TRUE(result);
    return true;
}

TEST(test_is_windows_iso_win11) {
    std::string filename = create_iso9660_iso("WIN11_22H2_X64");
    bool result = is_windows_iso(filename);
    fs::remove(filename);
    ASSERT_TRUE(result);
    return true;
}

TEST(test_is_windows_iso_cccoma) {
    // Media Creation Tool style label
    std::string filename = create_iso9660_iso("CCCOMA_X64FRE_EN");
    bool result = is_windows_iso(filename);
    fs::remove(filename);
    ASSERT_TRUE(result);
    return true;
}

TEST(test_is_windows_iso_negative) {
    std::string filename = create_iso9660_iso("UBUNTU_22_04");
    bool result = is_windows_iso(filename);
    fs::remove(filename);
    ASSERT_TRUE(!result);
    return true;
}

TEST(test_get_windows_version_win10) {
    std::string filename = create_iso9660_iso("WIN10_21H2_X64");
    WindowsVersion ver = get_windows_version(filename);
    fs::remove(filename);
    ASSERT_TRUE(ver == WindowsVersion::WIN10);
    return true;
}

TEST(test_get_windows_version_win11) {
    std::string filename = create_iso9660_iso("WIN11_23H2_X64");
    WindowsVersion ver = get_windows_version(filename);
    fs::remove(filename);
    ASSERT_TRUE(ver == WindowsVersion::WIN11);
    return true;
}

TEST(test_get_windows_version_unknown) {
    // CCCOMA style doesn't have version in label
    std::string filename = create_iso9660_iso("CCCOMA_X64FRE_EN");
    WindowsVersion ver = get_windows_version(filename);
    fs::remove(filename);
    ASSERT_TRUE(ver == WindowsVersion::WIN_UNKNOWN);
    return true;
}

TEST(test_get_windows_iso_info_full) {
    std::string filename = create_iso9660_iso("WIN11_23H2_X64", true);
    WindowsIsoInfo info = get_windows_iso_info(filename);
    fs::remove(filename);
    
    ASSERT_TRUE(info.is_windows);
    ASSERT_TRUE(info.version == WindowsVersion::WIN11);
    ASSERT_TRUE(info.has_legacy);  // El Torito present
    ASSERT_EQ(std::string("WIN11_23H2_X64"), info.volume_label);
    return true;
}

TEST(test_windows_version_to_string) {
    ASSERT_EQ(std::string("Windows 10"), windows_version_to_string(WindowsVersion::WIN10));
    ASSERT_EQ(std::string("Windows 11"), windows_version_to_string(WindowsVersion::WIN11));
    ASSERT_EQ(std::string("Windows (unknown version)"), windows_version_to_string(WindowsVersion::WIN_UNKNOWN));
    ASSERT_EQ(std::string("Not Windows"), windows_version_to_string(WindowsVersion::NONE));
    return true;
}

int main() {
    // Suppress log output during tests
    log_set_level(LogLevel::SILENT);
    
    return run_tests();
}
