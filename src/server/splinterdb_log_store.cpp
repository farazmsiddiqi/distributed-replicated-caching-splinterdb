#include "server/splinterdb_log_store.h"

#include <cstring>
#include <iostream>
#include <numeric>

#include "server/splinterdb_operation.h"

namespace replicated_splinterdb {

using nuraft::buffer;
using nuraft::log_entry;
using nuraft::ptr;

extern "C" int log_key_compare(const data_config* cfg, slice key1, slice key2) {
    uint64_t idx1 = *(uint64_t*)slice_data(key1);
    uint64_t idx2 = *(uint64_t*)slice_data(key2);

    if (idx1 > idx2) {
        return 1;
    } else if (idx1 < idx2) {
        return -1;
    } else {
        return 0;
    }
}

static slice log_key_create(const uint64_t& index) {
    return slice_create(sizeof(uint64_t), &index);
}

static ptr<buffer> slice_to_buffer(const slice& value) {
    ptr<buffer> buf = buffer::alloc(slice_length(value));
    buf->put_raw(reinterpret_cast<const nuraft::byte*>(slice_data(value)),
                 slice_length(value));
    return buf;
}

splinterdb_log_store::splinterdb_log_store(const std::string& file_name,
                                           uint64_t disk_size,
                                           uint64_t cache_size)
    : spl_(nullptr), start_idx_(1), last_idx_(0) {
    default_data_config_init(sizeof(uint64_t), &splinter_data_cfg_);
    splinter_data_cfg_.key_compare = log_key_compare;

    memset(&splinterdb_cfg_, 0, sizeof(splinterdb_cfg_));
    splinterdb_cfg_.filename = file_name.c_str();
    splinterdb_cfg_.disk_size = disk_size;
    splinterdb_cfg_.cache_size = cache_size;
    splinterdb_cfg_.data_cfg = &splinter_data_cfg_;

    int rc = splinterdb_create(&splinterdb_cfg_, &spl_);
    if (rc != 0) {
        throw std::runtime_error("Failed to create SplinterDB log instance");
    }
}

splinterdb_log_store::~splinterdb_log_store() { splinterdb_close(&spl_); }

uint64_t splinterdb_log_store::next_slot() const { return last_idx_ + 1; }

uint64_t splinterdb_log_store::start_index() const { return start_idx_; }

ptr<log_entry> splinterdb_log_store::last_entry() const {
    splinterdb_lookup_result result;
    splinterdb_lookup_result_init(spl_, &result, 0, NULL);

    uint64_t index = last_idx_;
    int rc = splinterdb_lookup(spl_, log_key_create(index), &result);
    if (rc == 0) {
        slice value;
        rc = splinterdb_lookup_result_value(&result, &value);
        if (rc == 0) {
            ptr<buffer> buf = slice_to_buffer(value);

            return log_entry::deserialize(*buf);
        }
    }

    return nuraft::cs_new<log_entry>(0, nullptr);
}

uint64_t splinterdb_log_store::append(ptr<log_entry>& entry) {
    uint64_t index = last_idx_++;
    slice key = log_key_create(index);

    ptr<buffer> buf = entry->serialize();
    slice serialized_log = slice_create(buf->size(), buf->data_begin());

    int rc = splinterdb_insert(spl_, key, serialized_log);
    if (rc != 0) {
        throw std::runtime_error("Failed to append log entry");
    }

    return index;
}

void splinterdb_log_store::write_at(uint64_t index, ptr<log_entry>& entry) {
    uint64_t old_last_idx = last_idx_;
    last_idx_ = index - 1;

    std::cout << "Writing log entry at index " << index << std::endl;

    std::vector<uint64_t> to_delete(old_last_idx - index + 1);
    std::iota(std::begin(to_delete), std::end(to_delete), index);

    for (const uint64_t& key : to_delete) {
        splinterdb_delete(spl_, log_key_create(key));
    }

    append(entry);
}

ptr<std::vector<ptr<log_entry>>> splinterdb_log_store::log_entries(
    uint64_t start, uint64_t end) {
    ptr<std::vector<ptr<log_entry>>> ret =
        nuraft::cs_new<std::vector<ptr<log_entry>>>();

    ret->resize(end - start);

    for (uint64_t i = start; i < end; i++) {
        splinterdb_lookup_result result;
        splinterdb_lookup_result_init(spl_, &result, 0, NULL);

        int rc = splinterdb_lookup(spl_, log_key_create(i), &result);
        if (rc == 0) {
            slice value;
            rc = splinterdb_lookup_result_value(&result, &value);
            if (rc == 0) {
                ptr<buffer> buf = slice_to_buffer(value);
                (*ret)[i - start] = log_entry::deserialize(*buf);
            }
        }

        if (rc != 0) {
            return nullptr;
        }
    }

    return ret;
}

ptr<log_entry> splinterdb_log_store::entry_at(uint64_t index) {
    splinterdb_lookup_result result;
    splinterdb_lookup_result_init(spl_, &result, 0, NULL);

    std::cout << "Get log entry at index " << index << std::endl;
    slice key = log_key_create(index);

    int rc = splinterdb_lookup(spl_, key, &result);
    if (rc == 0) {
        slice value;
        rc = splinterdb_lookup_result_value(&result, &value);
        if (rc == 0) {
            ptr<buffer> buf = slice_to_buffer(value);
            return log_entry::deserialize(*buf);
        }
    }

    return nullptr;
}

uint64_t splinterdb_log_store::term_at(uint64_t index) {
    splinterdb_lookup_result result;
    splinterdb_lookup_result_init(spl_, &result, 0, NULL);

    if (index >= next_slot()) {
        throw std::runtime_error("Log index out of range");
    } else if (index < start_index()) {
        return 0;
    }

    int rc = splinterdb_lookup(spl_, log_key_create(index), &result);
    if (rc == 0) {
        slice value;
        rc = splinterdb_lookup_result_value(&result, &value);
        if (rc == 0) {
            ptr<buffer> buf = slice_to_buffer(value);
            ptr<log_entry> entry = log_entry::deserialize(*buf);
            return entry->get_term();
        }
    }

    return 0;
}

ptr<buffer> splinterdb_log_store::pack(uint64_t index, int32_t signed_cnt) {
    uint64_t cnt = static_cast<uint64_t>(signed_cnt);
    std::vector<ptr<buffer>> logs;
    size_t size_total = 0;

    for (uint64_t i = index; i < index + cnt; i++) {
        ptr<log_entry> entry = entry_at(i);
        if (entry == nullptr) {
            break;
        }

        ptr<buffer> buf = entry->serialize();
        size_total += buf->size();
        logs.push_back(buf);
    }

    ptr<buffer> ret =
        buffer::alloc(sizeof(int32_t) + cnt * sizeof(int32_t) + size_total);

    ret->pos(0);
    ret->put((int32_t)signed_cnt);

    for (const auto& buf : logs) {
        ret->put((int32_t)buf->size());
        ret->put(*buf);
    }

    return ret;
}

void splinterdb_log_store::apply_pack(uint64_t index, buffer& pack) {
    pack.pos(0);
    int32_t signed_cnt = pack.get_int();
    if (signed_cnt <= 0) {
        throw std::runtime_error("Invalid log entry count");
    }
    uint64_t cnt = static_cast<uint64_t>(signed_cnt);

    for (uint64_t i = 0; i < cnt; ++i) {
        uint64_t cur_index = index + i;
        int32_t size = pack.get_int();

        if (size <= 0) {
            throw std::runtime_error("Invalid log entry size");
        }

        ptr<buffer> buf = buffer::alloc(static_cast<size_t>(size));
        pack.get(buf);

        int rc =
            splinterdb_insert(spl_, log_key_create(cur_index),
                              slice_create(buf->size(), buf->data_begin()));
        if (rc != 0) {
            throw std::runtime_error("Failed to append log entry at index " +
                                     std::to_string(cur_index));
        }
    }
}

bool splinterdb_log_store::compact(uint64_t last_log_index) {
    uint64_t start = start_idx_;
    std::vector<uint64_t> to_delete(last_log_index - start + 1);
    std::iota(std::begin(to_delete), std::end(to_delete), start);

    for (const uint64_t& key : to_delete) {
        splinterdb_delete(spl_, log_key_create(key));
    }

    if (start_idx_ <= last_log_index) {
        start_idx_ = last_log_index + 1;
    }

    return true;
}

bool splinterdb_log_store::flush() { return true; }

}  // namespace replicated_splinterdb