#ifndef REPLICATED_SPLINTERDB_REPLICA_H
#define REPLICATED_SPLINTERDB_REPLICA_H

#include "libnuraft/nuraft.hxx"
#include "logger.h"
#include "owned_slice.h"
#include "replica_config.h"
#include "result.h"
#include "splinterdb_operation.h"
#include "timer.h"

namespace replicated_splinterdb {

class splinterdb_state_machine;

void default_raft_params_init(nuraft::raft_params& params);

class replica {
  public:
    using raft_result = nuraft::cmd_result<nuraft::ptr<nuraft::buffer>>;

    using handle_commit_result = std::function<void(
        nuraft::ptr<Timer>, raft_result&, nuraft::ptr<std::exception>&)>;

    replica() = delete;

    ~replica();

    replica(const replica&) = delete;

    replica& operator=(const replica&) = delete;

    explicit replica(const replica_config& config);

    void initialize();

    result_t<owned_slice, int32_t> read(slice&& key);

    std::pair<nuraft::cmd_result_code, std::string>
    add_server(int32_t server_id, const std::string& endpoint);

    std::pair<nuraft::cmd_result_code, std::string>
    add_server(const nuraft::srv_config& config);

    void append_log(const splinterdb_operation& operation,
                    handle_commit_result handle_result);

    /**
     * Shutdown Raft server and ASIO service.
     * If this function is hanging even after the given timeout,
     * it will do force return.
     *
     * @param time_limit_sec Waiting timeout in seconds.
     */
    void shutdown(size_t time_limit_sec);

  private:
    int server_id_;
    std::string addr_;
    int port_;
    std::string endpoint_;

    replica_config config_;

    nuraft::ptr<SimpleLogger> logger_;
    FILE* spl_log_file_;
    nuraft::ptr<splinterdb_state_machine> sm_;
    nuraft::ptr<nuraft::state_mgr> smgr_;
    nuraft::raft_launcher launcher_;
    nuraft::ptr<nuraft::raft_server> raft_instance_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_REPLICA_H