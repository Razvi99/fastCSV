#include <iostream>
#include <chrono>

#include "lib/fastCSV/fastCSV.hpp"
#include "lib/fastCSV/rawReadBuffer.hpp"
#include "lib/fastCSV/gzipReadBuffer.hpp"

int main() {
    auto fastCSV = new FastCSV<500, GzipReadBuffer>("../data.csv.gz");

    auto start = std::chrono::steady_clock::now();

    int i = 0;
    for (auto row : *fastCSV) {
        if (row[0] == "-1") // std::string_view supports == between strings
            i++;
    }

    auto end = std::chrono::steady_clock::now();

    std::cerr << i << "\n";
    printf("total: %f ms\n", (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e+6);

    return 0;
}
