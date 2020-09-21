#include <iostream>
#include <chrono>

#include "lib/fastCSV/fastCSV.hpp"
#include "lib/fastCSV/rawReadBuffer.hpp"
#include "lib/fastCSV/gzipReadBuffer.hpp"

int main() {
    auto fastCSV = new FastCSV<500, GzipReadBuffer>("../data.csv.gz");

    auto start = std::chrono::steady_clock::now();

    fastCSV->skipRow(); // skips header
    auto second_row = fastCSV->getRow(); // gets current row (2nd)
    std::string_view first_col = second_row[-2]; // access second to last column of row
    std::string_view second_row_string = second_row.getRaw(); // get the whole row as a string

    std::cerr << first_col << "\n" << second_row_string << "\n";

    for (auto row : *fastCSV) {
        if (row[2] == "ColumnText") // std::string_view supports == between strings
            std::cerr << row.getRaw();
    }

    auto end = std::chrono::steady_clock::now();

    printf("total: %f ms\n", (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1e+6);

    return 0;
}
