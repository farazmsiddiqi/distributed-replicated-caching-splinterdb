#ifndef REPLICATED_SPLINTERDB_RESULT_H
#define REPLICATED_SPLINTERDB_RESULT_H

#include <variant>

namespace replicated_splinterdb {

template <typename T, typename E>
class Result {
    Result() = delete;

  public:
    Result(T&& result) : result_(std::forward<T>(result)) {}

    Result(E&& error) : result_(std::forward<E>(error)) {}

    inline operator bool() const { return is_ok(); }

    inline T& operator*() { return std::get<T>(result_); }

    inline T* operator->() { return std::get_if<T>(&result_); }

    inline bool is_ok() const { return std::holds_alternative<T>(result_); }

    inline bool is_err() const { return std::holds_alternative<E>(result_); }

    inline T& unwrap() { return **this; }

    inline E& unwrap_err() { return std::get<E>(result_); }

    inline T unwrap_or(T&& default_value) const {
        if (is_ok()) {
            return **this;
        } else {
            return std::forward<T>(default_value);
        }
    }
  private:
    std::variant<T, E> result_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_RESULT_H