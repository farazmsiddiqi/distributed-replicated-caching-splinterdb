#include "client/client.h"

#include <iostream>

#include "common/rpc.h"

namespace replicated_splinterdb {

client::client(const std::string& host, uint16_t port) : clients_() {
    rpc::client cl{host, port};

    try {
        if (cl.call(RPC_PING).as<std::string>() != "pong") {
            throw std::runtime_error("server returned unexpected response");
        }

        int32_t id = cl.call(RPC_GET_SRV_ID).as<int32_t>();
        clients_.emplace(id, std::move(cl));

        auto srvs = clients_[id]
                        .call(RPC_GET_ALL_SERVERS)
                        .as<std::vector<std::tuple<int32_t, std::string>>>();

        for (const auto& [srv_id, endpoint] : srvs) {
            if (id == srv_id) {
                continue;
            }

            auto delim_idx = endpoint.find(':');
            std::string srv_host = endpoint.substr(0, delim_idx);
            int srv_port = std::stoi(endpoint.substr(delim_idx + 1));

            if (1 > srv_port || srv_port > 65535) {
                std::string msg = "invalid port number for host \"" + srv_host +
                                  "\": " + std::to_string(srv_port);
                throw std::runtime_error(msg);
            }

            auto checked_port = static_cast<uint16_t>(srv_port);
            clients_.emplace(srv_id, rpc::client{srv_host, checked_port});
        }

        rpc::client& c = clients_.begin()->second;
        leader_id_ = c.call(RPC_GET_LEADER_ID).as<int32_t>();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        exit(1);
    }
}

// TODOs:
// 1. retry on failures
// 2. if failure is result of not being leader, query leader and try with them
// 3. implement round robin read policy
// 4. implement latency-based read policy?
rpc_read_result client::get(const std::vector<uint8_t>& key) {
    return clients_.begin()
        ->second.call(RPC_SPLINTERDB_GET, key)
        .as<rpc_read_result>();
}

rpc_mutation_result client::put(const std::vector<uint8_t>& key,
                                const std::vector<uint8_t>& value) {
    return clients_.begin()
        ->second.call(RPC_SPLINTERDB_PUT, key, value)
        .as<rpc_mutation_result>();
}

rpc_mutation_result client::update(const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& value) {
    return clients_.begin()
        ->second.call(RPC_SPLINTERDB_UPDATE, key, value)
        .as<rpc_mutation_result>();
}

rpc_mutation_result client::del(const std::vector<uint8_t>& key) {
    return clients_.begin()
        ->second.call(RPC_SPLINTERDB_DELETE, key)
        .as<rpc_mutation_result>();
}

}  // namespace replicated_splinterdb