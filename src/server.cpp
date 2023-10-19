#include "server.h"

namespace replicated_splinterdb {

using nuraft::buffer;
using nuraft::ptr;
using std::vector;

server::server(uint16_t client_port,
               uint16_t join_port,
               const replica_config& cfg) 
    : replica_instance_{cfg},
      client_srv_{client_port},
      join_srv_{join_port} {
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

void server::initialize() {
    join_srv_.bind("join", [this](int32_t server_id, std::string endpoint) {
        replica_instance_.add_server(server_id, endpoint);
    });

    client_srv_.bind("get", [this](vector<uint8_t> key) {
        slice key_slice = slice_create(key.size(), key.data());
        auto result = replica_instance_.read(std::move(key_slice));
        if (result.is_ok()) {
            return owned_slice::take_data(std::move(*result));
        } else {
            rpc::this_handler().respond_error(result.unwrap_err());
            return vector<uint8_t>{};
        }
    });

    client_srv_.bind("put", [this](vector<uint8_t> key, vector<uint8_t> value) {
        splinterdb_operation op{
            splinterdb_operation::make_put(std::move(key), std::move(value))};
        ptr<replica::raft_result> result = replica_instance_.append_log(op);

        if (result->get_accepted()) {
            ptr<buffer> buf = result->get();
            return buf->get_int();
        } else {
            int code = static_cast<int>(result->get_result_code());
            rpc::this_handler().respond_error(
                std::make_pair(code, result->get_result_str()));
            return -1;
        }
    });

    client_srv_.bind("delete", [this](vector<uint8_t> key) {
        splinterdb_operation op{
            splinterdb_operation::make_delete(std::move(key))};
        ptr<replica::raft_result> result = replica_instance_.append_log(op);

        if (result->get_accepted()) {
            ptr<buffer> buf = result->get();
            return buf->get_int();
        } else {
            int code = static_cast<int>(result->get_result_code());
            rpc::this_handler().respond_error(
                std::make_pair(code, result->get_result_str()));
            return -1;
        }
    });

    client_srv_.bind("update", [this](vector<uint8_t> key,
                                      vector<uint8_t> value) {
        splinterdb_operation op{
            splinterdb_operation::make_put(std::move(key), std::move(value))};
        ptr<replica::raft_result> result = replica_instance_.append_log(op);

        if (result->get_accepted()) {
            ptr<buffer> buf = result->get();
            return buf->get_int();
        } else {
            int code = static_cast<int>(result->get_result_code());
            rpc::this_handler().respond_error(
                std::make_pair(code, result->get_result_str()));
            return -1;
        }
    });
}

}  // namespace replicated_splinterdb