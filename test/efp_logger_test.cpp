#include <string>

#include "logger.hpp"

int main()
{
    using namespace efp;

    // Optional log level setting. Default is LogLevel::Info
    Logger::set_log_level(LogLevel::Trace);

    // Optional log output setting. // default is stdout
    // Logger::set_output("./efp_logger_test.log");
    // Logger::set_output(stdout);

    printf("sizeof PlainMessage %lu bytes\n", sizeof(detail::PlainMessage));
    printf("sizeof FormatedMessage %lu bytes\n", sizeof(detail::FormatedMessage));
    printf("The size of LogData: %lu bytes\n", sizeof(detail::LogData));
    printf("sizeof std::string %lu bytes\n", sizeof(std::string));

    // Use the logging functions
    trace("This is a trace message with no formating");
    debug("This is a debug message with a pointer: {:p}", (void *)nullptr);
    info("This is a info message with a float: {}", 3.14f);
    warn("This is a warn message with a int: {}", 42);
    error("This is a error message with a string literal: {}", "error");
    // ! Sending std::string to the buffer is O(n) and may increase risk of buffer overflow
    // ! Every 20 ~ 30 char will take one buffer space.
    fatal("This is a fatal message with a std::string: {}", std::string("fatal error"));

    // Since the logging is done in a separate thread, wait for a while to see the logs
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    return 0;
}