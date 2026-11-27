// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

// Value descriptions only. No native API, allocation, mapping or ownership.
namespace RenderBuffers
{
constexpr std::uint32_t MaxElements = 65535;
constexpr unsigned MaxAttributes = 16;

enum class Role { Vertex, Index };
enum class Storage { Gpu, CpuSorting };
enum class Update { Immutable, Rewritable, Streaming };
enum class Recreation { RetainSource, Regenerate, Discard };
enum class IndexFormat { Absent, UInt16 };
enum class Semantic { Position, TransformedPosition, Normal, PointSize, Diffuse, Specular, TexCoord, BlendWeight, BlendIndices };
// PackedArgb8: numerical 0xAARRGGBB; little-endian bytes B,G,R,A; normalized color.
enum class ElementType { Float1, Float2, Float3, Float4, PackedArgb8, UByte4 };

struct Attribute
{
    Semantic semantic;
    unsigned semantic_index;
    ElementType type;
    std::uint32_t byte_offset;
};

struct VertexLayout
{
    std::uint32_t stride_bytes;
    unsigned count;
    Attribute attributes[MaxAttributes];
};

struct BufferDesc
{
    Role role;
    Storage storage;
    Update update;
    Recreation recreation;
    std::uint32_t element_count;
    std::uint32_t stride_bytes;
    IndexFormat index_format;
    VertexLayout layout;
};

struct ByteRange
{
    std::uint32_t offset;
    std::uint32_t length;
};

inline unsigned ElementSize(ElementType type)
{
    switch (type)
    {
    case ElementType::Float1:
    case ElementType::PackedArgb8:
    case ElementType::UByte4: return 4;
    case ElementType::Float2: return 8;
    case ElementType::Float3: return 12;
    case ElementType::Float4: return 16;
    }
    return 0;
}

inline bool IsFloat(ElementType type)
{
    return type == ElementType::Float1 || type == ElementType::Float2 ||
           type == ElementType::Float3 || type == ElementType::Float4;
}

inline bool ValidAttribute(const Attribute & a)
{
    switch (a.semantic)
    {
    case Semantic::Position: return a.semantic_index == 0 && a.type == ElementType::Float3;
    case Semantic::TransformedPosition: return a.semantic_index == 0 && a.type == ElementType::Float4;
    case Semantic::Normal: return a.semantic_index == 0 && a.type == ElementType::Float3;
    case Semantic::PointSize: return a.semantic_index == 0 && a.type == ElementType::Float1;
    case Semantic::Diffuse:
    case Semantic::Specular: return a.semantic_index == 0 && a.type == ElementType::PackedArgb8;
    case Semantic::TexCoord: return a.semantic_index < 8 && IsFloat(a.type);
    case Semantic::BlendWeight: return a.semantic_index < 2 && IsFloat(a.type);
    case Semantic::BlendIndices: return a.semantic_index == 0 && a.type == ElementType::UByte4;
    }
    return false;
}

inline bool ValidLayout(const VertexLayout & layout)
{
    if (!layout.stride_bytes || !layout.count || layout.count > MaxAttributes)
        return false;
    unsigned positions = 0;
    for (unsigned i = 0; i < layout.count; ++i)
    {
        const Attribute & a = layout.attributes[i];
        const unsigned size = ElementSize(a.type);
        const std::uint64_t end = std::uint64_t(a.byte_offset) + size;
        if (!size || !ValidAttribute(a) || end > layout.stride_bytes)
            return false;
        if (a.semantic == Semantic::Position || a.semantic == Semantic::TransformedPosition)
            ++positions;
        for (unsigned j = 0; j < i; ++j)
        {
            const Attribute & b = layout.attributes[j];
            if ((a.semantic == b.semantic && a.semantic_index == b.semantic_index) ||
                (a.byte_offset < std::uint64_t(b.byte_offset) + ElementSize(b.type) && b.byte_offset < end))
                return false;
        }
    }
    return positions == 1;
}

// The allocation and map sizes must fit the legacy Win32 unsigned byte domain.
// Out parameters in all Try helpers are unchanged on failure.
inline bool TryByteSize(std::uint32_t count, std::uint32_t stride, std::uint32_t & bytes)
{
    const std::uint64_t total = std::uint64_t(count) * stride;
    if (!count || count > MaxElements || !stride || total > UINT32_MAX)
        return false;
    bytes = static_cast<std::uint32_t>(total);
    return true;
}

inline bool TryBufferSize(const BufferDesc & desc, std::uint32_t & bytes)
{
    if (desc.storage != Storage::Gpu && desc.storage != Storage::CpuSorting)
        return false;
    if (desc.update != Update::Immutable && desc.update != Update::Rewritable && desc.update != Update::Streaming)
        return false;
    if (desc.recreation != Recreation::RetainSource && desc.recreation != Recreation::Regenerate && desc.recreation != Recreation::Discard)
        return false;
    if (desc.role == Role::Vertex)
    {
        if (desc.index_format != IndexFormat::Absent || desc.stride_bytes != desc.layout.stride_bytes || !ValidLayout(desc.layout))
            return false;
    }
    else if (desc.role == Role::Index)
    {
        if (desc.index_format != IndexFormat::UInt16 || desc.stride_bytes != 2 || desc.layout.count || desc.layout.stride_bytes)
            return false;
    }
    else
        return false;
    return TryByteSize(desc.element_count, desc.stride_bytes, bytes);
}

// Empty maps are rejected, never translated to a native whole-buffer lock.
inline bool TryByteRange(std::uint32_t capacity, std::uint32_t stride, std::uint32_t first,
                         std::uint32_t count, ByteRange & range)
{
    std::uint32_t bytes;
    if (!TryByteSize(capacity, stride, bytes) || !count || first >= capacity || count > capacity - first)
        return false;
    const ByteRange result = { static_cast<std::uint32_t>(std::uint64_t(first) * stride),
                               static_cast<std::uint32_t>(std::uint64_t(count) * stride) };
    range = result;
    return true;
}

struct DrawOffsets
{
    std::int32_t base_vertex;
    std::uint32_t first_index;
};

// Element addresses, NOT bytes. Current legacy base terms are nonnegative;
// bound the sum to the signed base-vertex domain of the future backend.
inline bool TryDrawOffsets(std::uint32_t vba_offset, std::uint32_t index_base_offset,
                           std::uint32_t iba_offset, std::uint32_t start_index, DrawOffsets & offsets)
{
    const std::uint64_t base = std::uint64_t(vba_offset) + index_base_offset;
    const std::uint64_t first = std::uint64_t(iba_offset) + start_index;
    if (base > INT32_MAX || first > UINT32_MAX)
        return false;
    const DrawOffsets result = { static_cast<std::int32_t>(base), static_cast<std::uint32_t>(first) };
    offsets = result;
    return true;
}

enum class Topology { PointList, LineList, LineStrip, TriangleList, TriangleStrip, TriangleFan };

inline bool TryIndexCount(Topology topology, std::uint32_t primitives, std::uint32_t & count)
{
    if (!primitives)
        return false;
    std::uint64_t n = primitives;
    switch (topology)
    {
    case Topology::PointList: break;
    case Topology::LineList: n *= 2; break;
    case Topology::LineStrip: n += 1; break;
    case Topology::TriangleList: n *= 3; break;
    case Topology::TriangleStrip:
    case Topology::TriangleFan: n += 2; break;
    default: return false;
    }
    if (n > MaxElements)
        return false;
    count = static_cast<std::uint32_t>(n);
    return true;
}

// min_vertex/count is the caller's declared span of referenced index values,
// not the index-buffer position. This does not inspect index data or ownership.
// Resolve any legacy vertex_count fallback before calling.
inline bool ValidIndexedDraw(const DrawOffsets & offsets, Topology topology, std::uint32_t primitives,
                             std::uint32_t index_capacity, std::uint32_t min_vertex,
                             std::uint32_t vertex_count, std::uint32_t vertex_capacity)
{
    std::uint32_t count;
    if (!TryIndexCount(topology, primitives, count) ||
        index_capacity > MaxElements || offsets.first_index >= index_capacity ||
        count > index_capacity - offsets.first_index ||
        vertex_capacity > MaxElements || offsets.base_vertex < 0 ||
        !vertex_count || min_vertex > MaxElements)
        return false;
    const std::uint64_t vertex_end = std::uint64_t(offsets.base_vertex) + min_vertex + vertex_count;
    return vertex_end <= vertex_capacity;
}
} // namespace RenderBuffers
