/**
 * @file compression.hpp
 * @brief Compression and decompression functionality for DataPak archives
 * @author DataPak Team
 */

#pragma once

#include "format.hpp"
#include <vector>
#include <cstddef>
#include <expected>
#include <string>

namespace dp {

/**
 * @brief Error codes returned by compression operations
 */
enum class compression_error {
    invalid_method,      /**< Unsupported compression method specified */
    compression_failed,  /**< Compression operation failed */
    decompression_failed,/**< Decompression operation failed */
    buffer_too_small     /**< Output buffer is too small for operation */
};

/**
 * @brief Static compression engine providing compress/decompress functionality
 *
 * This class provides static methods for compressing and decompressing data
 * using various compression algorithms supported by the DataPak format.
 */
class compression_engine {
public:
    /**
     * @brief Compress data using the specified compression method
     * @param data The input data to compress
     * @param method The compression method to use
     * @return Expected containing compressed data on success, or compression_error on failure
     */
    static std::expected<std::vector<std::byte>, compression_error>
    compress(const std::vector<std::byte>& data, compression_method method);

    /**
     * @brief Decompress data using the specified compression method
     * @param compressed_data The compressed input data
     * @param method The compression method that was used
     * @param uncompressed_size The expected size of uncompressed data
     * @return Expected containing decompressed data on success, or compression_error on failure
     */
    static std::expected<std::vector<std::byte>, compression_error>
    decompress(const std::vector<std::byte>& compressed_data,
              compression_method method,
              std::size_t uncompressed_size);

private:
    /**
     * @brief Compress data using DEFLATE algorithm
     * @param data The input data to compress
     * @return Expected containing compressed data on success, or compression_error on failure
     */
    static std::expected<std::vector<std::byte>, compression_error>
    deflate_compress(const std::vector<std::byte>& data);

    /**
     * @brief Decompress DEFLATE-compressed data
     * @param compressed_data The compressed input data
     * @param uncompressed_size The expected size of uncompressed data
     * @return Expected containing decompressed data on success, or compression_error on failure
     */
    static std::expected<std::vector<std::byte>, compression_error>
    deflate_decompress(const std::vector<std::byte>& compressed_data,
                      std::size_t uncompressed_size);
};

} // namespace dp