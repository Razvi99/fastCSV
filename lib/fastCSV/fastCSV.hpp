#pragma once

#include "rawReadBuffer.hpp"
#include <cstring>

template<int max_columns, class ReadBuffer = RawReadBuffer>
class FastCSV {
    ReadBuffer io{};

    char *buff_pos;

    // holds a pointer to the beginning of the element on column X
    char *column[max_columns]{};
    int real_columns = -1;

    void parseNextRow() {
        int current_column = 0;
        char *old_col_0 = column[0];
        column[current_column++] = buff_pos;

        while (*buff_pos != '\n') {
            if (*buff_pos == ',') column[current_column++] = buff_pos + 1;

            if (buff_pos + 1 >= io.buffer_end) {
                // this should not be the first row (increase buffer space if this assert fails)
                assert(real_columns);

                int row_length_so_far = buff_pos - column[0] + 1;

                assert(row_length_so_far + io.buffer < column[0]); // no overlap, increase buffer if this fails

                // copy what the data for this row to the beginning of the buffer, and read more data after that
                memcpy(io.buffer, column[0], row_length_so_far);
                io.readMore(row_length_so_far);

                // reset the current buffer position to the beginning
                buff_pos = io.buffer;

                // if we could read more bytes, then parse the row from the beginning, if not.. stop
                if (!io.eof) parseNextRow();
                else {
                    assert(current_column == 1);
                    // if this is the end of file, restore the first column (modified at the beginning of this function)
                    column[0] = old_col_0;
                }
                return;
            }

            ++buff_pos;
        }

        // this is the start of the next row, used in size calculation for string_view
        column[current_column] = ++buff_pos;

        if (real_columns == -1) { real_columns = current_column; }
        else { assert(current_column == real_columns); }
    }

    // also works with negative indexes, -1 will get the last element of row
    std::string_view at(int index) {
        if (index < 0) index = real_columns + index;
        assert(index < real_columns && index >= 0);

        return std::string_view{column[index], (size_t) (column[index + 1] - column[index]) - 1};
    }

    bool eof() { return io.eof; }

public:
    explicit FastCSV(const char *path) : io{path}, buff_pos{io.buffer} { parseNextRow(); }

    class iterator {
    public:
        explicit iterator(FastCSV *fastCsv, int index) : index{index}, fastCsv{fastCsv} {}

        iterator &operator++() {
            index++;
            fastCsv->parseNextRow();

            if (fastCsv->io.eof) index = -1; // if this is the end of file, set index = -1, indicating the end() iterator
            return *this;
        }
        bool operator!=(const iterator &other) { return index != other.index; }
        iterator operator*() { return *this; }

        // this operator is used after dereferencing, so this class is a row & iterator combination
        std::string_view operator[](int subscript) { return fastCsv->at(subscript); }

    private:
        int index{};
        FastCSV *fastCsv{};
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, -1); }
};