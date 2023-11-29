# EFP RtLog 

## Overview

EFP RT Log is a C++ logging library designed for real-time concurrency application. The typical log-writing takes only "tens of nano seconds". 

## Exmaple

```c++
int main()
{
    using namespace efp;

    // Optional log level setting
    Logger::log_level = LogLevel::Debug;

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
```

```log
2023-11-29 01:16:46 DEBUG The address of Logger::log_level: 0x1029c8870
2023-11-29 01:16:46 INFO  This is a info message with a float: 3.14
2023-11-29 01:16:46 WARN  This is an warn message with a int: 42
2023-11-29 01:16:46 ERROR This is a error message with a string literal: error
2023-11-29 01:16:46 FATAL This is a fatal message with a std::string: fatal error
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

