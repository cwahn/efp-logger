#include "rt_log.hpp"

int main()
{
    using namespace efp;

    // Optional log level setting
    RtLog::log_level = LogLevel::Info;

    // Use the logging functions
    debug("The size of LogData: {} bytes", sizeof(detail::LogData));
    info("The size of FormatString: {} bytes", sizeof(detail::FormatString));
    warn("This is a warning message with a float: {}", 3.14f);
    error("This is an error message with a double: {}", 2.71828);
    fatal("This is a fatal message with a string: {}", "fatal error");

    // Since the logging is done in a separate thread, wait for a while to see the logs
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}