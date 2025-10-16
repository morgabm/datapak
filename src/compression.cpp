#include "datapak/compression.hpp"
#include <zlib.h>
#include <cstring>
#include <array>

namespace dp {

std::expected<std::vector<std::byte>, compression_error>
compression_engine::compress(const std::vector<std::byte>& data, compression_method method) {
    switch (method) {
    case compression_method::none:
        return data;
    case compression_method::deflate:
        return deflate_compress(data);
    default:
        return std::unexpected{compression_error::invalid_method};
    }
}

std::expected<std::vector<std::byte>, compression_error>
compression_engine::decompress(const std::vector<std::byte>& compressed_data,
                               compression_method method,
                               std::size_t uncompressed_size) {
    switch (method) {
    case compression_method::none:
        return compressed_data;
    case compression_method::deflate:
        return deflate_decompress(compressed_data, uncompressed_size);
    default:
        return std::unexpected{compression_error::invalid_method};
    }
}

std::expected<std::vector<std::byte>, compression_error>
compression_engine::deflate_compress(const std::vector<std::byte>& data) {
    z_stream stream{};

    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        return std::unexpected{compression_error::compression_failed};
    }

    const auto cleanup = [&stream]() { deflateEnd(&stream); };

    stream.avail_in = static_cast<uInt>(data.size());
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data.data()));

    std::vector<std::byte> compressed;
    compressed.reserve(data.size());

    constexpr std::size_t chunk_size = 16384;
    std::array<std::byte, chunk_size> buffer;

    int result;
    do {
        stream.avail_out = chunk_size;
        stream.next_out = reinterpret_cast<Bytef*>(buffer.data());

        result = deflate(&stream, Z_FINISH);

        if (result == Z_STREAM_ERROR) {
            cleanup();
            return std::unexpected{compression_error::compression_failed};
        }

        const auto bytes_written = chunk_size - stream.avail_out;
        compressed.insert(compressed.end(), buffer.begin(), buffer.begin() + bytes_written);

    } while (stream.avail_out == 0);

    cleanup();

    if (result != Z_STREAM_END) {
        return std::unexpected{compression_error::compression_failed};
    }

    return compressed;
}

std::expected<std::vector<std::byte>, compression_error>
compression_engine::deflate_decompress(const std::vector<std::byte>& compressed_data,
                                       std::size_t uncompressed_size) {
    z_stream stream{};

    if (inflateInit(&stream) != Z_OK) {
        return std::unexpected{compression_error::decompression_failed};
    }

    const auto cleanup = [&stream]() { inflateEnd(&stream); };

    stream.avail_in = static_cast<uInt>(compressed_data.size());
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(compressed_data.data()));

    std::vector<std::byte> decompressed;
    decompressed.reserve(uncompressed_size);

    constexpr std::size_t chunk_size = 16384;
    std::array<std::byte, chunk_size> buffer;

    int result;
    do {
        stream.avail_out = chunk_size;
        stream.next_out = reinterpret_cast<Bytef*>(buffer.data());

        result = inflate(&stream, Z_NO_FLUSH);

        if (result == Z_STREAM_ERROR || result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
            cleanup();
            return std::unexpected{compression_error::decompression_failed};
        }

        const auto bytes_written = chunk_size - stream.avail_out;
        decompressed.insert(decompressed.end(), buffer.begin(), buffer.begin() + bytes_written);

    } while (stream.avail_out == 0 && result != Z_STREAM_END);

    cleanup();

    if (result != Z_STREAM_END && stream.avail_in > 0) {
        return std::unexpected{compression_error::decompression_failed};
    }

    return decompressed;
}

} // namespace dp