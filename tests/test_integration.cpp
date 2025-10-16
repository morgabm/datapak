#include <gtest/gtest.h>
#include <datapak/datapak.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>

class IntegrationTest : public ::testing::Test {
protected:
    std::filesystem::path test_dir;
    std::filesystem::path cli_path;

    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "datapak_integration_test";

        // Find CLI path - try multiple possible locations
        std::vector<std::filesystem::path> possible_paths = {
            std::filesystem::current_path() / "datapak_cli",                    // Direct run from build dir
            std::filesystem::current_path() / ".." / "datapak_cli",            // Run from tests subdir
            std::filesystem::current_path().parent_path() / "datapak_cli",      // Alternative parent lookup
            std::filesystem::absolute(".") / "datapak_cli",                     // Absolute current + cli
            // Additional paths for different build configurations
            std::filesystem::current_path() / "tools" / "datapak_cli",          // tools subdirectory
            std::filesystem::current_path() / ".." / "tools" / "datapak_cli",   // parent/tools
        };

        for (const auto& path : possible_paths) {
            if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
                cli_path = path;
                break;
            }
        }

        // If none found, skip integration tests gracefully
        if (cli_path.empty()) {
            GTEST_SKIP() << "datapak_cli not found in any expected location. Skipping integration tests.";
        }

        // Clean up from previous runs
        std::filesystem::remove_all(test_dir);

        // Create test directory with various file types
        std::filesystem::create_directories(test_dir / "assets" / "textures");
        std::filesystem::create_directories(test_dir / "configs");
        std::filesystem::create_directories(test_dir / "data");

        // Create test files
        {
            std::ofstream file(test_dir / "README.md");
            file << "# Test Archive\n\nThis is a test archive for integration testing.\n";
        }

        {
            std::ofstream file(test_dir / "configs" / "game.ini");
            file << "[graphics]\nresolution=1920x1080\nfullscreen=true\n\n";
            file << "[audio]\nmaster_volume=0.8\nmusic_volume=0.6\n";
        }

        {
            std::ofstream file(test_dir / "assets" / "manifest.json");
            file << "{\n  \"version\": \"1.0\",\n  \"assets\": [\n";
            file << "    \"textures/player.png\",\n    \"textures/enemy.png\"\n  ]\n}";
        }

        // Create binary files
        {
            std::ofstream file(test_dir / "data" / "level1.dat", std::ios::binary);
            for (int i = 0; i < 1024; ++i) {
                file.put(static_cast<char>(i % 256));
            }
        }

        // Create texture files (dummy PPM format)
        {
            std::ofstream file(test_dir / "assets" / "textures" / "player.ppm");
            file << "P3\n2 2\n255\n255 0 0 0 255 0\n0 0 255 255 255 0\n";
        }

        {
            std::ofstream file(test_dir / "assets" / "textures" / "enemy.ppm");
            file << "P3\n2 2\n255\n128 0 0 0 128 0\n0 0 128 128 128 0\n";
        }
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }

    int run_cli(const std::vector<std::string>& args) {
        std::string command = cli_path.string();
        for (const auto& arg : args) {
            command += " \"" + arg + "\"";
        }
        return std::system(command.c_str());
    }
};

TEST_F(IntegrationTest, FullWorkflow) {
    // Additional safety check in case SetUp didn't properly skip
    if (cli_path.empty()) {
        GTEST_SKIP() << "CLI not available for integration testing";
    }

    auto archive_path = test_dir.parent_path() / "integration_test.pak";

    // Step 1: Create archive using CLI
    int create_result = run_cli({"create", archive_path.string(), test_dir.string(), "deflate"});
    EXPECT_EQ(create_result, 0);
    EXPECT_TRUE(std::filesystem::exists(archive_path));

    // Step 2: Use library to load and verify archive
    dp::vfs filesystem;
    auto mount_result = filesystem.mount(archive_path);
    ASSERT_TRUE(mount_result.has_value());

    // Verify all expected files are present
    EXPECT_TRUE(filesystem.contains("README.md"));
    EXPECT_TRUE(filesystem.contains("configs/game.ini"));
    EXPECT_TRUE(filesystem.contains("assets/manifest.json"));
    EXPECT_TRUE(filesystem.contains("data/level1.dat"));
    EXPECT_TRUE(filesystem.contains("assets/textures/player.ppm"));
    EXPECT_TRUE(filesystem.contains("assets/textures/enemy.ppm"));

    // Step 3: Extract and verify file contents
    auto readme_stream = filesystem.open("README.md");
    ASSERT_TRUE(readme_stream.has_value());

    std::string readme_content;
    std::getline(**readme_stream, readme_content);
    EXPECT_EQ(readme_content, "# Test Archive");

    // Step 4: Verify binary data
    auto binary_stream = filesystem.open("data/level1.dat");
    ASSERT_TRUE(binary_stream.has_value());

    std::vector<char> binary_data(1024);
    binary_stream.value()->read(binary_data.data(), 1024);
    EXPECT_EQ(binary_stream.value()->gcount(), 1024);

    for (int i = 0; i < 1024; ++i) {
        EXPECT_EQ(static_cast<unsigned char>(binary_data[i]), i % 256);
    }

    // Step 5: Test caching behavior with multiple accesses
    filesystem.clear_cache(); // Clear any existing cache first
    filesystem.enable_cache(true);
    EXPECT_EQ(filesystem.cache_size(), 0);

    // First access
    auto stream1 = filesystem.open("assets/manifest.json");
    ASSERT_TRUE(stream1.has_value());
    std::string json_content1((std::istreambuf_iterator<char>(**stream1)),
                              std::istreambuf_iterator<char>());

    // Should be cached now
    EXPECT_GT(filesystem.cache_size(), 0);

    // Second access (should be faster)
    auto stream2 = filesystem.open("assets/manifest.json");
    ASSERT_TRUE(stream2.has_value());
    std::string json_content2((std::istreambuf_iterator<char>(**stream2)),
                              std::istreambuf_iterator<char>());

    EXPECT_EQ(json_content1, json_content2);

    // Clean up
    std::filesystem::remove(archive_path);
}

