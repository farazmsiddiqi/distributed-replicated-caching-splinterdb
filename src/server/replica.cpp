#include "server/replica.h"

#include <filesystem>
#include <iostream>

#include "in_memory_state_mgr.hxx"
#include "logger.h"
#include "server/splinterdb_wrapper.h"
#include "splinterdb_state_machine.h"

#define s_err _s_err(std::dynamic_pointer_cast<SimpleLogger>(logger_))
#define s_info _s_info(std::dynamic_pointer_cast<SimpleLogger>(logger_))
#define s_warn _s_warn(std::dynamic_pointer_cast<SimpleLogger>(logger_))

namespace replicated_splinterdb {

using nuraft::asio_service;
using nuraft::buffer;
using nuraft::cmd_result_code;
using nuraft::cs_new;
using nuraft::inmem_state_mgr;
using nuraft::ptr;
using nuraft::raft_params;
using nuraft::srv_config;

void replica::default_raft_params_init(raft_params& params) {
    // heartbeat: 100 ms, election timeout: 200 - 400 ms.
    params.heart_beat_interval_ = 100;
    params.election_timeout_lower_bound_ = 300;
    params.election_timeout_upper_bound_ = 500;

    // Up to 5 logs will be preserved ahead the last snapshot.
    params.reserved_log_items_ = 1000000;
    // Client timeout: 3000 ms.
    params.client_req_timeout_ = 3000;
    // According to this method, `append_log` function
    // should be handled differently.
    params.return_method_ = raft_params::blocking;
}

replica::replica(const replica_config& config)
    : config_(config),
      server_id_(config.server_id_),
      addr_(config.addr_),
      raft_port_(config.raft_port_),
      raft_endpoint_(addr_ + ":" + std::to_string(config.raft_port_)),
      client_endpoint_(addr_ + ":" + std::to_string(config_.client_port_)),
      logger_(nullptr),
      spl_log_file_(nullptr),
      sm_(nullptr),
      smgr_(nullptr),
      launcher_(),
      raft_instance_(nullptr) {
    if (!config_.server_id_) {
        throw std::invalid_argument("server_id must be set");
    }

    if (!std::filesystem::create_directories(".logs")) {
        std::cout << ".logs already exists ... skipping create" << std::endl;
    }

    // Set up Raft logging
    std::string raft_log_file_name = config_.raft_log_file_.value_or(
        ".logs/srv-" + std::to_string(server_id_) + ".log");
    nuraft::ptr<SimpleLogger> log =
        cs_new<SimpleLogger>(raft_log_file_name, config_.log_level_);
    log->setLogLevel(config_.log_level_);
    log->setDispLevel(config_.display_level_);
    log->setCrashDumpPath(".logs", true);
    log->start();

    logger_ = log;

    // Set up SplinterDB logging
    std::string spl_log_file_name = config_.splinterdb_log_file_.value_or(
        ".logs/spl-" + std::to_string(server_id_) + ".log");
    spl_log_file_ = fopen(spl_log_file_name.c_str(), "w");
    platform_set_log_streams(spl_log_file_, spl_log_file_);

    // Initialize SplinterDB state machine and state manager
    sm_ = cs_new<splinterdb_state_machine>(config_.splinterdb_cfg_,
                                           config_.snapshot_frequency_ <= 0);
    smgr_ =
        cs_new<inmem_state_mgr>(server_id_, raft_endpoint_, client_endpoint_);

    initialize();
}

replica::~replica() { fclose(spl_log_file_); }

void replica::initialize() {
    raft_params params;
    default_raft_params_init(params);
    params.snapshot_distance_ = std::max(0, config_.snapshot_frequency_);

    params.return_method_ = config_.get_return_method();

    asio_service::options asio_opt;
    asio_opt.thread_pool_size_ = config_.asio_thread_pool_size_;
    // asio_opt.worker_start_ = [](uint32_t) { };
    // asio_opt.worker_stop_ = [](uint32_t) { };

    raft_instance_ =
        launcher_.init(sm_, smgr_, logger_, raft_port_, asio_opt, params);

    if (!raft_instance_) {
        std::cerr << "Failed to initialize launcher (see the message "
                     "in the log file)."
                  << std::endl;
        logger_.reset();
        exit(-1);
    }

    // Wait until Raft server is ready (up to 5 seconds).
    std::cout << "Initializing Raft instance ";
    for (size_t ii = 0; ii < config_.initialization_retries_; ++ii) {
        if (raft_instance_->is_initialized()) {
            std::cout << " done" << std::endl;
            return;
        }

        std::cout << ".";
        fflush(stdout);
        replicated_splinterdb::sleep_ms(config_.initialization_delay_ms_);
    }

    std::cout << " FAILED" << std::endl;
    logger_.reset();
    exit(-1);
}

void replica::shutdown(size_t time_limit_sec) {
    launcher_.shutdown(time_limit_sec);
}

void replica::register_thread() {
    splinterdb_register_thread(sm_->get_splinterdb_handle());
}

void replica::dump_cache() {
    splinterdb_print_cache(sm_->get_splinterdb_handle(), "cachedump");
}

std::pair<owned_slice, int32_t> replica::read(slice&& key) {
    splinterdb_lookup_result result;
    splinterdb_lookup_result_init(sm_->get_splinterdb_handle(), &result, 0,
                                  NULL);

    int rc = splinterdb_lookup(sm_->get_splinterdb_handle(),
                               std::forward<slice>(key), &result);
    if (rc) {
        return {owned_slice{}, rc};
    }

    slice value;
    rc = splinterdb_lookup_result_value(&result, &value);
    if (rc) {
        return {owned_slice{}, rc};
    }

    return {owned_slice(value), rc};
}

std::pair<cmd_result_code, std::string> replica::add_server(
    int32_t server_id, const std::string& raft_endpoint,
    const std::string& client_endpoint) {
    srv_config conf(server_id, 0, raft_endpoint, client_endpoint, false);
    return add_server(conf);
}

std::pair<cmd_result_code, std::string> replica::add_server(
    const srv_config& srv_conf_to_add) {
    ptr<raft_result> ret = raft_instance_->add_srv(srv_conf_to_add);
    if (!ret->get_accepted()) {
        s_err << "failed to add server: " << ret->get_result_code();
    } else {
        s_info << "add_server succeeded [id=" << srv_conf_to_add.get_id()
               << ", endpoint=" << srv_conf_to_add.get_endpoint() << "]";
    }

    return std::make_pair(ret->get_result_code(), ret->get_result_str());
}

ptr<replica::raft_result> replica::append_log(const splinterdb_operation& op) {
    ptr<buffer> new_log(op.serialize());
    ptr<raft_result> ret = raft_instance_->append_entries({new_log});

    if (config_.get_return_method() == raft_params::blocking) {
        // Blocking mode:
        //   `append_entries` returns after getting a consensus,
        //   so that `ret` already has the result from state machine.
        return ret;
    } else /* if (config_.return_method_ == raft_params::async_handler) */ {
        throw std::runtime_error("async handler not implemented");
    }
}

}  // namespace replicated_splinterdb
