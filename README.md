# EFP Logger

## Overview

EFP Logger is a C++ logging library designed for real-time concurrency application. The typical log-writing takes only "tens of nano seconds". 

## Exmaple

```c++
int main()
{
    using namespace efp;

    // Optional log level setting. Default is LogLevel::Info
    Logger::set_log_level(LogLevel::Trace);

    // Optional log output setting. // default is stdout
    Logger::set_output("./efp_logger_test.log");
    // Logger::set_output(stdout);

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
```

```log
2023-12-02 03:25:28 TRACE This is a trace message with no formating
2023-12-02 03:25:28 DEBUG This is a debug message with a pointer: 0x0
2023-12-02 03:25:28 INFO  This is a info message with a float: 3.14
2023-12-02 03:25:28 WARN  This is a warn message with a int: 42
2023-12-02 03:25:28 ERROR This is a error message with a string literal: error
2023-12-02 03:25:28 FATAL This is a fatal message with a std::string: fatal error
```

## Features

In order to implement such functionality a few techniques are combined.

- **Asynchronous Processing**: Festival the lock is processed by separate external thread. Regardless of the destination of log, overhead of making side effect is not acceptable. By utilizing `efp::Enum` type, parsing, and printing will be handled by external thread.

- **Zero Allocation Logging**: Allocation often cause unbounded amount of overhead, which is not acceptable for real time application. EFP RtLog utilize zero allocation ring, buffer, which guarantees uniform operation. The small cost is doubled memory and dual copy.

- **Double Buffering with Spinlock**: Synchronization Should be also minimized in real time application. EFP RtLog Implement double buffering, Which will make logging sync free and non-blocking. Upper bound of blocking time would be approximately the same with a single pointer swap.

- **Formatting**: By utilizing the excellent `fmt` internally, `efp::RtLog`  buffers virtually the same core API with `fmt::format` and `fmt::print`.


## Performance

EFP RT Log's design is benchmarked to handle high-throughput scenarios, ensuring efficient logging even under heavy load.

```plaintext
Logged 1000000 messages in 0.0203815 seconds.
Average time per message: 2.03815e-08 seconds.
Messages per second: 4.90641e+07
```

## Todo
- Pointer type auto conversion to void *

## Integration

To use EFP RT Log, include `logger.hpp` in your project and ensure dependencies, especially the `fmt` library, are correctly linked.

