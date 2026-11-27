// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "WW3D2/RenderTexture.h"

namespace RenderTargets
{
// Logical descriptors only. Preserve all legacy depth/stencil identities.
// Backend capability and sample-quality checks remain separate.
struct DepthLayout { unsigned depth_bits, stencil_bits, storage_bytes; bool lockable; };
inline bool TryDepthLayout(WW3DZFormat format, DepthLayout &out)
{
    switch (format)
    {
    case WW3D_ZFORMAT_D16_LOCKABLE: out={16,0,2,true}; return true;
    case WW3D_ZFORMAT_D32: out={32,0,4,false}; return true;
    case WW3D_ZFORMAT_D15S1: out={15,1,2,false}; return true;
    case WW3D_ZFORMAT_D24S8: out={24,8,4,false}; return true;
    case WW3D_ZFORMAT_D16: out={16,0,2,false}; return true;
    case WW3D_ZFORMAT_D24X8: out={24,0,4,false}; return true;
    case WW3D_ZFORMAT_D24X4S4: out={24,4,4,false}; return true;
    default: return false;
    }
}

struct ColorDesc { std::uint32_t width,height; WW3DFormat format; unsigned samples; };
struct DepthDesc { std::uint32_t width,height; WW3DZFormat format; unsigned samples; };

// Initial compatibility seam supports only explicit single-sample targets.
// Multisampling needs a separate native capability/resolve contract.
inline bool TryColorFootprint(const ColorDesc &desc,std::uint32_t &bytes)
{
    RenderTextures::FormatLayout format;
    RenderTextures::MipLayout mip;
    if (desc.samples!=1 || !RenderTextures::TryFormatLayout(desc.format,format) ||
        format.block_width!=1 || format.block_height!=1 ||
        !RenderTextures::TryMipLayout({desc.width,desc.height,1,desc.format},0,mip))
        return false;
    bytes=mip.row_bytes*mip.row_count;
    return true;
}
inline bool TryDepthFootprint(const DepthDesc &desc,std::uint32_t &bytes)
{
    DepthLayout format;
    if (!desc.width || !desc.height || desc.samples!=1 || !TryDepthLayout(desc.format,format))
        return false;
    const std::uint64_t pixels=std::uint64_t(desc.width)*desc.height;
    if (pixels>UINT32_MAX/format.storage_bytes) return false;
    bytes=static_cast<std::uint32_t>(pixels*format.storage_bytes);
    return true;
}

// DX8 may use a larger depth surface than the color target; preserve that
// option and leave backend-specific restrictions to native validation.
inline bool Compatible(const ColorDesc &color,const DepthDesc &depth,unsigned min_stencil_bits=0)
{
    std::uint32_t bytes;
    DepthLayout format;
    return TryColorFootprint(color,bytes) && TryDepthFootprint(depth,bytes) &&
        TryDepthLayout(depth.format,format) && depth.width>=color.width &&
        depth.height>=color.height && color.samples==depth.samples &&
        format.stencil_bits>=min_stencil_bits;
}

struct Viewport { unsigned x,y,width,height; float min_depth,max_depth; };
inline bool ValidViewport(const ColorDesc &target,const Viewport &vp)
{
    std::uint32_t bytes;
    return TryColorFootprint(target,bytes) && vp.width && vp.height &&
        vp.x<target.width && vp.y<target.height &&
        vp.width<=target.width-vp.x && vp.height<=target.height-vp.y &&
        vp.min_depth>=0.0f && vp.max_depth<=1.0f && vp.min_depth<=vp.max_depth;
}
}
