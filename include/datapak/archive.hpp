/**
 * @file archive.hpp
 * @brief DataPak archive reader interface
 * @author DataPak Team
 */

#pragma once

#include "format.hpp"
#include "compression.hpp"
#include "vfstream.hpp"
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <expected>
#include <string_view>

namespace dp {

/**
 * @brief Error codes returned by archive operations
 */
enum class archive_error {
    file_not_found,    /**< Archive file does not exist */
    invalid_format,    /**< File is not a valid DataPak archive */
    read_error,        /**< I/O error occurred while reading */
    compression_error, /**< Error during decompression */
    entry_not_found    /**< Requested file not found in archive */
};

/**
 * @brief Archive access modes
 */
enum class access_mode {
    disk,   /**< Read archive data from disk as needed */
    memory  /**< Load entire archive into memory for faster access */
};

/**
 * @brief DataPak archive reader
 *
 * This class provides read-only access to DataPak archive files.
 * It can open files from the archive as virtual streams and provides
 * methods to query archive contents.
 */
class archive {
public:
    /**
     * @brief Construct archive reader for the specified file
     * @param path Path to the DataPak archive file
     * @param mode Access mode (disk or memory)
     */
    explicit archive(const std::filesystem::path& path, access_mode mode = access_mode::disk);

    /**
     * @brief Default destructor
     */
    ~archive() = default;

    // Disable copy operations
    archive(const archive&) = delete;
    archive& operator=(const archive&) = delete;

    // Enable move operations
    archive(archive&&) noexcept = default;
    archive& operator=(archive&&) noexcept = default;

    /**
     * @brief Open a file from the archive as a virtual stream
     * @param filename The virtual path of the file within the archive
     * @return Expected containing vfstream on success, or archive_error on failure
     */
    std::expected<std::unique_ptr<vfstream>, archive_error>
    open(std::string_view filename) const;

    /**
     * @brief Check if the archive contains a specific file
     * @param filename The virtual path to check
     * @return True if the file exists in the archive, false otherwise
     */
    bool contains(std::string_view filename) const;

    /**
     * @brief Get a list of all files in the archive
     * @return Vector of virtual file paths within the archive
     */
    std::vector<std::string> list_files() const;

    /**
     * @brief Create an archive reader from file path
     * @param path Path to the DataPak archive file
     * @return Expected containing archive on success, or archive_error on failure
     */
    static std::expected<archive, archive_error>
    create(const std::filesystem::path& path);

private:
    /**
     * @brief Load the archive directory from file
     * @return Expected void on success, or archive_error on failure
     */
    std::expected<void, archive_error> load_directory();

    /**
     * @brief Read and decompress file data for a directory entry
     * @param entry The directory entry describing the file
     * @return Expected containing decompressed file data on success, or archive_error on failure
     */
    std::expected<std::vector<std::byte>, archive_error>
    read_file_data(const directory_entry& entry) const;

    std::filesystem::path path_;                                    /**< Path to archive file */
    access_mode mode_;                                              /**< Access mode */
    mutable std::ifstream file_stream_;                             /**< File stream for disk access */
    std::vector<std::byte> memory_data_;                            /**< Memory buffer for memory access */
    std::unordered_map<std::string, directory_entry> directory_;   /**< Archive directory */
};

} // namespace dp