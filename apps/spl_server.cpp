#include <gflags/gflags.h>
#include <unistd.h>

#include <iostream>

#include "common/rpc.h"
#include "replica_config.h"
#include "rpc/client.h"
#include "server.h"

static bool validate_port(const char* flagname, int32 value) {
    if (value > 0 && value < 32768) {  // value is ok
        return true;
    }

    fprintf(stderr, "ERROR: Invalid value for -%s (%d)", flagname, (int)value);
    fprintf(stderr, ": port must be in [1, 32768)\n");
    return false;
}

static bool validate_nthreads(const char* flagname, int64 value) {
    if (value >= 4 && value <= 50) {  // value is ok
        return true;
    }

    fprintf(stderr, "ERROR: Invalid value for -%s (%d)", flagname, (int)value);
    fprintf(stderr, ": must use between 4 and 50 threads\n");
    return false;
}

// Server initialization flags
DEFINE_int32(serverid, -1, "The ID of the server");
DEFINE_int32(
    raftport, 10001,
    "The port over which communication for replication should take place");
DEFINE_int32(joinport, 10002,
             "The port over which replica server joining communication should "
             "take place");
DEFINE_int32(clientport, 10003,
             "The port over which client communication should take place");
DEFINE_string(seed, "",
              "The endpoint of the seed replica server that will introduce "
              "this server to the cluster. If empty, this server will start a "
              "new cluster.");
DEFINE_int64(nthreads, 4, "The number of threads to use for RPC handling");

DEFINE_validator(raftport, &validate_port);
DEFINE_validator(clientport, &validate_port);
DEFINE_validator(joinport, &validate_port);
DEFINE_validator(nthreads, &validate_nthreads);

// SplinterDB initialization flags
DEFINE_uint64(dbfilesize, 1024,
              "The size of the SplinterDB device (in MB); fixed when created");
DEFINE_uint64(cachesize, 64,
              "The size of the cache (in MB); can be changed across boots");
DEFINE_uint64(
    maxkeysize, 100,
    "The maximum size of a key (in bytes) that can be stored in SplinterDB");

using replicated_splinterdb::LogLevel;
using replicated_splinterdb::replica_config;
using replicated_splinterdb::server;

static void try_join_cluster(const replica_config& cfg) {
    auto delim = FLAGS_seed.find(':');
    std::string join_srv_host = FLAGS_seed.substr(0, delim);
    int join_srv_port = std::stoi(FLAGS_seed.substr(delim + 1));

    if (1 > join_srv_port || join_srv_port > 65535) {
        std::string msg = "invalid port number for host \"" + join_srv_host +
                          "\": " + std::to_string(join_srv_port);
        throw std::runtime_error(msg);
    }

    auto checked_port = static_cast<uint16_t>(join_srv_port);
    rpc::client cl{join_srv_host, checked_port};

    std::cout << "Attempting to join cluster at " << FLAGS_seed << " ... "
              << std::flush;

    auto [rc, msg] = cl.call(RPC_JOIN_REPLICA_GROUP, cfg.server_id_,
                             cfg.addr_ + ":" + std::to_string(cfg.raft_port_),
                             cfg.addr_ + ":" + std::to_string(cfg.client_port_))
                         .as<std::tuple<int32_t, std::string>>();
    std::cout << msg << " (rc=" << rc << ")" << std::endl;

    if (rc != 0) {
        std::cerr << "ERROR: failed to join cluster: " << msg << std::endl;
        exit(1);
    }
}

int main(int argc, char** argv) {
    gflags::SetUsageMessage(
        "The executable to start a replicated SplinterDB instance");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_serverid < 1) {
        std::cerr << "ERROR: flag '-serverid' is required. "
                  << "The server ID must be larger than 0." << std::endl;
        return 1;
    }

    uint16_t raft_port = static_cast<uint16_t>(FLAGS_raftport);
    uint16_t client_port = static_cast<uint16_t>(FLAGS_clientport);
    uint16_t join_port = static_cast<uint16_t>(FLAGS_joinport);

    // Initialize data configuration, using default key-comparison handling.
    data_config splinter_data_cfg;
    default_data_config_init(FLAGS_maxkeysize, &splinter_data_cfg);

    // Basic configuration of a SplinterDB instance
    splinterdb_config splinterdb_cfg;
    memset(&splinterdb_cfg, 0, sizeof(splinterdb_cfg));
    std::string dbname = "sm-state-" + std::to_string(FLAGS_serverid) + ".db";
    splinterdb_cfg.filename = dbname.c_str();
    splinterdb_cfg.disk_size = (FLAGS_dbfilesize * 1024 * 1024);
    splinterdb_cfg.cache_size = (FLAGS_cachesize * 1024 * 1024);
    splinterdb_cfg.data_cfg = &splinter_data_cfg;

    replica_config cfg{splinter_data_cfg, splinterdb_cfg};
    cfg.server_id_ = FLAGS_serverid;

    char hostnamebuf[100];
    gethostname(hostnamebuf, sizeof(hostnamebuf));
    cfg.addr_ = hostnamebuf;
    cfg.raft_port_ = raft_port;
    cfg.client_port_ = client_port;

    cfg.log_level_ = LogLevel::TRACE;
    cfg.display_level_ = LogLevel::DISABLED;

    server s{client_port, join_port, cfg};
    std::cout << "Listening for replication RPCs on port " << cfg.raft_port_
              << std::endl;

    if (!FLAGS_seed.empty()) {
        try_join_cluster(cfg);
    }

    s.run(FLAGS_nthreads);

    return 0;
}