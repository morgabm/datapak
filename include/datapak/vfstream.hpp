/**
 * @file vfstream.hpp
 * @brief Virtual file stream classes for reading archive data
 * @author DataPak Team
 */

#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <cstddef>

namespace dp {

/**
 * @brief Custom stream buffer for virtual file data
 *
 * This stream buffer implementation allows treating in-memory byte data
 * as a stream buffer, enabling standard C++ stream operations on
 * archive file contents.
 */
class vfstreambuf : public std::streambuf {
public:
    /**
     * @brief Construct stream buffer from byte data
     * @param data The byte data to create a stream buffer from
     */
    explicit vfstreambuf(std::vector<std::byte> data);

protected:
    /**
     * @brief Called when buffer is empty and more data is needed
     * @return Next character or EOF
     */
    int_type underflow() override;

    /**
     * @brief Extract characters from stream buffer
     * @param s Pointer to character array to fill
     * @param count Maximum number of characters to extract
     * @return Number of characters actually extracted
     */
    std::streamsize xsgetn(char_type* s, std::streamsize count) override;

    /**
     * @brief Seek to a position relative to some base
     * @param off Offset value
     * @param way Seek direction (beg, cur, end)
     * @param which Stream open mode flags
     * @return New position on success, invalid position on failure
     */
    pos_type seekoff(off_type off, std::ios_base::seekdir way,
                     std::ios_base::openmode which = std::ios_base::in) override;

    /**
     * @brief Seek to an absolute position
     * @param sp Absolute position to seek to
     * @param which Stream open mode flags
     * @return New position on success, invalid position on failure
     */
    pos_type seekpos(pos_type sp, std::ios_base::openmode which = std::ios_base::in) override;

private:
    std::vector<std::byte> data_; /**< The underlying byte data */
    std::size_t position_;        /**< Current read position */
};

/**
 * @brief Virtual file input stream for archive data
 *
 * This class provides a standard C++ input stream interface for reading
 * files from DataPak archives. It allows treating compressed archive
 * contents as regular input streams.
 */
class vfstream : public std::istream {
public:
    /**
     * @brief Construct virtual file stream from byte data
     * @param data The decompressed file data to stream
     */
    explicit vfstream(std::vector<std::byte> data);

    /**
     * @brief Virtual destructor
     */
    ~vfstream() override = default;

    // Disable copy operations
    vfstream(const vfstream&) = delete;
    vfstream& operator=(const vfstream&) = delete;

    // Enable move operations
    vfstream(vfstream&&) noexcept = default;
    vfstream& operator=(vfstream&&) noexcept = default;

private:
    std::unique_ptr<vfstreambuf> buffer_; /**< Custom stream buffer */
};

} // namespace dp