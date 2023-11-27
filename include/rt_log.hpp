#ifndef EFP_RT_LOG_HPP_
#define EFP_RT_LOG_HPP_

#include <atomic>
#include <thread>
#include <chrono>

#include "efp.hpp"

#include "fmt/core.h"
#include "fmt/args.h"
#include "fmt/chrono.h"
#include "fmt/color.h"

#define EFP_LOG_GLOBAL_BUFFER true
// todo Maybe optional runtime configuration
#define EFP_LOG_TIME_STAMP true
#define EFP_LOG_BUFFER_SIZE 128
// todo Maybe compile time log-level
// todo Processing period

namespace efp
{
    // todo Make user could change the destination of log

    enum class LogLevel : uint8_t
    {
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
    };

    namespace detail
    {
        const char *log_level_cstr(LogLevel log_level)
        {
            switch (log_level)
            {
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
            }
        }

        const fmt::text_style log_level_print_style(LogLevel level)
        {
            switch (level)
            {
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

        struct PlainString
        {
            const char *str;
            LogLevel level;
        };

        struct FormatString
        {
            const char *fmt_str;
            uint8_t arg_num;
            LogLevel level;
        };

        // todo Add all the argument types
        using LogData = Enum<
            PlainString,
            FormatString,
            int,
            float,
            double,
            const char *,
            size_t>;

        class Spinlock
        {
        public:
            void lock()
            {
                while (flag_.test_and_set(std::memory_order_acquire))
                {
                }
            }

            void unlock()
            {
                flag_.clear(std::memory_order_release);
            }

        private:
            std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
        };

        class LogBuffer
        {
        public:
            explicit LogBuffer()
                : read_buffer_(new Vcq<detail::LogData, EFP_LOG_BUFFER_SIZE>{}),
                  write_buffer_(new Vcq<detail::LogData, EFP_LOG_BUFFER_SIZE>{})
            {
            }

            ~LogBuffer()
            {
                delete read_buffer_;
                delete write_buffer_;
            }

            LogBuffer(const LogBuffer &other) = delete;
            LogBuffer &operator=(const LogBuffer &other) = delete;

            LogBuffer(LogBuffer &&other) noexcept = delete;
            LogBuffer &operator=(LogBuffer &&other) noexcept = delete;

            template <typename A>
            Unit enqueue_arg(A a)
            {
                write_buffer_->push_back(a);
                return unit;
            }

            template <typename... Args>
            void enqueue(LogLevel level, const char *fmt_str, Args... args)
            {
                if (sizeof...(args) == 0)
                {
                    spinlock_.lock();

                    write_buffer_->push_back(
                        detail::PlainString{
                            fmt_str,
                            level,
                        });

                    spinlock_.unlock();
                }
                else
                {
                    spinlock_.lock();

                    write_buffer_->push_back(
                        detail::FormatString{
                            fmt_str,
                            sizeof...(args),
                            level,
                        });

                    execute_pack(enqueue_arg(args)...);

                    spinlock_.unlock();
                }
            }

            void swap_buffer()
            {
                spinlock_.lock();
                swap(write_buffer_, read_buffer_);
                spinlock_.unlock();
            }

            void collect_dyn_args(size_t arg_num)
            {
                const auto collect_arg = [&](size_t)
                {
                    read_buffer_->pop_front().match(
                        [&](int arg)
                        { dyn_args_.push_back(arg); },
                        [&](float arg)
                        { dyn_args_.push_back(arg); },
                        [&](double arg)
                        { dyn_args_.push_back(arg); },
                        [&](const char *arg)
                        { dyn_args_.push_back(arg); },
                        [&](size_t arg)
                        { dyn_args_.push_back(arg); },
                        //  Number of arguments are decided by number of argument, not parsing result.
                        [&]()
                        { fmt::println("potential error. this messege should not be displayed"); });
                };

                for_index(collect_arg, arg_num);
            }

            void clear_dyn_args()
            {
                dyn_args_.clear();
            }

            void dequeue()
            {
                read_buffer_->pop_front()
                    .match(
                        [&](const PlainString &str)
                        {
                            fmt::print(log_level_print_style(str.level), "{} ", log_level_cstr(str.level));
                            fmt::print("{}", str.str);
                            fmt::print("\n");
                        },
                        [&](const FormatString &fstr)
                        {
                            collect_dyn_args(fstr.arg_num);

                            fmt::print(log_level_print_style(fstr.level), "{} ", log_level_cstr(fstr.level));
                            fmt::vprint(fstr.fmt_str, dyn_args_);
                            fmt::print("\n");

                            clear_dyn_args();
                        },
                        []()
                        { fmt::println("invalid log data. first data has to be format string"); });
            }

            void dequeue_with_time(const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> &time_point)
            {
                read_buffer_->pop_front()
                    .match(
                        [&](const PlainString &str)
                        {
                            fmt::print(fg(fmt::color::gray), "{:%Y-%m-%d %H:%M:%S} ", time_point);
                            fmt::print(log_level_print_style(str.level), "{} ", log_level_cstr(str.level));
                            fmt::print("{}", str.str);
                            fmt::print("\n");
                        },
                        [&](const FormatString &fstr)
                        {
                            collect_dyn_args(fstr.arg_num);

                            fmt::print(fg(fmt::color::gray), "{:%Y-%m-%d %H:%M:%S} ", time_point);
                            fmt::print(log_level_print_style(fstr.level), "{} ", log_level_cstr(fstr.level));
                            fmt::vprint(fstr.fmt_str, dyn_args_);
                            fmt::print("\n");

                            clear_dyn_args();
                        },
                        []()
                        { fmt::println("invalid log data. first data has to be format string"); });
            }

            bool empty()
            {
                return read_buffer_->empty();
            }

        private:
            Spinlock spinlock_;
            Vcq<LogData, EFP_LOG_BUFFER_SIZE> *write_buffer_;
            Vcq<LogData, EFP_LOG_BUFFER_SIZE> *read_buffer_;
            fmt::dynamic_format_arg_store<fmt::format_context> dyn_args_;
        };

        // Forward declaration of LocalLogger
        class LocalLogger
        {
        public:
            LocalLogger();
            ~LocalLogger();

            template <typename A>
            Unit enqueue_arg(A a);

            template <typename... Args>
            void enqueue(LogLevel level, const char *fmt_str, Args... args);

            void swap_buffer();
            void dequeue();
            void dequeue_with_time(
                const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> &time_point);

            bool empty();

        private:
            LogBuffer log_buffer_;
        };
    }

    class RtLog
    {
        friend class detail::LocalLogger;

    public:
        ~RtLog()
        {
            run_.store(false);

            if (thread_.joinable())
                thread_.join();
        }

        inline static RtLog &instance()
        {
            static RtLog inst{};
            return inst;
        }

        void process()
        {
#if EFP_LOG_GLOBAL_BUFFER == true

            log_buffer_.swap_buffer();

            while (!log_buffer_.empty())
            {
                log_buffer_.dequeue();
            }

#else
            const auto process_one = [](detail::LocalLogger *local_logger)
            {
                while (!local_logger->empty())
                {
                    local_logger->dequeue();
                }
            };

            for_each(process_one, local_loggers_);
#endif
        }

        void process_with_time()
        {
            const auto now = std::chrono::system_clock::now();
            const auto now_sec = std::chrono::time_point_cast<std::chrono::seconds>(now);

#if EFP_LOG_GLOBAL_BUFFER == true
            // printf("running\n");
            log_buffer_.swap_buffer();

            while (!log_buffer_.empty())
            {
                log_buffer_.dequeue_with_time(now_sec);
            }

#else
            const auto process_one = [&](detail::LocalLogger *local_logger)
            {
                local_logger->swap_buffer();

                while (!local_logger->empty())
                {
                    local_logger->dequeue_with_time(now_sec);
                }
            };

            for_each(process_one, local_loggers_);
#endif
        }

        static LogLevel log_level;
        // bool with_time_stamp;

#if EFP_LOG_GLOBAL_BUFFER == true

        template <typename... Args>
        void enqueue(LogLevel level, const char *fmt_str, Args... args)
        {
            log_buffer_.enqueue(level, fmt_str, args...);
        }

        void dequeue()
        {
            log_buffer_.dequeue();
        }

        void dequeue_with_time(const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> &time_point)
        {
            log_buffer_.dequeue_with_time(time_point);
        }
#else

#endif

    protected:
#if EFP_LOG_GLOBAL_BUFFER == true

#else
        void add(detail::LocalLogger *local_logger)
        {
            std::lock_guard<std::mutex> lock(m_);
            local_loggers_.push_back(local_logger);
        }

        void remove(detail::LocalLogger *local_logger)
        {
            std::lock_guard<std::mutex> lock(m_);

            const auto maybe_idx = elem_index(local_logger, local_loggers_);
            if (maybe_idx)
                local_loggers_.erase(maybe_idx.value());
        }
#endif

    private:
        RtLog()
            : // with_time_stamp(true),
              run_(true),
              thread_(
                  [&]()
                  {
                      while (run_.load())
                      {
#if EFP_LOG_TIME_STAMP == true
                          process_with_time();
#else
                          process();
#endif
                          // todo periodic
                          std::this_thread::sleep_for(std::chrono::milliseconds{1});
                      }
                  })
        {
        }

#if EFP_LOG_GLOBAL_BUFFER == true
        detail::LogBuffer log_buffer_;

#else
        std::mutex m_;
        Vector<detail::LocalLogger *> local_loggers_;
#endif

        std::atomic<bool> run_;
        std::thread thread_;
    };

    LogLevel RtLog::log_level = LogLevel::Debug;

    namespace detail
    {
#if EFP_LOG_GLOBAL_BUFFER == true

#else
        LocalLogger::LocalLogger()
        {
            RtLog::instance().add(this);
        }

        LocalLogger::~LocalLogger()
        {
            RtLog::instance().remove(this);
        }

        template <typename... Args>
        void LocalLogger::enqueue(LogLevel level, const char *fmt_str, Args... args)
        {
            log_buffer_.enqueue(level, fmt_str, args...);
        }

        void LocalLogger::dequeue()
        {
            log_buffer_.dequeue();
        }

        void LocalLogger::dequeue_with_time(const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> &time_point)
        {
            log_buffer_.dequeue_with_time(time_point);
        }

        bool LocalLogger::empty()
        {
            log_buffer_.empty();
            // return read_buffer->empty();
        }
#endif

#if EFP_LOG_GLOBAL_BUFFER == true

#else
        extern thread_local LocalLogger local_logger;
        thread_local LocalLogger local_logger{};
#endif

        template <typename... Args>
        inline void enqueue_log(LogLevel level, const char *fmt_str, Args... args)
        {

            if (level >= RtLog::log_level)
            {
#if EFP_LOG_GLOBAL_BUFFER == true
                RtLog::instance().enqueue(level, fmt_str, args...);
#else
                detail::local_logger.enqueue(level, fmt_str, args...);
#endif
            }
        }
    }

    template <typename... Args>
    inline void debug(const char *fmt_str, Args... args)
    {
        detail::enqueue_log(LogLevel::Debug, fmt_str, args...);
    }

    template <typename... Args>
    inline void info(const char *fmt_str, Args... args)
    {
        detail::enqueue_log(LogLevel::Info, fmt_str, args...);
    }

    template <typename... Args>
    inline void warn(const char *fmt_str, Args... args)
    {
        detail::enqueue_log(LogLevel::Warn, fmt_str, args...);
    }

    template <typename... Args>
    inline void error(const char *fmt_str, Args... args)
    {
        detail::enqueue_log(LogLevel::Error, fmt_str, args...);
    }

    template <typename... Args>
    inline void fatal(const char *fmt_str, Args... args)
    {
        detail::enqueue_log(LogLevel::Fatal, fmt_str, args...);
    }
};

#endif