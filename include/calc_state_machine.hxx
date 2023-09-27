/************************************************************************
Copyright 2017-2019 eBay Inc.
Author/Developer(s): Jung-Sang Ahn

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**************************************************************************/

#pragma once

#include "libnuraft/nuraft.hxx"

#include <atomic>
#include <cassert>
#include <iostream>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include <optional>

#include <string.h>

using namespace nuraft;

namespace calc_server {

class calc_state_machine : public state_machine {
public:
    // calc_state_machine(bool async_snapshot = false)
    //     : cur_value_(0)
    //     , last_committed_idx_(0)
    //     , async_snapshot_(async_snapshot)
    //     {}

    // ~calc_state_machine() {}

    calc_state_machine(bool async_snapshot = false)
        : cur_map_()
        , last_committed_idx_(0)
        , async_snapshot_(async_snapshot)
        {}

    ~calc_state_machine() {}

    enum op_type : int {
        ADD = 0x0,
        SUB = 0x1,
        MUL = 0x2,
        DIV = 0x3,
        SET = 0x4
    };

    enum kv_op_type : uint8_t {
        PUT = 0x0,
        UDPATE = 0x1,
        READ = 0x2
    };

    struct kv_op_payload {
        kv_op_type type_;
        std::string key_;
        std::string val_;
    };

    struct op_payload {
        op_type type_;
        int oprnd_;
    };

    static ptr<buffer> enc_log(const kv_op_payload& payload) {
        // Encode from {operator, operand} to Raft log.
        ptr<buffer> ret = buffer::alloc(sizeof(kv_op_payload));
        buffer_serializer bs(ret);

        std::cout << "creating log for " << payload.key_ << "=" << payload.val_ << std::endl;

        bs.put_u8(payload.type_);
        bs.put_str(payload.key_);
        bs.put_str(payload.val_);

        return ret;
    }

    static void dec_log(buffer& log, kv_op_payload& payload_out) {
        // Decode from Raft log to {operator, operand} pair.
        assert(log.size() == sizeof(kv_op_payload));


        buffer_serializer bs(log);
        payload_out.type_ = static_cast<kv_op_type>(bs.get_u8());
        payload_out.key_ = bs.get_str();
        payload_out.val_ = bs.get_str();

        std::cout << "deserializing log for " << payload_out.key_ << "=" << payload_out.val_ << std::endl;
    }

    ptr<buffer> pre_commit(const ulong log_idx, buffer& data) {
        // Nothing to do with pre-commit in this example.
        return nullptr;
    }

    // ptr<buffer> commit(const ulong log_idx, buffer& data) {
    //     op_payload payload;
    //     dec_log(data, payload);

    //     int64_t prev_value = cur_value_;
    //     switch (payload.type_) {
    //     case ADD:   prev_value += payload.oprnd_;   break;
    //     case SUB:   prev_value -= payload.oprnd_;   break;
    //     case MUL:   prev_value *= payload.oprnd_;   break;
    //     case DIV:   prev_value /= payload.oprnd_;   break;
    //     default:
    //     case SET:   prev_value  = payload.oprnd_;   break;
    //     }
    //     cur_value_ = prev_value;

    //     last_committed_idx_ = log_idx;

    //     // Return Raft log number as a return result.
    //     ptr<buffer> ret = buffer::alloc( sizeof(log_idx) );
    //     buffer_serializer bs(ret);
    //     bs.put_u64(log_idx);
    //     return ret;
    // }

    ptr<buffer> commit(const ulong log_idx, buffer& data) {
        kv_op_payload payload;
        dec_log(data, payload);

        switch (payload.type_) {
        case PUT:
            cur_map_.insert({payload.key_, payload.val_});
            break;

        case UDPATE:
            cur_map_[payload.key_] = payload.val_;
            break;

        default:
            std::cout << "current state machine value"
                      << get_current_map()
                      << std::endl; 
        }

        last_committed_idx_ = log_idx;

        // Return Raft log number as a return result.
        ptr<buffer> ret = buffer::alloc( sizeof(log_idx) );
        buffer_serializer bs(ret);
        bs.put_u64(log_idx);
        return ret;
    }

