#include <cstring>
#include <iostream>

#include "replica.h"
#include "replica_config.h"
#include "rpc/server.h"
#include "rpc/this_handler.h"
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

    replica replica_instance{replica_cfg};

    rpc::server client_srv(client_port);
    rpc::server join_srv(join_port);

    join_srv.bind("join",
                  [&replica_instance](int32_t server_id, std::string endpoint) {
                      replica_instance.add_server(server_id, endpoint);
                  });

    client_srv.bind("get", [&replica_instance](std::vector<uint8_t> key) {
        slice key_slice = slice_create(key.size(), key.data());
        auto result = replica_instance.read(std::move(key_slice));
        if (result.is_ok()) {
            return owned_slice::take_data(std::move(*result));
        } else {
            rpc::this_handler().respond_error(result.unwrap_err());
            return std::vector<uint8_t>{};
        }
    });

    client_srv.bind("put", [&replica_instance](std::vector<uint8_t> key,
                                               std::vector<uint8_t> value) {
        splinterdb_operation op{
            splinterdb_operation::make_put(std::move(key), std::move(value))};
        ptr<replica::raft_result> result = replica_instance.append_log(op);

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

    client_srv.bind("delete", [&replica_instance](std::vector<uint8_t> key) {
        splinterdb_operation op{
            splinterdb_operation::make_delete(std::move(key))};
        ptr<replica::raft_result> result = replica_instance.append_log(op);

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

    client_srv.bind("update", [&replica_instance](std::vector<uint8_t> key,
                                                  std::vector<uint8_t> value) {
        splinterdb_operation op{
            splinterdb_operation::make_put(std::move(key), std::move(value))};
        ptr<replica::raft_result> result = replica_instance.append_log(op);

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

    client_srv.async_run(4);
    join_srv.run();

    return 0;
}