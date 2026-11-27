// SPDX-License-Identifier: GPL-3.0-or-later
#include <gtest/gtest.h>
#include <limits>
#include "WW3D2/RenderTargetDesc.h"
using namespace RenderTargets;

TEST(RenderTargetDesc, LegacyDepthFormatsKeepStencilCapacity)
{
    const unsigned depth[]={16,32,15,24,16,24,24};
    const unsigned stencil[]={0,0,1,8,0,0,4};
    const unsigned bytes[]={2,4,2,4,2,4,4};
    DepthLayout layout={};
    for(unsigned i=0;i<7;++i)
    {
        ASSERT_TRUE(TryDepthLayout(static_cast<WW3DZFormat>(i+1),layout));
        EXPECT_EQ(depth[i],layout.depth_bits); EXPECT_EQ(stencil[i],layout.stencil_bits);
        EXPECT_EQ(bytes[i],layout.storage_bytes); EXPECT_EQ(i==0,layout.lockable);
    }
    EXPECT_FALSE(TryDepthLayout(WW3D_ZFORMAT_UNKNOWN,layout));
    EXPECT_EQ(4u,layout.stencil_bits);
}
TEST(RenderTargetDesc, CheckedSingleSampleFootprints)
{
    std::uint32_t bytes=0;
    ASSERT_TRUE(TryColorFootprint({640,480,WW3D_FORMAT_A8R8G8B8,1},bytes));
    EXPECT_EQ(1228800u,bytes);
    EXPECT_FALSE(TryColorFootprint({640,480,WW3D_FORMAT_DXT1,1},bytes));
    EXPECT_FALSE(TryColorFootprint({640,480,WW3D_FORMAT_A8R8G8B8,4},bytes));
    EXPECT_FALSE(TryDepthFootprint({UINT32_MAX,UINT32_MAX,WW3D_ZFORMAT_D24S8,1},bytes));
    EXPECT_EQ(1228800u,bytes);
}
TEST(RenderTargetDesc, DepthAndViewportCompatibility)
{
    const ColorDesc color={640,480,WW3D_FORMAT_A8R8G8B8,1};
    EXPECT_TRUE(Compatible(color,{1024,512,WW3D_ZFORMAT_D24S8,1},8));
    EXPECT_FALSE(Compatible(color,{640,479,WW3D_ZFORMAT_D24S8,1},8));
    EXPECT_FALSE(Compatible(color,{640,480,WW3D_ZFORMAT_D15S1,1},8));
    EXPECT_TRUE(ValidViewport(color,{1,1,639,479,0,1}));
    EXPECT_FALSE(ValidViewport(color,{1,1,640,479,0,1}));
    EXPECT_FALSE(ValidViewport(color,{0,0,640,480,0,std::numeric_limits<float>::quiet_NaN()}));
    EXPECT_FALSE(ValidViewport(color,{0,0,640,480,1,0}));
}
