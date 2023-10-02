#ifndef REPLICATED_SPLINTERDB_OWNED_SLICE_H
#define REPLICATED_SPLINTERDB_OWNED_SLICE_H

#include "libnuraft/buffer_serializer.hxx"
#include "splinterdb_wrapper.h"
#include <cstring>

namespace replicated_splinterdb {

class owned_slice {
  public:
    owned_slice();

    owned_slice(const std::string& str);

    owned_slice(const char* cstring);

    owned_slice(const char* data, size_t length);

    owned_slice(const slice& spl_slice);

    ~owned_slice();

    owned_slice(const owned_slice&) = delete;

    owned_slice& operator=(const owned_slice&) = delete;

    owned_slice(owned_slice&& other);

    owned_slice& operator=(owned_slice&& other);

    static void alloc(owned_slice& slice_out, size_t length);

    static void from_cstring(owned_slice& slice_out, const char* cstring);

    static void deserialize(owned_slice& slice_out, nuraft::buffer_serializer& bs);

    void serialize(nuraft::buffer_serializer& bs) const;

    size_t serialized_size() const;

    size_t size() const;

    void fill_slice(slice& slice_out) const;

    std::string to_string() const;

  private:
    uint64_t length;
    const char* data;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_OWNED_SLICE_H