// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <memory>

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

struct PackedMip
{
    MipLayout layout;
    std::uint32_t offset;
};

// A 32-bit texture dimension permits at most 32 mip levels.
using PackedMipChain = std::array<PackedMip, 32>;

// Owns the selected mips between file I/O and the render-thread copy.
// Offsets are relative to the first byte read, with no native surfaces retained.
struct PackedDDSData
{
    std::unique_ptr<unsigned char[]> bytes;
    PackedMipChain mips = {};
    unsigned byte_count = 0;
    unsigned mip_count = 0;

    bool CanCopyTo(unsigned char * const * surfaces, const unsigned * pitches) const;
    // The same surfaces and pitches must have passed CanCopyTo and remain locked.
    void CopyTo(unsigned char * const * surfaces, const unsigned * pitches) const;
};

// Layout describes CPU data. Native dimensions and capabilities are validated
// by the allocation path. All output parameters retain their value on failure.
bool TryFormatLayout(WW3DFormat format, FormatLayout & out);
bool TryMipLayout(const Texture2DDesc & desc, unsigned level, MipLayout & out);

// Compute offsets from the start of a legacy DXT DDS payload. The stored chain
// must fit exactly, including levels the renderer chooses to skip.
bool TryPackedDDSMips(const Texture2DDesc & desc, std::uint32_t payload_bytes,
                      PackedMipChain & out);

// Measure the span through the last useful byte, including the padding between
// rows. Trailing padding after the last row is optional.
bool TryRowSpan(const MipLayout & layout, std::uint32_t row_pitch,
                std::uint32_t available_bytes, std::uint32_t & out);
}
