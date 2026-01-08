#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class ParallelProcessor {
public:
    explicit ParallelProcessor(std::size_t threadCount = std::thread::hardware_concurrency())
        : threads_(threadCount ? threadCount : 1) {}

    std::size_t thread_count() const noexcept { return threads_; }

    template <typename Func>
    auto parallel_map(const std::vector<T>& input, Func func) const
        -> std::vector<std::decay_t<std::invoke_result_t<Func, T>>>  // T -> R
    {
        using Out = std::decay_t<std::invoke_result_t<Func, T>>;
        const std::size_t n = input.size();

        std::vector<Out> output(n);
        if (n == 0) return output;

        std::size_t workerCount = std::min(threads_, n);
        if (workerCount <= 1) {
            for (std::size_t i = 0; i < n; ++i) output[i] = func(input[i]);
            return output;
        }

        std::atomic<bool> failed{false};
        std::exception_ptr firstException = nullptr;
        std::mutex exMutex;

        auto worker = [&](std::size_t begin, std::size_t end) {
            try {
                for (std::size_t i = begin; i < end && !failed.load(std::memory_order_relaxed); ++i) {
                    output[i] = func(input[i]);
                }
            } catch (...) {
                failed.store(true, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(exMutex);
                if (!firstException) firstException = std::current_exception();
            }
        };

        std::vector<std::thread> pool;
        pool.reserve(workerCount);

        const std::size_t block = n / workerCount;
        const std::size_t rem   = n % workerCount;

        std::size_t start = 0;
        for (std::size_t t = 0; t < workerCount; ++t) {
            std::size_t len = block + (t < rem ? 1 : 0);
            std::size_t end = start + len;
            pool.emplace_back(worker, start, end);
            start = end;
        }

        for (auto& th : pool) th.join();

        if (firstException) std::rethrow_exception(firstException);
        return output;
    }

private:
    std::size_t threads_;
};
