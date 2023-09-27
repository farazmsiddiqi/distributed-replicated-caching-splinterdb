/************************************************************************
Author/Developer(s): Neil Kaushikkar
**************************************************************************/

#include "splinterdb_log_store.h"
#include "libnuraft/nuraft.hxx"
#include "libnuraft/pp_util.hxx"

namespace replicated_splinterdb {

using nuraft::buffer;
using nuraft::log_entry;
using nuraft::ulong;
using nuraft::ptr;
    
splinterdb_log_store::splinterdb_log_store(const splinterdb_config& cfg) 
    : logs_(),
      logs_lock_(),
      start_idx_(1),
      raft_server_bwd_pointer_(nullptr) {
    // Dummy entry for index 0.
    ptr<buffer> buf = buffer::alloc(sizeof(ulong));
    logs_[0] = nuraft::cs_new<log_entry>(0, buf);

    // Initialize SplinterDB instance.

    int rc = splinterdb_create(&cfg, &spl_handle);
}

splinterdb_log_store::~splinterdb_log_store() {

}


}  // namespace replicated_splinterdb