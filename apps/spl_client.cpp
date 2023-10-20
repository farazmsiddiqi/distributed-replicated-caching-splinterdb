#include <gflags/gflags.h>

#include <iostream>

#include "client/client.h"

DEFINE_string(endpoint, "", "server endpoint formatted as <host>:<port>");

#ifndef _CLM_DEFINED
#define _CLM_DEFINED (1)

#ifdef LOGGER_NO_COLOR
#define _CLM_GREEN ""
#define _CLM_END ""
#else
#define _CLM_GREEN "\033[32m"
#define _CLM_END "\033[0m"
#endif
#endif

using replicated_splinterdb::client;
using replicated_splinterdb::rpc_mutation_result;

static void handle_mutation_result(rpc_mutation_result&& result);

static std::vector<std::string> tokenize(const char* str, char c = ' ');

static void handle_mutation_result(rpc_mutation_result&& result) {
    auto [spl_rc, raft_rc, msg] = result;

    if (raft_rc == 0 && spl_rc == 0) {
        std::cout << "succeeded" << std::endl;
    } else if (raft_rc != 0) {
        std::cout << "append log failed, rc=" << raft_rc << ": " << msg
                  << std::endl;
    } else if (spl_rc != 0) {
        std::cout << "put failed, rc=" << spl_rc << std::endl;
    }
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

int main(int argc, char** argv) {
    gflags::SetUsageMessage("A CLI client to interact with a replicated SplinterDB server");
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
    std::string prompt = "spl-client> ";
    char cmd[1000];

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
        } else if (tokens[0] == "put" && tokens.size() >= 3) {
            std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
            std::vector<uint8_t> value(tokens[2].begin(), tokens[2].end());

            auto res = c.put(key, value);
            handle_mutation_result(std::move(res));
        } else if (tokens[0] == "update" && tokens.size() >= 3) {
            std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
            std::vector<uint8_t> value(tokens[2].begin(), tokens[2].end());

            auto res = c.update(key, value);
            handle_mutation_result(std::move(res));
        } else if (tokens[0] == "delete" && tokens.size() >= 2) {
            std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());

            auto res = c.del(key);
            handle_mutation_result(std::move(res));
        } else if (tokens[0] == "get" && tokens.size() >= 2) {
            std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
            auto [value, spl_rc] = c.get(key);

            if (spl_rc == 0) {
                std::cout << "value: "
                          << std::string(value.begin(), value.end())
                          << std::endl;
            } else {
                std::cout << "get failed, rc=" << spl_rc << std::endl;
            }
        }
    }

    return 0;
}