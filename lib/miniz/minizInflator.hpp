#pragma once

#include <algorithm>
#include "miniz.h"

class MinizInflator {
    static constexpr size_t DICT_SIZE = 32768;

    tinfl_decompressor inflator{};
    uint8_t m_dict[DICT_SIZE]{};

    size_t m_dict_avail = 0;
    size_t m_dict_ofs = 0;

    enum gzip_flag_bits {
        FHCRC = 1,
        FEXTRA,
        FNAME,
        FCOMMENT
    };

    uint32_t len32 = 0; // len32 for footer verification

public:
    bool done = false;

    // takes in a memory address to the beginning of a raw gzip file
    // returns the size of the gzip header, in bytes
    static size_t parseGzipHeader(uint8_t *mem) {
        uint8_t *p = mem;

        p += 2; // p is now at compression type
        assert(*p == 0x08); //only support defalte compression

        p += 1; // p is now the flags byte
        uint8_t flags = *p;

        p += 7; // skip to after the end of the fixed header format

        if (flags & 1U << FEXTRA) {
            uint16_t extra_size;
            memcpy(&extra_size, p, 2);
            p += 2 + extra_size; // skip the 2 size bytes + the actual size
        }

        if (flags & 1U << FNAME) {
            while (*p) ++p; // skip to null character
            ++p; // now at next character after null
        }

        if (flags & 1U << FCOMMENT) {
            while (*p) ++p;  // skip to null character
            ++p; // now at next character after null
        }

        if (flags & 1U << FHCRC) {
            p += 2; // skip header CRC16
        }

        return p - mem;
    }

    bool verifyFooter(const uint8_t *footer) const {
        return *(uint32_t *) (footer + 4) == len32;
    }

    void inflate(uint8_t *next_in, size_t *available_in, char *next_out, size_t *available_out, bool needs_more_input) {
        // if it fits, write the decompressed data to destination
        if (m_dict_avail) {
            size_t n = std::min(m_dict_avail, *available_out);
            memcpy(next_out, m_dict + m_dict_ofs, n);
            next_out += n;
            len32 += n;
            *available_out -= n;

            m_dict_avail -= n;
            m_dict_ofs = (m_dict_ofs + n) & (DICT_SIZE - 1);

            if (*available_in == 0) done = true;
        }

        int status = -1;
        while (status != TINFL_STATUS_DONE && *available_in && *available_out && !m_dict_avail) {
            size_t out_bytes = DICT_SIZE - m_dict_ofs;
            size_t in_bytes = *available_in;

            status = tinfl_decompress(&inflator,
                                      next_in, &in_bytes,
                                      m_dict, m_dict + m_dict_ofs, &out_bytes,
                                      needs_more_input ? TINFL_FLAG_HAS_MORE_INPUT : 0);

            // increment input data
            next_in += in_bytes;
            *available_in -= in_bytes;

            m_dict_avail = out_bytes;

            // if it fits, write the decompressed data to destination
            size_t n = std::min(m_dict_avail, *available_out);
            memcpy(next_out, m_dict + m_dict_ofs, n);
            next_out += n;
            len32 += n;
            *available_out -= n;

            m_dict_avail -= n;
            m_dict_ofs = (m_dict_ofs + n) & (DICT_SIZE - 1);

            assert(status >= 0);
            if (status == TINFL_STATUS_DONE && !m_dict_avail) done = true;
        }
    }

    MinizInflator() { tinfl_init(&inflator); }
};