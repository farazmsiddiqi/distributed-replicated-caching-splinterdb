#include "server.h"

#include "common/types.h"

namespace replicated_splinterdb {

using nuraft::buffer;
using nuraft::ptr;
using std::vector;

server::server(uint16_t client_port, uint16_t join_port,
               const replica_config& cfg)
    : replica_instance_{cfg}, client_srv_{client_port}, join_srv_{join_port} {
    initialize();
}

server::~server() {
    client_srv_.stop();
    join_srv_.stop();
    replica_instance_.shutdown(5);
}

void server::run() {
    client_srv_.async_run(4);
    join_srv_.run();
}

rpc_mutation_result extract_result(ptr<replica::raft_result> result) {
    int32_t spl_rc = -1;
    if (result->get_accepted()) {
        ptr<buffer> buf = result->get();
        spl_rc = buf->get_int();
    }

    int32_t raft_rc = static_cast<int32_t>(result->get_result_code());
    return std::tuple<int32_t, int32_t, std::string>{spl_rc, raft_rc,
                                                     result->get_result_str()};
}

void server::initialize() {
    join_srv_.bind("join", [this](int32_t server_id, std::string endpoint) {
        replica_instance_.add_server(server_id, endpoint);
    });

    client_srv_.bind("ping", []() { return "pong"; });

    client_srv_.bind("get", [this](vector<uint8_t> key) {
        slice key_slice = slice_create(key.size(), key.data());
        auto [slice, rc] = replica_instance_.read(std::move(key_slice));

        if (rc == 0) {
            return rpc_read_result{owned_slice::take_data(std::move(slice)), 0};
        } else {
            return rpc_read_result{{}, rc};
        }
    });

    client_srv_.bind("put", [this](vector<uint8_t> key, vector<uint8_t> value) {
        splinterdb_operation op{
            splinterdb_operation::make_put(std::move(key), std::move(value))};
        ptr<replica::raft_result> result = replica_instance_.append_log(op);

        return extract_result(result);
    });

    client_srv_.bind("delete", [this](vector<uint8_t> key) {
        splinterdb_operation op{
            splinterdb_operation::make_delete(std::move(key))};
        ptr<replica::raft_result> result = replica_instance_.append_log(op);

        return extract_result(result);
    });

    client_srv_.bind(
        "update", [this](vector<uint8_t> key, vector<uint8_t> value) {
            splinterdb_operation op{splinterdb_operation::make_put(
                std::move(key), std::move(value))};
            ptr<replica::raft_result> result = replica_instance_.append_log(op);

            return extract_result(result);
        });
}

}  // namespace replicated_splinterdb