#ifndef REPLICATED_SPLINTERDB_SERVER_SERVER_H
#define REPLICATED_SPLINTERDB_SERVER_SERVER_H

#include "rpc/server.h"
#include "rpc/this_handler.h"
#include "server/replica.h"
#include "server/replica_config.h"

namespace replicated_splinterdb {

class server {
  public:
    server() = delete;

    ~server();

    server(const server&) = delete;

    server& operator=(const server&) = delete;

    server(uint16_t client_port, uint16_t join_port, const replica_config& cfg);

    void run();

  private:
    replica replica_instance_;

    rpc::server client_srv_;

    rpc::server join_srv_;

    void initialize();
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_SERVER_SERVER_H