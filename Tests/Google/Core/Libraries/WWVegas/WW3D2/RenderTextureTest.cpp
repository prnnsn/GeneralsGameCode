// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "WW3D2/RenderTexture.h"

using namespace RenderTextures;

TEST(RenderTexture, LegacyUncompressedRowSizes)
{
    for (int value = WW3D_FORMAT_R8G8B8; value <= WW3D_FORMAT_X8L8V8U8; ++value)
    {
        const WW3DFormat format = static_cast<WW3DFormat>(value);
        if (format == WW3D_FORMAT_P8 || format == WW3D_FORMAT_A8P8)
            continue;
        FormatLayout storage = {};
        ASSERT_TRUE(TryFormatLayout(format, storage)) << value;
        EXPECT_EQ(1u, storage.block_width);
        EXPECT_EQ(1u, storage.block_height);
        EXPECT_EQ(Get_Bytes_Per_Pixel(format), storage.bytes_per_block);
        EXPECT_EQ(Has_Alpha(format) ? Alpha::Unspecified : Alpha::None, storage.alpha);
    }
}

TEST(RenderTexture, DdsBlockSizesRetainAlphaSemantics)
{
    const struct { WW3DFormat format; unsigned size; Alpha alpha; } types[] = {
        { WW3D_FORMAT_DXT1, 8, Alpha::OptionalOneBit },
        { WW3D_FORMAT_DXT2, 16, Alpha::Premultiplied },
        { WW3D_FORMAT_DXT3, 16, Alpha::Straight },
        { WW3D_FORMAT_DXT4, 16, Alpha::Premultiplied },
        { WW3D_FORMAT_DXT5, 16, Alpha::Straight },
    };
    for (const auto & type : types)
    {
        FormatLayout storage = {};
        ASSERT_TRUE(TryFormatLayout(type.format, storage));
        EXPECT_EQ(4u, storage.block_width);
        EXPECT_EQ(4u, storage.block_height);
        EXPECT_EQ(type.size, storage.bytes_per_block);
        EXPECT_EQ(type.alpha, storage.alpha);
    }
}

TEST(RenderTexture, OddMipSizesAndBlockTails)
{
    const Texture2DDesc linear = { 7, 3, 3, WW3D_FORMAT_R8G8B8 };
    const Texture2DDesc compressed = { 7, 5, 3, WW3D_FORMAT_DXT1 };
    MipLayout mip = {};
    ASSERT_TRUE(TryMipLayout(linear, 1, mip));
    EXPECT_EQ(3u, mip.width);
    EXPECT_EQ(1u, mip.height);
    EXPECT_EQ(9u, mip.row_bytes);
    EXPECT_EQ(1u, mip.row_count);
    ASSERT_TRUE(TryMipLayout(compressed, 0, mip));
    EXPECT_EQ(16u, mip.row_bytes);
    EXPECT_EQ(2u, mip.row_count);
    ASSERT_TRUE(TryMipLayout(compressed, 2, mip));
    EXPECT_EQ(1u, mip.width);
    EXPECT_EQ(1u, mip.height);
    EXPECT_EQ(8u, mip.row_bytes);
    EXPECT_EQ(1u, mip.row_count);
}

TEST(RenderTexture, RectangularMipsKeepAtLeastOneBlockPerAxis)
{
    const Texture2DDesc desc = { 4, 16, 5, WW3D_FORMAT_DXT1 };
    const std::uint32_t expected[] = { 32, 16, 8, 8, 8 };
    for (unsigned level = 0; level < desc.mip_levels; ++level)
    {
        MipLayout mip = {};
        ASSERT_TRUE(TryMipLayout(desc, level, mip));
        EXPECT_EQ(8u, mip.row_bytes);
        EXPECT_EQ(expected[level], mip.row_bytes * mip.row_count);
    }
}

TEST(RenderTexture, RejectsUnknownFormatsInvalidMipChainsAndOverflow)
{
    FormatLayout storage = { 1, 1, 4, Alpha::Unspecified };
    EXPECT_FALSE(TryFormatLayout(WW3D_FORMAT_UNKNOWN, storage));
    EXPECT_FALSE(TryFormatLayout(WW3D_FORMAT_A8P8, storage));
    EXPECT_FALSE(TryFormatLayout(WW3D_FORMAT_P8, storage));
    EXPECT_FALSE(TryFormatLayout(WW3D_FORMAT_COUNT, storage));
    EXPECT_EQ(4u, storage.bytes_per_block);
    EXPECT_EQ(Alpha::Unspecified, storage.alpha);

    MipLayout mip = { 8, 2, 32, 2 };
    EXPECT_FALSE(TryMipLayout({ 0, 4, 1, WW3D_FORMAT_DXT5 }, 0, mip));
    EXPECT_FALSE(TryMipLayout({ 8, 4, 0, WW3D_FORMAT_DXT5 }, 0, mip));
    EXPECT_FALSE(TryMipLayout({ 8, 4, 5, WW3D_FORMAT_DXT5 }, 0, mip));
    EXPECT_FALSE(TryMipLayout({ 8, 4, 4, WW3D_FORMAT_DXT5 }, 4, mip));
    EXPECT_FALSE(TryMipLayout({ UINT32_MAX, 1, 1, WW3D_FORMAT_A8R8G8B8 }, 0, mip));
    EXPECT_FALSE(TryMipLayout({ 65536, 65536, 1, WW3D_FORMAT_L8 }, 0, mip));
    EXPECT_EQ(8u, mip.width);
    EXPECT_EQ(32u, mip.row_bytes);

    ASSERT_TRUE(TryMipLayout({ UINT32_MAX, 1, 32, WW3D_FORMAT_L8 }, 31, mip));
    EXPECT_EQ(1u, mip.width);
    EXPECT_EQ(1u, mip.row_bytes);
}

TEST(RenderTexture, PaddedRowsUseOnlyRequiredBytes)
{
    MipLayout mip = {};
    ASSERT_TRUE(TryMipLayout({ 7, 5, 3, WW3D_FORMAT_DXT5 }, 0, mip));
    EXPECT_EQ(32u, mip.row_bytes);
    EXPECT_EQ(2u, mip.row_count);
    std::uint32_t bytes = 0;
    ASSERT_TRUE(TryRowSpan(mip, 48, 80, bytes));
    EXPECT_EQ(80u, bytes);
    EXPECT_FALSE(TryRowSpan(mip, 48, 79, bytes));
    EXPECT_FALSE(TryRowSpan(mip, 31, 100, bytes));
    EXPECT_FALSE(TryRowSpan(mip, UINT32_MAX, UINT32_MAX, bytes));
    EXPECT_FALSE(TryRowSpan({ 1, 1, 0, 1 }, 32, 32, bytes));
    EXPECT_FALSE(TryRowSpan({ 1, 1, 8, 0 }, 32, 32, bytes));
    EXPECT_EQ(80u, bytes);
}
