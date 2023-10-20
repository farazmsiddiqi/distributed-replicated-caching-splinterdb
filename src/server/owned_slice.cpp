#include "server/owned_slice.h"

#include <cstring>

namespace replicated_splinterdb {

using nuraft::buffer_serializer;

owned_slice::owned_slice(std::vector<uint8_t>&& data) : data_(data) {}

owned_slice::owned_slice(size_t length) : data_(length) {}

owned_slice::owned_slice(const void* data, size_t length)
    : data_(static_cast<const uint8_t*>(data),
            static_cast<const uint8_t*>(data) + length) {}

owned_slice::owned_slice(const slice& spl_slice)
    : owned_slice((char*)slice_data(spl_slice), slice_length(spl_slice)) {}

owned_slice::owned_slice(const std::string& str)
    : data_(str.begin(), str.end()) {}

owned_slice::owned_slice(const char* cstring)
    : owned_slice(cstring, strlen(cstring)) {}

void owned_slice::serialize(buffer_serializer& bs) const {
    bs.put_u64(data_.size());
    bs.put_raw(data_.data(), data_.size());
}

void owned_slice::deserialize(owned_slice& slice_out, buffer_serializer& bs) {
    size_t len = bs.get_u64();
    uint8_t* bytes = static_cast<uint8_t*>(bs.get_raw(len));
    slice_out.data_.assign(bytes, bytes + len);
}

size_t owned_slice::serialized_size() const {
    return sizeof(uint64_t) + data_.size();
}

size_t owned_slice::size() const { return data_.size(); }

const std::vector<uint8_t>& owned_slice::data() const { return data_; }

void owned_slice::fill_slice(slice& slice_out) const {
    slice_out.length = data_.size();
    slice_out.data = data_.data();
}

std::string owned_slice::to_string() const {
    return {data_.begin(), data_.end()};
}

std::vector<uint8_t>&& owned_slice::take_data(owned_slice&& slice) {
    return std::move(slice.data_);
}

}  // namespace replicated_splinterdb