#include <gtest/gtest.h>
#include <datapak/compression.hpp>
#include <string>
#include <vector>

class CompressionTest : public ::testing::Test {
protected:
    std::vector<std::byte> text_data;
    std::vector<std::byte> binary_data;

    void SetUp() override {
        std::string text = "This is a test string that should compress well due to repetitive patterns. "
                          "This is a test string that should compress well due to repetitive patterns. "
                          "This is a test string that should compress well due to repetitive patterns.";

        text_data.resize(text.size());
        std::transform(text.begin(), text.end(), text_data.begin(),
                      [](char c) { return static_cast<std::byte>(c); });

        // Create binary data with patterns
        binary_data.resize(1024);
        for (size_t i = 0; i < binary_data.size(); ++i) {
            binary_data[i] = static_cast<std::byte>(i % 256);
        }
    }
};

TEST_F(CompressionTest, DeflateCompression) {
    auto result = dp::compression_engine::compress(text_data, dp::compression_method::deflate);
    ASSERT_TRUE(result.has_value());

    auto compressed = result.value();
    EXPECT_LT(compressed.size(), text_data.size()); // Should be smaller
    EXPECT_GT(compressed.size(), 0); // Should not be empty
}

TEST_F(CompressionTest, DeflateDecompression) {
    auto compressed_result = dp::compression_engine::compress(text_data, dp::compression_method::deflate);
    ASSERT_TRUE(compressed_result.has_value());

    auto decompressed_result = dp::compression_engine::decompress(
        compressed_result.value(), dp::compression_method::deflate, text_data.size());

    ASSERT_TRUE(decompressed_result.has_value());
    EXPECT_EQ(decompressed_result.value(), text_data);
}

TEST_F(CompressionTest, RoundTripBinaryData) {
    auto compressed = dp::compression_engine::compress(binary_data, dp::compression_method::deflate);
    ASSERT_TRUE(compressed.has_value());

    auto decompressed = dp::compression_engine::decompress(
        compressed.value(), dp::compression_method::deflate, binary_data.size());

    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(decompressed.value(), binary_data);
}

TEST_F(CompressionTest, NoCompression) {
    auto result = dp::compression_engine::compress(text_data, dp::compression_method::none);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), text_data);

    auto decompressed = dp::compression_engine::decompress(
        result.value(), dp::compression_method::none, text_data.size());
    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(decompressed.value(), text_data);
}

TEST_F(CompressionTest, InvalidMethod) {
    auto result = dp::compression_engine::compress(text_data, static_cast<dp::compression_method>(99));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), dp::compression_error::invalid_method);
}

TEST_F(CompressionTest, EmptyData) {
    std::vector<std::byte> empty_data;

    auto compressed = dp::compression_engine::compress(empty_data, dp::compression_method::deflate);
    ASSERT_TRUE(compressed.has_value());

    auto decompressed = dp::compression_engine::decompress(
        compressed.value(), dp::compression_method::deflate, 0);
    ASSERT_TRUE(decompressed.has_value());
    EXPECT_TRUE(decompressed.value().empty());
}