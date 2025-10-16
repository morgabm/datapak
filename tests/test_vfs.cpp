#include <gtest/gtest.h>
#include <datapak/vfs.hpp>
#include <datapak/archive_builder.hpp>
#include <filesystem>
#include <fstream>

class VFSTest : public ::testing::Test {
protected:
    std::filesystem::path test_dir1, test_dir2;
    std::filesystem::path archive1_path, archive2_path;

    void SetUp() override {
        test_dir1 = std::filesystem::temp_directory_path() / "datapak_vfs_test1";
        test_dir2 = std::filesystem::temp_directory_path() / "datapak_vfs_test2";
        archive1_path = std::filesystem::temp_directory_path() / "archive1.pak";
        archive2_path = std::filesystem::temp_directory_path() / "archive2.pak";

        // Clean up from previous runs
        std::filesystem::remove_all(test_dir1);
        std::filesystem::remove_all(test_dir2);
        std::filesystem::remove(archive1_path);
        std::filesystem::remove(archive2_path);

        // Create first test directory
        std::filesystem::create_directories(test_dir1);
        {
            std::ofstream file(test_dir1 / "common.txt");
            file << "Content from archive 1";
        }
        {
            std::ofstream file(test_dir1 / "unique1.txt");
            file << "Unique to archive 1";
        }

        // Create second test directory
        std::filesystem::create_directories(test_dir2 / "subdir");
        {
            std::ofstream file(test_dir2 / "common.txt");
            file << "Content from archive 2";
        }
        {
            std::ofstream file(test_dir2 / "unique2.txt");
            file << "Unique to archive 2";
        }
        {
            std::ofstream file(test_dir2 / "subdir" / "nested.txt");
            file << "Nested file in archive 2";
        }

        // Create archives
        dp::archive_builder builder1;
        builder1.add_directory(test_dir1);
        ASSERT_TRUE(builder1.build(archive1_path).has_value());

        dp::archive_builder builder2;
        builder2.add_directory(test_dir2);
        ASSERT_TRUE(builder2.build(archive2_path).has_value());
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir1);
        std::filesystem::remove_all(test_dir2);
        std::filesystem::remove(archive1_path);
        std::filesystem::remove(archive2_path);
    }
};

TEST_F(VFSTest, SingleArchiveMount) {
    dp::vfs filesystem;

    auto mount_result = filesystem.mount(archive1_path);
    ASSERT_TRUE(mount_result.has_value());

    // Check file existence
    EXPECT_TRUE(filesystem.contains("common.txt"));
    EXPECT_TRUE(filesystem.contains("unique1.txt"));
    EXPECT_FALSE(filesystem.contains("unique2.txt"));

    // List files
    auto files = filesystem.list_files();
    EXPECT_EQ(files.size(), 2);
}

TEST_F(VFSTest, MultipleArchiveMounting) {
    dp::vfs filesystem;

    // Mount both archives
    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());
    ASSERT_TRUE(filesystem.mount(archive2_path).has_value());

    // Check all files are accessible
    EXPECT_TRUE(filesystem.contains("common.txt"));
    EXPECT_TRUE(filesystem.contains("unique1.txt"));
    EXPECT_TRUE(filesystem.contains("unique2.txt"));
    EXPECT_TRUE(filesystem.contains("subdir/nested.txt"));

    // List all files
    auto files = filesystem.list_files();
    EXPECT_EQ(files.size(), 4);
}

TEST_F(VFSTest, DefaultReverseMountOrderPrecedence) {
    dp::vfs filesystem;

    // Default behavior: most recently mounted takes precedence
    ASSERT_EQ(filesystem.get_search_order(), dp::search_order::reverse_mount_order);

    // Mount archive1 first, then archive2
    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());
    ASSERT_TRUE(filesystem.mount(archive2_path).has_value());

    // Open common.txt - should get content from archive2 (most recently mounted)
    auto stream = filesystem.open("common.txt");
    ASSERT_TRUE(stream.has_value());

    std::string content;
    std::getline(**stream, content);
    EXPECT_EQ(content, "Content from archive 2");
}

