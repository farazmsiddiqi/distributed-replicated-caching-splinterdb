#ifndef REPLICATED_SPLINTERDB_RESULT_H
#define REPLICATED_SPLINTERDB_RESULT_H

#include <variant>

namespace replicated_splinterdb {

template <typename T, typename E>
class result_t {
    result_t() = delete;

  public:
    result_t(T&& res) : res_(std::forward<T>(res)) {}

    result_t(E&& error) : res_(std::forward<E>(error)) {}

    inline operator bool() const { return is_ok(); }

    inline T& operator*() { return std::get<T>(res_); }

    inline T* operator->() { return std::get_if<T>(&res_); }

    inline bool is_ok() const { return std::holds_alternative<T>(res_); }

    inline bool is_err() const { return std::holds_alternative<E>(res_); }

    inline T& unwrap() { return **this; }

    inline E& unwrap_err() { return std::get<E>(res_); }

    inline T unwrap_or(T&& default_value) const {
        if (is_ok()) {
            return **this;
        } else {
            return std::forward<T>(default_value);
        }
    }

  private:
    std::variant<T, E> res_;
};

}  // namespace replicated_splinterdb

#endif  // REPLICATED_SPLINTERDB_RESULT_H