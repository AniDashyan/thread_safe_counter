#include <atomic>
#include <thread>
#include <vector>
#include <format>
#include <fstream>
#include <string>
#include <utility>
#include "kaizen.h"

enum class MemoryOrder {
    SEQ_CST,
    RELAXED,
    ACQUIRE,
    RELEASE,
    ACQ_REL
};

void increment_counter(std::atomic<int>& counter, int increments, MemoryOrder order) {
    for (int i = 0; i < increments; ++i) {
        switch (order) {
            case MemoryOrder::SEQ_CST:
                counter.fetch_add(1, std::memory_order_seq_cst);
                break;
            case MemoryOrder::RELAXED:
                counter.fetch_add(1, std::memory_order_relaxed);
                break;
            case MemoryOrder::ACQUIRE:
                counter.fetch_add(1, std::memory_order_acquire);
                break;
            case MemoryOrder::RELEASE:
                counter.fetch_add(1, std::memory_order_release);
                break;
            case MemoryOrder::ACQ_REL:
                counter.fetch_add(1, std::memory_order_acq_rel);
                break;
        }
    }
}

struct TestResult {
    std::string order_name;
    int counter_value;
    long long duration_ms;
};

TestResult run_test(int num_threads, int increments_per_thread, MemoryOrder order) {
    std::atomic<int> counter(0);
    std::vector<std::thread> threads;

    zen::timer t;

    t.start();
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(increment_counter, std::ref(counter), increments_per_thread, order);
    }

    for (auto& t : threads) {
        t.join();
    }

    t.stop();
    auto duration = t.duration<zen::timer::msec>();

    int actual;
    switch (order) {
        case MemoryOrder::SEQ_CST:
            actual = counter.load(std::memory_order_seq_cst);
            break;
        case MemoryOrder::RELAXED:
            actual = counter.load(std::memory_order_relaxed);
            break;
        case MemoryOrder::ACQUIRE:
            actual = counter.load(std::memory_order_acquire);
            break;
        case MemoryOrder::RELEASE:
            actual = counter.load(std::memory_order_relaxed);
            break;
        case MemoryOrder::ACQ_REL:
            actual = counter.load(std::memory_order_acquire);
            break;
    }

    std::string order_name;
    switch (order) {
        case MemoryOrder::SEQ_CST: order_name = "seq_cst"; break;
        case MemoryOrder::RELAXED: order_name = "relaxed"; break;
        case MemoryOrder::ACQUIRE: order_name = "acquire"; break;
        case MemoryOrder::RELEASE: order_name = "release"; break;
        case MemoryOrder::ACQ_REL: order_name = "acq_rel"; break;
    }

    return {order_name, actual, duration.count()};
}

std::pair<int, int> parse_args(int argc, char** argv) {
    zen::cmd_args args(argv, argc);
    if (!args.is_present("--thread") || args.is_present("--iter")) {
        zen::log(zen::color::yellow("Warning: "), "Using default values for threads and increments.");
        return {4, 1000000};
    }
    int num_threads = std::stoi(args.get_options("--thread")[0]);
    int increments_per_thread = std::stoi(args.get_options("--iter")[0]);
    return {num_threads, increments_per_thread};
}

int main(int argc, char** argv) {
    
    auto [num_threads, increments_per_thread] = parse_args(argc, argv);

    std::vector<TestResult> results;
    results.reserve(5);
    results.push_back(run_test(num_threads, increments_per_thread, MemoryOrder::SEQ_CST));
    results.push_back(run_test(num_threads, increments_per_thread, MemoryOrder::RELAXED));
    results.push_back(run_test(num_threads, increments_per_thread, MemoryOrder::ACQUIRE));
    results.push_back(run_test(num_threads, increments_per_thread, MemoryOrder::RELEASE));
    results.push_back(run_test(num_threads, increments_per_thread, MemoryOrder::ACQ_REL));

    // Write results to CSV
    std::ofstream out_file("results.csv");
    if (!out_file) {
        zen::log(zen::color::red("Error: Could not open results.csv"));
        return 1;
    }

    out_file << "MemoryOrder,CounterValue,DurationMs\n";
    for (const auto& result : results) {
        auto row = std::format(
            "{},{},{}\n",
            result.order_name,
            result.counter_value,
            result.duration_ms
        );
        out_file << row;
    }
    out_file.close();


    auto config = std::format(
        "Configuration:\n"
        "Threads: {}\n"
        "Increments/thread: {}\n",
        num_threads,
        increments_per_thread
    );
    zen::log(config.c_str());
    zen::log("");

    auto header = std::format(
        "{:<12} {:<12} {:<10}\n",
        "Memory Order",
        "Counter",
        "Time (ms)"
    );
    zen::log(header);
    zen::log("------------------------------------");

    for (const auto& result : results) {
        auto row = std::format(
            "{:<12} {:<12} {:<10}\n",
            result.order_name,
            result.counter_value,
            result.duration_ms
        );
        zen::print(row.c_str());
    }

    zen::log("");
    zen::log("Results written to results.csv for plotting");

    return 0;
}