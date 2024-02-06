#ifndef EFP_RT_LOG_HPP_
#define EFP_RT_LOG_HPP_

#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "efp.hpp"

#include "fmt/args.h"
#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/core.h"

#define EFP_LOG_GLOBAL_BUFFER true
// todo Maybe optional runtime configuration
#define EFP_LOG_TIME_STAMP true
#define EFP_LOG_BUFFER_SIZE 256
// todo Maybe compile time log-level
// todo Processing period configuration

namespace efp {
enum class LogLevel : char {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Fatal,
  Off,
};

namespace detail {
inline const char *log_level_cstr(LogLevel log_level) {
  switch (log_level) {
  case LogLevel::Trace:
    return "TRACE";
    break;
  case LogLevel::Debug:
    return "DEBUG";
    break;
  case LogLevel::Info:
    return "INFO ";
    break;
  case LogLevel::Warn:
    return "WARN ";
    break;
  case LogLevel::Error:
    return "ERROR";
    break;
  case LogLevel::Fatal:
    return "FATAL";
    break;
  default:
    return "";
  }
}

inline const fmt::text_style log_level_print_style(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
    return fg(fmt::color::snow);
  case LogLevel::Debug:
    return fg(fmt::color::dodger_blue);
  case LogLevel::Info:
    return fg(fmt::color::green);
  case LogLevel::Warn:
    return fg(fmt::color::gold);
  case LogLevel::Error:
    return fg(fmt::color::red);
  case LogLevel::Fatal:
    return fg(fmt::color::purple) | fmt::emphasis::bold;
  default:
    return fg(fmt::color::white);
  }
}

struct PlainMessage {
  const char *str;
  LogLevel level;
};

struct FormatedMessage {
  const char *fmt_str;
  uint8_t arg_num;
  LogLevel level;
};

constexpr uint8_t stl_string_data_capacity = sizeof(FormatedMessage);
constexpr uint8_t stl_string_head_capacity = stl_string_data_capacity - 1;

// Data structure for std::string preventing each char takes size of full Enum;
struct StlStringHead {
  uint8_t length;
  char chars[stl_string_head_capacity];
};

// Data structure for std::string preventing each char takes size of full Enum;
// Some of the char may not be valid depending on the length specified by head
struct StlStringData {
  char chars[stl_string_data_capacity];
};

using LogData =
    Enum<PlainMessage, FormatedMessage, int, short, long, long long,
         unsigned int, unsigned short, unsigned long, unsigned long long, char,
         signed char, unsigned char, bool, float, double, long double,
         const char *, void *, StlStringHead, StlStringData>;

class Spinlock {
public:
  inline void lock() {
    while (flag_.test_and_set(std::memory_order_acquire)) {
    }
  }

  inline void unlock() { flag_.clear(std::memory_order_release); }

private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

class LogBuffer {
public:
  explicit LogBuffer()
      : read_buffer_(new Vcq<detail::LogData, EFP_LOG_BUFFER_SIZE>{}),
        write_buffer_(new Vcq<detail::LogData, EFP_LOG_BUFFER_SIZE>{}) {}

  ~LogBuffer() {
    delete read_buffer_;
    delete write_buffer_;
  }

  LogBuffer(const LogBuffer &other) = delete;
  LogBuffer &operator=(const LogBuffer &other) = delete;

  LogBuffer(LogBuffer &&other) noexcept = delete;
  LogBuffer &operator=(LogBuffer &&other) noexcept = delete;

  template <typename A> inline Unit enqueue_arg(A a) {
    write_buffer_->push_back(a);
    return unit;
  }

