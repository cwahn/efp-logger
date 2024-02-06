#include <vector>
#include <iostream>
#include <chrono>

#define EFP_LOG_COUNT true

#include "efp/logger.hpp"
// int log_count;

int main() {
    const int num_message = 1000000; // Number of messages to log for the benchmark

    // efp::Logger::log_level = efp::LogLevel::Fatal;

    // Start timer
    auto start = std::chrono::high_resolution_clock::now();

    // Logger messages in a loop
    for (int i = 0; i < num_message; ++i) {
        efp::info("Logging message number: {}", i);
    }

    // // Wait for all messages to be processed
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop timer
    auto end = std::chrono::high_resolution_clock::now();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Calculate and print the duration
    std::chrono::duration<double> duration = end - start;
    double total_seconds = duration.count();
    double messages_per_second = num_message / total_seconds;

    std::cout << "Logged " << num_message << " messages in " << total_seconds << " seconds.\n";
    std::cout << "Average time per message: " << (total_seconds / num_message) << " seconds.\n";
    std::cout << "Messages written per second: " << messages_per_second << std::endl;
    std::cout << "Total Messages processed: " << efp::Logger::instance().log_count << std::endl;

    return 0;
}
