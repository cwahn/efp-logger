#include "logger.hpp"

int main()
{
    efp::Log::instance().level = efp::LogLevel::Debug;

    // Use the logging functions
    efp::debug("The size of int: {}", (int)sizeof(int));
    efp::debug("The size of double: {}", (int)sizeof(double));
    efp::info("The size of LogData: {} bytes", (int)sizeof(efp::detail::LogData));
    efp::info("The size of FormatString: {} bytes", (int)sizeof(efp::detail::FormatString));
    efp::info("The size of const char *: {} bytes", (int)sizeof(const char *));
    efp::warn("This is a warning message with a float: {}", 3.14f);
    efp::error("This is an error message with a double: {}", 2.71828);
    efp::fatal("This is a fatal message with a string: {}", "fatal error");

    // Since the logging is done in a separate thread, wait for a while to see the logs
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}