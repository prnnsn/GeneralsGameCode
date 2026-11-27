// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "WW3D2/dx8fvf.h"
#include "WW3D2/RenderBuffer.h"

// The native FVF interpretation is isolated here. Nothing in the neutral
// descriptor depends on Direct3D. These helpers never change a live FVF.
namespace DX8VertexLayout
{
using namespace RenderBuffers;

inline bool Append(VertexLayout & layout, Semantic semantic, unsigned index, ElementType type)
{
    if (layout.count >= MaxAttributes)
        return false;
    const Attribute a = { semantic, index, type, layout.stride_bytes };
    layout.attributes[layout.count++] = a;
    layout.stride_bytes += ElementSize(type);
    return true;
}

// Interpret the native declaration, not FVFInfoClass's legacy offset algorithm.
// A caller must supply an independently verified producer stride. Blend and
// shader-specific declarations are deliberately not converted in this stage.
inline bool TryDecode(unsigned fvf, std::uint32_t producer_stride, VertexLayout & out)
{
    const unsigned position = fvf & D3DFVF_POSITION_MASK;
    const unsigned tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    if ((position != D3DFVF_XYZ && position != D3DFVF_XYZRHW) || tex_count > 8 ||
        (position == D3DFVF_XYZRHW && (fvf & D3DFVF_NORMAL)))
        return false;
    unsigned allowed = D3DFVF_POSITION_MASK | D3DFVF_NORMAL | D3DFVF_PSIZE |
                       D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEXCOUNT_MASK;
    VertexLayout layout = {};
    if (!Append(layout, position == D3DFVF_XYZ ? Semantic::Position : Semantic::TransformedPosition,
                0, position == D3DFVF_XYZ ? ElementType::Float3 : ElementType::Float4))
        return false;
    if ((fvf & D3DFVF_NORMAL) && !Append(layout, Semantic::Normal, 0, ElementType::Float3))
        return false;
    if ((fvf & D3DFVF_PSIZE) && !Append(layout, Semantic::PointSize, 0, ElementType::Float1))
        return false;
    if ((fvf & D3DFVF_DIFFUSE) && !Append(layout, Semantic::Diffuse, 0, ElementType::PackedArgb8))
        return false;
    if ((fvf & D3DFVF_SPECULAR) && !Append(layout, Semantic::Specular, 0, ElementType::PackedArgb8))
        return false;
    for (unsigned i = 0; i < tex_count; ++i)
    {
        const unsigned shift = 16 + 2 * i;
        allowed |= 3u << shift;
        // D3DFVF_TEXCOORDSIZE encodes 2,3,4,1 floats as 0,1,2,3.
        const ElementType types[] = { ElementType::Float2, ElementType::Float3, ElementType::Float4, ElementType::Float1 };
        if (!Append(layout, Semantic::TexCoord, i, types[(fvf >> shift) & 3u]))
            return false;
    }
    if ((fvf & ~allowed) || layout.stride_bytes != producer_stride || !ValidLayout(layout))
        return false;
    out = layout;
    return true;
}

// W3D consumers may rely on historical metadata, including mutable FVF/size.
// Check its actual values independently; never silently reinterpret a mismatch.
// Producer offsets must ALSO be checked by the migrating consumer.
inline bool TryFromLegacy(const FVFInfoClass & info, std::uint32_t producer_stride, VertexLayout & out)
{
    VertexLayout layout;
    if (info.Get_FVF_Size() != producer_stride || !TryDecode(info.Get_FVF(), producer_stride, layout))
        return false;
    for (unsigned i = 0; i < layout.count; ++i)
    {
        const Attribute & a = layout.attributes[i];
        unsigned offset;
        switch (a.semantic)
        {
        case Semantic::Position: offset = info.Get_Location_Offset(); break;
        case Semantic::Normal: offset = info.Get_Normal_Offset(); break;
        case Semantic::Diffuse: offset = info.Get_Diffuse_Offset(); break;
        case Semantic::Specular: offset = info.Get_Specular_Offset(); break;
        case Semantic::TexCoord: offset = info.Get_Tex_Offset(a.semantic_index); break;
        // Legacy offsets for RHW, point size and blends are not established.
        default: return false;
        }
        if (offset != a.byte_offset)
            return false;
    }
    out = layout;
    return true;
}
} // namespace DX8VertexLayout
