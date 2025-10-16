#include "datapak/vfs.hpp"
#include <algorithm>

namespace dp {

std::expected<void, vfs_error>
vfs::mount(const std::filesystem::path& archive_path, access_mode mode) {
    try {
        auto arch = std::make_unique<archive>(archive_path, mode);
        archives_.push_back(std::move(arch));
        return {};
    } catch (const std::exception&) {
        return std::unexpected{vfs_error::archive_error};
    }
}

std::expected<std::unique_ptr<vfstream>, vfs_error>
vfs::open(std::string_view filename) {
    const std::string filename_str{filename};

    if (cache_enabled_) {
        const auto cache_it = cache_.find(filename_str);
        if (cache_it != cache_.end()) {
            return std::make_unique<vfstream>(cache_it->second);
        }
    }

    // Search archives in the specified order
    if (search_order_ == search_order::reverse_mount_order) {
        // Search from most recently mounted to first mounted
        for (auto it = archives_.rbegin(); it != archives_.rend(); ++it) {
            const auto& arch = *it;
            if (arch->contains(filename)) {
                auto result = arch->open(filename);
                if (result) {
                    if (cache_enabled_) {
                        auto stream = result.value().get();
                        std::vector<std::byte> data;

                        stream->seekg(0, std::ios::end);
                        const auto size = stream->tellg();
                        stream->seekg(0, std::ios::beg);

                        data.resize(size);
                        stream->read(reinterpret_cast<char*>(data.data()), size);

                        cache_[filename_str] = data;
                        stream->seekg(0, std::ios::beg);
                    }
                    return std::move(*result);
                }
            }
        }
    } else {
        // Search from first mounted to most recently mounted
        for (const auto& arch : archives_) {
            if (arch->contains(filename)) {
                auto result = arch->open(filename);
                if (result) {
                    if (cache_enabled_) {
                        auto stream = result.value().get();
                        std::vector<std::byte> data;

                        stream->seekg(0, std::ios::end);
                        const auto size = stream->tellg();
                        stream->seekg(0, std::ios::beg);

                        data.resize(size);
                        stream->read(reinterpret_cast<char*>(data.data()), size);

                        cache_[filename_str] = data;
                        stream->seekg(0, std::ios::beg);
                    }
                    return std::move(*result);
                }
            }
        }
    }

    return std::unexpected{vfs_error::file_not_found};
}

bool vfs::contains(std::string_view filename) const {
    if (cache_enabled_ && cache_.contains(std::string{filename})) {
        return true;
    }

    // Check archives in the specified search order
    if (search_order_ == search_order::reverse_mount_order) {
        for (auto it = archives_.rbegin(); it != archives_.rend(); ++it) {
            if ((*it)->contains(filename)) {
                return true;
            }
        }
    } else {
        for (const auto& arch : archives_) {
            if (arch->contains(filename)) {
                return true;
            }
        }
    }

    return false;
}

std::vector<std::string> vfs::list_files() const {
    std::vector<std::string> all_files;

    for (const auto& arch : archives_) {
        const auto arch_files = arch->list_files();
        all_files.insert(all_files.end(), arch_files.begin(), arch_files.end());
    }

    std::ranges::sort(all_files);
    all_files.erase(std::ranges::unique(all_files).begin(), all_files.end());

    return all_files;
}

} // namespace dp