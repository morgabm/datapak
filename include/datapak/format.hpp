/**
 * @file format.hpp
 * @brief Core format definitions for the DataPak archive format
 * @author DataPak Team
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dp {

/** @brief Magic number identifying DataPak archive files ("PAKF") */
constexpr std::uint32_t MAGIC_NUMBER = 0x50414B46; // "PAKF"

/** @brief Current format version */
constexpr std::uint32_t FORMAT_VERSION = 1;

/**
 * @brief Supported compression methods for archive entries
 */
enum class compression_method : std::uint8_t {
    none = 0,    /**< No compression */
    deflate = 1, /**< DEFLATE compression (zlib) */
    zstd = 2     /**< Zstandard compression */
};

/**
 * @brief Archive file header structure
 *
 * This structure appears at the beginning of every DataPak archive file
 * and contains metadata about the archive format and directory location.
 */
struct archive_header {
    std::uint32_t magic;            /**< Magic number (MAGIC_NUMBER) */
    std::uint32_t version;          /**< Format version (FORMAT_VERSION) */
    std::uint64_t directory_offset; /**< Byte offset to directory table */
    std::uint32_t directory_count;  /**< Number of entries in directory */
    std::uint32_t reserved;         /**< Reserved for future use */
};

/**
 * @brief Directory entry describing a file within the archive
 *
 * Each file stored in the archive has a corresponding directory entry
 * that describes its location, size, and compression settings.
 */
struct directory_entry {
    std::string filename;                /**< Virtual file path within archive */
    std::uint64_t data_offset;           /**< Byte offset to compressed data */
    std::uint64_t compressed_size;       /**< Size of compressed data in bytes */
    std::uint64_t uncompressed_size;     /**< Size of uncompressed data in bytes */
    compression_method compression;      /**< Compression method used */
};

} // namespace dp