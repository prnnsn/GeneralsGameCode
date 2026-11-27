// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>
#include "WW3D2/RenderTextureSubresources.h"
using namespace RenderTextures;

TEST(RenderTextureSubresources, CubeFacesAndPackedOffsets)
{
    const CubeDesc desc={8,4,WW3D_FORMAT_DXT1};
    unsigned index=99; MipLayout layout={};
    ASSERT_TRUE(TryCubeSubresource(desc,CubeFace::NegativeZ,2,index,layout));
    EXPECT_EQ(22u,index); EXPECT_EQ(2u,layout.width); EXPECT_EQ(8u,layout.row_bytes);
    std::uint32_t offset=0,total=0;
    ASSERT_TRUE(TryPackedCube(desc,CubeFace::NegativeZ,2,offset,total));
    EXPECT_EQ(320u,offset); EXPECT_EQ(336u,total);
    EXPECT_FALSE(TryCubeSubresource(desc,static_cast<CubeFace>(6),0,index,layout));
    EXPECT_FALSE(TryPackedCube({65536,1,WW3D_FORMAT_L8},CubeFace::PositiveX,0,offset,total));
    EXPECT_EQ(320u,offset); EXPECT_EQ(336u,total);
}

TEST(RenderTextureSubresources, VolumeDepthShrinksIndependently)
{
    const VolumeDesc desc={2,1,8,4,WW3D_FORMAT_L8};
    VolumeMipLayout layout={};
    ASSERT_TRUE(TryVolumeMip(desc,2,layout));
    EXPECT_EQ(1u,layout.plane.width); EXPECT_EQ(2u,layout.depth); EXPECT_EQ(2u,layout.byte_count);
    std::uint32_t offset=0,total=0;
    ASSERT_TRUE(TryPackedVolume(desc,2,offset,total));
    EXPECT_EQ(20u,offset); EXPECT_EQ(23u,total);
    EXPECT_FALSE(TryVolumeMip({1,1,1,2,WW3D_FORMAT_L8},0,layout));
    EXPECT_FALSE(TryVolumeMip({65535,65535,2,1,WW3D_FORMAT_L8},0,layout));
    EXPECT_EQ(2u,layout.depth);
}

TEST(RenderTextureSubresources, VolumePaddedRowsAndSlices)
{
    VolumeMipLayout layout={};
    ASSERT_TRUE(TryVolumeMip({7,5,3,1,WW3D_FORMAT_DXT1},0,layout));
    EXPECT_EQ(16u,layout.plane.row_bytes); EXPECT_EQ(2u,layout.plane.row_count);
    EXPECT_EQ(32u,layout.slice_bytes); EXPECT_EQ(96u,layout.byte_count);
    std::uint32_t span=0;
    ASSERT_TRUE(TryVolumeSpan(layout,20,48,132,span)); EXPECT_EQ(132u,span);
    EXPECT_FALSE(TryVolumeSpan(layout,20,35,132,span));
    EXPECT_FALSE(TryVolumeSpan(layout,20,48,131,span));
    EXPECT_FALSE(TryVolumeSpan(layout,15,48,132,span));
    EXPECT_EQ(132u,span);
}
