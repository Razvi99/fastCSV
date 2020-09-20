#pragma once

#include <string_view>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>

class RawReadBuffer {
private:
    int fd = -1;

public:
    static constexpr size_t BUFF_SIZE_MB = 1;
    static constexpr size_t BUFF_SIZE_TOTAL = BUFF_SIZE_MB * (1U << 20U);

    char buffer[BUFF_SIZE_TOTAL]{};
    char *buffer_end{};

    bool eof = false;

    // open file when object is created
    explicit RawReadBuffer(const char *path) {
        fd = open(path, O_RDONLY);
        assert(fd != -1);

        readMore();
    }

    // close the file when this object is deleted
    ~RawReadBuffer() {
        assert(close(fd) == 0);
    }

    // read bytes from file, and write to buffer + starting_from
    // sets eof = true when there are no more bytes to be read
    void readMore(int starting_from = 0) {
        int result = read(fd, buffer + starting_from, BUFF_SIZE_TOTAL - starting_from);
        assert(result != -1);
        buffer_end = buffer + starting_from + result;
        if (result == 0) eof = true;
    }
};