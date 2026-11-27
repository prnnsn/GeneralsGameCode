// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace TerrainAtlas
{
// All coordinates are in pixels; pitch is in bytes. The last row need not
// include its padding. Reject an entire rectangle before touching pixels.
inline bool Fits(unsigned width, unsigned height, int x, int y, std::uint64_t extent, unsigned border)
{
    return x >= 0 && y >= 0 && unsigned(x) >= border && unsigned(y) >= border &&
        extent && extent <= width && extent <= height &&
        std::uint64_t(unsigned(x)) + extent + border <= width &&
        std::uint64_t(unsigned(y)) + extent + border <= height;
}

inline bool ValidView(unsigned char* data, unsigned width, unsigned height, unsigned pitch, unsigned bytes)
{
    if (!data || !width || !height || std::uint64_t(width)*2 > pitch) return false;
    return std::uint64_t(height-1)*pitch + std::uint64_t(width)*2 <= bytes;
}

// Source is tightly packed BGRA, bottom-up. Caller prevalidates all atlas
// positions before the first write, then uses these checked helpers.
inline bool WriteTile(unsigned char* data, unsigned width, unsigned height, unsigned pitch,
                      unsigned bytes, int x, int y, unsigned extent, const unsigned char* source,
                      std::size_t source_bytes)
{
    if (!ValidView(data,width,height,pitch,bytes) || !Fits(width,height,x,y,extent,0) ||
        !source || !extent || extent > source_bytes/4/extent) return false;
    for (unsigned j=0; j<extent; ++j)
    {
        auto* dst = data + std::size_t(unsigned(y)+j)*pitch + std::size_t(unsigned(x))*2;
        const auto* src = source + std::size_t(extent-1-j)*extent*4;
        for (unsigned i=0; i<extent; ++i, src+=4, dst+=2)
        {
            const std::uint16_t color = 0x8000u | ((src[2]>>3)<<10) | ((src[1]>>3)<<5) | (src[0]>>3);
            std::memcpy(dst,&color,2);
        }
    }
    return true;
}

inline bool WrapBorder(unsigned char* data, unsigned width, unsigned height, unsigned pitch,
                       unsigned bytes, int x, int y, unsigned extent)
{
    if (!ValidView(data,width,height,pitch,bytes) || extent < 4 ||
        !Fits(width,height,x,y,extent,4)) return false;
    auto row = [&](unsigned yy) { return data + std::size_t(yy)*pitch + std::size_t(unsigned(x)-4)*2; };
    for (unsigned j=0;j<extent;++j)
    {
        auto* p = row(unsigned(y)+j);
        std::memcpy(p, p+std::size_t(extent)*2, 8);
        std::memcpy(p+std::size_t(extent+4)*2, p+8, 8);
    }
    for (unsigned j=0;j<4;++j)
    {
        std::memcpy(row(unsigned(y)-j-1), row(unsigned(y)+extent-j-1), std::size_t(extent+8)*2);
        std::memcpy(row(unsigned(y)+extent+j), row(unsigned(y)+j), std::size_t(extent+8)*2);
    }
    return true;
}
}
