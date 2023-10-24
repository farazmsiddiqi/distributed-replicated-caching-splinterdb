#include "client/client.h"

#include <iostream>

#include "common/rpc.h"

// TODOS: Implement latency-based read policy

#define NOT_LEADER (-3)

namespace replicated_splinterdb {

client::client(const std::string& host, uint16_t port, uint64_t timeout_ms,
               uint16_t num_retries)
    : clients_(), read_policy_(nullptr), num_retries_(num_retries) {
    rpc::client cl{host, port};

    std::vector<std::tuple<int32_t, std::string>> srvs;
    try {
        if (cl.call(RPC_PING).as<std::string>() != "pong") {
            throw std::runtime_error("server returned unexpected response");
        }

        srvs = cl.call(RPC_GET_ALL_SERVERS)
                   .as<std::vector<std::tuple<int32_t, std::string>>>();

        leader_id_ = cl.call(RPC_GET_LEADER_ID).as<int32_t>();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        exit(1);
    }

    for (const auto& [srv_id, endpoint] : srvs) {
        auto delim_idx = endpoint.find(':');
        std::string srv_host = endpoint.substr(0, delim_idx);
        int srv_port = std::stoi(endpoint.substr(delim_idx + 1));

        if (1 > srv_port || srv_port > 65535) {
            std::string msg = "invalid port number for host \"" + srv_host +
                              "\": " + std::to_string(srv_port);
            throw std::runtime_error(msg);
        }

        auto checked_port = static_cast<uint16_t>(srv_port);
        try {
            clients_.emplace(std::piecewise_construct,
                             std::forward_as_tuple(srv_id),
                             std::forward_as_tuple(srv_host, checked_port));
        } catch (const std::exception& e) {
            std::cerr << "WARNING: failed to connect to " << endpoint
                      << " ... skipping. Reason:\n\t" << e.what() << std::endl;
            continue;
        }
    }

    std::vector<int32_t> srv_ids;
    for (auto& [srv_id, c] : clients_) {
        c.set_timeout(static_cast<int64_t>(timeout_ms));
        srv_ids.push_back(srv_id);
    }

    read_policy_ = std::make_unique<round_robin_read_policy>(srv_ids);
}

void client::dump_cache() {
    for (auto& [_, c] : clients_) {
        c.call(RPC_SPLINTERDB_DUMPCACHE);
    }
}

rpc::client& client::get_leader_handle() { return clients_.at(leader_id_); }

bool client::try_handle_leader_change(int32_t raft_result_code) {
    if (raft_result_code == NOT_LEADER) {
        leader_id_ = get_leader_id();
        return true;
    }

    return false;
}

rpc_read_result client::get(const std::vector<uint8_t>& key) {
    return clients_.find(read_policy_->next_server())
        ->second.call(RPC_SPLINTERDB_GET, key)
        .as<rpc_read_result>();
}

rpc_mutation_result client::put(const std::vector<uint8_t>& key,
                                const std::vector<uint8_t>& value) {
    rpc_mutation_result result;
    for (uint16_t i = 0; i < num_retries_; ++i) {
        result = get_leader_handle()
                     .call(RPC_SPLINTERDB_PUT, key, value)
                     .as<rpc_mutation_result>();

        if (was_accepted(result)) {
            break;
        } else if (try_handle_leader_change(get_nuraft_return_code(result))) {
            std::cerr << "\nWARNING: leader changed, retrying..." << std::endl;
        }
    }

    return result;
}

rpc_mutation_result client::update(const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& value) {
    rpc_mutation_result result;
    for (uint16_t i = 0; i < num_retries_; ++i) {
        result = get_leader_handle()
                     .call(RPC_SPLINTERDB_UPDATE, key, value)
                     .as<rpc_mutation_result>();

        if (was_accepted(result)) {
            break;
        } else if (try_handle_leader_change(get_nuraft_return_code(result))) {
            std::cerr << "\nWARNING: leader changed, retrying..." << std::endl;
        }
    }

    return result;
}

rpc_mutation_result client::del(const std::vector<uint8_t>& key) {
    rpc_mutation_result result;
    for (uint16_t i = 0; i < num_retries_; ++i) {
        result = get_leader_handle()
                     .call(RPC_SPLINTERDB_DELETE, key)
                     .as<rpc_mutation_result>();

        if (was_accepted(result)) {
            break;
        } else if (try_handle_leader_change(get_nuraft_return_code(result))) {
            std::cerr << "\nWARNING: leader changed, retrying..." << std::endl;
        }
    }

    return result;
}

std::vector<std::tuple<int32_t, std::string>> client::get_all_servers() {
    for (auto& [srv_id, c] : clients_) {
        try {
            return c.call(RPC_GET_ALL_SERVERS)
                .as<std::vector<std::tuple<int32_t, std::string>>>();
        } catch (const std::exception& e) {
            std::cerr << "WARNING: failed to connect to " << srv_id
                      << " ... skipping. Reason:\n\t" << e.what() << std::endl;
            continue;
        }
    }

    throw std::runtime_error("failed to connect to any server");
}

int32_t client::get_leader_id() {
    for (auto& [srv_id, c] : clients_) {
        try {
            return c.call(RPC_GET_LEADER_ID).as<int32_t>();
        } catch (const std::exception& e) {
            std::cerr << "WARNING: failed to connect to " << srv_id
                      << " ... skipping. Reason:\n\t" << e.what() << std::endl;
            continue;
        }
    }

    throw std::runtime_error("failed to connect to any server");
}

}  // namespace replicated_splinterdb