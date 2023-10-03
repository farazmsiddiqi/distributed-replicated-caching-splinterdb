#include <cstring>
#include <iostream>

#include "replica.h"
#include "replica_config.h"
#include "splinterdb_wrapper.h"

#define DB_FILE_NAME "replicated_splinterdb"
#define DB_FILE_SIZE_MB 1024  // Size of SplinterDB device; Fixed when created
#define CACHE_SIZE_MB 64      // Size of cache; can be changed across boots

/* Application declares the limit of key-sizes it intends to use */
#define USER_MAX_KEY_SIZE ((int)100)

using nuraft::buffer;
using nuraft::cmd_result_code;
using nuraft::ptr;
using replicated_splinterdb::owned_slice;
using replicated_splinterdb::replica;
using replicated_splinterdb::replica_config;
using replicated_splinterdb::Result;
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
    // Initialize data configuration, using default key-comparison handling.
    data_config splinter_data_cfg;
    default_data_config_init(USER_MAX_KEY_SIZE, &splinter_data_cfg);

    // Basic configuration of a SplinterDB instance
    splinterdb_config splinterdb_cfg;
    memset(&splinterdb_cfg, 0, sizeof(splinterdb_cfg));
    splinterdb_cfg.filename = DB_FILE_NAME;
    splinterdb_cfg.disk_size = (DB_FILE_SIZE_MB * 1024 * 1024);
    splinterdb_cfg.cache_size = (CACHE_SIZE_MB * 1024 * 1024);
    splinterdb_cfg.data_cfg = &splinter_data_cfg;

    replica_config replica_cfg{splinter_data_cfg, splinterdb_cfg};
    replica_cfg.server_id_ = std::atoi(argv[1]);
    replica_cfg.addr_ = "localhost";
    replica_cfg.port_ = std::atoi(argv[2]);

    replica_cfg.log_level_ = SimpleLogger::TRACE;
    replica_cfg.display_level_ = SimpleLogger::DISABLED;

    replica replica_instance{replica_cfg};
    replica_instance.initialize();

    std::string prompt = "spl " + std::to_string(replica_cfg.server_id_) + "> ";
    char cmd[1000];

    while (true) {
#if defined(__linux__) || defined(__APPLE__)
        std::cout << _CLM_GREEN << prompt << _CLM_END;
#else
        std::cout << prompt;
#endif
        if (!std::cin.getline(cmd, 1000)) {
            break;
        }
        auto tokens(tokenize(cmd));

        if (tokens[0] == "exit") {
            break;
        } else if (tokens[0] == "add") {
            if (tokens.size() >= 3) {
                replica_instance.add_server(std::atoi(tokens[1].c_str()),
                                            tokens[2]);
            }
        } else if (tokens[0] == "put" && tokens.size() >= 3) {
            replica_instance.append_log(
                splinterdb_operation::make_put(tokens[1], tokens[2]),
                handle_result);
        } else if (tokens[0] == "update" && tokens.size() >= 3) {
            replica_instance.append_log(
                splinterdb_operation::make_update(tokens[1], tokens[2]),
                handle_result);
        } else if (tokens[0] == "delete" && tokens.size() >= 2) {
            replica_instance.append_log(
                splinterdb_operation::make_delete(tokens[1]), handle_result);
        } else if (tokens[0] == "get" && tokens.size() >= 2) {
            Result<owned_slice, int32_t> lookup = replica_instance.read(
                slice_create(tokens[1].size(), tokens[1].c_str()));

            if (lookup.is_ok()) {
                std::cout << "value: " << lookup->to_string() << std::endl;
            } else {
                std::cout << "key " << std::quoted(tokens[1])
                          << " not found: rc=" << lookup.unwrap_err()
                          << std::endl;
            }
        }
    }

    return 0;
}