// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "WW3D2/RenderTexture.h"

namespace RenderTextures
{
// CPU footprints only. Native capability checks and DDS face/order conversion
// remain in their callers. A cube has six complete, equally sized faces.
enum class CubeFace { PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ };
struct CubeDesc { std::uint32_t edge; unsigned mip_levels; WW3DFormat format; };
struct VolumeDesc { std::uint32_t width, height, depth; unsigned mip_levels; WW3DFormat format; };
struct VolumeMipLayout
{
    MipLayout plane;
    std::uint32_t depth, slice_bytes, byte_count;
};

inline unsigned MaxMipLevels(std::uint32_t extent)
{
    unsigned levels = 0;
    for (; extent; extent >>= 1) ++levels;
    return levels;
}

inline std::uint32_t MipExtent(std::uint32_t extent, unsigned level)
{
    const auto shifted = level < 32 ? extent >> level : 0;
    return shifted ? shifted : 1;
}

// Canonical face-major subresource numbering, independent of native API.
inline bool TryCubeSubresource(const CubeDesc &desc, CubeFace face, unsigned level,
                               unsigned &out_index, MipLayout &out_layout)
{
    const auto face_index = static_cast<unsigned>(face);
    MipLayout layout;
    if (face_index >= 6 || !TryMipLayout({desc.edge,desc.edge,desc.mip_levels,desc.format},level,layout))
        return false;
    out_index = face_index * desc.mip_levels + level;
    out_layout = layout;
    return true;
}

inline bool TryVolumeMip(const VolumeDesc &desc, unsigned level, VolumeMipLayout &out)
{
    if (!desc.width || !desc.height || !desc.depth || !desc.mip_levels || level >= desc.mip_levels)
        return false;
    std::uint32_t extent = desc.width > desc.height ? desc.width : desc.height;
    if (desc.depth > extent) extent = desc.depth;
    if (desc.mip_levels > MaxMipLevels(extent)) return false;
    MipLayout plane;
    if (!TryMipLayout({MipExtent(desc.width,level),MipExtent(desc.height,level),1,desc.format},0,plane))
        return false;
    const auto depth = MipExtent(desc.depth,level);
    const std::uint64_t slice = std::uint64_t(plane.row_bytes) * plane.row_count;
    if (slice > UINT32_MAX || slice * depth > UINT32_MAX) return false;
    out = {plane,depth,static_cast<std::uint32_t>(slice),static_cast<std::uint32_t>(slice*depth)};
    return true;
}

// Each slice may have independent trailing padding; the final slice/row need
// only contain useful data. A zero slice pitch is not a whole-resource sentinel.
inline bool TryVolumeSpan(const VolumeMipLayout &layout, std::uint32_t row_pitch,
                          std::uint32_t slice_pitch, std::uint32_t available_bytes, std::uint32_t &out)
{
    std::uint32_t plane_span;
    if (!layout.depth || !TryRowSpan(layout.plane,row_pitch,UINT32_MAX,plane_span) ||
        slice_pitch < plane_span) return false;
    const std::uint64_t span = std::uint64_t(layout.depth-1)*slice_pitch + plane_span;
    if (span > available_bytes) return false;
    out = static_cast<std::uint32_t>(span);
    return true;
}

// Offset in a tightly packed canonical face-major chain, not a promise
// about an arbitrary file's header or native memory layout.
inline bool TryPackedCube(const CubeDesc &desc, CubeFace face, unsigned level,
                          std::uint32_t &offset, std::uint32_t &total)
{
    unsigned index;
    MipLayout layout;
    if (!TryCubeSubresource(desc,face,level,index,layout)) return false;
    std::uint64_t face_bytes = 0, prefix = 0;
    for (unsigned i=0;i<desc.mip_levels;++i)
    {
        if (!TryMipLayout({desc.edge,desc.edge,desc.mip_levels,desc.format},i,layout)) return false;
        if (i==level) prefix=face_bytes;
        face_bytes += std::uint64_t(layout.row_bytes)*layout.row_count;
        if (face_bytes > UINT32_MAX/6u) return false;
    }
    offset = static_cast<std::uint32_t>((index/desc.mip_levels)*face_bytes+prefix);
    total = static_cast<std::uint32_t>(face_bytes*6);
    return true;
}

inline bool TryPackedVolume(const VolumeDesc &desc, unsigned level,
                            std::uint32_t &offset, std::uint32_t &total)
{
    VolumeMipLayout layout;
    if (!TryVolumeMip(desc,level,layout)) return false;
    std::uint64_t bytes=0,prefix=0;
    for (unsigned i=0;i<desc.mip_levels;++i)
    {
        if (!TryVolumeMip(desc,i,layout)) return false;
        if (i==level) prefix=bytes;
        bytes += layout.byte_count;
        if (bytes>UINT32_MAX) return false;
    }
    offset=static_cast<std::uint32_t>(prefix);
    total=static_cast<std::uint32_t>(bytes);
    return true;
}
}
