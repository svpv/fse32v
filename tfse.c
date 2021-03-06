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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "tfse.h"

#ifdef TFSE_DEBUG
#define TFSE_assert(x) assert(x)
#else
#define TFSE_assert(x) ((void)0)
#endif

#ifndef htole32
#if defined(__GNUC__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#define htole32(x) __builtin_bswap32(x)
#else
#define htole32(x) (x)
#endif
#endif

static inline unsigned highbit32(int32_t x)
{
#ifdef __LZCNT__
    return 31 - __builtin_clz(x);
#endif
    return 31 ^ __builtin_clz(x);
}

static inline void MiniWriter_addBits(struct MiniWriter *mw, uint32_t val, unsigned nbBits)
{
    // Shift by mw->fill must be valid.
    TFSE_assert(mw->fill < sizeof mw->reg * 8);
    // The value must be clean.
    TFSE_assert(val < (UINT32_C(1) << nbBits));

    mw->reg |= val << mw->fill;
    mw->fill += nbBits;
}

static inline void MiniWriter_flushBits(struct MiniWriter *mw)
{
    // The result must not overflow.
    TFSE_assert(mw->fill <= sizeof mw->reg * 8);

    uint32_t x = htole32(mw->reg);
    static_assert(sizeof mw->reg == sizeof x, "");
    memcpy(mw->p, &x, sizeof x);
    mw->p += mw->fill / 8;
    mw->fill &= 7;
}

void TFSE_writeNCount(struct NC *nc, unsigned tableLog, struct MiniWriter *mw)
{
    TFSE_assert(tableLog >= 5 && tableLog <= 8);
    MiniWriter_addBits(mw, tableLog - 5, 3);

    int maxCnt = INT_MIN;
    int needCover = 1 << tableLog;
    for (size_t i = 0; needCover > 0; i++) {
	maxCnt = (nc[i].cnt > maxCnt) ? nc[i].cnt : maxCnt;
	needCover -= abs(nc[i].cnt);
    }
    TFSE_assert(needCover == 0);
    TFSE_assert(maxCnt > 0);

    // Each value can be encoded with k0 bits.
    unsigned k0 = highbit32(maxCnt + 1) + 1;
    if (k0 == tableLog - 1)
	MiniWriter_addBits(mw, 1, 1);
    else if (k0 > tableLog - 1) {
	k0 = tableLog + 1;
	MiniWriter_addBits(mw, 0, 2);
    }
    else {
	k0 = tableLog - 2;
	MiniWriter_addBits(mw, 2, 2);
    }
    // Further set up a threshold, the maximum value encoded with k0 bits.
    int n0 = (1 << k0) - 1;

    needCover = 1 << tableLog;
    for (size_t i = 0; needCover > 0; i++) {
	// The value we're about to write.
	int x = nc[i].cnt + 1;
	// The maximum possible value.
	int n = needCover + 1;
	// If the maximum possible value is somewhat large,
	// the best we can do is to encode it with k0 bits.
	unsigned k = k0;
	// If the expected value is small, it can be written
	// in even fewer than k0 bits.
	if (n < n0) {
	    // In truncated binary, the value can be written with either k or k+1 bits.
	    k = highbit32(n);
	    // Variables as in en.wikipedia.org/wiki/Truncated_binary_encoding
	    int u = (1 << (k + 1)) - n;
	    if (x >= u)
		x += u, k++;
	    TFSE_assert(k <= k0);
	}
	MiniWriter_addBits(mw, x, k);
	MiniWriter_flushBits(mw);
	needCover -= abs(nc[i].cnt);
    }
}
