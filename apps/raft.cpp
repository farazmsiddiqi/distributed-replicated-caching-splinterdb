#include <cstring>
#include <iostream>

#include "replica.h"
#include "replica_config.h"
#include "rpc/server.h"
#include "splinterdb_wrapper.h"

#define DB_FILE_SIZE_MB 1024  // Size of SplinterDB device; Fixed when created
#define CACHE_SIZE_MB 64      // Size of cache; can be changed across boots

/* Application declares the limit of key-sizes it intends to use */
#define USER_MAX_KEY_SIZE ((int)100)

using nuraft::buffer;
using nuraft::cmd_result_code;
using nuraft::ptr;
using replicated_splinterdb::LogLevel;
using replicated_splinterdb::owned_slice;
using replicated_splinterdb::replica;
using replicated_splinterdb::replica_config;
using replicated_splinterdb::result_t;
using replicated_splinterdb::splinterdb_operation;
using replicated_splinterdb::Timer;

static std::vector<std::string> tokenize(const char* str, char c = ' ');

static void handle_result(ptr<Timer> timer, replica::raft_result& result,
                          ptr<std::exception>& err);

std::vector<std::string> tokenize(const char* str, char c) {
    std::vector<std::string> tokens;
    do {
        const char* begin = str;
        while (*str != c && *str) str++;
        if (begin != str) tokens.push_back(std::string(begin, str));
    } while (0 != *str++);

    return tokens;
}

void handle_result(ptr<Timer> timer, replica::raft_result& result,
                   ptr<std::exception>& err) {
    if (result.get_result_code() != cmd_result_code::OK) {
        // Something went wrong.
        // This means committing this log failed,
        // but the log itself is still in the log store.
        std::cout << "failed: " << result.get_result_code() << ", "
                  << replicated_splinterdb::usToString(timer->getTimeUs())
                  << std::endl;
        return;
    }
    ptr<buffer> buf = result.get();
    int32_t ret_value = buf->get_int();
    std::cout << "succeeded, "
              << replicated_splinterdb::usToString(timer->getTimeUs())
              << ", return code: " << ret_value << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <server id> <raft port> <rpc port>" << std::endl;
        return 1;
    }

    int server_id = std::atoi(argv[1]);
    uint16_t raft_port = std::atoi(argv[2]);
    uint16_t client_port = std::atoi(argv[3]);
    uint16_t join_port = std::atoi(argv[3]);

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
    replica_cfg.port_ = std::atoi(argv[2]);

    replica_cfg.log_level_ = LogLevel::TRACE;
    replica_cfg.display_level_ = LogLevel::DISABLED;

    replica replica_instance{replica_cfg};
    replica_instance.initialize();

    rpc::server client_srv(client_port);
    rpc::server join_srv(join_port);

    // join_port.bind("get", [&replica_instance](const char* key) { return
    // m.multiply(a, b); });

    join_srv.bind("join",
                  [&replica_instance](int32_t server_id, std::string endpoint) {
                      replica_instance.add_server(server_id, endpoint);
                  });

    return 0;
}