TEST_F(VFSTest, MountOrderPrecedence) {
    dp::vfs filesystem;

    // Set to mount order precedence
    filesystem.set_search_order(dp::search_order::mount_order);

    // Mount archive1 first, then archive2
    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());
    ASSERT_TRUE(filesystem.mount(archive2_path).has_value());

    // Open common.txt - should get content from archive1 (first mounted)
    auto stream = filesystem.open("common.txt");
    ASSERT_TRUE(stream.has_value());

    std::string content;
    std::getline(**stream, content);
    EXPECT_EQ(content, "Content from archive 1");
}

TEST_F(VFSTest, ReverseMountOrderPrecedence) {
    dp::vfs filesystem;

    // Explicitly set to reverse mount order
    filesystem.set_search_order(dp::search_order::reverse_mount_order);

    // Mount archive1 first, then archive2
    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());
    ASSERT_TRUE(filesystem.mount(archive2_path).has_value());

    // Open common.txt - should get content from archive2 (most recently mounted)
    auto stream = filesystem.open("common.txt");
    ASSERT_TRUE(stream.has_value());

    std::string content;
    std::getline(**stream, content);
    EXPECT_EQ(content, "Content from archive 2");
}

TEST_F(VFSTest, ChangeSearchOrderDynamically) {
    dp::vfs filesystem;

    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());
    ASSERT_TRUE(filesystem.mount(archive2_path).has_value());

    // Test reverse mount order (default)
    filesystem.set_search_order(dp::search_order::reverse_mount_order);
    {
        auto stream = filesystem.open("common.txt");
        ASSERT_TRUE(stream.has_value());
        std::string content;
        std::getline(**stream, content);
        EXPECT_EQ(content, "Content from archive 2");
    }

    // Clear cache before changing search order
    filesystem.clear_cache();

    // Change to mount order
    filesystem.set_search_order(dp::search_order::mount_order);
    {
        auto stream = filesystem.open("common.txt");
        ASSERT_TRUE(stream.has_value());
        std::string content;
        std::getline(**stream, content);
        EXPECT_EQ(content, "Content from archive 1");
    }
}

TEST_F(VFSTest, FileNotFound) {
    dp::vfs filesystem;
    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());

    auto stream = filesystem.open("nonexistent.txt");
    EXPECT_FALSE(stream.has_value());
    EXPECT_EQ(stream.error(), dp::vfs_error::file_not_found);
}

TEST_F(VFSTest, CachingBehavior) {
    dp::vfs filesystem;
    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());

    // Enable caching
    filesystem.enable_cache(true);
    EXPECT_EQ(filesystem.cache_size(), 0);

    // Access a file - should be cached
    auto stream1 = filesystem.open("common.txt");
    ASSERT_TRUE(stream1.has_value());

    std::string content1;
    std::getline(**stream1, content1);

    EXPECT_GT(filesystem.cache_size(), 0);

    // Access same file again
    auto stream2 = filesystem.open("common.txt");
    ASSERT_TRUE(stream2.has_value());

    std::string content2;
    std::getline(**stream2, content2);

    EXPECT_EQ(content1, content2);

    // Clear cache
    filesystem.clear_cache();
    EXPECT_EQ(filesystem.cache_size(), 0);
}

TEST_F(VFSTest, DisableCaching) {
    dp::vfs filesystem;
    ASSERT_TRUE(filesystem.mount(archive1_path).has_value());

    // Disable caching
    filesystem.enable_cache(false);

    // Access a file
    auto stream = filesystem.open("common.txt");
    ASSERT_TRUE(stream.has_value());

    std::string content;
    std::getline(**stream, content);

    // Cache should remain empty
    EXPECT_EQ(filesystem.cache_size(), 0);
}

TEST_F(VFSTest, MemoryModeArchives) {
    dp::vfs filesystem;

    // Mount archives in memory mode
    auto result1 = filesystem.mount(archive1_path, dp::access_mode::memory);
    auto result2 = filesystem.mount(archive2_path, dp::access_mode::memory);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    // Verify functionality is the same
    EXPECT_TRUE(filesystem.contains("common.txt"));
    EXPECT_TRUE(filesystem.contains("unique1.txt"));
    EXPECT_TRUE(filesystem.contains("unique2.txt"));

    auto stream = filesystem.open("subdir/nested.txt");
    ASSERT_TRUE(stream.has_value());

    std::string content;
    std::getline(**stream, content);
    EXPECT_EQ(content, "Nested file in archive 2");
}