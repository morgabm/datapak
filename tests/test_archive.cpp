#include <gtest/gtest.h>
#include <datapak/archive.hpp>
#include <datapak/archive_builder.hpp>
#include <filesystem>
#include <fstream>

class ArchiveTest : public ::testing::Test {
protected:
    std::filesystem::path test_dir;
    std::filesystem::path archive_path;

    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() / "datapak_test";
        archive_path = std::filesystem::temp_directory_path() / "test_archive.pak";

        // Clean up from previous runs
        std::filesystem::remove_all(test_dir);
        std::filesystem::remove(archive_path);

        // Create test directory structure
        std::filesystem::create_directories(test_dir / "subdir");

        // Create test files
        {
            std::ofstream file(test_dir / "test.txt");
            file << "This is a test file";
        }

        {
            std::ofstream file(test_dir / "subdir" / "nested.txt");
            file << "This is a nested file with more content for compression testing";
        }

        {
            std::ofstream file(test_dir / "binary.dat", std::ios::binary);
            for (int i = 0; i < 256; ++i) {
                file.put(static_cast<char>(i));
            }
        }
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir);
        std::filesystem::remove(archive_path);
    }
};

TEST_F(ArchiveTest, BuildArchive) {
    dp::archive_builder builder(dp::compression_method::deflate);
    builder.add_directory(test_dir);

    auto result = builder.build(archive_path);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(archive_path));
    EXPECT_GT(std::filesystem::file_size(archive_path), 0);
}

TEST_F(ArchiveTest, LoadAndListFiles) {
    // First create the archive
    dp::archive_builder builder;
    builder.add_directory(test_dir);
    ASSERT_TRUE(builder.build(archive_path).has_value());

    // Now load it
    dp::archive archive(archive_path);
    auto files = archive.list_files();

    EXPECT_EQ(files.size(), 3);

    // Check if all expected files are present
    bool found_test_txt = false, found_nested_txt = false, found_binary_dat = false;

    for (const auto& file : files) {
        if (file == "test.txt") found_test_txt = true;
        if (file == "subdir/nested.txt") found_nested_txt = true;
        if (file == "binary.dat") found_binary_dat = true;
    }

    EXPECT_TRUE(found_test_txt);
    EXPECT_TRUE(found_nested_txt);
    EXPECT_TRUE(found_binary_dat);
}

TEST_F(ArchiveTest, ExtractFiles) {
    // Create archive
    dp::archive_builder builder;
    builder.add_directory(test_dir);
    ASSERT_TRUE(builder.build(archive_path).has_value());

    // Load archive
    dp::archive archive(archive_path);

    // Test file existence
    EXPECT_TRUE(archive.contains("test.txt"));
    EXPECT_TRUE(archive.contains("subdir/nested.txt"));
    EXPECT_FALSE(archive.contains("nonexistent.txt"));

    // Extract and verify text file
    auto stream = archive.open("test.txt");
    ASSERT_TRUE(stream.has_value());

    std::string content;
    std::getline(**stream, content);
    EXPECT_EQ(content, "This is a test file");

    // Extract and verify nested file
    auto nested_stream = archive.open("subdir/nested.txt");
    ASSERT_TRUE(nested_stream.has_value());

    std::string nested_content;
    std::getline(**nested_stream, nested_content);
    EXPECT_EQ(nested_content, "This is a nested file with more content for compression testing");
}

TEST_F(ArchiveTest, BinaryFileHandling) {
    // Create archive
    dp::archive_builder builder;
    builder.add_directory(test_dir);
    ASSERT_TRUE(builder.build(archive_path).has_value());

    // Load and extract binary file
    dp::archive archive(archive_path);
    auto stream = archive.open("binary.dat");
    ASSERT_TRUE(stream.has_value());

    // Read binary data
    std::vector<char> data(256);
    stream.value()->read(data.data(), 256);
    EXPECT_EQ(stream.value()->gcount(), 256);

    // Verify binary content
    for (int i = 0; i < 256; ++i) {
        EXPECT_EQ(static_cast<unsigned char>(data[i]), i);
    }
}

