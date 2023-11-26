#ifndef EFP_LOGGER_HPP_
#define EFP_LOGGER_HPP_

#include <atomic>
#include "efp.hpp"

namespace efp
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
    };

    using LogMsg = Enum<const char *, int, float, double>;

    namespace detail
    {
        constexpr size_t local_buffer_size;

        struct Log
        {
            LogLevel level;
            LogMsg msg;
        };
    }

    class LocalLogger;

    class LogProcessor
    {
    public:
        static LogProcessor &instance()
        {
            static LogProcessor inst{};
            return inst;
        }

        void add(LocalLogger &local_logger)
        {
            // todo need lock()?
            local_loggers.push_back(local_logger);
        }

        void remove(LocalLogger &local_logger)
        {
            // todo need lock():
            // todo find and erase
            // local_loggers.erase(local_logger);
        }

        LogLevel level;

    private:
        LogProcessor()
            : level(LogLevel::Info) {}

        Vector<LocalLogger &> local_loggers;
    }

    class LocalLogger
    {
    public:
        LocalLogger()
        {
            LogProcessor::instance().add(this);
        }

        ~LocalLogger()
        {
            LogProcessor::instance().remove(this);
        }

        void log(LogLevel level, const char *msg)
        {
            if (level >= log_processor_.level)
            {
                while (writing_flag_.test_and_set())
                {
                }

                write_buffer.enqueue(detail::Log{level, msg});

                writing_flag_.clear();
            }
        }

        void swap()
        {
            while (writing_flag_.test_and_set())
            {
            }

            swap(write_buffer, read_buffer);

            writing_flag_.clear();
        }

    private:
        // * Include sync. might be expensive

        std::atomic_flag writing_flag_;
        Vcq<detail::Log, detail::local_buffer_size> write_buffer;
        Vcq<detail::Log, detail::local_buffer_size> read_buffer;
    };

    extern thread_local LocalLogger local_logger{};

    void debug(const char *msg)
    {
        local_logger.log(LogLevel::Debug, msg);
    }

    void info(const char *msg)
    {
        local_logger.log(LogLevel::Info, msg);
    }

    void warn(const char *msg)
    {
        local_logger.log(LogLevel::Warn, msg);
    }

    void error(const char *msg)
    {
        local_logger.log(LogLevel::Error, msg);
    }

    void fatal(const char *msg)
    {
        local_logger.log(LogLevel::Fatal, msg);
    }
};

#endif