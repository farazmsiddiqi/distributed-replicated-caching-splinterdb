#include <gflags/gflags.h>

#include <iostream>

#include "client/client.h"

DEFINE_string(endpoint, "", "server endpoint formatted as <host>:<port>");
DEFINE_string(e, "",
              "One time command to execute in non-interactive mode. If this "
              "argument is empty, the client will run in interactive mode. "
              "Format: <command> <arg1> <arg2> ...");

#ifndef _CLM_DEFINED
#define _CLM_DEFINED (1)
#define _CLM_GREEN "\033[32m"
#define _CLM_END "\033[0m"
#endif

using replicated_splinterdb::client;
using replicated_splinterdb::rpc_mutation_result;

static bool handle_mutation_result(rpc_mutation_result&& result);

static std::vector<std::string> tokenize(const char* str, char c = ' ');

static bool handle_command(rpc::client& c,
                           const std::vector<std::string>& tokens);

bool handle_mutation_result(rpc_mutation_result&& result) {
    auto [spl_rc, raft_rc, msg] = result;

    if (raft_rc == 0 && spl_rc == 0) {
        std::cout << "succeeded" << std::endl;
        return true;
    } else if (raft_rc != 0) {
        std::cout << "append log failed, rc=" << raft_rc << ": " << msg
                  << std::endl;
    } else if (spl_rc != 0) {
        std::cout << "put failed, rc=" << spl_rc << std::endl;
    }

    return false;
}

std::vector<std::string> tokenize(const char* str, char c) {
    std::vector<std::string> tokens;
    do {
        const char* begin = str;
        while (*str != c && *str) str++;
        if (begin != str) tokens.push_back(std::string(begin, str));
    } while (0 != *str++);

    return tokens;
}

static bool handle_command(client& c, const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return false;
    }

    std::string cmd = tokens[0];
    std::transform(tokens[0].begin(), tokens[0].end(), cmd.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (cmd == "put" && tokens.size() >= 3) {
        std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
        std::vector<uint8_t> value(tokens[2].begin(), tokens[2].end());

        auto res = c.put(key, value);
        return handle_mutation_result(std::move(res));
    } else if (cmd == "update" && tokens.size() >= 3) {
        std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
        std::vector<uint8_t> value(tokens[2].begin(), tokens[2].end());

        auto res = c.update(key, value);
        return handle_mutation_result(std::move(res));
    } else if (cmd == "delete" && tokens.size() >= 2) {
        std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());

        auto res = c.del(key);
        return handle_mutation_result(std::move(res));
    } else if (cmd == "get" && tokens.size() >= 2) {
        std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
        auto [value, spl_rc] = c.get(key);

        if (spl_rc == 0) {
            std::cout << "value: " << std::string(value.begin(), value.end())
                      << std::endl;
            return true;
        } else {
            std::cout << "get failed, rc=" << spl_rc << std::endl;
            return false;
        }
    } else if (cmd == "ls") {
        std::vector<std::tuple<int32_t, std::string>> srvs =
            c.get_all_servers();

        int32_t leader_id = c.get_leader_id();

        std::cout << "server id : client-facing endpoint" << std::endl;
        for (const auto& [srv_id, endpoint] : srvs) {
            std::string extra{};
            if (srv_id == leader_id) {
                extra = " (LEADER)";
            }

            std::cout << srv_id << " : " << endpoint << extra << std::endl;
        }

        return true;
    } else if (cmd == "dumpcache") {
        c.trigger_cache_dumps();
        return true;
    } else if (cmd == "clearcache") {
        c.trigger_cache_clear();
    } else if (cmd == "help") {
        std::cout << "Commands:" << std::endl;
        std::cout << "  put <key> <value>" << std::endl;
        std::cout << "  update <key> <value>" << std::endl;
        std::cout << "  delete <key>" << std::endl;
        std::cout << "  get <key>" << std::endl;
        std::cout << "  ls" << std::endl;
        std::cout << "  dumpcache" << std::endl;
        std::cout << "  help" << std::endl;
        std::cout << "  exit (interactive mode only)" << std::endl;

        return true;
    } else {
        std::cout << "ERROR: unrecognized command" << std::endl;
        return false;
    }
}

int main(int argc, char** argv) {
    gflags::SetUsageMessage(
        "A CLI client to interact with a replicated SplinterDB server");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_endpoint.empty()) {
        std::cerr << "ERROR: flag '-endpoint' is required" << std::endl;
        return 1;
    }

    auto pos = FLAGS_endpoint.find(":");
    if (pos == std::string::npos) {
        std::cerr << "ERROR: flag '-endpoint' has invalid format, "
                  << "expected <host>:<port>" << std::endl;
        return 1;
    }

    std::string host = FLAGS_endpoint.substr(0, pos);
    int port = std::stoi(FLAGS_endpoint.substr(pos + 1));
    if (port < 1024 || port > 65535) {
        std::cerr << "ERROR: flag '-endpoint' has invalid port number, "
                  << "expected 1024 <= port <= 65535" << std::endl;
        return 1;
    }

    client c(host, static_cast<uint16_t>(port));

    if (!FLAGS_e.empty()) {
        auto tokens(tokenize(FLAGS_e.c_str()));
        return handle_command(c, tokens) ? 0 : 1;
    }

    std::string prompt = "spl-client> ";
    char cmd[1000];
    int rc = 0;

    while (true) {
#if defined(__linux__) || defined(__APPLE__)
        std::cout << _CLM_GREEN << prompt << _CLM_END;
#else
        std::cout << prompt;
#endif
        if (!std::cin.getline(cmd, 1000)) {
            break;
        }
        auto tokens(tokenize(cmd));

        if (tokens[0] == "exit") {
            break;
        } else {
            rc = handle_command(c, tokens) ? 0 : 1;
        }
    }

    return rc;
}