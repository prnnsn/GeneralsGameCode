// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "WW3D2/ww3dformat.h"

namespace RenderTextures
{
// Ordinary alpha-bearing color formats leave association to their producer.
enum class Alpha { None, Unspecified, OptionalOneBit, Straight, Premultiplied };

struct FormatLayout
{
    unsigned block_width;
    unsigned block_height;
    unsigned bytes_per_block;
    Alpha alpha;
};

// The number of stored levels is explicit. Resolve any loader sentinel before
// constructing the descriptor.
struct Texture2DDesc
{
    std::uint32_t width;
    std::uint32_t height;
    unsigned mip_levels;
    WW3DFormat format;
};

struct MipLayout
{
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t row_bytes;
    std::uint32_t row_count;
};

// Layout describes CPU data. Native dimensions and capabilities are validated
// by the allocation path. All output parameters retain their value on failure.
bool TryFormatLayout(WW3DFormat format, FormatLayout & out);
bool TryMipLayout(const Texture2DDesc & desc, unsigned level, MipLayout & out);

// Measure the span through the last useful byte, including the padding between
// rows. Trailing padding after the last row is optional.
bool TryRowSpan(const MipLayout & layout, std::uint32_t row_pitch,
                std::uint32_t available_bytes, std::uint32_t & out);
}
