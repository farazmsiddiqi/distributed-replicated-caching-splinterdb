#pragma once

#include <atomic>
#include <map>
#include <mutex>

#include "libnuraft/event_awaiter.hxx"
#include "libnuraft/internal_timer.hxx"
#include "libnuraft/log_store.hxx"
#include "splinterdb_wrapper.h"

namespace replicated_splinterdb {

extern "C" int log_key_compare(const data_config* cfg, slice key1, slice key2);

class splinterdb_log_store : public nuraft::log_store {
  public:
    splinterdb_log_store(const std::string& file_name = "log.db",
                         uint64_t disk_size = 1024 * 1024 * 1024,
                         uint64_t cache_size = 64 * 1024 * 1024);

    ~splinterdb_log_store();

    __nocopy__(splinterdb_log_store);

  public:
    /**
     * The first available slot of the store, starts with 1
     *
     * @return Last log index number + 1
     */
    uint64_t next_slot() const override;

    /**
     * The start index of the log store, at the very beginning, it must be 1.
     * However, after some compact actions, this could be anything equal to or
     * greater than one
     */
    uint64_t start_index() const override;

    /**
     * The last log entry in store.
     *
     * @return If no log entry exists: a dummy constant entry with
     *         value set to null and term set to zero.
     */
    nuraft::ptr<nuraft::log_entry> last_entry() const override;

    /**
     * Append a log entry to store.
     *
     * @param entry Log entry
     * @return Log index number.
     */
    uint64_t append(nuraft::ptr<nuraft::log_entry>& entry) override;

    /**
     * Overwrite a log entry at the given `index`.
     * This API should make sure that all log entries
     * after the given `index` should be truncated (if exist),
     * as a result of this function call.
     *
     * @param index Log index number to overwrite.
     * @param entry New log entry to overwrite.
     */
    void write_at(uint64_t index,
                  nuraft::ptr<nuraft::log_entry>& entry) override;

    /**
     * Get log entries with index [start, end).
     *
     * Return nullptr to indicate error if any log entry within the requested
     * range could not be retrieved (e.g. due to external log truncation).
     *
     * @param start The start log index number (inclusive).
     * @param end The end log index number (exclusive).
     * @return The log entries between [start, end).
     */
    nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> log_entries(
        uint64_t start, uint64_t end) override;

    /**
     * Get the log entry at the specified log index number.
     *
     * @param index Should be equal to or greater than 1.
     * @return The log entry or null if index >= this->next_slot().
     */
    nuraft::ptr<nuraft::log_entry> entry_at(uint64_t index) override;

    /**
     * Get the term for the log entry at the specified index.
     * Suggest to stop the system if the index >= this->next_slot()
     *
     * @param index Should be equal to or greater than 1.
     * @return The term for the specified log entry, or
     *         0 if index < this->start_index().
     */
    uint64_t term_at(uint64_t index) override;

    /**
     * Pack the given number of log items starting from the given index.
     *
     * @param index The start log index number (inclusive).
     * @param cnt The number of logs to pack.
     * @return Packed (encoded) logs.
     */
    nuraft::ptr<nuraft::buffer> pack(uint64_t index, int32_t cnt) override;

    /**
     * Apply the log pack to current log store, starting from index.
     *
     * @param index The start log index number (inclusive).
     * @param Packed logs.
     */
    void apply_pack(uint64_t index, nuraft::buffer& pack) override;

    /**
     * Compact the log store by purging all log entries,
     * including the given log index number.
     *
     * If current maximum log index is smaller than given `last_log_index`,
     * set start log index to `last_log_index + 1`.
     *
     * @param last_log_index Log index number that will be purged up to
     * (inclusive).
     * @return `true` on success.
     */
    bool compact(uint64_t last_log_index) override;

    /**
     * Synchronously flush all log entries in this log store to the backing
     * storage so that all log entries are guaranteed to be durable upon process
     * crash.
     *
     * @return `true` on success.
     */
    bool flush() override;

    splinterdb* get_splinterdb_handle() { return spl_; }

  private:
    splinterdb* spl_;

    data_config splinter_data_cfg_;

    splinterdb_config splinterdb_cfg_;

    /**
     * The index of the first log.
     */
    std::atomic<uint64_t> start_idx_;

    /**
     * The index of the last log.
     */
    std::atomic<uint64_t> last_idx_;
};

}  // namespace replicated_splinterdb
