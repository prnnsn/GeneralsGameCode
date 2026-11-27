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

TEST(RenderTexture, PackedDdsOffsetsIncludeSkippedAndDiscardedLevels)
{
    const Texture2DDesc desc = { 8, 64, 7, WW3D_FORMAT_DXT1 };
    const std::uint32_t offsets[] = { 0, 256, 320, 352, 368, 376, 384 };
    PackedMipChain chain = {};
    ASSERT_TRUE(TryPackedDDSMips(desc, 392, chain));
    for (unsigned level = 0; level < desc.mip_levels; ++level)
        EXPECT_EQ(offsets[level], chain[level].offset) << level;
    EXPECT_EQ(8u, chain[2].layout.row_bytes);
    EXPECT_EQ(4u, chain[2].layout.row_count);
}

TEST(RenderTexture, PackedDdsRequiresExactlyTheStoredChain)
{
    const Texture2DDesc desc = { 256, 255, 9, WW3D_FORMAT_DXT5 };
    PackedMipChain chain = {};
    ASSERT_TRUE(TryPackedDDSMips(desc, 87408, chain));
    EXPECT_EQ(81920u, chain[2].offset);
    EXPECT_EQ(87360u, chain[6].offset);
    EXPECT_FALSE(TryPackedDDSMips(desc, 87407, chain));
    EXPECT_FALSE(TryPackedDDSMips(desc, 87409, chain));
    EXPECT_FALSE(TryPackedDDSMips({ 4, 4, 4, WW3D_FORMAT_DXT1 }, 32, chain));
    EXPECT_FALSE(TryPackedDDSMips({ 4, 4, 1, WW3D_FORMAT_L8 }, 16, chain));
    EXPECT_FALSE(TryPackedDDSMips({ 65536, 65536, 17, WW3D_FORMAT_DXT5 }, UINT32_MAX, chain));
    EXPECT_EQ(87360u, chain[6].offset);
}

TEST(RenderTexture, StagedDdsValidatesAllMipsAndCopiesOnlyTheirBlockRows)
{
    PackedDDSData data;
    data.mips[0] = { { 4, 8, 8, 2 }, 0 };
    data.mips[1] = { { 2, 4, 8, 1 }, 16 };
    data.byte_count = 24;
    data.mip_count = 2;
    data.bytes = std::make_unique<unsigned char[]>(data.byte_count);
    for (unsigned i = 0; i < data.byte_count; ++i)
        data.bytes[i] = static_cast<unsigned char>(i + 1);

    std::array<unsigned char, 24> main_surface;
    std::array<unsigned char, 8> small_surface;
    main_surface.fill(0xcd);
    small_surface.fill(0xcd);
    const auto original_main = main_surface;
    const auto original_small = small_surface;
    unsigned char * surfaces[] = { main_surface.data(), small_surface.data() };
    unsigned pitches[] = { 12, 7 };
    EXPECT_FALSE(data.CanCopyTo(surfaces, pitches));
    pitches[1] = 8;
    --data.byte_count;
    EXPECT_FALSE(data.CanCopyTo(surfaces, pitches));
    ++data.byte_count;
    surfaces[1] = nullptr;
    EXPECT_FALSE(data.CanCopyTo(surfaces, pitches));
    surfaces[1] = small_surface.data();

    EXPECT_EQ(original_main, main_surface);
    EXPECT_EQ(original_small, small_surface);

    ASSERT_TRUE(data.CanCopyTo(surfaces, pitches));
    data.CopyTo(surfaces, pitches);
    const std::array<unsigned char, 24> expected_main = {
        1, 2, 3, 4, 5, 6, 7, 8, 0xcd, 0xcd, 0xcd, 0xcd,
        9, 10, 11, 12, 13, 14, 15, 16, 0xcd, 0xcd, 0xcd, 0xcd
    };
    const std::array<unsigned char, 8> expected_small = { 17, 18, 19, 20, 21, 22, 23, 24 };
    EXPECT_EQ(expected_main, main_surface);
    EXPECT_EQ(expected_small, small_surface);
}

TEST(RenderTexture, FillPixelRectWritesInclusivePaddedRows)
{
    std::array<unsigned char, 14> bytes;
    bytes.fill(0xcd);
    const SurfaceWriteView view = {
        bytes.data(), { 3, 2, 6, 2 }, 8, static_cast<std::uint32_t>(bytes.size()), 2, WW3D_FORMAT_A4R4G4B4
    };
    ASSERT_TRUE(TryFillPixelRect(view, 1, 0, 2, 1, 0x4321));
    const std::array<unsigned char, 14> expected = {
        0xcd, 0xcd, 0x21, 0x43, 0x21, 0x43, 0xcd, 0xcd,
        0xcd, 0xcd, 0x21, 0x43, 0x21, 0x43
    };
    EXPECT_EQ(expected, bytes);
}

TEST(RenderTexture, FillPixelRectValidatesTheWholeRectangleBeforeWriting)
{
    std::array<unsigned char, 16> bytes;
    bytes.fill(0xcd);
    const auto original = bytes;
    SurfaceWriteView view = {
        bytes.data(), { 2, 2, 8, 2 }, 8, 15, 4, WW3D_FORMAT_A8R8G8B8
    };
    EXPECT_FALSE(TryFillPixelRect(view, 0, 0, 1, 1, 0xff000000));
    view.byte_count = static_cast<std::uint32_t>(bytes.size());
    EXPECT_FALSE(TryFillPixelRect(view, 0, 0, 2, 1, 0xff000000));
    EXPECT_FALSE(TryFillPixelRect(view, 1, 0, 0, 1, 0xff000000));
    EXPECT_EQ(original, bytes);
    EXPECT_TRUE(TryFillPixelRect(view, 1, 1, 1, 1, 0xff000000));
    EXPECT_EQ(0xff, bytes.back());
    EXPECT_EQ(0xcd, bytes[11]);
}

TEST(RenderTexture, ClearRowsPreservesPaddingAndNeedsNoTrailingPadding)
{
    MipLayout layout = {};
    ASSERT_TRUE(TryMipLayout({ 3, 2, 1, WW3D_FORMAT_A8R8G8B8 }, 0, layout));
    std::array<unsigned char, 28> pixels;
    pixels.fill(0xcd);

    ASSERT_TRUE(TryClearRows(layout, pixels.data(), 16, pixels.size()));
    const std::array<unsigned char, 28> expected = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xcd, 0xcd, 0xcd, 0xcd,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    EXPECT_EQ(expected, pixels);
}

TEST(RenderTexture, ClearRowsRejectsInvalidViewsWithoutWriting)
{
    const MipLayout layout = { 3, 2, 12, 2 };
    std::array<unsigned char, 28> pixels;
    pixels.fill(0xcd);
    const auto original = pixels;

    EXPECT_FALSE(TryClearRows(layout, pixels.data(), 16, 27));
    EXPECT_FALSE(TryClearRows(layout, pixels.data(), 11, pixels.size()));
    EXPECT_FALSE(TryClearRows(layout, nullptr, 16, pixels.size()));
    EXPECT_EQ(original, pixels);
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
