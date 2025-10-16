#include "datapak/archive_builder.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

namespace dp {

archive_builder::archive_builder(compression_method default_compression)
    : default_compression_(default_compression) {}

void archive_builder::add_file(const std::filesystem::path& source_path,
                              const std::string& archive_path,
                              compression_method compression) {
    if (compression == compression_method::none) {
        compression = default_compression_;
    }

    files_.emplace_back(source_path, archive_path, compression);
}

void archive_builder::add_directory(const std::filesystem::path& directory_path,
                                   const std::string& archive_prefix,
                                   compression_method compression) {
    if (!std::filesystem::exists(directory_path) || !std::filesystem::is_directory(directory_path)) {
        return;
    }

    if (compression == compression_method::none) {
        compression = default_compression_;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
        if (entry.is_regular_file()) {
            auto relative_path = std::filesystem::relative(entry.path(), directory_path);
            std::string archive_path = archive_prefix;
            if (!archive_path.empty() && archive_path.back() != '/') {
                archive_path += '/';
            }
            archive_path += relative_path.string();

            // Convert Windows backslashes to forward slashes
            std::replace(archive_path.begin(), archive_path.end(), '\\', '/');

            add_file(entry.path(), archive_path, compression);
        }
    }
}

std::expected<void, builder_error>
archive_builder::build(const std::filesystem::path& output_path) {
    std::ofstream output(output_path, std::ios::binary);
    if (!output) {
        return std::unexpected{builder_error::write_error};
    }

    // Write placeholder header
    archive_header header{};
    header.magic = MAGIC_NUMBER;
    header.version = FORMAT_VERSION;
    header.directory_count = static_cast<std::uint32_t>(files_.size());
    header.directory_offset = 0; // Will be updated later
    header.reserved = 0;

    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!output) {
        return std::unexpected{builder_error::write_error};
    }

    // Write file data and collect directory entries
    std::vector<directory_entry> directory;
    directory.reserve(files_.size());

    std::uint64_t current_offset = sizeof(header);

    for (const auto& file : files_) {
        // Read source file
        std::ifstream input(file.source_path, std::ios::binary);
        if (!input) {
            return std::unexpected{builder_error::file_not_found};
        }

        input.seekg(0, std::ios::end);
        const auto file_size = input.tellg();
        input.seekg(0, std::ios::beg);

        std::vector<std::byte> data(file_size);
        input.read(reinterpret_cast<char*>(data.data()), file_size);
        if (!input) {
            return std::unexpected{builder_error::file_not_found};
        }

        // Compress if needed
        std::vector<std::byte> compressed_data;
        if (file.compression != compression_method::none) {
            auto result = compression_engine::compress(data, file.compression);
            if (!result) {
                return std::unexpected{builder_error::compression_error};
            }
            compressed_data = std::move(*result);
        } else {
            compressed_data = std::move(data);
        }

        // Write compressed data to archive
        output.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
        if (!output) {
            return std::unexpected{builder_error::write_error};
        }

        // Create directory entry
        directory_entry entry;
        entry.filename = file.archive_path;
        entry.data_offset = current_offset;
        entry.compressed_size = compressed_data.size();
        entry.uncompressed_size = data.size();
        entry.compression = file.compression;

        directory.push_back(std::move(entry));
        current_offset += compressed_data.size();
    }

    // Update header with directory offset
    const auto directory_offset = output.tellp();
    header.directory_offset = static_cast<std::uint64_t>(directory_offset);

    // Write directory entries
    for (const auto& entry : directory) {
        const auto filename_length = static_cast<std::uint32_t>(entry.filename.size());

        output.write(reinterpret_cast<const char*>(&filename_length), sizeof(filename_length));
        output.write(entry.filename.data(), entry.filename.size());
        output.write(reinterpret_cast<const char*>(&entry.data_offset), sizeof(entry.data_offset));
        output.write(reinterpret_cast<const char*>(&entry.compressed_size), sizeof(entry.compressed_size));
        output.write(reinterpret_cast<const char*>(&entry.uncompressed_size), sizeof(entry.uncompressed_size));
        output.write(reinterpret_cast<const char*>(&entry.compression), sizeof(entry.compression));

        if (!output) {
            return std::unexpected{builder_error::write_error};
        }
    }

    // Update header at beginning of file
    output.seekp(0);
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!output) {
        return std::unexpected{builder_error::write_error};
    }

    return {};
}

} // namespace dp