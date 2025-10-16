#include <datapak/datapak.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <filesystem>

void print_usage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [COMMAND] [OPTIONS]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  create <archive.pak> <input_dir> [compression]  Create archive from directory\n";
    std::cout << "  list <archive.pak>                              List files in archive\n";
    std::cout << "  extract <archive.pak> <file_path> [output]      Extract file from archive\n";
    std::cout << "  info <archive.pak>                              Show archive information\n";
    std::cout << "\n";
    std::cout << "Compression options: none, deflate (default: deflate)\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " create assets.pak ./data deflate\n";
    std::cout << "  " << program_name << " list assets.pak\n";
    std::cout << "  " << program_name << " extract assets.pak config.txt output.txt\n";
    std::cout << "  " << program_name << " info assets.pak\n";
}

dp::compression_method parse_compression(const std::string& comp_str) {
    std::string lower_comp = comp_str;
    std::transform(lower_comp.begin(), lower_comp.end(), lower_comp.begin(), ::tolower);

    if (lower_comp == "none") return dp::compression_method::none;
    if (lower_comp == "deflate") return dp::compression_method::deflate;

    return dp::compression_method::deflate; // default
}

int cmd_create(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        std::cerr << "Error: create command requires archive path and input directory\n";
        return 1;
    }

    const std::string archive_path = args[2];
    const std::string input_dir = args[3];
    dp::compression_method compression = dp::compression_method::deflate;

    if (args.size() > 4) {
        compression = parse_compression(args[4]);
    }

    if (!std::filesystem::exists(input_dir)) {
        std::cerr << "Error: Input directory '" << input_dir << "' does not exist\n";
        return 1;
    }

    dp::archive_builder builder(compression);
    builder.add_directory(input_dir);

    std::cout << "Creating archive '" << archive_path << "' from '" << input_dir << "'...\n";
    std::cout << "Compression: " << (compression == dp::compression_method::none ? "none" : "deflate") << "\n";
    std::cout << "Files to archive: " << builder.file_count() << "\n";

    auto result = builder.build(archive_path);
    if (!result) {
        std::cerr << "Error: Failed to create archive\n";
        return 1;
    }

    std::cout << "Archive created successfully!\n";
    return 0;
}

int cmd_list(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Error: list command requires archive path\n";
        return 1;
    }

    const std::string archive_path = args[2];

    try {
        dp::archive arch(archive_path);
        auto files = arch.list_files();

        std::cout << "Files in archive '" << archive_path << "':\n";
        std::cout << "Total files: " << files.size() << "\n\n";

        for (const auto& file : files) {
            std::cout << "  " << file << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to open archive: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

int cmd_extract(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        std::cerr << "Error: extract command requires archive path and file path\n";
        return 1;
    }

    const std::string archive_path = args[2];
    const std::string file_path = args[3];
    std::string output_path = file_path;

    if (args.size() > 4) {
        output_path = args[4];
    }

    try {
        dp::archive arch(archive_path);

        if (!arch.contains(file_path)) {
            std::cerr << "Error: File '" << file_path << "' not found in archive\n";
            return 1;
        }

        auto stream = arch.open(file_path);
        if (!stream) {
            std::cerr << "Error: Failed to open file '" << file_path << "' from archive\n";
            return 1;
        }

        std::ofstream output(output_path, std::ios::binary);
        if (!output) {
            std::cerr << "Error: Failed to create output file '" << output_path << "'\n";
            return 1;
        }

        output << stream.value()->rdbuf();

        std::cout << "Extracted '" << file_path << "' to '" << output_path << "'\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

int cmd_info(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Error: info command requires archive path\n";
        return 1;
    }

    const std::string archive_path = args[2];

    try {
        dp::archive arch(archive_path);
        auto files = arch.list_files();

        // Get file size
        auto file_size = std::filesystem::file_size(archive_path);

        std::cout << "Archive Information\n";
        std::cout << "===================\n";
        std::cout << "File: " << archive_path << "\n";
        std::cout << "Size: " << file_size << " bytes\n";
        std::cout << "Files: " << files.size() << "\n";
        std::cout << "Format: DataPak (.pak)\n";

        // Calculate total uncompressed size by opening each file
        std::size_t total_uncompressed = 0;
        for (const auto& file : files) {
            if (auto stream = arch.open(file); stream) {
                stream.value()->seekg(0, std::ios::end);
                total_uncompressed += stream.value()->tellg();
            }
        }

        if (total_uncompressed > 0) {
            double compression_ratio = static_cast<double>(file_size) / total_uncompressed;
            std::cout << "Uncompressed size: " << total_uncompressed << " bytes\n";
            std::cout << "Compression ratio: " << std::fixed << std::setprecision(2) << compression_ratio << ":1\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to open archive: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    const std::string command = args[1];

    if (command == "create") {
        return cmd_create(args);
    } else if (command == "list") {
        return cmd_list(args);
    } else if (command == "extract") {
        return cmd_extract(args);
    } else if (command == "info") {
        return cmd_info(args);
    } else if (command == "help" || command == "-h" || command == "--help") {
        print_usage(args[0]);
        return 0;
    } else {
        std::cerr << "Error: Unknown command '" << command << "'\n\n";
        print_usage(args[0]);
        return 1;
    }
}