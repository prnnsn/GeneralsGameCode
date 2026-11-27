// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "WW3D2/RenderBuffer.h"

using namespace RenderBuffers;

TEST(RenderBuffer, CheckedByteRanges)
{
    ByteRange range = {};
    ASSERT_TRUE(TryByteRange(64, 32, 7, 11, range));
    EXPECT_EQ(224u, range.offset);
    EXPECT_EQ(352u, range.length);
    ASSERT_TRUE(TryByteRange(64, 2, 13, 9, range));
    EXPECT_EQ(26u, range.offset);
    EXPECT_EQ(18u, range.length);
    ASSERT_TRUE(TryByteRange(65535, 2, 65534, 1, range));
    EXPECT_EQ(131068u, range.offset);
    EXPECT_EQ(2u, range.length);
    EXPECT_FALSE(TryByteRange(64, 32, 64, 0, range));
    EXPECT_FALSE(TryByteRange(64, 32, 0, 0, range));
    EXPECT_FALSE(TryByteRange(64, 32, 63, 2, range));
    EXPECT_FALSE(TryByteRange(64, 32, UINT32_MAX, 1, range));
    EXPECT_FALSE(TryByteRange(64, 0, 0, 1, range));
    EXPECT_FALSE(TryByteRange(2, UINT32_MAX, 0, 1, range));
    EXPECT_EQ(131068u, range.offset);
    EXPECT_EQ(2u, range.length);
}

TEST(RenderBuffer, ByteSizeAndIndexDescriptor)
{
    std::uint32_t bytes = 17;
    EXPECT_FALSE(TryByteSize(65536, 2, bytes));
    EXPECT_FALSE(TryByteSize(0, 2, bytes));
    EXPECT_FALSE(TryByteSize(2, UINT32_MAX, bytes));
    EXPECT_EQ(17u, bytes);
    EXPECT_TRUE(TryByteSize(1, UINT32_MAX, bytes));
    EXPECT_EQ(UINT32_MAX, bytes);

    BufferDesc desc = {};
    desc.role = Role::Index;
    desc.storage = Storage::Gpu;
    desc.update = Update::Rewritable;
    desc.recreation = Recreation::Regenerate;
    desc.index_format = IndexFormat::UInt16;
    desc.element_count = 65535;
    desc.stride_bytes = 2;
    ASSERT_TRUE(TryBufferSize(desc, bytes));
    EXPECT_EQ(131070u, bytes);
    desc.storage = Storage::CpuSorting;
    EXPECT_TRUE(TryBufferSize(desc, bytes));
    desc.stride_bytes = 4;
    EXPECT_FALSE(TryBufferSize(desc, bytes));
    desc.stride_bytes = 2;
    desc.index_format = IndexFormat::Absent;
    EXPECT_FALSE(TryBufferSize(desc, bytes));
}

TEST(RenderBuffer, VertexLayoutValidation)
{
    VertexLayout layout = { 32, 3, {
        { Semantic::Position, 0, ElementType::Float3, 0 },
        { Semantic::Normal, 0, ElementType::Float3, 12 },
        { Semantic::TexCoord, 0, ElementType::Float2, 24 }
    } };
    ASSERT_TRUE(ValidLayout(layout));
    BufferDesc desc = {};
    desc.role = Role::Vertex;
    desc.element_count = 64;
    desc.stride_bytes = 32;
    desc.layout = layout;
    std::uint32_t bytes = 0;
    ASSERT_TRUE(TryBufferSize(desc, bytes));
    EXPECT_EQ(2048u, bytes);
    desc.stride_bytes = 36;
    EXPECT_FALSE(TryBufferSize(desc, bytes));
    desc.stride_bytes = 32;
    desc.index_format = IndexFormat::UInt16;
    EXPECT_FALSE(TryBufferSize(desc, bytes));

    layout.attributes[2].byte_offset = 23;
    EXPECT_FALSE(ValidLayout(layout));
    layout.attributes[2].byte_offset = 25;
    EXPECT_FALSE(ValidLayout(layout));
    layout.attributes[2].byte_offset = UINT32_MAX;
    EXPECT_FALSE(ValidLayout(layout));
    layout.attributes[2] = { Semantic::Normal, 0, ElementType::Float3, 24 };
    layout.stride_bytes = 36;
    EXPECT_FALSE(ValidLayout(layout)); // duplicate semantic, no overlap
    layout.attributes[2] = { Semantic::TexCoord, 8, ElementType::Float2, 24 };
    EXPECT_FALSE(ValidLayout(layout));
    layout.attributes[2] = { Semantic::TexCoord, 0, static_cast<ElementType>(99), 24 };
    EXPECT_FALSE(ValidLayout(layout));
    layout.count = MaxAttributes + 1;
    EXPECT_FALSE(ValidLayout(layout));
}

TEST(RenderBuffer, IndexedDrawUsesElements)
{
    DrawOffsets offsets = {};
    ASSERT_TRUE(TryDrawOffsets(7, 5, 13, 3, offsets));
    EXPECT_EQ(12, offsets.base_vertex);
    EXPECT_EQ(16u, offsets.first_index);
    EXPECT_TRUE(ValidIndexedDraw(offsets, Topology::TriangleList, 3, 25, 2, 5, 19));
    EXPECT_FALSE(ValidIndexedDraw(offsets, Topology::TriangleList, 3, 24, 2, 5, 19));
    EXPECT_FALSE(ValidIndexedDraw(offsets, Topology::TriangleList, 3, 25, 2, 5, 18));
    EXPECT_FALSE(ValidIndexedDraw(offsets, Topology::TriangleList, 0, 25, 2, 5, 19));
    EXPECT_FALSE(TryDrawOffsets(INT32_MAX, 1, 0, 0, offsets));
    EXPECT_FALSE(TryDrawOffsets(0, 0, UINT32_MAX, 1, offsets));
    EXPECT_EQ(12, offsets.base_vertex);
    EXPECT_EQ(16u, offsets.first_index);
}

TEST(RenderBuffer, PrimitiveCounts)
{
    std::uint32_t n = 0;
    ASSERT_TRUE(TryIndexCount(Topology::PointList, 3, n)); EXPECT_EQ(3u, n);
    ASSERT_TRUE(TryIndexCount(Topology::LineList, 3, n)); EXPECT_EQ(6u, n);
    ASSERT_TRUE(TryIndexCount(Topology::LineStrip, 3, n)); EXPECT_EQ(4u, n);
    ASSERT_TRUE(TryIndexCount(Topology::TriangleList, 3, n)); EXPECT_EQ(9u, n);
    ASSERT_TRUE(TryIndexCount(Topology::TriangleStrip, 3, n)); EXPECT_EQ(5u, n);
    ASSERT_TRUE(TryIndexCount(Topology::TriangleFan, 3, n)); EXPECT_EQ(5u, n);
    EXPECT_FALSE(TryIndexCount(Topology::TriangleList, UINT32_MAX, n));
    EXPECT_FALSE(TryIndexCount(Topology::TriangleStrip, 65535, n));
    EXPECT_EQ(5u, n);
}
