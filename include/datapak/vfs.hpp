/**
 * @file vfs.hpp
 * @brief Virtual File System for managing multiple DataPak archives
 * @author DataPak Team
 */

#pragma once

#include "archive.hpp"
#include "vfstream.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string_view>
#include <filesystem>

namespace dp {

/**
 * @brief Error codes returned by VFS operations
 */
enum class vfs_error {
    archive_error,   /**< Error occurred with underlying archive */
    file_not_found,  /**< Requested file not found in any mounted archive */
    cache_error      /**< Error occurred with cache operations */
};

/**
 * @brief Search order for file lookup across multiple archives
 */
enum class search_order {
    mount_order,        /**< Search in order archives were mounted (first mounted has priority) */
    reverse_mount_order /**< Search in reverse order (most recently mounted has priority) */
};

/**
 * @brief Virtual File System for DataPak archives
 *
 * This class provides a unified interface for accessing files across
 * multiple mounted DataPak archives. It supports file caching and
 * configurable search ordering for handling overlapping files.
 */
class vfs {
public:
    /**
     * @brief Default constructor
     */
    vfs() = default;

    /**
     * @brief Default destructor
     */
    ~vfs() = default;

    // Disable copy operations
    vfs(const vfs&) = delete;
    vfs& operator=(const vfs&) = delete;

    // Enable move operations
    vfs(vfs&&) noexcept = default;
    vfs& operator=(vfs&&) noexcept = default;

    /**
     * @brief Mount a DataPak archive into the virtual file system
     * @param archive_path Path to the DataPak archive file
     * @param mode Access mode for the archive (disk or memory)
     * @return Expected void on success, or vfs_error on failure
     */
    std::expected<void, vfs_error>
    mount(const std::filesystem::path& archive_path, access_mode mode = access_mode::disk);

    /**
     * @brief Open a file from any mounted archive as a virtual stream
     * @param filename The virtual path of the file to open
     * @return Expected containing vfstream on success, or vfs_error on failure
     */
    std::expected<std::unique_ptr<vfstream>, vfs_error>
    open(std::string_view filename);

    /**
     * @brief Check if any mounted archive contains the specified file
     * @param filename The virtual path to check
     * @return True if the file exists in any archive, false otherwise
     */
    bool contains(std::string_view filename) const;

    /**
     * @brief Get a list of all files from all mounted archives
     * @return Vector of virtual file paths from all archives
     */
    std::vector<std::string> list_files() const;

    /**
     * @brief Enable or disable file caching
     * @param enable True to enable caching, false to disable
     */
    void enable_cache(bool enable = true) { cache_enabled_ = enable; }

    /**
     * @brief Clear all cached file data
     */
    void clear_cache() { cache_.clear(); }

    /**
     * @brief Get the current cache size (number of cached files)
     * @return Number of files currently in cache
     */
    std::size_t cache_size() const { return cache_.size(); }

    /**
     * @brief Set the search order for file lookups
     * @param order The search order to use
     */
    void set_search_order(search_order order) { search_order_ = order; }

    /**
     * @brief Get the current search order
     * @return The current search order setting
     */
    search_order get_search_order() const { return search_order_; }

private:
    std::vector<std::unique_ptr<archive>> archives_;                        /**< Mounted archives */
    bool cache_enabled_ = true;                                             /**< Cache enable flag */
    search_order search_order_ = search_order::reverse_mount_order;         /**< Default: most recent first */
    mutable std::unordered_map<std::string, std::vector<std::byte>> cache_; /**< File data cache */
};

} // namespace dp