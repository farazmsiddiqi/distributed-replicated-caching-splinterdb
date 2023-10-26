#ifndef REPLICATED_SPLINTERDB_READ_POLICY_H
#define REPLICATED_SPLINTERDB_READ_POLICY_H

#include <cstdint>
#include <queue>

namespace replicated_splinterdb {

class read_policy {
  public:
    virtual ~read_policy() = default;

    virtual int32_t next_server() = 0;
};

class round_robin_read_policy : public read_policy {
  public:
    round_robin_read_policy(const std::vector<int32_t>& server_ids)
        : server_ids_() {
        for (auto id : server_ids) {
            server_ids_.push(id);
        }
    }

    int32_t next_server() override {
        auto id = server_ids_.front();
        server_ids_.pop();
        server_ids_.push(id);
        return id;
    }

  private:
    std::queue<int32_t> server_ids_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_READ_POLICY_H