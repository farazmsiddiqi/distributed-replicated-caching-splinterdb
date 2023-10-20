#ifndef REPLICATED_SPLINTERDB_SERVER_OWNED_SLICE_H
#define REPLICATED_SPLINTERDB_SERVER_OWNED_SLICE_H

#include <vector>

#include "libnuraft/buffer_serializer.hxx"
#include "splinterdb_wrapper.h"

namespace replicated_splinterdb {

class owned_slice {
  public:
    owned_slice();

    owned_slice(std::vector<uint8_t>&& data);

    owned_slice(const std::string& str);

    owned_slice(const char* cstring);

    owned_slice(const void* data, size_t length);

    owned_slice(const slice& spl_slice);

    owned_slice(const owned_slice&) = delete;

    owned_slice& operator=(const owned_slice&) = delete;

    owned_slice(owned_slice&& other) noexcept = default;

    owned_slice& operator=(owned_slice&& other) noexcept = default;

    static void deserialize(owned_slice& slice_out,
                            nuraft::buffer_serializer& bs);

    void serialize(nuraft::buffer_serializer& bs) const;

    size_t serialized_size() const;

    size_t size() const;

    void fill_slice(slice& slice_out) const;

    std::string to_string() const;

    const std::vector<uint8_t>& data() const;

    static std::vector<uint8_t>&& take_data(owned_slice&& slice);

  private:
    std::vector<uint8_t> data_;

    explicit owned_slice(size_t length);

    void free();
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_SERVER_OWNED_SLICE_H