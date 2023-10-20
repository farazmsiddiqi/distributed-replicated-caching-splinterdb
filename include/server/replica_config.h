#ifndef REPLICATED_SPLINTERDB_SERVER_REPLICA_CONFIG_H
#define REPLICATED_SPLINTERDB_SERVER_REPLICA_CONFIG_H

#include <optional>

#include "libnuraft/nuraft.hxx"
#include "server/log_level.h"
#include "server/splinterdb_wrapper.h"

namespace replicated_splinterdb {

struct replica_config {
    replica_config(const data_config& splinterdb_data_cfg,
                   const splinterdb_config& splinterdb_cfg)
        : server_id_(0),
          port_(25000),
          addr_("localhost"),
          asio_thread_pool_size_(4),
          return_method_(nuraft::raft_params::blocking),
          snapshot_frequency_(0),
          initialization_delay_ms_(250),
          initialization_retries_(20),
          raft_log_file_(std::nullopt),
          log_level_(LogLevel::INFO),
          display_level_(LogLevel::WARNING),
          splinterdb_log_file_(std::nullopt),
          splinterdb_data_cfg_(splinterdb_data_cfg),
          splinterdb_cfg_(splinterdb_cfg) {
        splinterdb_cfg_.data_cfg = &splinterdb_data_cfg_;
    }

    // Replica identifier parameters

    int server_id_;
    uint16_t port_;
    std::string addr_;

    // Asio-specific parameters

    size_t asio_thread_pool_size_;

    // Raft-specific parameters

    nuraft::raft_params::return_method_type return_method_;
    int32_t snapshot_frequency_;
    size_t initialization_delay_ms_;
    size_t initialization_retries_;

    // Logging information

    std::optional<std::string> raft_log_file_;
    LogLevel log_level_;
    LogLevel display_level_;

    std::optional<std::string> splinterdb_log_file_;

    // SplinterDB state machine configuration

    data_config splinterdb_data_cfg_;
    splinterdb_config splinterdb_cfg_;

  private:
    replica_config() = delete;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_SERVER_REPLICA_CONFIG_H