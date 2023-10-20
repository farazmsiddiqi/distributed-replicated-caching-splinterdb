#ifndef REPLICATED_SPLINTERDB_SPLINTERDB_STATE_MACHINE_H
#define REPLICATED_SPLINTERDB_SPLINTERDB_STATE_MACHINE_H

#include <map>

#include "libnuraft/nuraft.hxx"
#include "server/splinterdb_wrapper.h"
#include "splinterdb_snapshot.h"

namespace replicated_splinterdb {

class splinterdb_state_machine : public nuraft::state_machine {
  private:
    using Base = nuraft::state_machine;

    splinterdb_state_machine(const splinterdb_state_machine&) = delete;

    splinterdb_state_machine& operator=(const splinterdb_state_machine&) =
        delete;

  public:
    splinterdb_state_machine(const splinterdb_config& config,
                             bool disable_snapshots = false);

    ~splinterdb_state_machine();

    /**
     * Commit the given Raft log.
     *
     * NOTE:
     *   Given memory buffer is owned by caller, so that
     *   commit implementation should clone it if user wants to
     *   use the memory even after the commit call returns.
     *
     *   Here provide a default implementation for facilitating the
     *   situation when application does not care its implementation.
     *
     * @param log_idx Raft log number to commit.
     * @param data Payload of the Raft log.
     * @return Result value of state machine.
     */
    nuraft::ptr<nuraft::buffer> commit(const nuraft::ulong log_idx,
                                       nuraft::buffer& data) override;

    /**
     * Handler on the commit of a configuration change.
     *
     * @param log_idx Raft log number of the configuration change.
     * @param new_conf New cluster configuration.
     */
    void commit_config(const nuraft::ulong log_idx,
                       nuraft::ptr<nuraft::cluster_config>& new_conf) override;

    // We are not implementing any pre-commit or rollback functionality. Revert
    // to the default implementation.

    using Base::pre_commit;
    using Base::rollback;

    /**
     * Save the given snapshot object to local snapshot.
     * This API is for snapshot receiver (i.e., follower).
     *
     * This is an optional API for users who want to use logical
     * snapshot. Instead of splitting a snapshot into multiple
     * physical chunks, this API uses logical objects corresponding
     * to a unique object ID. Users are responsible for defining
     * what object is: it can be a key-value pair, a set of
     * key-value pairs, or whatever.
     *
     * Same as `commit()`, memory buffer is owned by caller.
     *
     * @param s Snapshot instance to save.
     * @param obj_id[in,out]
     *     Object ID.
     *     As a result of this API call, the next object ID
     *     that reciever wants to get should be set to
     *     this parameter.
     * @param data Payload of given object.
     * @param is_first_obj `true` if this is the first object.
     * @param is_last_obj `true` if this is the last object.
     */
    void save_logical_snp_obj(nuraft::snapshot& s, nuraft::ulong& obj_id,
                              nuraft::buffer& data, bool is_first_obj,
                              bool is_last_obj) override;

    /**
     * Apply received snapshot to state machine.
     *
     * @param s Snapshot instance to apply.
     * @returm `true` on success.
     */
    bool apply_snapshot(nuraft::snapshot& s) override;

    /**
     * Read the given snapshot object.
     * This API is for snapshot sender (i.e., leader).
     *
     * Same as above, this is an optional API for users who want to
     * use logical snapshot.
     *
     * @param s Snapshot instance to read.
     * @param[in,out] user_snp_ctx
     *     User-defined instance that needs to be passed through
     *     the entire snapshot read. It can be a pointer to
     *     state machine specific iterators, or whatever.
     *     On the first `read_logical_snp_obj` call, it will be
     *     set to `null`, and this API may return a new pointer if necessary.
     *     Returned pointer will be passed to next `read_logical_snp_obj`
     *     call.
     * @param obj_id Object ID to read.
     * @param[out] data Buffer where the read object will be stored.
     * @param[out] is_last_obj Set `true` if this is the last object.
     * @return Negative number if failed.
     */
    int read_logical_snp_obj(nuraft::snapshot& s, void*& user_snp_ctx,
                             nuraft::ulong obj_id,
                             nuraft::ptr<nuraft::buffer>& data_out,
                             bool& is_last_obj) override;

    /**
     * Free user-defined instance that is allocated by
     * `read_logical_snp_obj`.
     * This is an optional API for users who want to use logical snapshot.
     *
     * @param user_snp_ctx User-defined instance to free.
     */
    void free_user_snp_ctx(void*& user_snp_ctx) override;

    /**
     * Get the latest snapshot instance.
     *
     * This API will be invoked at the initialization of Raft server,
     * so that the last last snapshot should be durable for server restart,
     * if you want to avoid unnecessary catch-up.
     *
     * @return Pointer to the latest snapshot.
     */
    nuraft::ptr<nuraft::snapshot> last_snapshot() override;

    /**
     * Get the last committed Raft log number.
     *
     * This API will be invoked at the initialization of Raft server
     * to identify what the last committed point is, so that the last
     * committed index number should be durable for server restart,
     * if you want to avoid unnecessary catch-up.
     *
     * @return Last committed Raft log number.
     */
    nuraft::ulong last_commit_index() override;

    /**
     * Create a snapshot corresponding to the given info.
     *
     * @param s Snapshot info to create.
     * @param when_done Callback function that will be called after
     *                  snapshot creation is done.
     */
    void create_snapshot(
        nuraft::snapshot& s,
        nuraft::async_result<bool>::handler_type& when_done) override;

    /**
     * Decide to create snapshot or not.
     * Once the pre-defined condition is satisfied, Raft core will invoke
     * this function to ask if it needs to create a new snapshot.
     * If user-defined state machine does not want to create snapshot
     * at this time, this function will return `false`.
     *
     * @return `true` if wants to create snapshot.
     *         `false` if does not want to create snapshot.
     */
    inline bool chk_create_snapshot() override { return !disable_snapshots_; }

    using Base::allow_leadership_transfer;

    inline splinterdb* get_splinterdb_handle() const { return spl_handle_; }

  private:
    splinterdb* spl_handle_;

    // Last committed Raft log number.
    std::atomic<uint64_t> last_committed_idx_;

    // Track whether the commit thread has been registered by splinterdb
    std::atomic<bool> commit_thread_initialized_;

    // Keeps the last 3 snapshots, by their Raft log numbers.
    std::map<uint64_t, nuraft::ptr<splinterdb_snapshot>> snapshots_;

    // Mutex for `snapshots_`.
    std::mutex snapshots_lock_;

    bool disable_snapshots_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_SPLINTERDB_STATE_MACHINE_H
