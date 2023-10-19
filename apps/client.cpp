#include <iostream>

#include "rpc/client.h"
#include "rpc/rpc_error.h"

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


static std::vector<std::string> tokenize(const char* str, char c = ' ');

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
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server port>" << std::endl;
        return 1;
    }

    int server_port = std::atoi(argv[1]);
    rpc::client c("localhost", server_port);

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
            try {
                std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
                std::vector<uint8_t> value(tokens[2].begin(), tokens[2].end());
                auto ret_value = c.call("put", key, value).as<int32_t>();

                std::cout << "succeeded, return code: " 
                          << ret_value << std::endl;
            } catch (rpc::rpc_error &e) {
                std::cout << std::endl << e.what() << std::endl;
                std::cout << "in function '" << e.get_function_name() << "': ";

                using err_t = std::tuple<int, std::string>;
                auto err = e.get_error().as<err_t>();
                std::cout << "[error " << std::get<0>(err) << "]: " 
                          << std::get<1>(err) << std::endl;
            }
        } else if (tokens[0] == "update" && tokens.size() >= 3) {
            std::cout << "not yet implemented" << std::endl;
        } else if (tokens[0] == "delete" && tokens.size() >= 2) {
            std::cout << "not yet implemented" << std::endl;
        } else if (tokens[0] == "get" && tokens.size() >= 2) {
            try {
                std::vector<uint8_t> key(tokens[1].begin(), tokens[1].end());
                auto ret_value = c.call("get", key).as<std::vector<uint8_t>>();
                std::string value(ret_value.begin(), ret_value.end());
                std::cout << "value: " << value << std::endl;
            } catch (rpc::rpc_error &e) {
                std::cout << std::endl << e.what() << std::endl;
                std::cout << "in function '" << e.get_function_name() << "': ";

                using err_t = int32_t;
                auto err = e.get_error().as<err_t>();
                std::cout << "[error code " << err << "]" << std::endl;
            }
        }
    }

    return 0;
}