TEST_F(ArchiveTest, MemoryMode) {
    // Create archive
    dp::archive_builder builder;
    builder.add_directory(test_dir);
    ASSERT_TRUE(builder.build(archive_path).has_value());

    // Load in memory mode
    dp::archive archive(archive_path, dp::access_mode::memory);

    // Verify same functionality
    EXPECT_TRUE(archive.contains("test.txt"));

    auto stream = archive.open("test.txt");
    ASSERT_TRUE(stream.has_value());

    std::string content;
    std::getline(**stream, content);
    EXPECT_EQ(content, "This is a test file");
}

TEST_F(ArchiveTest, CompressionMethods) {
    std::filesystem::path compressed_path = std::filesystem::temp_directory_path() / "compressed.pak";
    std::filesystem::path uncompressed_path = std::filesystem::temp_directory_path() / "uncompressed.pak";

    // Create compressed archive
    {
        dp::archive_builder builder(dp::compression_method::deflate);
        builder.add_directory(test_dir);
        ASSERT_TRUE(builder.build(compressed_path).has_value());
    }

    // Create uncompressed archive
    {
        dp::archive_builder builder(dp::compression_method::none);
        builder.add_directory(test_dir);
        ASSERT_TRUE(builder.build(uncompressed_path).has_value());
    }

    // Compare sizes with a more robust approach
    auto compressed_size = std::filesystem::file_size(compressed_path);
    auto uncompressed_size = std::filesystem::file_size(uncompressed_path);

    // Create larger, more compressible test data for better compression ratio testing
    std::filesystem::path large_compressed_path = std::filesystem::temp_directory_path() / "large_compressed.pak";
    std::filesystem::path large_uncompressed_path = std::filesystem::temp_directory_path() / "large_uncompressed.pak";

    // Create test directory with highly compressible content
    std::filesystem::path large_test_dir = std::filesystem::temp_directory_path() / "large_compression_test";
    std::filesystem::create_directories(large_test_dir);

    // Generate repetitive text files that compress well
    for (int i = 0; i < 5; ++i) {
        std::ofstream file(large_test_dir / ("repetitive" + std::to_string(i) + ".txt"));
        std::string pattern = "This is a highly repetitive pattern that should compress very well! ";
        for (int j = 0; j < 100; ++j) {
            file << pattern;
        }
    }

    // Test with larger files
    {
        dp::archive_builder builder(dp::compression_method::deflate);
        builder.add_directory(large_test_dir);
        ASSERT_TRUE(builder.build(large_compressed_path).has_value());
    }

    {
        dp::archive_builder builder(dp::compression_method::none);
        builder.add_directory(large_test_dir);
        ASSERT_TRUE(builder.build(large_uncompressed_path).has_value());
    }

    auto large_compressed_size = std::filesystem::file_size(large_compressed_path);
    auto large_uncompressed_size = std::filesystem::file_size(large_uncompressed_path);

    // For repetitive content, compression should provide significant space savings
    double compression_ratio = static_cast<double>(large_compressed_size) / large_uncompressed_size;
    EXPECT_LT(compression_ratio, 0.8); // Expect at least 20% compression
    EXPECT_GT(compression_ratio, 0.01); // Very good compression is possible with repetitive data

    // Original small files test - just verify both work and sizes are reasonable
    EXPECT_GT(compressed_size, 0);
    EXPECT_GT(uncompressed_size, 0);

    // For very small files, compressed might be larger due to header overhead
    // But the difference shouldn't be extreme
    double small_file_ratio = static_cast<double>(std::max(compressed_size, uncompressed_size)) /
                              std::min(compressed_size, uncompressed_size);
    EXPECT_LT(small_file_ratio, 2.0); // Neither should be more than 2x the other

    // Verify both can be read correctly
    dp::archive compressed_archive(compressed_path);
    dp::archive uncompressed_archive(uncompressed_path);

    auto compressed_stream = compressed_archive.open("test.txt");
    auto uncompressed_stream = uncompressed_archive.open("test.txt");

    ASSERT_TRUE(compressed_stream.has_value());
    ASSERT_TRUE(uncompressed_stream.has_value());

    std::string compressed_content, uncompressed_content;
    std::getline(**compressed_stream, compressed_content);
    std::getline(**uncompressed_stream, uncompressed_content);

    EXPECT_EQ(compressed_content, uncompressed_content);

    // Clean up
    std::filesystem::remove(compressed_path);
    std::filesystem::remove(uncompressed_path);
    std::filesystem::remove(large_compressed_path);
    std::filesystem::remove(large_uncompressed_path);
    std::filesystem::remove_all(large_test_dir);
}