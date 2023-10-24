#include "server/server.h"

#include <iostream>

#include "common/rpc.h"
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

void server::run(uint64_t nthreads) {
    client_srv_.async_run(static_cast<size_t>(nthreads));
    std::cout << "Listening for client RPCs on port " << client_srv_.port()
              << std::endl;

    std::cout << "Listening for cluster join RPCs on port " << join_srv_.port()
              << std::endl;

    join_srv_.run();
}

static rpc_mutation_result extract_result(ptr<replica::raft_result> result) {
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
    // (int32_t, std::string, std::string) -> (int32_t, std::string)
    join_srv_.bind(RPC_JOIN_REPLICA_GROUP,
                   [this](int32_t server_id, std::string raft_endpoint,
                          std::string client_endpoint) {
                       auto [rc, msg] = replica_instance_.add_server(
                           server_id, raft_endpoint, client_endpoint);
                       return std::make_tuple(static_cast<int32_t>(rc), msg);
                   });

    // void -> void
    client_srv_.bind(RPC_SPLINTERDB_DUMPCACHE,
                     [this]() { replica_instance_.dump_cache(); });

    // void -> std::string
    client_srv_.bind(RPC_PING, []() { return "pong"; });

    // void -> int32_t
    client_srv_.bind(RPC_GET_SRV_ID,
                     [this]() { return replica_instance_.get_id(); });

    // void -> int32_t
    client_srv_.bind(RPC_GET_LEADER_ID,
                     [this]() { return replica_instance_.get_leader(); });

    // void -> std::vector<std::tuple<int32_t, std::string>>
    client_srv_.bind(RPC_GET_ALL_SERVERS, [this]() {
        std::vector<ptr<nuraft::srv_config>> configs;
        replica_instance_.get_all_servers(configs);

        std::vector<std::tuple<int32_t, std::string>> result;
        for (auto& srv : configs) {
            result.emplace_back(srv->get_id(), srv->get_aux());
        }

        return result;
    });

    // int32_t -> std::string
    client_srv_.bind(RPC_GET_SRV_ENDPOINT, [this](int32_t server_id) {
        auto srv = replica_instance_.get_server_info(server_id);

        if (srv) {
            return srv->get_aux();
        } else {
            rpc::this_handler().respond_error(
                std::make_tuple("Invalid server id"));
            return std::string{};
        }
    });

    // std::vector<uint8_t> -> rpc_read_result
    client_srv_.bind(RPC_SPLINTERDB_GET, [this](vector<uint8_t> key) {
        slice key_slice = slice_create(key.size(), key.data());
        auto [slice, rc] = replica_instance_.read(std::move(key_slice));

        if (rc == 0) {
            return rpc_read_result{owned_slice::take_data(std::move(slice)), 0};
        } else {
            return rpc_read_result{{}, rc};
        }
    });

    // (std::vector<uint8_t>, std::vector<uint8_t>) -> rpc_mutation_result
    client_srv_.bind(
        RPC_SPLINTERDB_PUT, [this](vector<uint8_t> key, vector<uint8_t> value) {
            splinterdb_operation op{splinterdb_operation::make_put(
                std::move(key), std::move(value))};
            ptr<replica::raft_result> result = replica_instance_.append_log(op);

            return extract_result(result);
        });

    // std::vector<uint8_t> -> rpc_mutation_result
    client_srv_.bind(RPC_SPLINTERDB_DELETE, [this](vector<uint8_t> key) {
        splinterdb_operation op{
            splinterdb_operation::make_delete(std::move(key))};
        ptr<replica::raft_result> result = replica_instance_.append_log(op);

        return extract_result(result);
    });

    // (std::vector<uint8_t>, std::vector<uint8_t>) -> rpc_mutation_result
    client_srv_.bind(RPC_SPLINTERDB_UPDATE, [this](vector<uint8_t> key,
                                                   vector<uint8_t> value) {
        splinterdb_operation op{
            splinterdb_operation::make_put(std::move(key), std::move(value))};
        ptr<replica::raft_result> result = replica_instance_.append_log(op);

        return extract_result(result);
    });
}

}  // namespace replicated_splinterdb