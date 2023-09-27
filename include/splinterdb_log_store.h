/************************************************************************
Author/Developer(s): Neil Kaushikkar
**************************************************************************/

#pragma once

#include "libnuraft/event_awaiter.hxx"
#include "libnuraft/internal_timer.hxx"
#include "libnuraft/log_store.hxx"

#include <atomic>
#include <map>
#include <mutex>

extern "C" {
#include <stdbool.h>
#define _Bool bool

#include "splinterdb/default_data_config.h"
#include "splinterdb/splinterdb.h"
}

namespace replicated_splinterdb {

class raft_server;

class splinterdb_log_store : public nuraft::log_store {
public:
    splinterdb_log_store() = delete;

    splinterdb_log_store(const splinterdb_config& cfg);

    ~splinterdb_log_store();

    __nocopy__(splinterdb_log_store);

public:
    nuraft::ulong next_slot() const;

    nuraft::ulong start_index() const;

    nuraft::ptr<nuraft::log_entry> last_entry() const;

    nuraft::ulong append(nuraft::ptr<nuraft::log_entry>& entry);

    void write_at(nuraft::ulong index, nuraft::ptr<nuraft::log_entry>& entry);

    nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> log_entries(nuraft::ulong start, nuraft::ulong end);

    nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> log_entries_ext(
            nuraft::ulong start, nuraft::ulong end, nuraft::int64 batch_size_hint_in_bytes = 0);

    nuraft::ptr<nuraft::log_entry> entry_at(nuraft::ulong index);

    nuraft::ulong term_at(nuraft::ulong index);

    nuraft::ptr<nuraft::buffer> pack(nuraft::ulong index, nuraft::int32 cnt);

    void apply_pack(nuraft::ulong index, nuraft::buffer& pack);

    bool compact(nuraft::ulong last_log_index);

    bool flush();

    void close();

    nuraft::ulong last_durable_index();

    void set_disk_delay(raft_server* raft, size_t delay_ms);

private:
    static nuraft::ptr<nuraft::log_entry> make_clone(const nuraft::ptr<nuraft::log_entry>& entry);

    void disk_emul_loop();

    /**
     * Pointer to a running SplinterDB instance.
     */
    splinterdb *spl_handle;

    /**
     * Map of <log index, log data>.
     */
    std::map<nuraft::ulong, nuraft::ptr<nuraft::log_entry>> logs_;

    /**
     * Lock for `logs_`.
     */
    mutable std::mutex logs_lock_;

    /**
     * The index of the first log.
     */
    std::atomic<nuraft::ulong> start_idx_;

    /**
     * Backward pointer to Raft server.
     */
    raft_server* raft_server_bwd_pointer_;
};

}

