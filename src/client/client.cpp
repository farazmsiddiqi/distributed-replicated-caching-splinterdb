#include "client/client.h"

#include <iostream>

namespace replicated_splinterdb {

client::client(const std::string& host, uint16_t port) : c_{host, port} {
    try {
        if (c_.call("ping").as<std::string>() != "pong") {
            throw std::runtime_error("server returned unexpected response");
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        exit(1);
    }
}

rpc_read_result client::get(const std::vector<uint8_t>& key) {
    return c_.call("get", key).as<rpc_read_result>();
}

rpc_mutation_result client::put(const std::vector<uint8_t>& key,
                                const std::vector<uint8_t>& value) {
    return c_.call("put", key, value).as<rpc_mutation_result>();
}

rpc_mutation_result client::update(const std::vector<uint8_t>& key,
                                   const std::vector<uint8_t>& value) {
    return c_.call("update", key, value).as<rpc_mutation_result>();
}

rpc_mutation_result client::del(const std::vector<uint8_t>& key) {
    return c_.call("delete", key).as<rpc_mutation_result>();
}

}  // namespace replicated_splinterdb