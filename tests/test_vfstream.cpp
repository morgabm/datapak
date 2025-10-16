#include <gtest/gtest.h>
#include <datapak/vfstream.hpp>
#include <string>
#include <sstream>

class VFStreamTest : public ::testing::Test {
protected:
    std::vector<std::byte> test_data;
    std::string test_string;

    void SetUp() override {
        test_string = "Hello, World!\nThis is a test string.\nWith multiple lines.\n";
        test_data.resize(test_string.size());
        std::transform(test_string.begin(), test_string.end(), test_data.begin(),
                      [](char c) { return static_cast<std::byte>(c); });
    }
};

TEST_F(VFStreamTest, BasicReading) {
    dp::vfstream stream(test_data);

    std::string line;
    std::getline(stream, line);
    EXPECT_EQ(line, "Hello, World!");

    std::getline(stream, line);
    EXPECT_EQ(line, "This is a test string.");

    std::getline(stream, line);
    EXPECT_EQ(line, "With multiple lines.");

    // Try to read one more line to trigger EOF
    std::string empty_line;
    std::getline(stream, empty_line);
    EXPECT_TRUE(stream.eof());
}

TEST_F(VFStreamTest, SeekOperations) {
    dp::vfstream stream(test_data);

    // Seek to end
    stream.seekg(0, std::ios::end);
    auto end_pos = stream.tellg();
    EXPECT_EQ(end_pos, static_cast<std::streampos>(test_data.size()));

    // Seek to beginning
    stream.seekg(0, std::ios::beg);
    auto beg_pos = stream.tellg();
    EXPECT_EQ(beg_pos, 0);

    // Read from beginning
    char buffer[6];
    stream.read(buffer, 5);
    buffer[5] = '\0';
    EXPECT_STREQ(buffer, "Hello");
}

TEST_F(VFStreamTest, ReadOperations) {
    dp::vfstream stream(test_data);

    // Read single character
    char c = stream.get();
    EXPECT_EQ(c, 'H');

    // Read block
    char buffer[12];
    stream.read(buffer, 11);
    buffer[11] = '\0';
    EXPECT_STREQ(buffer, "ello, World");

    // Check stream state
    EXPECT_TRUE(stream.good());
    EXPECT_FALSE(stream.eof());
}

TEST_F(VFStreamTest, EmptyStream) {
    std::vector<std::byte> empty_data;
    dp::vfstream stream(empty_data);

    std::string line;
    bool result = static_cast<bool>(std::getline(stream, line));
    EXPECT_FALSE(result);
    EXPECT_TRUE(stream.eof());
}

TEST_F(VFStreamTest, BinaryData) {
    std::vector<std::byte> binary_data(256);
    for (size_t i = 0; i < binary_data.size(); ++i) {
        binary_data[i] = static_cast<std::byte>(i);
    }

    dp::vfstream stream(binary_data);

    // Read all binary data
    std::vector<char> read_data(256);
    stream.read(read_data.data(), 256);

    EXPECT_EQ(stream.gcount(), 256);

    for (size_t i = 0; i < 256; ++i) {
        EXPECT_EQ(static_cast<unsigned char>(read_data[i]), i);
    }
}

TEST_F(VFStreamTest, StreamCompatibility) {
    dp::vfstream stream(test_data);

    // Test that it works with standard stream functions
    std::ostringstream oss;
    oss << stream.rdbuf();

    EXPECT_EQ(oss.str(), test_string);
}