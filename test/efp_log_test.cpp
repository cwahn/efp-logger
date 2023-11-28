#include <string>
#include "rt_log.hpp"

int main()
{
    using namespace efp;

    // Optional log level setting
    Logger::log_level = LogLevel::Debug;

    printf("sizeof PlainString %u\n", sizeof(detail::PlainString));
    printf("sizeof FormatString %u\n", sizeof(detail::FormatString));
    printf("sizeof std::string %u\n", sizeof(std::string));

    // Use the logging functions
    debug("The size of LogData: {} bytes", sizeof(detail::LogData));
    info("This is a info message with a float: {}", 3.14f);
    warn("This is an warn message with a double: {}", 42);
    error("This is a error message with a string: {}", "error");
    fatal("This is a fatal message with a std::string: {}", std::string("fatal error"));

    // Since the logging is done in a separate thread, wait for a while to see the logs
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}