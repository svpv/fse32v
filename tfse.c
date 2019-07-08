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

#include <string.h>
#include <assert.h>
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
