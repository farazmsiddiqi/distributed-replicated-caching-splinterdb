#ifndef REPLICATED_SPLINTERDB_TYPES_H
#define REPLICATED_SPLINTERDB_TYPES_H

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace replicated_splinterdb {

using splinterdb_return_code = int32_t;

using nuraft_return_code = int32_t;

using nuraft_return_msg = std::string;

using rpc_read_result =
    std::tuple<std::vector<uint8_t>, splinterdb_return_code>;

using rpc_mutation_result =
    std::tuple<splinterdb_return_code, nuraft_return_code, nuraft_return_msg>;

bool is_success(const rpc_mutation_result& result);

nuraft_return_code get_nuraft_return_code(const rpc_mutation_result& result);

bool was_accepted(const rpc_mutation_result& result);

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_TYPES_H