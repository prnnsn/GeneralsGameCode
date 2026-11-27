// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <cstddef>

#include "WW3D2/Backend/DX8VertexLayout.h"

using namespace RenderBuffers;

TEST(DX8VertexLayout, OrdinaryProducerStrides)
{
    const struct { unsigned fvf; std::uint32_t stride; } cases[] = {
        { DX8_FVF_XYZ, sizeof(VertexFormatXYZ) },
        { DX8_FVF_XYZN, sizeof(VertexFormatXYZN) },
        { DX8_FVF_XYZNUV1, sizeof(VertexFormatXYZNUV1) },
        { DX8_FVF_XYZNUV2, sizeof(VertexFormatXYZNUV2) },
        { DX8_FVF_XYZNDUV1, sizeof(VertexFormatXYZNDUV1) },
        { DX8_FVF_XYZNDUV2, sizeof(VertexFormatXYZNDUV2) },
        { DX8_FVF_XYZDUV1, sizeof(VertexFormatXYZDUV1) },
        { DX8_FVF_XYZDUV2, sizeof(VertexFormatXYZDUV2) },
        { DX8_FVF_XYZUV1, sizeof(VertexFormatXYZUV1) },
        { DX8_FVF_XYZUV2, sizeof(VertexFormatXYZUV2) },
        { DX8_FVF_XYZNDCUBEMAP, sizeof(VertexFormatXYZNDCUBEMAP) }
    };
    for (const auto & c : cases)
    {
        FVFInfoClass info(c.fvf);
        VertexLayout layout = {};
        ASSERT_TRUE(DX8VertexLayout::TryFromLegacy(info, c.stride, layout)) << c.fvf;
        EXPECT_EQ(c.stride, layout.stride_bytes);
        EXPECT_TRUE(ValidLayout(layout));
    }
    FVFInfoClass info(DX8_FVF_XYZNDUV2);
    VertexLayout layout = {};
    ASSERT_TRUE(DX8VertexLayout::TryFromLegacy(info, sizeof(VertexFormatXYZNDUV2), layout));
    ASSERT_EQ(5u, layout.count);
    EXPECT_EQ(offsetof(VertexFormatXYZNDUV2, x), layout.attributes[0].byte_offset);
    EXPECT_EQ(offsetof(VertexFormatXYZNDUV2, nx), layout.attributes[1].byte_offset);
    EXPECT_EQ(offsetof(VertexFormatXYZNDUV2, diffuse), layout.attributes[2].byte_offset);
    EXPECT_EQ(ElementType::PackedArgb8, layout.attributes[2].type);
    EXPECT_EQ(offsetof(VertexFormatXYZNDUV2, u1), layout.attributes[3].byte_offset);
    EXPECT_EQ(offsetof(VertexFormatXYZNDUV2, u2), layout.attributes[4].byte_offset);
}

TEST(DX8VertexLayout, NativeCoordinateWidthsAndTransformedPosition)
{
    const unsigned fvf = D3DFVF_XYZ | D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE1(0) |
                         D3DFVF_TEXCOORDSIZE2(1) | D3DFVF_TEXCOORDSIZE3(2) | D3DFVF_TEXCOORDSIZE4(3);
    VertexLayout layout = {};
    ASSERT_TRUE(DX8VertexLayout::TryDecode(fvf, 52, layout));
    ASSERT_EQ(5u, layout.count);
    EXPECT_EQ(12u, layout.attributes[1].byte_offset);
    EXPECT_EQ(16u, layout.attributes[2].byte_offset);
    EXPECT_EQ(24u, layout.attributes[3].byte_offset);
    EXPECT_EQ(36u, layout.attributes[4].byte_offset);
    EXPECT_EQ(ElementType::Float1, layout.attributes[1].type);
    EXPECT_EQ(ElementType::Float2, layout.attributes[2].type);
    EXPECT_EQ(ElementType::Float3, layout.attributes[3].type);
    EXPECT_EQ(ElementType::Float4, layout.attributes[4].type);
    ASSERT_TRUE(DX8VertexLayout::TryDecode(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1, 32, layout));
    EXPECT_EQ(Semantic::TransformedPosition, layout.attributes[0].semantic);
    EXPECT_EQ(16u, layout.attributes[1].byte_offset);
    EXPECT_EQ(20u, layout.attributes[2].byte_offset);
    EXPECT_EQ(24u, layout.attributes[3].byte_offset);
}

TEST(DX8VertexLayout, LegacyMismatchAndUnsupportedFormats)
{
    VertexLayout layout = {};
    FVFInfoClass tg3(DX8_FVF_XYZNDUV1TG3);
    EXPECT_TRUE(DX8VertexLayout::TryDecode(DX8_FVF_XYZNDUV1TG3, sizeof(VertexFormatXYZNDUV1TG3), layout));
    EXPECT_FALSE(DX8VertexLayout::TryFromLegacy(tg3, sizeof(VertexFormatXYZNDUV1TG3), layout));
    EXPECT_FALSE(DX8VertexLayout::TryDecode(DX8_FVF_XYZNUV2DMAP, sizeof(VertexFormatXYZNUV2DMAP), layout));
    EXPECT_FALSE(DX8VertexLayout::TryDecode(D3DFVF_XYZB4 | D3DFVF_LASTBETA_UBYTE4, 28, layout));
    EXPECT_FALSE(DX8VertexLayout::TryDecode(D3DFVF_XYZRHW | D3DFVF_NORMAL, 28, layout));
    EXPECT_FALSE(DX8VertexLayout::TryDecode(D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0), 12, layout));
    FVFInfoClass mutable_info(DX8_FVF_XYZDUV1);
    mutable_info.Set_FVF_Size(100);
    EXPECT_FALSE(DX8VertexLayout::TryFromLegacy(mutable_info, sizeof(VertexFormatXYZDUV1), layout));
    EXPECT_FALSE(DX8VertexLayout::TryDecode(DX8_FVF_XYZ, 16, layout));
}
