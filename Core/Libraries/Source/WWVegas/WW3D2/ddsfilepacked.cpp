// SPDX-License-Identifier: GPL-3.0-or-later

#include <climits>
#include <cstring>
#include <memory>

#include "ddsfile.h"
#include "RenderTexture.h"
#include "WWLib/ffactory.h"

DDSFileClass::PackedLoadResult DDSFileClass::Try_Load_Packed_2D(
    WW3DFormat dest_format, unsigned width, unsigned height, unsigned mip_count,
    unsigned char * const * dest_surfaces, const unsigned * dest_pitches) const
{
    constexpr unsigned FourCCFlag = 0x4;
    constexpr int HeaderBytes = 128;
    constexpr int MaxSurfaceBytes = INT_MAX;

    if (!Is_Available() || Type != DDS_TEXTURE || dest_format != Format ||
        !mip_count || mip_count > MipLevels || width != Get_Width(0) || height != Get_Height(0) ||
        SurfaceDesc.Size != HeaderBytes - 4 || SurfaceDesc.PixelFormat.Size != 32 ||
        !(SurfaceDesc.PixelFormat.Flags & FourCCFlag) || SurfaceDesc.Caps.Caps2 != 0 ||
        SurfaceDesc.Depth > 1)
        return PackedLoadResult::Ineligible;

    RenderTextures::FormatLayout format;
    if (!RenderTextures::TryFormatLayout(Format, format) || format.block_width != 4)
        return PackedLoadResult::Ineligible;

    const unsigned stored_levels = SurfaceDesc.MipMapCount ? SurfaceDesc.MipMapCount : 1;
    if (ReductionFactor >= stored_levels || mip_count > stored_levels - ReductionFactor)
        return PackedLoadResult::Ineligible;

    file_auto_ptr file(_TheFileFactory, Name);
    if (!file->Open())
        return PackedLoadResult::Failed;

    const int file_bytes = file->Size();
    char magic[4];
    if (file_bytes <= HeaderBytes || file->Read(magic, sizeof(magic)) != sizeof(magic) ||
        std::memcmp(magic, "DDS ", sizeof(magic)) != 0)
        return PackedLoadResult::Ineligible;

    const unsigned payload_bytes = file_bytes - HeaderBytes;
    RenderTextures::PackedMipChain chain;
    if (!RenderTextures::TryPackedDDSMips({ FullWidth, FullHeight, stored_levels, Format },
                                          payload_bytes, chain))
        return PackedLoadResult::Ineligible;

    // Check every locked mip before writing any of them.
    for (unsigned level = 0; level < mip_count; ++level)
    {
        const auto & mip = chain[ReductionFactor + level].layout;
        std::uint32_t span;
        if (!dest_surfaces[level] || dest_pitches[level] > MaxSurfaceBytes ||
            !RenderTextures::TryRowSpan(mip, dest_pitches[level], MaxSurfaceBytes, span))
            return PackedLoadResult::Failed;
    }

    const unsigned first_offset = chain[ReductionFactor].offset;
    const unsigned after_last = ReductionFactor + mip_count;
    const unsigned end_offset = after_last < stored_levels ? chain[after_last].offset : payload_bytes;
    const int read_offset = HeaderBytes + first_offset;
    const int read_bytes = end_offset - first_offset;
    if (file->Seek(read_offset, SEEK_SET) != read_offset)
        return PackedLoadResult::Failed;

    std::unique_ptr<unsigned char[]> packed(W3DNEWARRAY unsigned char[read_bytes]);
    if (file->Read(packed.get(), read_bytes) != read_bytes)
        return PackedLoadResult::Failed;
    file->Close();

    for (unsigned level = 0; level < mip_count; ++level)
    {
        const auto & mip = chain[ReductionFactor + level];
        const auto * source = packed.get() + (mip.offset - first_offset);
        auto * dest = dest_surfaces[level];
        const unsigned dest_pitch = dest_pitches[level];
        if (dest_pitch == mip.layout.row_bytes)
        {
            std::memcpy(dest, source, mip.layout.row_count * mip.layout.row_bytes);
            continue;
        }

        for (unsigned row = 0; row < mip.layout.row_count; ++row)
            std::memcpy(dest + row * dest_pitch, source + row * mip.layout.row_bytes,
                        mip.layout.row_bytes);
    }

    return PackedLoadResult::Loaded;
}
