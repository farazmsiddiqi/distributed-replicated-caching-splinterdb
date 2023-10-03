#include "owned_slice.h"

#include <cstring>

namespace replicated_splinterdb {

owned_slice::owned_slice() : length_(0), data_(nullptr) {}

owned_slice::owned_slice(size_t length)
    : length_(length), data_(new char[length]) {}

void owned_slice::free() {
    if (data_ == nullptr) {
        return;
    }

    delete[] data_;
    length_ = 0;
    data_ = nullptr;
}

owned_slice::~owned_slice() { free(); }

owned_slice::owned_slice(const void* data, size_t length)
    : owned_slice(length) {
    memcpy(const_cast<char*>(this->data_), data, length);
}

owned_slice::owned_slice(const slice& spl_slice)
    : owned_slice((char*)spl_slice.data, spl_slice.length) {}

owned_slice::owned_slice(const std::string& str)
    : owned_slice(str.data(), str.length()) {}

owned_slice::owned_slice(const char* cstring)
    : owned_slice(cstring, strlen(cstring)) {}

owned_slice::owned_slice(owned_slice&& other) {
    free();
    length_ = other.length_;
    data_ = other.data_;

    other.length_ = 0;
    other.data_ = nullptr;
}

owned_slice& owned_slice::operator=(owned_slice&& other) {
    if (this != &other) {
        free();
        length_ = other.length_;
        data_ = other.data_;

        other.length_ = 0;
        other.data_ = nullptr;
    }

    return *this;
}

void owned_slice::serialize(nuraft::buffer_serializer& bs) const {
    bs.put_u64(length_);
    bs.put_raw(data_, length_);
}

void owned_slice::deserialize(owned_slice& slice_out,
                              nuraft::buffer_serializer& bs) {
    slice_out.length_ = bs.get_u64();

    void* bytes = bs.get_raw(slice_out.length_);
    slice_out.data_ = new char[slice_out.length_];
    memcpy(const_cast<char*>(slice_out.data_), bytes, slice_out.length_);
}

size_t owned_slice::serialized_size() const {
    return sizeof(length_) + length_;
}

size_t owned_slice::size() const { return length_; }

void owned_slice::fill_slice(slice& slice_out) const {
    slice_out.length = length_;
    slice_out.data = data_;
}

std::string owned_slice::to_string() const {
    return std::string(data_, length_);
}

}  // namespace replicated_splinterdb