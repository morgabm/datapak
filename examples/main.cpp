#include <datapak/datapak.hpp>
#include <iostream>
#include <string>

int main() {
    try {
        dp::vfs filesystem;

        std::cout << "DataPak Virtual Filesystem Example\n";
        std::cout << "==================================\n\n";

        std::cout << "Note: This example demonstrates the API usage.\n";
        std::cout << "To actually run this, you would need to create a .pak archive file first.\n\n";

        std::cout << "Example usage:\n";
        std::cout << "1. Mount an archive:\n";
        std::cout << "   filesystem.mount(\"assets.pak\", dp::access_mode::memory);\n\n";

        std::cout << "2. Check if a file exists:\n";
        std::cout << "   if (filesystem.contains(\"textures/player.png\")) { ... }\n\n";

        std::cout << "3. Open and read a file:\n";
        std::cout << "   auto stream = filesystem.open(\"config.txt\");\n";
        std::cout << "   if (stream) {\n";
        std::cout << "       std::string line;\n";
        std::cout << "       std::getline(**stream, line);\n";
        std::cout << "   }\n\n";

        std::cout << "4. List all files:\n";
        std::cout << "   auto files = filesystem.list_files();\n";
        std::cout << "   for (const auto& file : files) {\n";
        std::cout << "       std::cout << file << std::endl;\n";
        std::cout << "   }\n\n";

        std::cout << "Features implemented:\n";
        std::cout << "- Custom .pak archive format with magic number and version\n";
        std::cout << "- Disk-based and memory-based access modes\n";
        std::cout << "- DEFLATE compression support via zlib\n";
        std::cout << "- LRU caching system for decompressed files\n";
        std::cout << "- STL-compatible stream interface\n";
        std::cout << "- Multiple archive mounting with search precedence\n";
        std::cout << "- C++23 features: std::expected, concepts, ranges\n\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}