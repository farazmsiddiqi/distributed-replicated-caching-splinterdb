#include "splinterdb_state_machine.h"

#include <iostream>

#include "common/timer.h"
#include "server/splinterdb_operation.h"

namespace replicated_splinterdb {

using nuraft::async_result;
using nuraft::buffer;
using nuraft::buffer_serializer;
using nuraft::cluster_config;
using nuraft::ptr;
using nuraft::snapshot;
using nuraft::ulong;

splinterdb_state_machine::splinterdb_state_machine(
    const splinterdb_config& cfg_ref, bool disable_snapshots)
    : spl_handle_(nullptr),
      last_committed_idx_(0),
      commit_thread_initialized_(false),
      snapshots_(),
      snapshots_lock_(),
      disable_snapshots_(disable_snapshots) {
    if (splinterdb_create(&cfg_ref, &spl_handle_)) {
        throw std::runtime_error("Failed to create SplinterDB instance.");
    }
}

splinterdb_state_machine::~splinterdb_state_machine() {
    splinterdb_close(&spl_handle_);
}

ptr<buffer> splinterdb_state_machine::commit(const ulong log_idx, buffer& buf) {
    bool expected = false;
    if (commit_thread_initialized_.compare_exchange_strong(expected, true)) {
        std::cout << "Registering commit thread." << std::endl;
        splinterdb_register_thread(spl_handle_);
    }

    Timer timer;

    splinterdb_operation operation = splinterdb_operation::deserialize(buf);
    slice key_slice, value_slice;
    int32_t ret_code;

    operation.key().fill_slice(key_slice);

    switch (operation.type()) {
        case splinterdb_operation::PUT:
            operation.value().fill_slice(value_slice);
            ret_code = splinterdb_insert(spl_handle_, key_slice, value_slice);
            std::cout << "PUT=" << ret_code << " took " << timer.getTimeUs() << " us" << std::endl;
            break;
        case splinterdb_operation::UPDATE:
            operation.value().fill_slice(value_slice);
            ret_code = splinterdb_update(spl_handle_, key_slice, value_slice);
            std::cout << "UPDATE=" << ret_code << " took " << timer.getTimeUs() << " us" << std::endl;
            break;
        case splinterdb_operation::DELETE:
            ret_code = splinterdb_delete(spl_handle_, key_slice);
            std::cout << "DELETE=" << ret_code << " took " << timer.getTimeUs() << " us" << std::endl;
            break;
        default:
            throw std::runtime_error("Unknown operation type.");
    }

    last_committed_idx_ = log_idx;

    ptr<buffer> ret = buffer::alloc(sizeof(ret_code));
    buffer_serializer bs(ret);
    bs.put_i32(ret_code);
    return ret;
}

void splinterdb_state_machine::commit_config(const ulong log_idx,
                                             ptr<cluster_config>& new_conf) {
    last_committed_idx_ = log_idx;
}

void splinterdb_state_machine::save_logical_snp_obj(snapshot& s, ulong& obj_id,
                                                    buffer& data,
                                                    bool is_first_obj,
                                                    bool is_last_obj) {
    throw std::runtime_error("Not implemented.");
}

bool splinterdb_state_machine::apply_snapshot(snapshot& s) {
    throw std::runtime_error("Not implemented.");
}

int splinterdb_state_machine::read_logical_snp_obj(nuraft::snapshot& s,
                                                   void*& user_snp_ctx,
                                                   ulong obj_id,
                                                   ptr<buffer>& data_out,
                                                   bool& is_last_obj) {
    throw std::runtime_error("Not implemented.");
}

void splinterdb_state_machine::free_user_snp_ctx(void*& user_snp_ctx) {
    throw std::runtime_error("Not implemented.");
}

nuraft::ptr<nuraft::snapshot> splinterdb_state_machine::last_snapshot() {
    // Just return the latest snapshot.
    std::lock_guard<std::mutex> ll(snapshots_lock_);
    auto entry = snapshots_.rbegin();
    if (entry == snapshots_.rend()) return nullptr;

    ptr<splinterdb_snapshot> ctx = entry->second;
    return ctx->snapshot_;
}

ulong splinterdb_state_machine::last_commit_index() {
    return last_committed_idx_;
}

void splinterdb_state_machine::create_snapshot(
    snapshot& s, async_result<bool>::handler_type& when_done) {
    throw std::runtime_error("Not implemented.");
}

}  // namespace replicated_splinterdb