#pragma once

#include "rawReadBuffer.hpp"

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

    template<bool first_row = false>
    void parseNextRow() {
        int current_column = 0;
        char *old_col_0 = row.column[0];
        row.column[current_column++] = buff_pos;

        while (*buff_pos != '\n') {
            if (unlikely(buff_pos + 1 >= io.buffer_end)) {
                // this should not be the first row (increase buffer space if this assert fails)
                if constexpr (first_row) assert(false);

                // also takes care of buff_pos + 1 > io.buffer_end (we went over at the end of previous it), by using io.buffer_end
                size_t row_length_so_far = io.buffer_end - row.column[0];

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

        if constexpr (first_row) row.columns = current_column;
        assert(row.columns == current_column);
    }

    struct sentinel {
    };

public:
    explicit FastCSV(const char *path, const std::initializer_list<std::pair<std::string_view, int &>> &&header_args = {})
            : io{path}, buff_pos{io.buffer_begin} {
        parseNextRow<true>();

        // parse header column names
        if (header_args.size()) {
            for (auto[header_column, index_pointer] : header_args) index_pointer = -1; // set to -1 initially
            for (int i = 0; i < row.columns; ++i) {
                for (auto[header_column, index_pointer] : header_args) {
                    if (row[i] == header_column) {
                        index_pointer = i; // set given reference to this index
                        break;
                    }
                }
            }
        }
    }
    FastCSV(FastCSV &) = delete;
    FastCSV(FastCSV &&) = delete;

    void nextRow() { parseNextRow(); }
    [[nodiscard]] const FastCSVRow &getRow() const { return row; }
    [[nodiscard]] int getColumns() const { return row.columns; }

    /* end-sentinel iterator */

    class iterator {
    public:
        explicit iterator(FastCSV *fastCsv) : fastCsv{fastCsv} {}
        void operator++() { fastCsv->parseNextRow(); }
        bool operator!=(const sentinel) { return !fastCsv->io.eof; }
        const FastCSVRow &operator*() { return fastCsv->row; }
    private:
        FastCSV *fastCsv{};
    };

    iterator begin() { return iterator{this}; }
    sentinel end() { return sentinel{}; }
};