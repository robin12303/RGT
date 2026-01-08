#ifdef _WIN32
#include <windows.h>
#endif
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include "CircularBuffer.hpp"

int main() {
    CircularBuffer<double> tempBuffer(5);

    tempBuffer.push_back(23.5);
    tempBuffer.push_back(24.1);
    tempBuffer.push_back(23.8);
    tempBuffer.push_back(25.2);
    tempBuffer.push_back(24.7);
    tempBuffer.push_back(26.1); 
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    // begin()부터 순회(오래된 것부터)
    std::cout << "begin()부터 순회 시: ";
    for (double v : tempBuffer) std::cout << v << " ";
    std::cout << "\n\n";

    std::cout << "tempBuffer.size() = " << tempBuffer.size() << "\n";
    std::cout << "tempBuffer.capacity() = " << tempBuffer.capacity() << "\n";
    std::cout << "tempBuffer.empty() = " << std::boolalpha << tempBuffer.empty() << "\n";

    double maxTemp = *std::max_element(tempBuffer.begin(), tempBuffer.end());
    double avgTemp = std::accumulate(tempBuffer.begin(), tempBuffer.end(), 0.0) / tempBuffer.size();

    std::cout << "maxTemp = " << maxTemp << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "avgTemp = " << avgTemp << "\n";

    std::cout << "tempBuffer.front() = " << tempBuffer.front() << "  // 가장 오래된 데이터\n";
    std::cout << "tempBuffer.back()  = " << tempBuffer.back()  << "  // 가장 최근 데이터\n";
}
