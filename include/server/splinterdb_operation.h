#ifndef REPLICATED_SPLINTERDB_SERVER_SPLINTERDB_OPERATION_H
#define REPLICATED_SPLINTERDB_SERVER_SPLINTERDB_OPERATION_H

#include <optional>

#include "server/owned_slice.h"

namespace replicated_splinterdb {

class splinterdb_operation {
  public:
    enum splinterdb_operation_type : uint8_t { PUT, UPDATE, DELETE };

    nuraft::ptr<nuraft::buffer> serialize() const;

    const owned_slice& key() const { return key_; }

    const owned_slice& value() const { return *value_; }

    splinterdb_operation_type type() const { return type_; }

    static splinterdb_operation deserialize(nuraft::buffer& payload_in);

    static splinterdb_operation make_put(owned_slice&& key,
                                         owned_slice&& value);

    static splinterdb_operation make_update(owned_slice&& key,
                                            owned_slice&& value);

    static splinterdb_operation make_delete(owned_slice&& key);

  private:
    splinterdb_operation(owned_slice&& key, std::optional<owned_slice>&& value,
                         splinterdb_operation_type type);

    splinterdb_operation() = delete;

    owned_slice key_;
    std::optional<owned_slice> value_;
    splinterdb_operation_type type_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_SERVER_SPLINTERDB_OPERATION_H