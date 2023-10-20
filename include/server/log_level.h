#pragma once

namespace replicated_splinterdb {

enum LogLevel {
    DISABLED = -1,
    SYS = 0,
    FATAL = 1,
    ERROR = 2,
    WARNING = 3,
    INFO = 4,
    DEBUG = 5,
    TRACE = 6,
    UNKNOWN = 99,
};

}  // namespace replicated_splinterdb