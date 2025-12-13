#ifndef MOCK_SYSFS_H
#define MOCK_SYSFS_H

#include <string>
#include <unordered_map>
#include <functional>

/**
 * @file mock_sysfs.h
 * @brief Mock filesystem for testing sysfs/configfs operations.
 * 
 * Provides an in-memory filesystem simulation that allows testing
 * ISO manager modules without requiring actual kernel interfaces.
 * 
 * Usage:
 *   1. Call mock_sysfs_init() before tests
 *   2. Use mock_sysfs_set() to pre-populate paths
 *   3. Run tests (sysfs_read/sysfs_write will use mock)
 *   4. Use mock_sysfs_get() to verify written values
 *   5. Call mock_sysfs_cleanup() after tests
 */

namespace mock_sysfs {

/**
 * @brief Initialize the mock filesystem.
 * Clears any existing mock data.
 */
void init();

/**
 * @brief Clean up the mock filesystem.
 * Clears all mock data.
 */
void cleanup();

/**
 * @brief Set a value in the mock filesystem.
 * 
 * @param path The path to set.
 * @param value The value to store at that path.
 */
void set(const std::string& path, const std::string& value);

/**
 * @brief Get a value from the mock filesystem.
 * 
 * @param path The path to read.
 * @return The stored value, or empty string if not found.
 */
std::string get(const std::string& path);

/**
 * @brief Check if a path exists in the mock filesystem.
 * 
 * @param path The path to check.
 * @return true if the path exists, false otherwise.
 */
bool exists(const std::string& path);

/**
 * @brief Get all paths in the mock filesystem.
 * 
 * @return Map of all paths and their values.
 */
const std::unordered_map<std::string, std::string>& get_all();

/**
 * @brief Clear all mock data.
 */
void clear();

/**
 * @brief Set a callback for write operations.
 * 
 * Allows tests to intercept and verify write operations.
 * 
 * @param callback Function called with (path, value) on each write.
 */
void set_write_callback(std::function<void(const std::string&, const std::string&)> callback);

/**
 * @brief Clear the write callback.
 */
void clear_write_callback();

} // namespace mock_sysfs

#endif // MOCK_SYSFS_H