    void commit_config(const ulong log_idx, ptr<cluster_config>& new_conf) {
        // Nothing to do with configuration change. Just update committed index.
        last_committed_idx_ = log_idx;
    }

    void rollback(const ulong log_idx, buffer& data) {
        // Nothing to do with rollback,
        // as this example doesn't do anything on pre-commit.
    }

    int read_logical_snp_obj(snapshot& s,
                             void*& user_snp_ctx,
                             ulong obj_id,
                             ptr<buffer>& data_out,
                             bool& is_last_obj)
    {
        ptr<snapshot_ctx> ctx = nullptr;
        {   std::lock_guard<std::mutex> ll(snapshots_lock_);
            auto entry = snapshots_.find(s.get_last_log_idx());
            if (entry == snapshots_.end()) {
                // Snapshot doesn't exist.
                data_out = nullptr;
                is_last_obj = true;
                return 0;
            }
            ctx = entry->second;
        }

        if (obj_id == 0) {
            // Object ID == 0: first object, put the size of the map.
            data_out = buffer::alloc( sizeof(int32) );
            buffer_serializer bs(data_out);
            bs.put_i32(ctx->value_.size());
            is_last_obj = false;
        } else {
            // for now, serialize the entire map. In the future, we will need a
            // special end marker so that we know when to stop deserializing
            // the map.
            serialize_map(data_out, ctx->value_);
            is_last_obj = true;
        }
        
        return 0;
    }

    void save_logical_snp_obj(snapshot& s,
                              ulong& obj_id,
                              buffer& data,
                              bool is_first_obj,
                              bool is_last_obj)
    {
        buffer_serializer bs(data);

        if (obj_id == 0) {
            // Object ID == 0: it contains dummy value, create snapshot context.
            ptr<buffer> snp_buf = s.serialize();
            ptr<snapshot> ss = snapshot::deserialize(*snp_buf);
            create_snapshot_internal(ss);
            int32 expected_size = bs.get_i32();
            std::cout << "Expected size: " << expected_size << std::endl;
        } else {
            // Object ID > 0: actual snapshot value.
            std::lock_guard<std::mutex> ll(snapshots_lock_);
            auto entry = snapshots_.find(s.get_last_log_idx());
            entry->second->value_.clear();
            assert(entry != snapshots_.end());
            entry->second->value_ = deserialize_map(bs);
            std::cout << "Got " << entry->second->value_.size() << " entries" << std::endl;
        }
        // Request next object.
        obj_id++;
    }

    // TODO: issue is likely here
    bool apply_snapshot(snapshot& s) {
        std::lock_guard<std::mutex> ll(snapshots_lock_);
        auto entry = snapshots_.find(s.get_last_log_idx());
        if (entry == snapshots_.end()) return false;

        ptr<snapshot_ctx> ctx = entry->second;
        cur_map_ = ctx->value_;
        return true;
    }

    void free_user_snp_ctx(void*& user_snp_ctx) {
        // In this example, `read_logical_snp_obj` doesn't create
        // `user_snp_ctx`. Nothing to do in this function.
    }

    ptr<snapshot> last_snapshot() {
        // Just return the latest snapshot.
        std::lock_guard<std::mutex> ll(snapshots_lock_);
        auto entry = snapshots_.rbegin();
        if (entry == snapshots_.rend()) return nullptr;

        ptr<snapshot_ctx> ctx = entry->second;
        return ctx->snapshot_;
    }

    ulong last_commit_index() {
        return last_committed_idx_;
    }

    void create_snapshot(snapshot& s,
                         async_result<bool>::handler_type& when_done)
    {
        if (!async_snapshot_) {
            // Create a snapshot in a synchronous way (blocking the thread).
            create_snapshot_sync(s, when_done);
        } else {
            // Create a snapshot in an asynchronous way (in a different thread).
            create_snapshot_async(s, when_done);
        }
    }

    std::optional<std::string> get_value(const std::string& key) {
        auto iter = cur_map_.find(key);
        if (iter == cur_map_.end()) {
            return std::nullopt;
        }

        return iter->second;
    }

