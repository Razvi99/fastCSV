#pragma once

#include "rawReadBuffer.hpp"

#ifdef __AVX2__

#include <x86intrin.h>

#endif

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
    bool eos = false; // end of stream

public:
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

        FastCSVRow() = default;
        FastCSVRow(FastCSVRow &) = delete;
        FastCSVRow(FastCSVRow &&) = delete;
    private:
        int columns = -1; // number of columns in the first row, used to check column number integrity in the document
        char *column[max_columns + 1]{}; // holds a pointer to the beginning of the element of column X
    } row{};

private:
#ifdef __AVX2__
    static inline __attribute__((always_inline, unused)) uint64_t maskForChar(char *ptr, char toFind) {
        // load
        __m256i reg_lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr));
        __m256i reg_hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 32));

        // compare
        const __m256i mask = _mm256_set1_epi8(toFind);
        uint64_t cmp_lo = static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(reg_lo, mask)));
        uint64_t cmp_hi = _mm256_movemask_epi8(_mm256_cmpeq_epi8(reg_hi, mask));

        return cmp_lo | (cmp_hi << 32ULL);
    }

    static inline __attribute__((always_inline, unused)) int trailing_zeroes(uint64_t input) {
#ifdef __BMI2__
        return (int) _tzcnt_u64(input);
#else
        return __builtin_ctzll(input);
#endif
    }

    inline __attribute__((always_inline, unused)) int popcount(uint64_t input) { return __builtin_popcountll(input); }

    template<bool first_row = false>
    void parseNextRow() {
        if (unlikely(io.eof)) {
            eos = true;
            return;
        }

        if (unlikely(buff_pos + 64 >= io.buffer_end && !io.eof)) {
            assert(io.buffer_end - buff_pos >= 0);

            // copy the data for this row to the beginning of the buffer, and read more data after that
            // io.buffer_end - buff_pos is the number of bytes to be kept in the buffer
            io.readMore(buff_pos, io.buffer_end - buff_pos);
            buff_pos = io.buffer_begin;
        }

        int current_column = 0;
        row.column[current_column++] = buff_pos;

        while (maskForChar(buff_pos, '\n') == 0ULL) { // if no newline found in the next 64 bytes
            uint64_t masked_commas = maskForChar(buff_pos, ','); // 1 bit where comma was found
            const int set_bits = popcount(masked_commas); // count 1 bits

            // process all comma locations
            for (int i = 0; i < set_bits; i++) {
                row.column[current_column++] = buff_pos + 1 + trailing_zeroes(masked_commas);
                masked_commas = masked_commas & (masked_commas - 1ULL); // remove trailing 1 bit
            }

            buff_pos += 64;

            // if next step would exit
            if (unlikely(buff_pos + 64 >= io.buffer_end)) {
                // this should not be the first row (increase buffer space if this assert fails)
                if constexpr (first_row) assert(false);

                // copy the data for this row to the beginning of the buffer, and read more data after that
                // io.buffer_end - row.column[0] is the number of bytes to be kept in the buffer
                io.readMore(row.column[0], io.buffer_end - row.column[0]);

                // if there are more bytes to process, reset buffer position and reparse this row
                if (likely(!io.eof)) {
                    buff_pos = io.buffer_begin;
                    return parseNextRow();
                }
                // else exit while
                // it is guaranteed by io.readMore() that (toKeep, toKeep + toKeepSize) is the same as before the call if io.eof
                break;
            }
        }

        // manually process last bytes
        int new_pos = trailing_zeroes(maskForChar(buff_pos, '\n'));
        for (int offset = 0; offset < new_pos; ++offset)
            if (buff_pos[offset] == ',') row.column[current_column++] = buff_pos + offset + 1;
        buff_pos += new_pos;

        // this is the start of the next row, used in size calculation for string_view
        row.column[current_column] = ++buff_pos; // also skip newline

        if constexpr (first_row) row.columns = current_column;
        assert(row.columns == current_column && "CSV file has inconsistent number of columns");
    }
#else
    template<bool first_row = false>
    void parseNextRow() {
        if (unlikely(io.eof)) {
            eos = true;
            return;
        }

        int current_column = 0;
        row.column[current_column++] = buff_pos;

        while (*buff_pos != '\n') {
            // mark the start of a new column right after the ','
            if (*buff_pos == ',') row.column[current_column++] = buff_pos + 1;

            ++buff_pos;

            if (unlikely(buff_pos + 1 >= io.buffer_end)) {
                // this should not be the first row (increase buffer space if this assert fails)
                if constexpr (first_row) assert(false);

                // copy the data for this row to the beginning of the buffer, and read more data after that
                // io.buffer_end - row.column[0] is the number of bytes to be kept in the buffer
                io.readMore(row.column[0], io.buffer_end - row.column[0]);

                // if there are more bytes to process, reset buffer position and reparse this row
                if (likely(!io.eof)) {
                    buff_pos = io.buffer_begin;
                    return parseNextRow();
                }
                // else exit while
                // it is guaranteed by io.readMore() that (toKeep, toKeep + toKeepSize) is the same as before the call if io.eof
                break;
            }
        }

        // this is the start of the next row, used in size calculation for string_view
        row.column[current_column] = ++buff_pos; // also skip newline

        if constexpr (first_row) row.columns = current_column;
        assert(row.columns == current_column && "CSV file has inconsistent number of columns");
    }
#endif

    struct sentinel {
    };

public:
    explicit FastCSV(const char *path, const std::initializer_list<std::pair<std::string_view, int &>> &&header_args = {})
            : io{path}, buff_pos{io.buffer_begin} {
        parseNextRow<true>();

        // make sure that max_columns were enough
        assert(row.columns < max_columns + 1 && "CSV file has more columns than given maximum");

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
    [[nodiscard]] bool finished() const { return eos; }
    [[nodiscard]] int getColumns() const { return row.columns; }

    /* end-sentinel iterator */

    class iterator {
    public:
        explicit iterator(FastCSV *fastCsv) : fastCsv{fastCsv} {}
        void operator++() { fastCsv->parseNextRow(); }
        bool operator!=(const sentinel) { return !fastCsv->eos; }
        const FastCSVRow &operator*() { return fastCsv->row; }
    private:
        FastCSV *fastCsv{};
    };

    iterator begin() { return iterator{this}; }
    sentinel end() { return sentinel{}; }
};