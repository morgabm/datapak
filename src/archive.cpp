#include "datapak/archive.hpp"
#include <iostream>
#include <cstring>

namespace dp {

archive::archive(const std::filesystem::path& path, access_mode mode)
    : path_(path), mode_(mode) {

    if (mode_ == access_mode::disk) {
        file_stream_.open(path_, std::ios::binary);
    } else {
        std::ifstream temp_file(path_, std::ios::binary);
        if (temp_file) {
            temp_file.seekg(0, std::ios::end);
            const auto size = temp_file.tellg();
            temp_file.seekg(0, std::ios::beg);

            memory_data_.resize(size);
            temp_file.read(reinterpret_cast<char*>(memory_data_.data()), size);
        }
    }

    if (auto result = load_directory(); !result) {
        throw std::runtime_error("Failed to load archive directory");
    }
}

std::expected<void, archive_error> archive::load_directory() {
    archive_header header{};

    if (mode_ == access_mode::disk) {
        if (!file_stream_.is_open()) {
            return std::unexpected{archive_error::file_not_found};
        }

        file_stream_.seekg(0);
        file_stream_.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (!file_stream_ || file_stream_.gcount() != sizeof(header)) {
            return std::unexpected{archive_error::read_error};
        }
    } else {
        if (memory_data_.size() < sizeof(header)) {
            return std::unexpected{archive_error::invalid_format};
        }

        std::memcpy(&header, memory_data_.data(), sizeof(header));
    }

    if (header.magic != MAGIC_NUMBER) {
        return std::unexpected{archive_error::invalid_format};
    }

    if (header.version != FORMAT_VERSION) {
        return std::unexpected{archive_error::invalid_format};
    }

    directory_.clear();
    directory_.reserve(header.directory_count);

    std::uint64_t current_pos = header.directory_offset;

    for (std::uint32_t i = 0; i < header.directory_count; ++i) {
        directory_entry entry{};
        std::uint32_t filename_length = 0;

        if (mode_ == access_mode::disk) {
            file_stream_.seekg(current_pos);
            file_stream_.read(reinterpret_cast<char*>(&filename_length), sizeof(filename_length));

            if (filename_length > 0 && filename_length < 4096) {
                std::string filename(filename_length, '\0');
                file_stream_.read(filename.data(), filename_length);
                entry.filename = std::move(filename);
            }

            file_stream_.read(reinterpret_cast<char*>(&entry.data_offset), sizeof(entry.data_offset));
            file_stream_.read(reinterpret_cast<char*>(&entry.compressed_size), sizeof(entry.compressed_size));
            file_stream_.read(reinterpret_cast<char*>(&entry.uncompressed_size), sizeof(entry.uncompressed_size));
            file_stream_.read(reinterpret_cast<char*>(&entry.compression), sizeof(entry.compression));
        } else {
            if (current_pos + sizeof(filename_length) > memory_data_.size()) {
                return std::unexpected{archive_error::read_error};
            }

            std::memcpy(&filename_length, memory_data_.data() + current_pos, sizeof(filename_length));
            current_pos += sizeof(filename_length);

            if (current_pos + filename_length > memory_data_.size()) {
                return std::unexpected{archive_error::read_error};
            }

            if (filename_length > 0 && filename_length < 4096) {
                entry.filename = std::string(
                    reinterpret_cast<const char*>(memory_data_.data() + current_pos),
                    filename_length
                );
            }
            current_pos += filename_length;

            if (current_pos + sizeof(entry.data_offset) + sizeof(entry.compressed_size) +
                sizeof(entry.uncompressed_size) + sizeof(entry.compression) > memory_data_.size()) {
                return std::unexpected{archive_error::read_error};
            }

            std::memcpy(&entry.data_offset, memory_data_.data() + current_pos, sizeof(entry.data_offset));
            current_pos += sizeof(entry.data_offset);
            std::memcpy(&entry.compressed_size, memory_data_.data() + current_pos, sizeof(entry.compressed_size));
            current_pos += sizeof(entry.compressed_size);
            std::memcpy(&entry.uncompressed_size, memory_data_.data() + current_pos, sizeof(entry.uncompressed_size));
            current_pos += sizeof(entry.uncompressed_size);
            std::memcpy(&entry.compression, memory_data_.data() + current_pos, sizeof(entry.compression));
            current_pos += sizeof(entry.compression);
        }

        if (!entry.filename.empty()) {
            directory_[entry.filename] = std::move(entry);
        }

        if (mode_ == access_mode::disk) {
            current_pos += sizeof(filename_length) + filename_length +
                          sizeof(entry.data_offset) + sizeof(entry.compressed_size) +
                          sizeof(entry.uncompressed_size) + sizeof(entry.compression);
        }
    }

    return {};
}

std::expected<std::unique_ptr<vfstream>, archive_error>
archive::open(std::string_view filename) const {
    const auto it = directory_.find(std::string{filename});
    if (it == directory_.end()) {
        return std::unexpected{archive_error::entry_not_found};
    }

    auto data_result = read_file_data(it->second);
    if (!data_result) {
        return std::unexpected{data_result.error()};
    }

    if (it->second.compression != compression_method::none) {
        auto decompressed = compression_engine::decompress(
            *data_result, it->second.compression, it->second.uncompressed_size
        );

        if (!decompressed) {
            return std::unexpected{archive_error::compression_error};
        }

        return std::make_unique<vfstream>(std::move(*decompressed));
    }

    return std::make_unique<vfstream>(std::move(*data_result));
}

bool archive::contains(std::string_view filename) const {
    return directory_.contains(std::string{filename});
}

std::vector<std::string> archive::list_files() const {
    std::vector<std::string> files;
    files.reserve(directory_.size());

    for (const auto& [filename, _] : directory_) {
        files.emplace_back(filename);
    }

    return files;
}

std::expected<std::vector<std::byte>, archive_error>
archive::read_file_data(const directory_entry& entry) const {
    std::vector<std::byte> data(entry.compressed_size);

    if (mode_ == access_mode::disk) {
        file_stream_.seekg(entry.data_offset);
        file_stream_.read(reinterpret_cast<char*>(data.data()), entry.compressed_size);

        if (!file_stream_ || file_stream_.gcount() != static_cast<std::streamsize>(entry.compressed_size)) {
            return std::unexpected{archive_error::read_error};
        }
    } else {
        if (entry.data_offset + entry.compressed_size > memory_data_.size()) {
            return std::unexpected{archive_error::read_error};
        }

        std::memcpy(data.data(), memory_data_.data() + entry.data_offset, entry.compressed_size);
    }

    return data;
}

std::expected<archive, archive_error>
archive::create(const std::filesystem::path& path) {
    try {
        return archive(path, access_mode::disk);
    } catch (const std::exception&) {
        return std::unexpected{archive_error::read_error};
    }
}

} // namespace dp