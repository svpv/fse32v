// Copyright (c) 2019 Alexey Tourbin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <stdint.h>

#ifdef __GNUC__
#pragma GCC visibility push(hidden)
#endif

// A separate bitstream writer (cannot use bitstream.h because
// it reads/writes in opposite directions).
struct MiniWriter {
    // Pointer to the next word to be written; initialize with a buffer;
    // after flush, points to the last/next byte written (i.e with fill=0,
    // points to the end, otherwise to the last byte with 1..7 bits).
    unsigned char *p;
    // The register that retains the bits.
    uint32_t reg;
    // How many bits are there in the register; after flush,
    // the number of bits filled in the last/next byte, 0..7.
    unsigned fill;
};

// Normalized count entry, describes an entropy-coded symbol.
struct NC {
    // The count of a symbol represents its probability.
    // The counts of all symbols must add up to (1 << tableLog).
    // Special value -1 means "very small" and counts as 1.
    // Furthermore entries with cnt=0 must be excluded from the table,
    // because no symbols are assigned to such entries.
    int16_t cnt;
    // Specifies the start of a range or a half-range.  The ranges
    // and half-ranges are: [0], [1], [2|3], [4-5|6-7], [8-11|12-15], ...
    // 32-bit numbers fall into 64 ranges, hence 0..63.
    uint8_t rangeStart;
    // How long the range spans: -1 - half-range, 0 - single range,
    // 1 - double range.  E.g. half-ranges [4-5|6-7] can be merged
    // into a single range [4-7], and further into a double range [4-11].
    // The span of a range is a power of two, because a value in the range
    // is encoded with a whole number of bits.
    int8_t rangeSpan;
};

// Serialize the table of normalized counts.
void TFSE_writeNCount(struct NC *nc, unsigned tableLog, struct MiniWriter *mw);

#ifdef __GNUC__
#pragma GCC visibility pop
#endif
