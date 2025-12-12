#include "simple_test.h"
#include "../src/include/util.h"
#include <fstream>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

// Helper to create a temp file
std::string create_temp_file(const std::string& content) {
    std::string filename = "temp_test_file.txt";
    std::ofstream f(filename);
    f << content;
    f.close();
    return filename;
}

// Helper to create a temp dummy ISO
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

int main() {
    return run_tests();
}
