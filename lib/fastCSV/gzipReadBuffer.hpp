#pragma once

#include <string_view>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>

#include "../miniz/miniz.h"

class GzipReadBuffer {
private:
    int fd = -1;

    tinfl_decompressor inflator{};
    static constexpr int inflate_flag = TINFL_FLAG_HAS_MORE_INPUT;

    static constexpr size_t MAX_LINE_SIZE = 32768; // in characters

    static constexpr size_t BUFF_SIZE_MB = 1; // needs to be a power of 2
    static constexpr size_t BUFF_SIZE_POW2 = BUFF_SIZE_MB * (1U << 20U);
    static constexpr size_t BUFF_SIZE_TOTAL = BUFF_SIZE_POW2 + MAX_LINE_SIZE;

    static constexpr size_t BUFF_SIZE_RAW = BUFF_SIZE_POW2 / 16;

    uint8_t raw_buffer[BUFF_SIZE_RAW]{};
    uint8_t *raw_begin = raw_buffer;
    uint8_t *raw_end = raw_buffer;
    bool raw_eof = false;
    bool need_header_parse = true;
public:
    char buffer[BUFF_SIZE_TOTAL]{};

    char *buffer_begin = buffer;
    char *buffer_end = buffer;

    bool eof = false;

    // open file when object is created
    explicit GzipReadBuffer(const char *path) {
        fd = open(path, O_RDONLY);
        assert(fd != -1);

        tinfl_init(&inflator);

        readMore(nullptr, 0);
    }

    // close the file when this object is deleted
    ~GzipReadBuffer() {
        assert(close(fd) == 0);
    }

    // read bytes from file, and write to buffer + starting_from
    // sets eof = true when there are no more bytes to be read
    void readMore(char *toKeep, size_t toKeepSize) {
        assert(toKeepSize < MAX_LINE_SIZE); // increase MAX_LINE_SIZE if this fails

        memcpy(buffer - MAX_LINE_SIZE + toKeepSize, toKeep, toKeepSize);

        memset(buffer + MAX_LINE_SIZE, 0 , BUFF_SIZE_POW2);

        // fetch more raw data if need be
        if (raw_begin == raw_end) {
            int readSize = read(fd, raw_buffer, BUFF_SIZE_RAW);
            assert(readSize != -1);

            if (readSize == 0) raw_eof = true;

            if (readSize != BUFF_SIZE_RAW) { // temporary
                readSize -= 8;
            }

            // update pointers
            raw_begin = raw_buffer;
            raw_end = raw_buffer + readSize;
        }

        // temp header skip
        if (need_header_parse) {
            raw_begin += 10;
            need_header_parse = false;
        }

        // we can start writing to the output buffer from this position
        buffer_end = buffer + MAX_LINE_SIZE;

        // decompress as much as possible
        size_t in_bytes = raw_end - raw_begin;
        size_t out_bytes = BUFF_SIZE_POW2;

        int tinfl_status = tinfl_decompress(&inflator,
                                            raw_begin, &in_bytes,
                                            (uint8_t *) buffer_end, (uint8_t *) buffer_end, &out_bytes,
                                            raw_eof ? 0 : TINFL_FLAG_HAS_MORE_INPUT);

        assert(tinfl_status >= 0); // negative means error occured

        raw_begin += in_bytes; // decompress used in_bytes of the raw input
        buffer_end += out_bytes; // decompress produced out_bytes of text output

        // set begin pointer
        buffer_begin = buffer + MAX_LINE_SIZE - toKeepSize;

        if (tinfl_status == 0) eof = true;
    }
};