  // todo Need pointer casting specialization
  inline Unit enqueue_arg(const std::string &a) {
    const uint8_t str_length = a.length();

    StlStringHead head{};
    uint8_t chars_in_head = str_length < stl_string_head_capacity
                                ? str_length
                                : stl_string_head_capacity;
    memcpy(head.chars, a.data(), chars_in_head);
    head.length = static_cast<uint8_t>(chars_in_head);

    write_buffer_->push_back(head);

    if (str_length > stl_string_head_capacity) {
      int remaining_length = str_length - chars_in_head;
      uint8_t offset = chars_in_head;

      while (remaining_length > 0) {
        StlStringData data{};
        uint8_t length_to_push = remaining_length < stl_string_data_capacity
                                     ? remaining_length
                                     : stl_string_data_capacity;

        memcpy(data.chars, a.data() + offset, length_to_push);

        write_buffer_->push_back(data);

        offset += length_to_push;
        remaining_length -= length_to_push;
      }
    }

    return unit;
  }

  template <typename... Args>
  inline void enqueue(LogLevel level, const char *fmt_str,
                      const Args &...args) {
    if (sizeof...(args) == 0) {
      spinlock_.lock();
      write_buffer_->push_back(detail::PlainMessage{
          fmt_str,
          level,
      });
      spinlock_.unlock();
    } else {
      spinlock_.lock();
      write_buffer_->push_back(detail::FormatedMessage{
          fmt_str,
          sizeof...(args),
          level,
      });

      execute_pack(enqueue_arg(args)...);
      spinlock_.unlock();
    }
  }

  inline void swap_buffer() {
    spinlock_.lock();
    efp::swap(write_buffer_, read_buffer_);
    spinlock_.unlock();
  }

  void collect_dyn_args(size_t arg_num) {
    const auto collect_arg = [&](size_t) {
      read_buffer_->pop_front().match(
          [&](int arg) { dyn_args_.push_back(arg); },
          [&](short arg) { dyn_args_.push_back(arg); },
          [&](long arg) { dyn_args_.push_back(arg); },
          [&](long long arg) { dyn_args_.push_back(arg); },
          [&](unsigned int arg) { dyn_args_.push_back(arg); },
          [&](unsigned short arg) { dyn_args_.push_back(arg); },
          [&](unsigned long arg) { dyn_args_.push_back(arg); },
          [&](unsigned long long arg) { dyn_args_.push_back(arg); },
          [&](char arg) { dyn_args_.push_back(arg); },
          [&](signed char arg) { dyn_args_.push_back(arg); },
          [&](unsigned char arg) { dyn_args_.push_back(arg); },
          [&](bool arg) { dyn_args_.push_back(arg); },
          [&](float arg) { dyn_args_.push_back(arg); },
          [&](double arg) { dyn_args_.push_back(arg); },
          [&](long double arg) { dyn_args_.push_back(arg); },
          [&](const char *arg) { dyn_args_.push_back(arg); },
          [&](void *arg) { dyn_args_.push_back(arg); },
          [&](const StlStringHead &arg) {
            std::string str;
            str.append(arg.chars, arg.length < stl_string_head_capacity
                                      ? arg.length
                                      : stl_string_head_capacity);

            // Extracting the remaining parts of the string if necessary
            int remaining_length = arg.length - stl_string_head_capacity;
            while (remaining_length > 0) {
              read_buffer_->pop_front().match(
                  [&](const StlStringData &d) {
                    const uint8_t append_length =
                        remaining_length < stl_string_data_capacity
                            ? remaining_length
                            : stl_string_data_capacity;
                    str.append(d.chars, append_length);
                    remaining_length -= append_length;
                  },
                  []() { fmt::println("String reconstruction error"); });
            }

            dyn_args_.push_back(str);
          },
          [&]() {
            fmt::println(
                "Potential error. this messege should not be displayed");
          });
    };

    for_index(collect_arg, arg_num);
  }
  void clear_dyn_args() { dyn_args_.clear(); }

