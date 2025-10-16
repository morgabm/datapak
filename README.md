# DataPak - C++23 Virtual Filesystem

A high-performance virtual filesystem library for C++23 that provides transparent access to files stored in custom `.pak` archive formats.

## Features

- **Custom Archive Format**: Efficient binary format optimized for fast file lookups
- **Dual Access Modes**: Disk-based (low memory) or memory-based (high performance) access
- **Compression Support**: DEFLATE compression via zlib with extensible compression system
- **Intelligent Caching**: LRU cache for decompressed files
- **STL Compatibility**: `std::istream`-compatible file streams
- **Multiple Archives**: Mount multiple archives with configurable search precedence
- **Modern C++23**: Uses `std::expected`, concepts, and ranges for clean error handling

## Architecture

The system consists of three main classes:

1. **`dp::vfs`** - The main interface for mounting archives and opening files
2. **`dp::archive`** - Represents a single `.pak` file with efficient file lookup
3. **`dp::vfstream`** - STL-compatible stream for reading files from archives

## Archive Format

```
[Header] [Data Blobs] [File Directory]
```

- **Header**: Magic number, version, and directory offset
- **Data Blobs**: Compressed file data packed sequentially
- **File Directory**: Metadata table at end of file for easy modification

## Usage Example

```cpp
#include <datapak/datapak.hpp>

int main() {
    dp::vfs filesystem;

    // Mount archive in memory mode for faster access
    if (auto result = filesystem.mount("assets.pak", dp::access_mode::memory); !result) {
        std::cerr << "Failed to mount archive\\n";
        return 1;
    }

    // Check if file exists
    if (filesystem.contains("config.txt")) {
        // Open file - returns std::expected<std::unique_ptr<vfstream>, vfs_error>
        auto stream = filesystem.open("config.txt");
        if (stream) {
            std::string content;
            std::getline(**stream, content);
            std::cout << "Config: " << content << std::endl;
        }
    }

    // List all files across mounted archives
    auto files = filesystem.list_files();
    for (const auto& file : files) {
        std::cout << file << std::endl;
    }

    return 0;
}
```

## Building

Requires:
- C++23 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- CMake 3.20+
- zlib development libraries

```bash
mkdir build && cd build
cmake ..
make
```

### Running Tests

```bash
# Run all tests
./tests/datapak_tests

# Run tests via CTest
ctest

# Run only unit tests (excluding integration tests)
./tests/datapak_tests --gtest_filter="-IntegrationTest*"
```

### Code Coverage

To enable code coverage analysis:

```bash
# Configure with coverage enabled
cmake -DENABLE_COVERAGE=ON ..
make

# Run tests and generate coverage report
make coverage

# Or generate just the summary
make coverage-summary
```

The HTML coverage report will be generated in `build/coverage/html/index.html`.

**Current Coverage**: 87.2% line coverage, 86.7% function coverage

## API Reference

### dp::vfs

```cpp
// Mount an archive file
std::expected<void, vfs_error> mount(const std::filesystem::path& archive_path,
                                    access_mode mode = access_mode::disk);

// Open a file for reading
std::expected<std::unique_ptr<vfstream>, vfs_error> open(std::string_view filename);

// Check if file exists in any mounted archive
bool contains(std::string_view filename) const;

// List all files across mounted archives
std::vector<std::string> list_files() const;

// Cache management
void enable_cache(bool enable = true);
void clear_cache();
std::size_t cache_size() const;

// Search order configuration
void set_search_order(search_order order);
search_order get_search_order() const;
```

### dp::archive

```cpp
// Access modes
enum class access_mode { disk, memory };

// Constructor
explicit archive(const std::filesystem::path& path, access_mode mode = access_mode::disk);

// File operations
std::expected<std::unique_ptr<vfstream>, archive_error> open(std::string_view filename) const;
bool contains(std::string_view filename) const;
std::vector<std::string> list_files() const;
```

### dp::vfstream

Inherits from `std::istream`, providing full STL compatibility:

```cpp
auto stream = vfs.open("data.txt");
if (stream) {
    std::string line;
    while (std::getline(**stream, line)) {
        // Process line
    }
}
```

## Design Principles

- **Zero-allocation file lookup**: O(1) filename lookup using hash maps
- **Lazy decompression**: Files decompressed only when read
- **Memory safety**: RAII and smart pointers throughout
- **Error handling**: `std::expected` for recoverable errors
- **STL compatibility**: Drop-in replacement for `std::ifstream`
- **Configurable precedence**: Choose between mount-order or reverse-mount-order file resolution

## Implementation Status (MVP)

✅ Custom archive format with header/data/directory layout
✅ Disk and memory access modes
✅ DEFLATE compression support
✅ STL-compatible stream interface
✅ Multiple archive mounting
✅ File caching system
✅ Configurable search order (reverse-mount-order by default)
✅ Modern C++23 error handling
✅ Command-line tool for archive creation and management
✅ Comprehensive unit, integration, and E2E tests

## Future Enhancements

- Additional compression algorithms (zstd, lz4)
- Encryption support
- Archive modification/patching
- Memory-mapped file access
- Async I/O support