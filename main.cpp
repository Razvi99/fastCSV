#include <iostream>
#include <chrono>

#include "lib/fastCSV/fastCSV.hpp"
#include "lib/fastCSV/rawReadBuffer.hpp"
#include "lib/fastCSV/gzipReadBuffer.hpp"

int main() {
    auto fastCSV = new FastCSV<500, GzipReadBuffer>("../data.csv.gz");

    auto start = std::chrono::steady_clock::now();

    int i = 0;

    auto it = fastCSV->begin();
    auto enda = fastCSV->end();
    while (it != enda) {
        ++it;
        if (it[1][0] == '0')
            i++;
    }
    /*for (auto row : fastCSV) {
        if(row[1][0] == '0')
            i++;
    }*/

    auto end = std::chrono::steady_clock::now();

    std::cerr << i << "\n";
    printf("total: %f ms\n", (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e+6);

    return 0;
}
