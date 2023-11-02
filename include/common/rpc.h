#ifndef REPLICATED_SPLINTERDB_COMMON_RPC_H
#define REPLICATED_SPLINTERDB_COMMON_RPC_H

// Join server RPCs
#define RPC_JOIN_REPLICA_GROUP "join_replica_group"

// Client-handling server RPCs
#define RPC_PING "ping"
#define RPC_GET_SRV_ID "get_srv_id"
#define RPC_GET_LEADER_ID "get_leader_id"
#define RPC_GET_ALL_SERVERS "get_all_servers"
#define RPC_GET_SRV_ENDPOINT "get_srv_endpoint"
#define RPC_SPLINTERDB_GET "splinterdb_get"
#define RPC_SPLINTERDB_PUT "splinterdb_put"
#define RPC_SPLINTERDB_UPDATE "splinterdb_update"
#define RPC_SPLINTERDB_DELETE "splinterdb_delete"
#define RPC_SPLINTERDB_DUMPCACHE "splinterdb_dumpcache"
#define RPC_SPLINTERDB_CLEARCACHE "splinterdb_clearcache"

#endif  // REPLICATED_SPLINTERDB_COMMON_RPC_H