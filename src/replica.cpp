#include "replica.h"

#include <iostream>

#include "in_memory_state_mgr.hxx"
#include "logger.h"
#include "splinterdb_state_machine.h"
#include "splinterdb_wrapper.h"

#define s_err  _s_err(std::dynamic_pointer_cast<SimpleLogger>(logger_))
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

void default_raft_params_init(raft_params& params) {
    // heartbeat: 100 ms, election timeout: 200 - 400 ms.
    params.heart_beat_interval_ = 100;
    params.election_timeout_lower_bound_ = 200;
    params.election_timeout_upper_bound_ = 400;

    // Up to 5 logs will be preserved ahead the last snapshot.
    params.reserved_log_items_ = 1000000;
    // Client timeout: 3000 ms.
    params.client_req_timeout_ = 3000;
    // According to this method, `append_log` function
    // should be handled differently.
    params.return_method_ = raft_params::blocking;
}

replica::replica(const replica_config& config)
    : server_id_(config.server_id_),
      addr_(config.addr_),
      port_(config.port_),
      endpoint_(addr_ + ":" + std::to_string(port_)),
      config_(config),
      logger_(nullptr),
      spl_log_file_(nullptr),
      sm_(nullptr),
      smgr_(nullptr),
      launcher_(),
      raft_instance_(nullptr) {
    if (!config_.server_id_) {
        throw std::invalid_argument("server_id must be set");
    }

    // Set up Raft logging
    std::string raft_log_file_name = config_.raft_log_file_.value_or(
        "./srv-" + std::to_string(server_id_) + ".log");
    nuraft::ptr<SimpleLogger> log =
        cs_new<SimpleLogger>(raft_log_file_name, config_.log_level_);
    log->setLogLevel(config_.log_level_);
    log->setDispLevel(config_.display_level_);
    log->setCrashDumpPath("./", true);
    log->start();

    logger_ = log;

    // Set up SplinterDB logging
    std::string spl_log_file_name = config_.splinterdb_log_file_.value_or(
        "./spl-" + std::to_string(server_id_) + ".log");
    spl_log_file_ = fopen(spl_log_file_name.c_str(), "w");
    platform_set_log_streams(spl_log_file_, spl_log_file_);

    // Initialize SplinterDB state machine and state manager
    sm_ = cs_new<splinterdb_state_machine>(config_.splinterdb_cfg_,
                                           config_.snapshot_frequency_ <= 0);
    smgr_ = cs_new<inmem_state_mgr>(server_id_, endpoint_);
}

replica::~replica() { fclose(spl_log_file_); }

void replica::initialize() {
    raft_params params;
    default_raft_params_init(params);
    params.snapshot_distance_ = std::max(0, config_.snapshot_frequency_);

    params.return_method_ = config_.return_method_;

    asio_service::options asio_opt;
    asio_opt.thread_pool_size_ = config_.asio_thread_pool_size_;
    asio_opt.worker_start_ = [this](uint32_t) {
        std::cout << "starting thread" << std::endl;
#if _USE_SPLINTERDB_LOG_STORE
        ptr<inmem_state_mgr> mgr =
            std::dynamic_pointer_cast<inmem_state_mgr>(smgr_);
        splinterdb_register_thread(mgr->get_splinterdb_handle());
#endif
    };
    asio_opt.worker_stop_ = [this](uint32_t) {
        std::cout << "stopping thread" << std::endl;
#if _USE_SPLINTERDB_LOG_STORE
        ptr<inmem_state_mgr> mgr =
            std::dynamic_pointer_cast<inmem_state_mgr>(smgr_);
        splinterdb_deregister_thread(mgr->get_splinterdb_handle());
#endif
    };

    raft_instance_ =
        launcher_.init(sm_, smgr_, logger_, port_, asio_opt, params);

    if (!raft_instance_) {
        std::cerr << "Failed to initialize launcher (see the message "
                     "in the log file)."
                  << std::endl;
        logger_.reset();
        exit(-1);
    }

    // Wait until Raft server is ready (up to 5 seconds).
    std::cout << "initializing Raft instance ";
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

result_t<owned_slice, int32_t> replica::read(slice&& key) {
    splinterdb_lookup_result result;
    splinterdb_lookup_result_init(sm_->get_splinterdb_handle(), &result, 0,
                                  NULL);

    int rc = splinterdb_lookup(sm_->get_splinterdb_handle(),
                               std::forward<slice>(key), &result);
    if (rc) {
        return rc;
    }

    slice value;
    rc = splinterdb_lookup_result_value(&result, &value);
    if (rc) {
        return rc;
    }

    return owned_slice(value);
}

std::pair<cmd_result_code, std::string> replica::add_server(
    int32_t server_id, const std::string& endpoint) {
    srv_config srv_conf_to_add(server_id, endpoint);
    return add_server(srv_conf_to_add);
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

void replica::append_log(const splinterdb_operation& operation,
                         handle_commit_result handle_result) {
    ptr<buffer> new_log(operation.serialize());
    ptr<Timer> timer = cs_new<Timer>();
    ptr<raft_result> ret = raft_instance_->append_entries({new_log});

    if (!ret->get_accepted()) {
        // Log append rejected, usually because this node is not a leader.
        s_warn << "failed to append log: " << ret->get_result_code() << " ("
               << usToString(timer->getTimeUs()) << ")";
        return;
    }

    if (config_.return_method_ == raft_params::blocking) {
        // Blocking mode:
        //   `append_entries` returns after getting a consensus,
        //   so that `ret` already has the result from state machine.
        ptr<std::exception> err(nullptr);
        handle_result(timer, *ret, err);
    } else if (config_.return_method_ == raft_params::async_handler) {
        // Async mode:
        //   `append_entries` returns immediately.
        //   `handle_result` will be invoked asynchronously,
        //   after getting a consensus.
        ret->when_ready(std::bind(handle_result, timer, std::placeholders::_1,
                                  std::placeholders::_2));
    }
}

}  // namespace replicated_splinterdb