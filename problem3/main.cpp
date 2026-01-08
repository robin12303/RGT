#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <algorithm>
#include <chrono>
#include <functional>   // std::function
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "ParallelProcessor.hpp"

static long long measure_ms(const std::function<void()>& fn) {
    auto s = std::chrono::high_resolution_clock::now();
    fn();
    auto e = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::vector<int> pixelData(1'000'000);
    std::iota(pixelData.begin(), pixelData.end(), 0);

    ParallelProcessor<int> processor(4);

    auto brighten = [&](int pixel) -> int {
        return std::min(255, pixel + 50); // NOMINMAX 있으면 안전
        // 또는 매크로 지뢰 100% 회피하려면: return (std::min)(255, pixel + 50);
    };

    std::vector<int> brightSeq;
    std::vector<int> brightPar;

    long long seqMs = measure_ms([&]{
        brightSeq.resize(pixelData.size());
        for (std::size_t i = 0; i < pixelData.size(); ++i) {
            brightSeq[i] = brighten(pixelData[i]);
        }
    });

    long long parMs = measure_ms([&]{
        brightPar = processor.parallel_map(pixelData, brighten);
    });

    std::cout << u8"// brightenedImage 결과\n";
    std::cout << "brightenedImage[0] = " << brightPar[0] << "  // 0 + 50\n";
    std::cout << "brightenedImage[1] = " << brightPar[1] << "  // 1 + 50\n";
    std::cout << "brightenedImage[100] = " << brightPar[100] << "  // 100 + 50\n";
    std::cout << "brightenedImage[999999] = " << brightPar[999999] << "  // min(255, 999999 + 50)\n\n";

    auto pixelStrings = processor.parallel_map(pixelData, [](int pixel) -> std::string {
        return "pixel_" + std::to_string(pixel);
    });

    std::cout << u8"// pixelStrings 결과\n";
    std::cout << "pixelStrings[0] = \"" << pixelStrings[0] << "\"\n";
    std::cout << "pixelStrings[1] = \"" << pixelStrings[1] << "\"\n";
    std::cout << "pixelStrings[100] = \"" << pixelStrings[100] << "\"\n\n";

    auto squaredPixels = processor.parallel_map(pixelData, [](int pixel) -> long long {
        return 1LL * pixel * pixel;
    });

    std::cout << u8"// squaredPixels 결과\n";
    std::cout << "squaredPixels[0] = " << squaredPixels[0] << "\n";
    std::cout << "squaredPixels[1] = " << squaredPixels[1] << "\n";
    std::cout << "squaredPixels[10] = " << squaredPixels[10] << "\n\n";

    std::cout << u8"// 성능 측정 결과 출력\n";
    std::cout << "Processing " << pixelData.size() << " elements with " << processor.thread_count() << " threads\n";
    std::cout << "Sequential time: " << seqMs << "ms\n";
    std::cout << "Parallel time: " << parMs << "ms\n";
    if (parMs > 0) {
        std::cout << "Speedup: " << (double)seqMs / (double)parMs << "x\n";
    }
}
