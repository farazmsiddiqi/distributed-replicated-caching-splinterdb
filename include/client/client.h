#ifndef REPLICATED_SPLINTERDB_CLIENT_CLIENT_H
#define REPLICATED_SPLINTERDB_CLIENT_CLIENT_H

#include "common/types.h"
#include "rpc/client.h"

namespace replicated_splinterdb {

class client {
  public:
    client() = delete;

    client(const client&) = delete;

    client& operator=(const client&) = delete;

    client(const std::string& host, uint16_t port);

    rpc_read_result get(const std::vector<uint8_t>& key);

    rpc_mutation_result put(const std::vector<uint8_t>& key,
                            const std::vector<uint8_t>& value);

    rpc_mutation_result update(const std::vector<uint8_t>& key,
                               const std::vector<uint8_t>& value);

    rpc_mutation_result del(const std::vector<uint8_t>& key);

  private:
    rpc::client c_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_CLIENT_CLIENT_H