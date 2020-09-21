#pragma once

#include "rawReadBuffer.hpp"
#include <cstring>

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

template<int max_columns, class ReadBuffer = RawReadBuffer>
class FastCSV {
    ReadBuffer io{};

    // indicates the current parsing position
    char *buff_pos;

    class FastCSVRow {
        friend class FastCSV;

    public:
        // returns the whole row, ',' included
        [[nodiscard]] std::string_view getRaw() const { return std::string_view{column[0], (size_t) (column[columns] - column[0]) - 1}; }

        // also works with negative indexes, -1 will get the last element of row
        [[nodiscard]] std::string_view operator[](int index) const {
            if (index < 0) index = columns + index;
            assert(index < columns && index >= 0);

            return std::string_view{column[index], (size_t) (column[index + 1] - column[index]) - 1};
        }

    private:
        int columns = -1; // number of columns in the first row, used to check column number integrity in the document
        char *column[max_columns + 1]{}; // holds a pointer to the beginning of the element of column X
    } row{};

    void parseNextRow() {
        int current_column = 0;
        char *old_col_0 = row.column[0];
        row.column[current_column++] = buff_pos;

        while (*buff_pos != '\n') {
            if (unlikely(buff_pos + 1 >= io.buffer_end)) {
                // this should not be the first row (increase buffer space if this assert fails)
                assert(row.columns);

                size_t row_length_so_far = buff_pos - row.column[0] + 1;

                // copy what the data for this row to the beginning of the buffer, and read more data after that
                io.readMore(row.column[0], row_length_so_far);

                // reset the current buffer position to the new beginning
                buff_pos = io.buffer_begin;

                // if we could read more bytes, then parse the row from the beginning, if not.. stop
                if (!io.eof) parseNextRow();
                else {
                    assert(current_column == 1);
                    // if this is the end of file, restore the first column (modified at the beginning of this function)
                    row.column[0] = old_col_0;
                }
                return;
            }

            // mark the start of a new column right after the ','
            if (*buff_pos == ',') row.column[current_column++] = buff_pos + 1;

            ++buff_pos;
        }

        // this is the start of the next row, used in size calculation for string_view
        row.column[current_column] = ++buff_pos;

        if (row.columns == -1) { row.columns = current_column; }
        else { assert(current_column == row.columns); }
    }

    struct sentinel {
    };

public:
    explicit FastCSV(const char *path) : io{path}, buff_pos{io.buffer_begin} { parseNextRow(); }
    FastCSV(FastCSV &) = delete;
    FastCSV(FastCSV &&) = delete;

    void skipRow() { parseNextRow(); }
    FastCSVRow &getRow() { return row; }
    int getColumns() { return row.columns; }

    class iterator {
    public:
        explicit iterator(FastCSV *fastCsv) : fastCsv{fastCsv} {}
        void operator++() { fastCsv->parseNextRow(); }
        bool operator!=(const sentinel) { return !fastCsv->io.eof; }
        FastCSVRow &operator*() { return fastCsv->row; }
    private:
        FastCSV *fastCsv{};
    };

    iterator begin() { return iterator{this}; }
    sentinel end() { return sentinel{}; }
};