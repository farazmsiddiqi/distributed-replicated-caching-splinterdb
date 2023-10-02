#include "owned_slice.h"

#include <cstring>

namespace replicated_splinterdb {

owned_slice::owned_slice() : length(0), data(nullptr) {}

owned_slice::~owned_slice() { delete[] data; }

owned_slice::owned_slice(const char* data, size_t length) {
    owned_slice::alloc(*this, length);
    memcpy(const_cast<char*>(this->data), data, length);
}

owned_slice::owned_slice(const slice& spl_slice)
    : owned_slice((char*)spl_slice.data, spl_slice.length) {}

owned_slice::owned_slice(const std::string& str)
    : owned_slice(str.c_str(), str.length()) {}

owned_slice::owned_slice(const char* cstring)
    : owned_slice(cstring, strlen(cstring)) {}

owned_slice::owned_slice(owned_slice&& other) {
    length = other.length;
    data = other.data;

    other.length = 0;
    other.data = nullptr;
}

owned_slice& owned_slice::operator=(owned_slice&& other) {
    if (this == &other) {
        return *this;
    }

    length = other.length;
    data = other.data;

    other.length = 0;
    other.data = nullptr;

    return *this;
}

void owned_slice::alloc(owned_slice& slice_out, size_t length) {
    slice_out.length = length;
    slice_out.data = new char[length];
}

void owned_slice::from_cstring(owned_slice& slice_out, const char* cstring) {
    owned_slice::alloc(slice_out, strlen(cstring));
    memcpy(const_cast<char*>(slice_out.data), cstring, slice_out.length);
}

void owned_slice::serialize(nuraft::buffer_serializer& bs) const {
    bs.put_u64(length);
    bs.put_raw(data, length);
}

void owned_slice::deserialize(owned_slice& slice_out,
                              nuraft::buffer_serializer& bs) {
    slice_out.length = bs.get_u64();

    void* bytes = bs.get_raw(slice_out.length);
    slice_out.data = new char[slice_out.length];
    memcpy(const_cast<char*>(slice_out.data), bytes, slice_out.length);
}

size_t owned_slice::serialized_size() const { return sizeof(length) + length; }

size_t owned_slice::size() const { return length; }

void owned_slice::fill_slice(slice& slice_out) const {
    slice_out.length = length;
    slice_out.data = data;
}

std::string owned_slice::to_string() const { return std::string(data, length); }

}  // namespace replicated_splinterdb