TEST_F(IntegrationTest, MultipleArchiveWorkflow) {
    // Additional safety check in case SetUp didn't properly skip
    if (cli_path.empty()) {
        GTEST_SKIP() << "CLI not available for integration testing";
    }

    auto base_archive = test_dir.parent_path() / "base.pak";
    auto patch_archive = test_dir.parent_path() / "patch.pak";

    // Create base archive with original files
    int create_base = run_cli({"create", base_archive.string(), test_dir.string(), "deflate"});
    EXPECT_EQ(create_base, 0);

    // Create a separate patch directory with modified files
    auto patch_dir = test_dir.parent_path() / "patch_content";
    std::filesystem::create_directories(patch_dir / "configs");

    // Modify some files for patch
    {
        std::ofstream file(patch_dir / "configs" / "game.ini");
        file << "[graphics]\nresolution=3840x2160\nfullscreen=false\n\n";
        file << "[audio]\nmaster_volume=1.0\nmusic_volume=0.8\n";
    }

    // Add new file
    {
        std::ofstream file(patch_dir / "patch_notes.txt");
        file << "Version 1.1 Patch Notes:\n- Updated graphics settings\n- Improved audio\n";
    }

    // Create patch archive from patch directory
    int create_patch = run_cli({"create", patch_archive.string(), patch_dir.string(), "deflate"});
    EXPECT_EQ(create_patch, 0);

    // Load both archives with patch taking precedence
    dp::vfs filesystem;
    ASSERT_TRUE(filesystem.mount(base_archive).has_value());
    ASSERT_TRUE(filesystem.mount(patch_archive).has_value());

    // Verify patch file exists
    EXPECT_TRUE(filesystem.contains("patch_notes.txt"));

    // Verify patched config has new content
    auto config_stream = filesystem.open("configs/game.ini");
    ASSERT_TRUE(config_stream.has_value());

    std::string line;
    std::getline(**config_stream, line); // [graphics]
    std::getline(**config_stream, line); // resolution
    EXPECT_EQ(line, "resolution=3840x2160");

    // Original files should still be accessible
    EXPECT_TRUE(filesystem.contains("README.md"));
    EXPECT_TRUE(filesystem.contains("data/level1.dat"));

    // Clean up
    std::filesystem::remove(base_archive);
    std::filesystem::remove(patch_archive);
    std::filesystem::remove_all(patch_dir);
}

TEST_F(IntegrationTest, CompressionEffectiveness) {
    auto compressed_archive = test_dir.parent_path() / "compressed.pak";
    auto uncompressed_archive = test_dir.parent_path() / "uncompressed.pak";

    // Create compressed archive
    int create_compressed = run_cli({"create", compressed_archive.string(), test_dir.string(), "deflate"});
    EXPECT_EQ(create_compressed, 0);

    // Create uncompressed archive
    int create_uncompressed = run_cli({"create", uncompressed_archive.string(), test_dir.string(), "none"});
    EXPECT_EQ(create_uncompressed, 0);

    // Compare file sizes
    auto compressed_size = std::filesystem::file_size(compressed_archive);
    auto uncompressed_size = std::filesystem::file_size(uncompressed_archive);

    EXPECT_LE(compressed_size, uncompressed_size);

    // Verify both archives have the same content
    dp::vfs compressed_fs, uncompressed_fs;
    ASSERT_TRUE(compressed_fs.mount(compressed_archive).has_value());
    ASSERT_TRUE(uncompressed_fs.mount(uncompressed_archive).has_value());

    auto compressed_files = compressed_fs.list_files();
    auto uncompressed_files = uncompressed_fs.list_files();

    EXPECT_EQ(compressed_files.size(), uncompressed_files.size());

    // Verify content is identical
    for (const auto& filename : compressed_files) {
        auto compressed_stream = compressed_fs.open(filename);
        auto uncompressed_stream = uncompressed_fs.open(filename);

        ASSERT_TRUE(compressed_stream.has_value());
        ASSERT_TRUE(uncompressed_stream.has_value());

        std::string compressed_content((std::istreambuf_iterator<char>(**compressed_stream)),
                                       std::istreambuf_iterator<char>());
        std::string uncompressed_content((std::istreambuf_iterator<char>(**uncompressed_stream)),
                                         std::istreambuf_iterator<char>());

        EXPECT_EQ(compressed_content, uncompressed_content);
    }

    // Clean up
    std::filesystem::remove(compressed_archive);
    std::filesystem::remove(uncompressed_archive);
}