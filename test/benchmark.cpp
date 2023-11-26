#include <vector>
#include <iostream>
#include <chrono>
#include "logger.hpp"

int main()
{
    const int num_message = 1'000'000; // Number of messages to log for the benchmark

    // Start timer
    auto start = std::chrono::high_resolution_clock::now();

    // Log messages in a loop
    for (int i = 0; i < num_message; ++i)
    {
        efp::info("Logging message number: {}", i);
    }

    // Wait for all messages to be processed
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop timer
    auto end = std::chrono::high_resolution_clock::now();

    // Calculate and print the duration
    std::chrono::duration<double> duration = end - start;
    double total_seconds = duration.count() - 1; // Subtracting the sleep time
    double messages_per_second = num_message / total_seconds;

    std::cout << "Logged " << num_message << " messages in " << total_seconds << " seconds.\n";
    std::cout << "Average time per message: " << (total_seconds / num_message) << " seconds.\n";
    std::cout << "Messages per second: " << messages_per_second << std::endl;

    return 0;
}
