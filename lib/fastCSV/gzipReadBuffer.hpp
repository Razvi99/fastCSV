#pragma once

#include <string_view>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

#include "../zlib/zlib.h"

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

    char buffer[BUFF_SIZE_TOTAL + 64]{};

    z_stream inflator{};
    bool zlib_eos = false;

public:
    char *buffer_begin = buffer;
    char *buffer_end = buffer;

    bool eof = false;

    // open file when object is created
    explicit GzipReadBuffer(const char *path) {
        fd = open(path, O_RDONLY);
        assert(fd != -1);

        // + 16 for gzip header & footer parsing
        assert(inflateInit2(&inflator, 15 + 16) == 0);

        readMore(nullptr, 0);
    }

    // close the file when this object is deleted
    ~GzipReadBuffer() {
        assert(close(fd) == 0);
        assert(inflateEnd(&inflator) == 0);
    }

    // read bytes from file, and write to buffer + starting_from
    // sets eof = true when there are no more bytes to be read
    void readMore(char *toKeep, size_t toKeepSize) {
        // fetch more raw data if needed
        if (raw_begin == raw_end) {
            int readSize = read(fd, raw_buffer, BUFF_SIZE_RAW);
            assert(readSize != -1);

            // update pointers
            raw_begin = raw_buffer;
            raw_end = raw_buffer + readSize;
        }

        // eof if inflator reported done last time && no more raw data left
        if (unlikely(zlib_eos)) {
            if (raw_begin != raw_end) { // appended file
                assert(inflateReset(&inflator) == 0);
            } else {
                eof = true;
                memset(buffer_end, 0, 64); // clear last 64 bytes
                return;
            }
        }

        // copy toKeep data exactly before the data we'll read below
        memmove(buffer, toKeep, toKeepSize);
        buffer_end = buffer + toKeepSize;

        // inflate data
        inflator.avail_in = raw_end - raw_begin;
        inflator.next_in = raw_begin;

        inflator.avail_out = BUFF_SIZE_TOTAL - toKeepSize;
        inflator.next_out = (uint8_t *) buffer_end;

        int status = inflate(&inflator, Z_SYNC_FLUSH);
        assert(status == Z_OK || status == Z_STREAM_END);
        zlib_eos = status;

        // the difference between the original available size and the available size after the call is the size of written bytes
        buffer_end += (BUFF_SIZE_TOTAL - toKeepSize) - inflator.avail_out;

        // same for raw_begin
        raw_begin += (raw_end - raw_begin) - inflator.avail_in;
    }
};