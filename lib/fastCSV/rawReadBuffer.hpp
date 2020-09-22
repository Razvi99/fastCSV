#pragma once

#include <string_view>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

class RawReadBuffer {
private:
    int fd = -1;

public:
    static constexpr size_t BUFF_SIZE_MB = 1;
    static constexpr size_t BUFF_SIZE_TOTAL = BUFF_SIZE_MB * (1U << 20U);

    char buffer[BUFF_SIZE_TOTAL]{};

public:
    char *buffer_begin = buffer;
    char *buffer_end = buffer;

    bool eof = false;

    // open file when object is created
    explicit RawReadBuffer(const char *path) {
        fd = open(path, O_RDONLY);
        assert(fd != -1);

        readMore(nullptr, 0);
    }

    // close the file when this object is deleted
    ~RawReadBuffer() {
        assert(close(fd) == 0);
    }

    // read bytes from file, and write to buffer + starting_from
    // sets eof = true when there are no more bytes to be read
    void readMore(char *toKeep, size_t toKeepSize) {
        // copy toKeep data exactly before the data we'll read below
        memmove(buffer, toKeep, toKeepSize);
        buffer_end = buffer + toKeepSize;

        int readSize = read(fd, buffer_end, BUFF_SIZE_TOTAL - toKeepSize);
        assert(readSize != -1);

        if (readSize == 0) eof = true;

        buffer_end += readSize;
    }
};