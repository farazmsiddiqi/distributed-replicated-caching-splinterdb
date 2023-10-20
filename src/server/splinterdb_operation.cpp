#include "server/splinterdb_operation.h"

#include "libnuraft/buffer.hxx"

namespace replicated_splinterdb {

using nuraft::buffer;
using nuraft::buffer_serializer;
using nuraft::ptr;

ptr<buffer> splinterdb_operation::serialize() const {
    size_t buffer_size = sizeof(type_) + key_.serialized_size();
    if (value_.has_value()) {
        buffer_size += value_.value().serialized_size();
    }

    ptr<buffer> buf = buffer::alloc(buffer_size);
    buffer_serializer bs(buf);

    bs.put_u8(type_);
    key_.serialize(bs);
    if (value_.has_value()) {
        value_.value().serialize(bs);
    }

    return buf;
}

splinterdb_operation::splinterdb_operation(owned_slice&& key,
                                           std::optional<owned_slice>&& value,
                                           splinterdb_operation_type type)
    : key_(std::forward<owned_slice>(key)),
      value_(std::forward<std::optional<owned_slice>>(value)),
      type_(type) {}

splinterdb_operation splinterdb_operation::deserialize(buffer& payload_in) {
    buffer_serializer bs(payload_in);

    auto opty = static_cast<splinterdb_operation_type>(bs.get_u8());
    owned_slice key_buf;
    owned_slice::deserialize(key_buf, bs);

    std::optional<owned_slice> value_buf;
    if (opty == splinterdb_operation::PUT ||
        opty == splinterdb_operation::UPDATE) {
        owned_slice value;
        owned_slice::deserialize(value, bs);
        value_buf = std::move(value);
    }

    return splinterdb_operation{std::move(key_buf), std::move(value_buf), opty};
}

splinterdb_operation splinterdb_operation::make_put(owned_slice&& key,
                                                    owned_slice&& value) {
    return splinterdb_operation{std::forward<owned_slice>(key),
                                std::forward<owned_slice>(value), PUT};
}

splinterdb_operation splinterdb_operation::make_update(owned_slice&& key,
                                                       owned_slice&& value) {
    return splinterdb_operation{std::forward<owned_slice>(key),
                                std::forward<owned_slice>(value), UPDATE};
}

splinterdb_operation splinterdb_operation::make_delete(owned_slice&& key) {
    return splinterdb_operation{std::forward<owned_slice>(key), std::nullopt,
                                DELETE};
}

}  // namespace replicated_splinterdb