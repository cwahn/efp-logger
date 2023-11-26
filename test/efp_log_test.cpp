#include "logger.hpp"

int main()
{
    efp::Log::instance().level = efp::LogLevel::Info;

    // Use the logging functions
    efp::debug("This is a debug message.");
    efp::info("This is an info message with an int: {}", 42);
    efp::warn("This is a warning message with a float: {}", 3.14f);
    efp::error("This is an error message with a double: {}", 2.71828);
    efp::fatal("This is a fatal message with a string: {}", "fatal error");

    // Since the logging is done in a separate thread, wait for a while to see the logs
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}