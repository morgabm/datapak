/**
 * @file archive_builder.hpp
 * @brief DataPak archive creation and building interface
 * @author DataPak Team
 */

#pragma once

#include "format.hpp"
#include "compression.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <expected>

namespace dp {

/**
 * @brief Error codes returned by archive builder operations
 */
enum class builder_error {
    file_not_found,    /**< Source file does not exist */
    write_error,       /**< I/O error occurred while writing */
    compression_error, /**< Error during compression */
    invalid_path       /**< Invalid source or output path */
};

/**
 * @brief Represents a file to be added to an archive
 */
struct file_entry {
    std::filesystem::path source_path; /**< Path to source file on disk */
    std::string archive_path;          /**< Virtual path within the archive */
    compression_method compression;    /**< Compression method to use */
};

/**
 * @brief Builder for creating DataPak archives
 *
 * This class provides functionality to build DataPak archive files
 * from source files and directories. Files can be added individually
 * or entire directories can be added recursively.
 */
class archive_builder {
public:
    /**
     * @brief Construct archive builder with default compression
     * @param default_compression Default compression method for added files
     */
    explicit archive_builder(compression_method default_compression = compression_method::deflate);

    /**
     * @brief Add a single file to the archive
     * @param source_path Path to the source file on disk
     * @param archive_path Virtual path for the file within the archive
     * @param compression Compression method to use (none means use default)
     */
    void add_file(const std::filesystem::path& source_path,
                  const std::string& archive_path,
                  compression_method compression = compression_method::none);

    /**
     * @brief Add all files from a directory recursively
     * @param directory_path Path to source directory on disk
     * @param archive_prefix Prefix to prepend to archive paths
     * @param compression Compression method to use (none means use default)
     */
    void add_directory(const std::filesystem::path& directory_path,
                      const std::string& archive_prefix = "",
                      compression_method compression = compression_method::none);

    /**
     * @brief Build the archive file from added entries
     * @param output_path Path where the archive file should be created
     * @return Expected void on success, or builder_error on failure
     */
    std::expected<void, builder_error>
    build(const std::filesystem::path& output_path);

    /**
     * @brief Set the default compression method for new files
     * @param compression The compression method to use as default
     */
    void set_default_compression(compression_method compression) {
        default_compression_ = compression;
    }

    /**
     * @brief Get the number of files currently added to the builder
     * @return Number of files that will be included in the archive
     */
    std::size_t file_count() const { return files_.size(); }

private:
    std::vector<file_entry> files_;             /**< List of files to include in archive */
    compression_method default_compression_;    /**< Default compression method */
};

} // namespace dp