    std::string get_current_map() const {
        std::ostringstream oss;

        oss << "{" << std::endl;
        for (const auto& pair : cur_map_) {
            oss << "\t" << pair.first << ": " << pair.second << std::endl;
        }
        oss << "}" << std::endl;

        return oss.str();
    }

    static void serialize_map(ptr<buffer>& data_out, const std::unordered_map<std::string, std::string>& to_serialize) {
        size_t buf_size = 0;
        for (const auto& pair : to_serialize) {
            buf_size += pair.first.length() + pair.second.length() + (2 * sizeof(uint32_t));
        }

        data_out = buffer::alloc(buf_size);
        buffer_serializer bs(data_out);

        for (const auto& pair : to_serialize) {
            bs.put_str(pair.first);
            bs.put_str(pair.second);
        }
    }

    std::unordered_map<std::string, std::string> deserialize_map(buffer_serializer& bs) {
        std::unordered_map<std::string, std::string> map;

        while (bs.pos() < bs.size()) {
            std::string key = bs.get_str();
            std::string value = bs.get_str();
            map[key] = value;
        }

        return map;
    }

private:
    struct snapshot_ctx {
        snapshot_ctx( ptr<snapshot>& s, std::unordered_map<std::string, std::string> v )
            : snapshot_(s), value_(v) {}
        ptr<snapshot> snapshot_;
        std::unordered_map<std::string, std::string> value_;
    };

    void create_snapshot_internal(ptr<snapshot> ss) {
        std::lock_guard<std::mutex> ll(snapshots_lock_);

        // Put into snapshot map.
        ptr<snapshot_ctx> ctx = cs_new<snapshot_ctx>(ss, cur_map_);
        snapshots_[ss->get_last_log_idx()] = ctx;

        // Maintain last 3 snapshots only.
        const int MAX_SNAPSHOTS = 3;
        int num = snapshots_.size();
        auto entry = snapshots_.begin();
        for (int ii = 0; ii < num - MAX_SNAPSHOTS; ++ii) {
            if (entry == snapshots_.end()) break;
            entry = snapshots_.erase(entry);
        }
    }

    void create_snapshot_sync(snapshot& s,
                              async_result<bool>::handler_type& when_done)
    {
        // Clone snapshot from `s`.
        ptr<buffer> snp_buf = s.serialize();
        ptr<snapshot> ss = snapshot::deserialize(*snp_buf);
        create_snapshot_internal(ss);

        ptr<std::exception> except(nullptr);
        bool ret = true;
        when_done(ret, except);

        std::cout << "snapshot (" << ss->get_last_log_term() << ", "
                  << ss->get_last_log_idx() << ") has been created synchronously"
                  << std::endl;
    }

    void create_snapshot_async(snapshot& s,
                               async_result<bool>::handler_type& when_done)
    {
        // Clone snapshot from `s`.
        ptr<buffer> snp_buf = s.serialize();
        ptr<snapshot> ss = snapshot::deserialize(*snp_buf);

        // Note that this is a very naive and inefficient example
        // that creates a new thread for each snapshot creation.
        std::thread t_hdl([this, ss, when_done]{
            create_snapshot_internal(ss);

            ptr<std::exception> except(nullptr);
            bool ret = true;
            when_done(ret, except);

            std::cout << "snapshot (" << ss->get_last_log_term() << ", "
                      << ss->get_last_log_idx() << ") has been created asynchronously"
                      << std::endl;
        });
        t_hdl.detach();
    }

    // State machine's current value.
    // std::atomic<int64_t> cur_value_;
    std::unordered_map<std::string, std::string> cur_map_;

    // Last committed Raft log number.
    std::atomic<uint64_t> last_committed_idx_;

    // Keeps the last 3 snapshots, by their Raft log numbers.
    std::map< uint64_t, ptr<snapshot_ctx> > snapshots_;

    // Mutex for `snapshots_`.
    std::mutex snapshots_lock_;

    // If `true`, snapshot will be created asynchronously.
    bool async_snapshot_;
};

}; // namespace calc_server

