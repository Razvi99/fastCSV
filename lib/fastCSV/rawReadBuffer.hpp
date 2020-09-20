#pragma once

#include <string_view>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

class RawReadBuffer {
private:
    int fd = -1;

public:
    static constexpr size_t MAX_LINE_SIZE = 25000; // in characters

    static constexpr size_t BUFF_SIZE_MB = 1;
    static constexpr size_t BUFF_SIZE_NO_PADDING = BUFF_SIZE_MB * (1U << 20U);

    char buffer[BUFF_SIZE_NO_PADDING + MAX_LINE_SIZE]{};

public:
    char *buffer_start = buffer + MAX_LINE_SIZE;
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
        assert(toKeepSize < MAX_LINE_SIZE);

        // copy toKeep data exactly before the data we'll read below
        memcpy(buffer + MAX_LINE_SIZE - toKeepSize, toKeep, toKeepSize);

        int readSize = read(fd, buffer + MAX_LINE_SIZE, BUFF_SIZE_NO_PADDING);
        assert(readSize != -1);

        if (readSize == 0) eof = true;

        // update start and end pointers
        buffer_start = buffer + MAX_LINE_SIZE - toKeepSize;
        buffer_end = buffer_start + readSize + toKeepSize;
    }
};