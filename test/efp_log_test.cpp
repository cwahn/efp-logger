#include <string>
#include "rt_log.hpp"

int main()
{
    using namespace efp;

    // Optional log level setting
    Logger::log_level = LogLevel::Debug;

    printf("sizeof PlainMessage %u bytes\n", sizeof(detail::PlainMessage));
    printf("sizeof FormatedMessage %u bytes\n", sizeof(detail::FormatedMessage));
    printf("The size of LogData: %u bytes\n", sizeof(detail::LogData));
    printf("sizeof std::string %u bytes\n", sizeof(std::string));

    // Use the logging functions
    debug("The address of Logger::log_level: {:p}", (void *)&Logger::log_level);
    info("This is a info message with a float: {}", 3.14f);
    warn("This is an warn message with a int: {}", 42);
    error("This is a error message with a string literal: {}", "error");
    // ! Sending std::string to the buffer is O(n) and may increase risk of buffer overflow
    fatal("This is a fatal message with a std::string: {}", std::string("fatal error"));

    // Since the logging is done in a separate thread, wait for a while to see the logs
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}