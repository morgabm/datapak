#include "datapak/vfstream.hpp"
#include <algorithm>

namespace dp {

vfstreambuf::vfstreambuf(std::vector<std::byte> data)
    : data_(std::move(data)), position_(0) {
    char* begin = reinterpret_cast<char*>(data_.data());
    char* end = begin + data_.size();
    setg(begin, begin, end);
}

std::streambuf::int_type vfstreambuf::underflow() {
    if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr());
    }
    return traits_type::eof();
}

std::streamsize vfstreambuf::xsgetn(char_type* s, std::streamsize count) {
    const std::streamsize available = egptr() - gptr();
    const std::streamsize to_read = std::min(count, available);

    if (to_read > 0) {
        std::copy_n(gptr(), to_read, s);
        gbump(static_cast<int>(to_read));
    }

    return to_read;
}

std::streambuf::pos_type vfstreambuf::seekoff(off_type off, std::ios_base::seekdir way,
                                              std::ios_base::openmode which) {
    if (which != std::ios_base::in) {
        return pos_type(off_type(-1));
    }

    off_type new_pos;
    switch (way) {
    case std::ios_base::beg:
        new_pos = off;
        break;
    case std::ios_base::cur:
        new_pos = (gptr() - eback()) + off;
        break;
    case std::ios_base::end:
        new_pos = static_cast<off_type>(data_.size()) + off;
        break;
    default:
        return pos_type(off_type(-1));
    }

    if (new_pos < 0 || new_pos > static_cast<off_type>(data_.size())) {
        return pos_type(off_type(-1));
    }

    char* new_gptr = eback() + new_pos;
    setg(eback(), new_gptr, egptr());

    return pos_type(new_pos);
}

std::streambuf::pos_type vfstreambuf::seekpos(pos_type sp, std::ios_base::openmode which) {
    return seekoff(off_type(sp), std::ios_base::beg, which);
}

vfstream::vfstream(std::vector<std::byte> data)
    : std::istream(nullptr), buffer_(std::make_unique<vfstreambuf>(std::move(data))) {
    rdbuf(buffer_.get());
}

} // namespace dp