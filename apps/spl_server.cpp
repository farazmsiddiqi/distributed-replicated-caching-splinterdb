#include <gflags/gflags.h>

#include <cstring>
#include <iostream>

#include "replica_config.h"
#include "server.h"
#include "splinterdb_wrapper.h"

static bool validate_port(const char* flagname, int32 value) {
    if (value > 0 && value < 32768) {  // value is ok
        return true;
    }

    fprintf(stderr, "ERROR: Invalid value for -%s (%d)", flagname, (int)value);
    fprintf(stderr, ": port must be in [1, 32768)\n");
    return false;
}

DEFINE_int32(serverid, -1, "The ID of the server");
DEFINE_int32(
    raftport, 10001,
    "The port over which communication for replication should take place");
DEFINE_int32(clientport, 10002,
             "The port over which client communication should take place");
DEFINE_int32(joinport, 10003,
             "The port over which replica server joining communication should "
             "take place");
DEFINE_string(join_endpoint, "",
              "The endpoint of the seed replica server to join");

DEFINE_validator(raftport, &validate_port);
DEFINE_validator(clientport, &validate_port);
DEFINE_validator(joinport, &validate_port);

#define DB_FILE_SIZE_MB 1024  // Size of SplinterDB device; Fixed when created
#define CACHE_SIZE_MB 64      // Size of cache; can be changed across boots

/* Application declares the limit of key-sizes it intends to use */
#define USER_MAX_KEY_SIZE ((int)100)

using replicated_splinterdb::LogLevel;
using replicated_splinterdb::replica_config;
using replicated_splinterdb::server;

int main(int argc, char** argv) {
    gflags::SetUsageMessage(
        "The executable to start a replicated SplinterDB instance");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_serverid < 1) {
        std::cerr << "ERROR: flag '-serverid' is required. "
                  << "The server ID must be larger than 0." << std::endl;
        return 1;
    }

    uint16_t client_port = static_cast<uint16_t>(FLAGS_clientport);
    uint16_t join_port = static_cast<uint16_t>(FLAGS_joinport);

    // Initialize data configuration, using default key-comparison handling.
    data_config splinter_data_cfg;
    default_data_config_init(USER_MAX_KEY_SIZE, &splinter_data_cfg);

    // Basic configuration of a SplinterDB instance
    splinterdb_config splinterdb_cfg;
    memset(&splinterdb_cfg, 0, sizeof(splinterdb_cfg));
    std::string dbname = "sm-state-" + std::to_string(FLAGS_serverid) + ".db";
    splinterdb_cfg.filename = dbname.c_str();
    splinterdb_cfg.disk_size = (DB_FILE_SIZE_MB * 1024 * 1024);
    splinterdb_cfg.cache_size = (CACHE_SIZE_MB * 1024 * 1024);
    splinterdb_cfg.data_cfg = &splinter_data_cfg;

    replica_config replica_cfg{splinter_data_cfg, splinterdb_cfg};
    replica_cfg.server_id_ = FLAGS_serverid;
    replica_cfg.addr_ = "localhost";
    replica_cfg.port_ = FLAGS_raftport;

    replica_cfg.log_level_ = LogLevel::TRACE;
    replica_cfg.display_level_ = LogLevel::DISABLED;

    server s{client_port, join_port, replica_cfg};
    s.run();

    return 0;
}