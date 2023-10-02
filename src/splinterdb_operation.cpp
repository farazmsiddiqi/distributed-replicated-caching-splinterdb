#include "splinterdb_operation.h"

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

void splinterdb_operation::deserialize(buffer& payload_in,
                                       splinterdb_operation& operation_out) {
    buffer_serializer bs(payload_in);

    operation_out.type_ = static_cast<splinterdb_operation_type>(bs.get_u8());
    owned_slice::deserialize(operation_out.key_, bs);

    if (operation_out.type_ == splinterdb_operation::PUT ||
        operation_out.type_ == splinterdb_operation::UPDATE) {
        owned_slice value;
        owned_slice::deserialize(value, bs);
        operation_out.value_ = std::move(value);
    }
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