  void dequeue() {
    read_buffer_->pop_front().match(
        [&](const PlainMessage &str) {
          if (output_file_ == stdout) {
            fmt::print(output_file_, log_level_print_style(str.level), "{} ",
                       log_level_cstr(str.level));
          } else {
            fmt::print(output_file_, "{} ", log_level_cstr(str.level));
          }
          fmt::print(output_file_, "{}", str.str);
          fmt::print(output_file_, "\n");
        },
        [&](const FormatedMessage &fstr) {
          collect_dyn_args(fstr.arg_num);
          if (output_file_ == stdout) {
            fmt::print(output_file_, log_level_print_style(fstr.level), "{} ",
                       log_level_cstr(fstr.level));
          } else {
            fmt::print(output_file_, "{} ", log_level_cstr(fstr.level));
          }
          fmt::vprint(output_file_, fstr.fmt_str, dyn_args_);
          fmt::print(output_file_, "\n");

          clear_dyn_args();
        },
        []() {
          fmt::println("invalid log data. first data has to be format string");
        });
  }

  void dequeue_with_time(
      const std::chrono::time_point<std::chrono::system_clock,
                                    std::chrono::seconds> &time_point) {
    read_buffer_->pop_front().match(
        [&](const PlainMessage &msg) {
          if (output_file_ == stdout) {
            fmt::print(output_file_, fg(fmt::color::gray),
                       "{:%Y-%m-%d %H:%M:%S} ", time_point);
            fmt::print(output_file_, log_level_print_style(msg.level), "{} ",
                       log_level_cstr(msg.level));
          } else {
            fmt::print(output_file_, "{:%Y-%m-%d %H:%M:%S} ", time_point);
            fmt::print(output_file_, "{} ", log_level_cstr(msg.level));
          }
          fmt::print(output_file_, "{}", msg.str);
          fmt::print(output_file_, "\n");
        },
        [&](const FormatedMessage &msg) {
          collect_dyn_args(msg.arg_num);

          if (output_file_ == stdout) {
            fmt::print(output_file_, fg(fmt::color::gray),
                       "{:%Y-%m-%d %H:%M:%S} ", time_point);
            fmt::print(output_file_, log_level_print_style(msg.level), "{} ",
                       log_level_cstr(msg.level));
          } else {
            fmt::print(output_file_, "{:%Y-%m-%d %H:%M:%S} ", time_point);
            fmt::print(output_file_, "{} ", log_level_cstr(msg.level));
          }
          fmt::vprint(output_file_, msg.fmt_str, dyn_args_);
          fmt::print(output_file_, "\n");

          clear_dyn_args();
        },
        []() {
          fmt::println("invalid log data. first data has to be format string");
        });
  }

  inline bool empty() { return read_buffer_->empty(); }

  inline void set_output_file(FILE *output_file) { output_file_ = output_file; }

  inline void set_log_level(LogLevel log_level) { log_level_ = log_level; }

  inline LogLevel get_log_level() { return log_level_; }

private:
  Spinlock spinlock_;
  Vcq<LogData, EFP_LOG_BUFFER_SIZE> *write_buffer_;
  Vcq<LogData, EFP_LOG_BUFFER_SIZE> *read_buffer_;
  fmt::dynamic_format_arg_store<fmt::format_context> dyn_args_;
  LogLevel log_level_ = LogLevel::Info;
  std::FILE *output_file_ = stdout;
};

// ! Deprecated. Global log seems fast enough.
// Forward declaration of LocalLogger
class LocalLogger {
public:
  LocalLogger();
  ~LocalLogger();

  template <typename A> Unit enqueue_arg(A a);

  template <typename... Args>
  void enqueue(LogLevel level, const char *fmt_str, const Args &...args);

  void swap_buffer();
  void dequeue();
  void dequeue_with_time(
      const std::chrono::time_point<std::chrono::system_clock,
                                    std::chrono::seconds> &time_point);

