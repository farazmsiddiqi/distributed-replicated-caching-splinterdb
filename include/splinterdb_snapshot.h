#ifndef REPLICATED_SPLINTERDB_SPLINTERDB_SNAPSHOT_H
#define REPLICATED_SPLINTERDB_SPLINTERDB_SNAPSHOT_H

#include "libnuraft/nuraft.hxx"

namespace replicated_splinterdb {

struct splinterdb_snapshot {
    splinterdb_snapshot(nuraft::ptr<nuraft::snapshot>& snapshot_in)
        : snapshot_(snapshot_in) {}

    nuraft::ptr<nuraft::snapshot> snapshot_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_SPLINTERDB_SNAPSHOT_H