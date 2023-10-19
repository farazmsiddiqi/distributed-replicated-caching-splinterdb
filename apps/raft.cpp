#include <cstring>
#include <iostream>

#include "replica_config.h"
#include "server.h"
#include "splinterdb_wrapper.h"

#define DB_FILE_SIZE_MB 1024  // Size of SplinterDB device; Fixed when created
#define CACHE_SIZE_MB 64      // Size of cache; can be changed across boots

/* Application declares the limit of key-sizes it intends to use */
#define USER_MAX_KEY_SIZE ((int)100)

using replicated_splinterdb::LogLevel;
using replicated_splinterdb::replica_config;
using replicated_splinterdb::server;

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <server id> <raft port> <client session port>"
                  << " <server session port>" << std::endl;
        return 1;
    }

    int server_id = std::atoi(argv[1]);
    uint16_t raft_port = std::atoi(argv[2]);
    uint16_t client_port = std::atoi(argv[3]);
    uint16_t join_port = std::atoi(argv[4]);

    // Initialize data configuration, using default key-comparison handling.
    data_config splinter_data_cfg;
    default_data_config_init(USER_MAX_KEY_SIZE, &splinter_data_cfg);

    // Basic configuration of a SplinterDB instance
    splinterdb_config splinterdb_cfg;
    memset(&splinterdb_cfg, 0, sizeof(splinterdb_cfg));
    std::string dbname = "spl-" + std::to_string(server_id) + ".db";
    splinterdb_cfg.filename = dbname.c_str();
    splinterdb_cfg.disk_size = (DB_FILE_SIZE_MB * 1024 * 1024);
    splinterdb_cfg.cache_size = (CACHE_SIZE_MB * 1024 * 1024);
    splinterdb_cfg.data_cfg = &splinter_data_cfg;

    replica_config replica_cfg{splinter_data_cfg, splinterdb_cfg};
    replica_cfg.server_id_ = server_id;
    replica_cfg.addr_ = "localhost";
    replica_cfg.port_ = raft_port;

    replica_cfg.log_level_ = LogLevel::TRACE;
    replica_cfg.display_level_ = LogLevel::DISABLED;

    server s{client_port, join_port, replica_cfg};
    s.run();

    return 0;
}