  bool empty();

private:
  LogBuffer log_buffer_;
};
} // namespace detail

// The main logger class

class Logger {
  friend class detail::LocalLogger;

public:
  ~Logger() {
    run_.store(false);

    if (thread_.joinable())
      thread_.join();

#if EFP_LOG_TIME_STAMP == true
    process_with_time();
    process_with_time();
#else
    process();
    process();
#endif
  }

  static inline Logger &instance() {
    static Logger inst{};
    return inst;
  }

  static inline void set_log_level(LogLevel log_level) {

    instance().log_buffer_.set_log_level(log_level);
  }

  static inline LogLevel get_log_level() {
    return instance().log_buffer_.get_log_level();
  }

  static void set_output(FILE *output_file) {
    instance().log_buffer_.set_output_file(output_file);
  }

  static void set_output(const char *path) {

    FILE *output_file = fopen(path, "a");
    if (output_file) {
      instance().log_buffer_.set_output_file(output_file);
    } else {
      printf("Can not open the ouput file");
      abort();
    }
  }

  // todo set_config

  void process() {

    log_buffer_.swap_buffer();

    while (!log_buffer_.empty()) {
      log_buffer_.dequeue();
    }
  }

  void process_with_time() {
    const auto now = std::chrono::system_clock::now();
    const auto now_sec =
        std::chrono::time_point_cast<std::chrono::seconds>(now);

    // printf("running\n");
    log_buffer_.swap_buffer();

    while (!log_buffer_.empty()) {
      log_buffer_.dequeue_with_time(now_sec);
    }
  }

  // bool with_time_stamp;

  template <typename... Args>
  void enqueue(LogLevel level, const char *fmt_str, const Args &...args) {
    log_buffer_.enqueue(level, fmt_str, args...);
  }

  void dequeue() { log_buffer_.dequeue(); }

  void dequeue_with_time(
      const std::chrono::time_point<std::chrono::system_clock,
                                    std::chrono::seconds> &time_point) {
    log_buffer_.dequeue_with_time(time_point);
  }

protected:
private:
  Logger()
      : // with_time_stamp(true),
        run_(true), thread_([&]() {
          while (run_.load()) {
#if EFP_LOG_TIME_STAMP == true
            process_with_time();
#else
            process();
#endif
            // todo periodic
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
          }
        }) {
  }

  LogLevel log_level_;

  detail::LogBuffer log_buffer_;

  std::atomic<bool> run_;
  std::thread thread_;
};

// LogLevel Logger::instance().log_level = LogLevel::Debug;

namespace detail {

template <typename... Args>
inline void enqueue_log(LogLevel level, const char *fmt_str,
                        const Args &...args) {
#if EFP_LOG_GLOBAL_BUFFER == true
  if (level >= Logger::get_log_level()) {
    Logger::instance().enqueue(level, fmt_str, args...);
  }
#else
  if (level >= detail::local_logger::get_log_level()) {
    detail::local_logger.enqueue(level, fmt_str, args...);
  }
#endif
}
} // namespace detail

template <typename... Args>
inline void trace(const char *fmt_str, const Args &...args) {
  detail::enqueue_log(LogLevel::Trace, fmt_str, args...);
}

template <typename... Args>
inline void debug(const char *fmt_str, const Args &...args) {
  detail::enqueue_log(LogLevel::Debug, fmt_str, args...);
}

template <typename... Args>
inline void info(const char *fmt_str, const Args &...args) {
  detail::enqueue_log(LogLevel::Info, fmt_str, args...);
}

template <typename... Args>
inline void warn(const char *fmt_str, const Args &...args) {
  detail::enqueue_log(LogLevel::Warn, fmt_str, args...);
}

template <typename... Args>
inline void error(const char *fmt_str, const Args &...args) {
  detail::enqueue_log(LogLevel::Error, fmt_str, args...);
}

template <typename... Args>
inline void fatal(const char *fmt_str, const Args &...args) {
  detail::enqueue_log(LogLevel::Fatal, fmt_str, args...);
}
}; // namespace efp

#endif