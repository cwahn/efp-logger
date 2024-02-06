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

#define EFP_LOG_TIME_STAMP true
#define EFP_LOG_BUFFER_SIZE 256
// todo Maybe compile time log-level
// todo Processing period configuration

// #define EFP_LOG_COUNT false

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
        inline const char* log_level_cstr(LogLevel log_level) {
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
            const char* str;
            LogLevel level;
        };

        struct FormatedMessage {
            const char* fmt_str;
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
                 const char*, void*, StlStringHead, StlStringData>;

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
                : _read_buffer(new Vcq<detail::LogData, EFP_LOG_BUFFER_SIZE>{}),
                  _write_buffer(new Vcq<detail::LogData, EFP_LOG_BUFFER_SIZE>{}) {}

            ~LogBuffer() {
                delete _read_buffer;
                delete _write_buffer;
            }

            LogBuffer(const LogBuffer& other) = delete;
            LogBuffer& operator=(const LogBuffer& other) = delete;

            LogBuffer(LogBuffer&& other) noexcept = delete;
            LogBuffer& operator=(LogBuffer&& other) noexcept = delete;

            template <typename A>
            inline Unit enqueue_arg(A a) {
                _write_buffer->push_back(a);
                return unit;
            }

            // todo Need pointer casting specialization
            inline Unit enqueue_arg(const std::string& a) {
                const uint8_t str_length = a.length();

                StlStringHead head{};
                uint8_t chars_in_head = str_length < stl_string_head_capacity
                                            ? str_length
                                            : stl_string_head_capacity;
                memcpy(head.chars, a.data(), chars_in_head);
                head.length = static_cast<uint8_t>(chars_in_head);

                _write_buffer->push_back(head);

                if (str_length > stl_string_head_capacity) {
                    int remaining_length = str_length - chars_in_head;
                    uint8_t offset = chars_in_head;

                    while (remaining_length > 0) {
                        StlStringData data{};
                        uint8_t length_to_push = remaining_length < stl_string_data_capacity
                                                     ? remaining_length
                                                     : stl_string_data_capacity;

                        memcpy(data.chars, a.data() + offset, length_to_push);

                        _write_buffer->push_back(data);

                        offset += length_to_push;
                        remaining_length -= length_to_push;
                    }
                }

                return unit;
            }

            template <typename... Args>
            inline void enqueue(LogLevel level, const char* fmt_str,
                                const Args&... args) {
                if (sizeof...(args) == 0) {
                    _spinlock.lock();
                    _write_buffer->push_back(detail::PlainMessage{
                        fmt_str,
                        level,
                    });
                    _spinlock.unlock();
                } else {
                    _spinlock.lock();
                    _write_buffer->push_back(detail::FormatedMessage{
                        fmt_str,
                        sizeof...(args),
                        level,
                    });

                    execute_pack(enqueue_arg(args)...);
                    _spinlock.unlock();
                }
            }

            inline void swap_buffer() {
                _spinlock.lock();
                efp::swap(_write_buffer, _read_buffer);
                _spinlock.unlock();
            }

            void collect_dyn_args(size_t arg_num) {
                const auto collect_arg = [&](size_t) {
                    _read_buffer->pop_front().match(
                        [&](int arg) { _dyn_args.push_back(arg); },
                        [&](short arg) { _dyn_args.push_back(arg); },
                        [&](long arg) { _dyn_args.push_back(arg); },
                        [&](long long arg) { _dyn_args.push_back(arg); },
                        [&](unsigned int arg) { _dyn_args.push_back(arg); },
                        [&](unsigned short arg) { _dyn_args.push_back(arg); },
                        [&](unsigned long arg) { _dyn_args.push_back(arg); },
                        [&](unsigned long long arg) { _dyn_args.push_back(arg); },
                        [&](char arg) { _dyn_args.push_back(arg); },
                        [&](signed char arg) { _dyn_args.push_back(arg); },
                        [&](unsigned char arg) { _dyn_args.push_back(arg); },
                        [&](bool arg) { _dyn_args.push_back(arg); },
                        [&](float arg) { _dyn_args.push_back(arg); },
                        [&](double arg) { _dyn_args.push_back(arg); },
                        [&](long double arg) { _dyn_args.push_back(arg); },
                        [&](const char* arg) { _dyn_args.push_back(arg); },
                        [&](void* arg) { _dyn_args.push_back(arg); },
                        [&](const StlStringHead& arg) {
                            std::string str;
                            str.append(arg.chars, arg.length < stl_string_head_capacity
                                                      ? arg.length
                                                      : stl_string_head_capacity);

                            // Extracting the remaining parts of the string if necessary
                            int remaining_length = arg.length - stl_string_head_capacity;
                            while (remaining_length > 0) {
                                _read_buffer->pop_front().match(
                                    [&](const StlStringData& d) {
                                        const uint8_t append_length =
                                            remaining_length < stl_string_data_capacity
                                                ? remaining_length
                                                : stl_string_data_capacity;
                                        str.append(d.chars, append_length);
                                        remaining_length -= append_length;
                                    },
                                    []() { fmt::println("String reconstruction error"); });
                            }

                            _dyn_args.push_back(str);
                        },
                        [&]() {
                            fmt::println(
                                "Potential error. this messege should not be displayed");
                        });
                };

                for_index(collect_arg, arg_num);
            }
            void clear_dyn_args() { _dyn_args.clear(); }

            void dequeue() {
                _read_buffer->pop_front().match(
                    [&](const PlainMessage& str) {
                        if (_output_file == stdout) {
                            fmt::print(_output_file, log_level_print_style(str.level), "{} ",
                                       log_level_cstr(str.level));
                        } else {
                            fmt::print(_output_file, "{} ", log_level_cstr(str.level));
                        }
                        fmt::print(_output_file, "{}", str.str);
                        fmt::print(_output_file, "\n");
                    },
                    [&](const FormatedMessage& fstr) {
                        collect_dyn_args(fstr.arg_num);
                        if (_output_file == stdout) {
                            fmt::print(_output_file, log_level_print_style(fstr.level), "{} ",
                                       log_level_cstr(fstr.level));
                        } else {
                            fmt::print(_output_file, "{} ", log_level_cstr(fstr.level));
                        }
                        fmt::vprint(_output_file, fstr.fmt_str, _dyn_args);
                        fmt::print(_output_file, "\n");

                        clear_dyn_args();
                    },
                    []() {
                        fmt::println("invalid log data. first data has to be format string");
                    });
            }

            void dequeue_with_time(
                const std::chrono::time_point<std::chrono::system_clock,
                                              std::chrono::seconds>& time_point) {
                _read_buffer->pop_front().match(
                    [&](const PlainMessage& msg) {
                        if (_output_file == stdout) {
                            fmt::print(_output_file, fg(fmt::color::gray),
                                       "{:%Y-%m-%d %H:%M:%S} ", time_point);
                            fmt::print(_output_file, log_level_print_style(msg.level), "{} ",
                                       log_level_cstr(msg.level));
                        } else {
                            fmt::print(_output_file, "{:%Y-%m-%d %H:%M:%S} ", time_point);
                            fmt::print(_output_file, "{} ", log_level_cstr(msg.level));
                        }
                        fmt::print(_output_file, "{}", msg.str);
                        fmt::print(_output_file, "\n");
                    },
                    [&](const FormatedMessage& msg) {
                        collect_dyn_args(msg.arg_num);

                        if (_output_file == stdout) {
                            fmt::print(_output_file, fg(fmt::color::gray),
                                       "{:%Y-%m-%d %H:%M:%S} ", time_point);
                            fmt::print(_output_file, log_level_print_style(msg.level), "{} ",
                                       log_level_cstr(msg.level));
                        } else {
                            fmt::print(_output_file, "{:%Y-%m-%d %H:%M:%S} ", time_point);
                            fmt::print(_output_file, "{} ", log_level_cstr(msg.level));
                        }
                        fmt::vprint(_output_file, msg.fmt_str, _dyn_args);
                        fmt::print(_output_file, "\n");

                        clear_dyn_args();
                    },
                    []() {
                        fmt::println("invalid log data. first data has to be format string");
                    });
            }

            inline bool empty() { return _read_buffer->empty(); }

            inline void set_output_file(FILE* output_file) { _output_file = output_file; }

            inline void set_log_level(LogLevel log_level) { _log_level = log_level; }

            inline LogLevel get_log_level() { return _log_level; }

        private:
            Spinlock _spinlock;
            Vcq<LogData, EFP_LOG_BUFFER_SIZE>* _write_buffer;
            Vcq<LogData, EFP_LOG_BUFFER_SIZE>* _read_buffer;
            fmt::dynamic_format_arg_store<fmt::format_context> _dyn_args;
            LogLevel _log_level = LogLevel::Info;
            std::FILE* _output_file = stdout;
        };

    } // namespace detail

    // The main logger class

    class Logger {
        // friend class detail::LocalLogger;

    public:
        ~Logger() {
            _run.store(false);

            if (_thread.joinable())
                _thread.join();

#if EFP_LOG_TIME_STAMP == true
            process_with_time();
            process_with_time();
#else
            process();
            process();
#endif
        }

        static inline Logger& instance() {
            static Logger inst{};
            return inst;
        }

        static inline void set_log_level(LogLevel log_level) {

            instance()._log_buffer.set_log_level(log_level);
        }

        static inline LogLevel get_log_level() {
            return instance()._log_buffer.get_log_level();
        }

        static void set_output(FILE* output_file) {
            instance()._log_buffer.set_output_file(output_file);
        }

        static void set_output(const char* path) {

            FILE* output_file = fopen(path, "a");
            if (output_file) {
                instance()._log_buffer.set_output_file(output_file);
            } else {
                printf("Can not open the ouput file");
                abort();
            }
        }

        // todo set_config

        void process() {

            _log_buffer.swap_buffer();

            while (!_log_buffer.empty()) {
                _log_buffer.dequeue();
#if EFP_LOG_COUNT == true
                log_count++;
#endif
            }
        }

        void process_with_time() {
            const auto now = std::chrono::system_clock::now();
            const auto now_sec =
                std::chrono::time_point_cast<std::chrono::seconds>(now);

            // printf("running\n");
            _log_buffer.swap_buffer();

            while (!_log_buffer.empty()) {
                _log_buffer.dequeue_with_time(now_sec);
#if EFP_LOG_COUNT == true
                log_count++;
#endif
            }
        }

        // bool with_time_stamp;

        template <typename... Args>
        void enqueue(LogLevel level, const char* fmt_str, const Args&... args) {
            _log_buffer.enqueue(level, fmt_str, args...);
        }

        void dequeue() { _log_buffer.dequeue(); }

        void dequeue_with_time(
            const std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::seconds>& time_point) {
            _log_buffer.dequeue_with_time(time_point);
        }

#if EFP_LOG_COUNT == true
        int log_count = 0;
#endif
    private:
        Logger()
            : // with_time_stamp(true),
              _run(true),
              _thread([&]() {
                  while (_run.load()) {
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

        LogLevel _log_level;

        detail::LogBuffer _log_buffer;

        std::atomic<bool> _run;
        std::thread _thread;
    };

    // LogLevel Logger::instance().log_level = LogLevel::Debug;

    namespace detail {

        template <typename... Args>
        inline void enqueue_log(LogLevel level, const char* fmt_str,
                                const Args&... args) {
            if (level >= Logger::get_log_level()) {
                Logger::instance().enqueue(level, fmt_str, args...);
            }
        }
    } // namespace detail

    template <typename... Args>
    inline void trace(const char* fmt_str, const Args&... args) {
        detail::enqueue_log(LogLevel::Trace, fmt_str, args...);
    }

    template <typename... Args>
    inline void debug(const char* fmt_str, const Args&... args) {
        detail::enqueue_log(LogLevel::Debug, fmt_str, args...);
    }

    template <typename... Args>
    inline void info(const char* fmt_str, const Args&... args) {
        detail::enqueue_log(LogLevel::Info, fmt_str, args...);
    }

    template <typename... Args>
    inline void warn(const char* fmt_str, const Args&... args) {
        detail::enqueue_log(LogLevel::Warn, fmt_str, args...);
    }

    template <typename... Args>
    inline void error(const char* fmt_str, const Args&... args) {
        detail::enqueue_log(LogLevel::Error, fmt_str, args...);
    }

    template <typename... Args>
    inline void fatal(const char* fmt_str, const Args&... args) {
        detail::enqueue_log(LogLevel::Fatal, fmt_str, args...);
    }
}; // namespace efp

#endif