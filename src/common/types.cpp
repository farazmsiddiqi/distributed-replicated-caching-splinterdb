#include "common/types.h"

namespace replicated_splinterdb {

bool is_success(const rpc_mutation_result& result) {
    return std::get<0>(result) == 0 && std::get<1>(result) == 0;
}

nuraft_return_code get_nuraft_return_code(const rpc_mutation_result& result) {
    return std::get<1>(result);
}

bool was_accepted(const rpc_mutation_result& result) {
    return get_nuraft_return_code(result) == 0;
}

}  // namespace replicated_splinterdb