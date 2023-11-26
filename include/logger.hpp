#ifndef EFP_LOGGER_HPP_
#define EFP_LOGGER_HPP_

#include <atomic>
#include <thread>
#include <chrono>

#include "efp.hpp"

#include "fmt/core.h"
#include "fmt/args.h"

namespace efp
{
    // todo implement spinlock mth

    enum class LogLevel
    {
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
    };

    namespace detail
    {
        constexpr size_t local_buffer_size = 128;

        const char *log_level_cstr(LogLevel log_level)
        {
            switch (log_level)
            {
            case LogLevel::Debug:
                return "[DEBUG]";
                break;
            case LogLevel::Info:
                return "[INFO] ";
                break;
            case LogLevel::Warn:
                return "[WARN] ";
                break;
            case LogLevel::Error:
                return "[ERROR]";
                break;
            case LogLevel::Fatal:
                return "[FATAL]";
                break;
            }
        }

        struct PlainString
        {
            LogLevel level;
            const char *str;
        };

        struct FormatString
        {
            LogLevel level;
            const char *fmt_str;
            size_t arg_num;
        };

        // todo Add all the argument types
        using LogData = Enum<
            PlainString,
            FormatString,
            int,
            float,
            double,
            const char *>;

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

            bool empty();

        private:
            std::atomic_flag writing_flag_ = ATOMIC_FLAG_INIT;
            Vcq<LogData, local_buffer_size> *write_buffer;
            Vcq<LogData, local_buffer_size> *read_buffer;
        };
    }

    class Log
    {
        friend class detail::LocalLogger;

    public:
        ~Log()
        {
            // fmt::println("Terminating Log");
            run_.store(false);

            if (thread_.joinable())
                thread_.join();
        }

        inline static Log &instance()
        {
            static Log inst{};
            return inst;
        }

        void process()
        {
            const auto process_one = [](detail::LocalLogger *local_logger)
            {
                local_logger->swap_buffer();

                while (!local_logger->empty())
                {
                    local_logger->dequeue();
                }
            };

            for_each(process_one, local_loggers_);
        }

        LogLevel level;

    protected:
        void add(detail::LocalLogger *local_logger)
        {
            // fmt::println("Adding logger");
            std::lock_guard<std::mutex> lock(m_);
            local_loggers_.push_back(local_logger);
        }

        void remove(detail::LocalLogger *local_logger)
        {
            // fmt::println("Removing logger");
            std::lock_guard<std::mutex> lock(m_);

            const auto maybe_idx = elem_index(local_logger, local_loggers_);
            if (maybe_idx)
                local_loggers_.erase(maybe_idx.value());
        }

    private:
        Log()
            : run_(true),
              thread_(
                  [&]()
                  {
                      while (run_.load())
                      {
                          // todo periodic
                          std::this_thread::sleep_for(std::chrono::milliseconds{1});
                          process();
                      }
                  })
        {
            // fmt::println("Log initialized");
        }

        std::mutex m_;
        Vector<detail::LocalLogger *> local_loggers_;
        std::atomic<bool> run_;
        std::thread thread_;
    };

    namespace detail
    {
        // todo use spinlock of mth
        // todo use pointer

        LocalLogger::LocalLogger()
            : read_buffer(new Vcq<LogData, local_buffer_size>{}),
              write_buffer(new Vcq<LogData, local_buffer_size>{})
        {
            Log::instance().add(this);
        }

        LocalLogger::~LocalLogger()
        {
            Log::instance().remove(this);
        }

        void LocalLogger::swap_buffer()
        {
            while (writing_flag_.test_and_set())
            {
            }

            swap(write_buffer, read_buffer);

            writing_flag_.clear();
        }

        template <typename A>
        Unit LocalLogger::enqueue_arg(A a)
        {
            write_buffer->push_back(a);
            return unit;
        }

        template <typename... Args>
        void LocalLogger::enqueue(LogLevel level, const char *fmt_str, Args... args)
        {
            if (level >= Log::instance().level)
            {
                if (sizeof...(args) == 0)
                {
                    while (writing_flag_.test_and_set())
                    {
                    }

                    write_buffer->push_back(
                        detail::PlainString{
                            level,
                            fmt_str});

                    writing_flag_.clear();
                }
                else
                {
                    while (writing_flag_.test_and_set())
                    {
                    }

                    write_buffer->push_back(
                        detail::FormatString{
                            level,
                            fmt_str,
                            sizeof...(args)});

                    execute_pack(enqueue_arg(args)...);

                    writing_flag_.clear();
                }
            }
        }

        void LocalLogger::dequeue()
        {
            read_buffer->pop_front()
                .match(
                    [&](const PlainString &str)
                    {
                        fmt::print("{} {}", log_level_cstr(str.level), str.str);
                        fmt::print("\n");
                    },
                    [&](const FormatString &fstr)
                    {
                        fmt::dynamic_format_arg_store<fmt::format_context> dyn_store;

                        const auto store_arg = [&](size_t)
                        {
                            read_buffer->pop_front().match(
                                [&](int arg)
                                { dyn_store.push_back(arg); },
                                [&](float arg)
                                { dyn_store.push_back(arg); },
                                [&](double arg)
                                { dyn_store.push_back(arg); },
                                [&](const char *arg)
                                { dyn_store.push_back(arg); },
                                //  Number of arguments are decided by number of argument, not parsing result.
                                [&]()
                                { fmt::println("potential error. this messege should not be displayed"); });
                        };

                        for_index(store_arg, fstr.arg_num);

                        fmt::print("{} ", log_level_cstr(fstr.level));
                        fmt::vprint(fstr.fmt_str, dyn_store);
                        fmt::print("\n");
                    },
                    []()
                    { fmt::println("invalid log data. first data has to be format string"); });
        }

        bool LocalLogger::empty()
        {
            return read_buffer->empty();
        }

        extern thread_local LocalLogger local_logger{};
    }

    template <typename... Args>
    void debug(const char *fmt_str, Args... args)
    {
        detail::local_logger.enqueue(LogLevel::Debug, fmt_str, args...);
    }

    template <typename... Args>
    void info(const char *fmt_str, Args... args)
    {
        detail::local_logger.enqueue(LogLevel::Info, fmt_str, args...);
    }

    template <typename... Args>
    void warn(const char *fmt_str, Args... args)
    {
        detail::local_logger.enqueue(LogLevel::Warn, fmt_str, args...);
    }

    template <typename... Args>
    void error(const char *fmt_str, Args... args)
    {
        detail::local_logger.enqueue(LogLevel::Error, fmt_str, args...);
    }

    template <typename... Args>
    void fatal(const char *fmt_str, Args... args)
    {
        detail::local_logger.enqueue(LogLevel::Fatal, fmt_str, args...);
    }
};

#endif