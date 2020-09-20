#pragma once

#include <algorithm>
#include "miniz.h"

class MinizInflator {
    static constexpr size_t DICT_SIZE = 32768;

    tinfl_decompressor inflator{};
    uint8_t m_dict[DICT_SIZE]{};

    size_t m_dict_avail = 0;
    size_t m_dict_ofs = 0;

public:
    bool done = false;

    void inflate(uint8_t *next_in, size_t *available_in, char *next_out, size_t *available_out, bool needs_more_input) {
        // if it fits, write the decompressed data to destination
        if (m_dict_avail) {
            size_t n = std::min(m_dict_avail, *available_out);
            memcpy(next_out, m_dict + m_dict_ofs, n);
            next_out += n;
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
            *available_out -= n;

            m_dict_avail -= n;
            m_dict_ofs = (m_dict_ofs + n) & (DICT_SIZE - 1);

            assert(status >= 0);
            if (status == TINFL_STATUS_DONE && !m_dict_avail) done = true;
        }
    }

    MinizInflator() { tinfl_init(&inflator); }
};