#pragma once

#include <string_view>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>

#include "../miniz/miniz.h"
#include "../miniz/minizInflator.hpp"

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

class GzipReadBuffer {
private:
    static constexpr size_t BUFF_SIZE_MB = 1;
    static constexpr size_t BUFF_SIZE_TOTAL = BUFF_SIZE_MB * (1U << 20U);
    static constexpr size_t BUFF_SIZE_RAW = BUFF_SIZE_TOTAL / 16;

    int fd = -1;

    uint8_t raw_buffer[BUFF_SIZE_RAW]{};
    uint8_t *raw_begin = raw_buffer;
    uint8_t *raw_end = raw_buffer;
    bool raw_eof = false;
    bool need_header_parse = true;

    char buffer[BUFF_SIZE_TOTAL]{};

    MinizInflator inflator{};

public:
    char *buffer_begin = buffer;
    char *buffer_end = buffer;

    bool eof = false;

    // open file when object is created
    explicit GzipReadBuffer(const char *path) {
        fd = open(path, O_RDONLY);
        assert(fd != -1);

        readMore(nullptr, 0);
    }

    // close the file when this object is deleted
    ~GzipReadBuffer() {
        assert(close(fd) == 0);
    }

    // read bytes from file, and write to buffer + starting_from
    // sets eof = true when there are no more bytes to be read
    void readMore(char *toKeep, size_t toKeepSize) {
        // copy toKeep data exactly before the data we'll read below
        memmove(buffer, toKeep, toKeepSize);

        // eof if inflator reported done last time
        if (unlikely(inflator.done)) {
            assert(inflator.verifyFooter(raw_begin));

            if (raw_begin != raw_end && raw_end - raw_begin > 8) { // appended file
                raw_begin += 8; // skip footer
                raw_begin += MinizInflator::parseGzipHeader(raw_begin); // parse header
                inflator = {}; // reset inflator
            } else {
                eof = true;
                return;
            }
        }

        buffer_end = buffer + toKeepSize;

        // fetch more raw data if need be
        if (raw_begin == raw_end) {
            int readSize = read(fd, raw_buffer, BUFF_SIZE_RAW);
            assert(readSize != -1);

            if (readSize == 0) raw_eof = true;

            // update pointers
            raw_begin = raw_buffer;
            raw_end = raw_buffer + readSize;
        }

        // temp header skip
        if (unlikely(need_header_parse)) {
            raw_begin += MinizInflator::parseGzipHeader(raw_begin);
            need_header_parse = false;
        }

        size_t available_in = raw_end - raw_begin;
        size_t available_out = BUFF_SIZE_TOTAL - toKeepSize;
        inflator.inflate(raw_begin, &available_in, buffer_end, &available_out, !raw_eof);

        // the difference between the original available size and the available size after the call is the size of written bytes
        buffer_end += (BUFF_SIZE_TOTAL - toKeepSize) - available_out;

        // same for raw_begin
        raw_begin += (raw_end - raw_begin) - available_in;
    }
};