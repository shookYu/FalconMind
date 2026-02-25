/**
 * @file Result.h
 * @brief Result type for error handling
 * 
 * Provides Rust/MAVSDK-style Result<T> for rich error handling.
 * Replace bool returns with Result<T> throughout SDK.
 */

#pragma once

#include "ErrorCode.h"
#include <variant>
#include <string>
#include <memory>
#include <optional>

namespace falconmind {
namespace sdk {
namespace high_level {

/**
 * @brief Result type for operations that can fail
 * 
 * Similar to Rust's Result<T, E> and MAVSDK's Result<T>.
 * Usage:
 *   Result<int> divide(int a, int b) {
 *       if (b == 0) return Result<int>::error(ErrorCode::InvalidParameterValue, "Division by zero");
 *       return Result<int>::success(a / b);
 *   }
 * 
 *   auto result = divide(10, 2);
 *   if (result.isSuccess()) {
 *       std::cout << "Result: " << result.value();
 *   } else {
 *       std::cerr << "Error: " << result.errorMessage();
 *   }
 */
template<typename T>
class Result {
public:
    using ValueType = T;
    
    // Constructors
    Result() = delete;
    
    /**
     * @brief Construct from value (success case)
     */
    Result(T&& value) 
        : data_(std::forward<T>(value)), 
          code_(ErrorCode::Success) {}
    
    Result(const T& value) 
        : data_(value), 
          code_(ErrorCode::Success) {}
    
    /**
     * @brief Construct from error code (error case)
     */
    Result(ErrorCode code, const std::string& message = "")
        : code_(code), message_(message) {}
    
    // Static factory methods
    static Result<T> success(T&& value) {
        return Result<T>(std::forward<T>(value));
    }
    
    static Result<T> success(const T& value) {
        return Result<T>(value);
    }
    
    static Result<T> error(ErrorCode code, const std::string& message = "") {
        return Result<T>(code, message);
    }
    
    // Query methods
    bool isSuccess() const { return code_ == ErrorCode::Success; }
    bool isError() const { return !isSuccess(); }
    explicit operator bool() const { return isSuccess(); }
    
    // Access value (throws if error)
    T& value() & {
        if (isError()) {
            throw std::runtime_error("Cannot access value of error Result: " + errorMessage());
        }
        return std::get<T>(data_);
    }
    
    const T& value() const & {
        if (isError()) {
            throw std::runtime_error("Cannot access value of error Result: " + errorMessage());
        }
        return std::get<T>(data_);
    }
    
    T&& value() && {
        if (isError()) {
            throw std::runtime_error("Cannot access value of error Result: " + errorMessage());
        }
        return std::get<T>(std::move(data_));
    }
    // Arrow operator for pointer-like access
    T* operator->() {
        if (isError()) {
            throw std::runtime_error("Cannot access value of error Result: " + errorMessage());
        }
        return &std::get<T>(data_);
    }
    
    const T* operator->() const {
        if (isError()) {
            throw std::runtime_error("Cannot access value of error Result: " + errorMessage());
        }
        return &std::get<T>(data_);
    }
    
    // Dereference operator
    T& operator*() & {
        return value();
    }
    
    const T& operator*() const & {
        return value();
    }
    
    T&& operator*() && {
        return std::move(value());
    }
    
    // Safe access with default
    T valueOr(const T& defaultValue) const {
        return isSuccess() ? value() : defaultValue;
    }
    
    // Access error
    ErrorCode error() const { return code_; }
    
    const std::string& errorMessage() const { 
        if (message_.empty() && isError()) {
            static std::string defaultMsg = errorCodeToString(code_);
            return defaultMsg;
        }
        return message_; 
    }
    
    // Get error as std::error_code
    std::error_code errorCode() const {
        return make_error_code(code_);
    }
    
    // Map operation (functional style)
    template<typename Func>
    auto map(Func&& f) && -> Result<decltype(f(std::declval<T>()))> {
        using ReturnType = decltype(f(std::declval<T>()));
        if (isError()) {
            return Result<ReturnType>::error(code_, message_);
        }
        try {
            return Result<ReturnType>::success(f(std::move(value())));
        } catch (const std::exception& e) {
            return Result<ReturnType>::error(ErrorCode::InternalError, e.what());
        }
    }
    
    // AndThen operation (monadic bind)
    template<typename Func>
    auto andThen(Func&& f) && -> decltype(f(std::declval<T>())) {
        using ReturnType = decltype(f(std::declval<T>()));
        if (isError()) {
            return ReturnType::error(code_, message_);
        }
        try {
            return f(std::move(value()));
        } catch (const std::exception& e) {
            return ReturnType::error(ErrorCode::InternalError, e.what());
        }
    }
    
    // OrElse operation
    template<typename Func>
    Result<T> orElse(Func&& f) const {
        if (isSuccess()) {
            return *this;
        }
        return f(code_, message_);
    }
    
private:
    std::variant<T, std::monostate> data_;
    ErrorCode code_;
    std::string message_;
};

/**
 * @brief Result specialization for void (operations with no return value)
 */
template<>
class Result<void> {
public:
    Result() : code_(ErrorCode::Success) {}
    
    Result(ErrorCode code, const std::string& message = "")
        : code_(code), message_(message) {}
    
    static Result<void> success() {
        return Result<void>();
    }
    
    static Result<void> error(ErrorCode code, const std::string& message = "") {
        return Result<void>(code, message);
    }
    
    bool isSuccess() const { return code_ == ErrorCode::Success; }
    bool isError() const { return !isSuccess(); }
    explicit operator bool() const { return isSuccess(); }
    
    ErrorCode error() const { return code_; }
    
    const std::string& errorMessage() const { 
        if (message_.empty() && isError()) {
            static std::string defaultMsg = errorCodeToString(code_);
            return defaultMsg;
        }
        return message_; 
    }
    
    std::error_code errorCode() const {
        return make_error_code(code_);
    }
    
    // Void doesn't have value(), but we can check success
    void expect(const std::string& msg) const {
        if (isError()) {
            throw std::runtime_error(msg + ": " + errorMessage());
        }
    }
    
private:
    ErrorCode code_;
    std::string message_;
};

// Convenience type aliases
template<typename T>
using ResultPtr = Result<std::shared_ptr<T>>;

} // namespace high_level
} // namespace sdk
} // namespace falconmind
