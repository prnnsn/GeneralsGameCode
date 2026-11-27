// SPDX-License-Identifier: GPL-3.0-or-later

#include "WW3D2/RenderTexture.h"

#include <cassert>
#include <climits>
#include <cstring>

namespace RenderTextures
{
bool PackedDDSData::CanCopyTo(unsigned char * const * surfaces, const unsigned * pitches) const
{
    if (!bytes || !mip_count || mip_count > mips.size())
        return false;

    for (unsigned level = 0; level < mip_count; ++level)
    {
        const auto & mip = mips[level];
        std::uint32_t span;
        if (!surfaces[level] || pitches[level] > INT_MAX ||
            !TryRowSpan(mip.layout, pitches[level], INT_MAX, span) ||
            mip.offset > byte_count ||
            !TryRowSpan(mip.layout, mip.layout.row_bytes, byte_count - mip.offset, span))
            return false;
    }
    return true;
}

void PackedDDSData::CopyTo(unsigned char * const * surfaces, const unsigned * pitches) const
{
    assert(CanCopyTo(surfaces, pitches));

    for (unsigned level = 0; level < mip_count; ++level)
    {
        const auto & mip = mips[level];
        const auto * source = bytes.get() + mip.offset;
        auto * dest = surfaces[level];
        const unsigned pitch = pitches[level];
        if (pitch == mip.layout.row_bytes)
        {
            std::memcpy(dest, source, mip.layout.row_count * mip.layout.row_bytes);
            continue;
        }

        for (unsigned row = 0; row < mip.layout.row_count; ++row)
            std::memcpy(dest + row * pitch, source + row * mip.layout.row_bytes,
                        mip.layout.row_bytes);
    }
}

bool TryFormatLayout(WW3DFormat format, FormatLayout & out)
{
    switch (format)
    {
    case WW3D_FORMAT_DXT1:
        out = { 4, 4, 8, Alpha::OptionalOneBit };
        return true;
    case WW3D_FORMAT_DXT2:
    case WW3D_FORMAT_DXT4:
        out = { 4, 4, 16, Alpha::Premultiplied };
        return true;
    case WW3D_FORMAT_DXT3:
    case WW3D_FORMAT_DXT5:
        out = { 4, 4, 16, Alpha::Straight };
        return true;
    case WW3D_FORMAT_P8:
    case WW3D_FORMAT_A8P8:
        // The data includes a palette dependency outside this descriptor.
        return false;
    default:
        if (format < WW3D_FORMAT_R8G8B8 || format > WW3D_FORMAT_X8L8V8U8)
            return false;
        out = { 1, 1, Get_Bytes_Per_Pixel(format), Has_Alpha(format) ? Alpha::Unspecified : Alpha::None };
        return true;
    }
}

bool TryMipLayout(const Texture2DDesc & desc, unsigned level, MipLayout & out)
{
    if (!desc.width || !desc.height || level >= desc.mip_levels)
        return false;

    unsigned max_levels = 1;
    for (std::uint32_t extent = desc.width > desc.height ? desc.width : desc.height; extent > 1; extent >>= 1)
        ++max_levels;
    if (desc.mip_levels > max_levels)
        return false;

    FormatLayout format;
    if (!TryFormatLayout(desc.format, format))
        return false;

    const std::uint32_t shifted_width = desc.width >> level;
    const std::uint32_t shifted_height = desc.height >> level;
    const std::uint32_t width = shifted_width ? shifted_width : 1;
    const std::uint32_t height = shifted_height ? shifted_height : 1;
    const std::uint64_t columns = (std::uint64_t(width) + format.block_width - 1) / format.block_width;
    const std::uint64_t rows = (std::uint64_t(height) + format.block_height - 1) / format.block_height;
    const std::uint64_t row_bytes = columns * format.bytes_per_block;
    if (row_bytes > UINT32_MAX || row_bytes * rows > UINT32_MAX)
        return false;

    out = { width, height, static_cast<std::uint32_t>(row_bytes), static_cast<std::uint32_t>(rows) };
    return true;
}

bool TryPackedDDSMips(const Texture2DDesc & desc, std::uint32_t payload_bytes,
                      PackedMipChain & out)
{
    FormatLayout format;
    if (!TryFormatLayout(desc.format, format) || format.block_width != 4 ||
        !desc.mip_levels || desc.mip_levels > out.size())
        return false;

    PackedMipChain chain = {};
    std::uint64_t offset = 0;
    for (unsigned level = 0; level < desc.mip_levels; ++level)
    {
        MipLayout mip;
        if (!TryMipLayout(desc, level, mip))
            return false;

        chain[level] = { mip, static_cast<std::uint32_t>(offset) };
        offset += std::uint64_t(mip.row_bytes) * mip.row_count;
        if (offset > payload_bytes)
            return false;
    }

    if (offset != payload_bytes)
        return false;

    out = chain;
    return true;
}

bool TryRowSpan(const MipLayout & layout, std::uint32_t row_pitch, std::uint32_t available_bytes,
                std::uint32_t & out)
{
    if (!layout.row_count || !layout.row_bytes || row_pitch < layout.row_bytes)
        return false;
    const std::uint64_t span = std::uint64_t(layout.row_count - 1) * row_pitch + layout.row_bytes;
    if (span > available_bytes)
        return false;
    out = static_cast<std::uint32_t>(span);
    return true;
}

bool TryClearRows(const MipLayout & layout, unsigned char * data,
                  std::uint32_t row_pitch, std::uint32_t available_bytes)
{
    std::uint32_t span;
    if (!data || !TryRowSpan(layout, row_pitch, available_bytes, span))
        return false;

    for (std::uint32_t row = 0; row < layout.row_count; ++row)
        std::memset(data + row * row_pitch, 0, layout.row_bytes);
    return true;
}

bool TryFillPixelRect(const SurfaceWriteView & view, unsigned left, unsigned top,
                      unsigned right, unsigned bottom, std::uint32_t value)
{
    if (!view.data || !view.pixel_bytes || view.pixel_bytes > sizeof(value) ||
        left > right || top > bottom || right >= view.layout.width || bottom >= view.layout.height)
        return false;

    const std::uint64_t row_end = (std::uint64_t(right) + 1) * view.pixel_bytes;
    if (row_end > view.layout.row_bytes || row_end > view.pitch ||
        std::uint64_t(bottom) * view.pitch + row_end > view.byte_count)
        return false;

    for (unsigned y = top; y <= bottom; ++y)
    {
        auto * dest = view.data + y * view.pitch + left * view.pixel_bytes;
        for (unsigned x = left; x <= right; ++x, dest += view.pixel_bytes)
            std::memcpy(dest, &value, view.pixel_bytes);
    }
    return true;
}
}
