// SPDX-License-Identifier: GPL-3.0-or-later

#include "WW3D2/RenderTexture.h"

namespace RenderTextures
{
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
}
