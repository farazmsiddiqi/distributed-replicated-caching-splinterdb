#pragma once

#include "splinterdb/public_util.h"
#include "libnuraft/nuraft.hxx"

#define LOG_KEY_LENGTH 16
#define LOG_KEY_PREFIX '0'

namespace replicated_splinterdb {

slice log_entry_to_slice(const nuraft::ptr<nuraft::log_entry>& entry) {
    nuraft::buffer& buf = entry->get_buf();
    return slice(static_cast<uint64>(buf.size()), buf.data());
}

slice string_to_slice(const std::string& str) {
    return slice(static_cast<uint64>(str.length()), str.c_str());
}

std::string log_index_to_string(ulong index) {
    auto index_str = std::to_string(index);
    auto index_str_len = KEY_LENGTH - std::min(KEY_LENGTH, index_str.length());
    return std::string(index_str_len, LOG_KEY_PREFIX) + index_str;
}

}  // namespace replicated_splinterdb