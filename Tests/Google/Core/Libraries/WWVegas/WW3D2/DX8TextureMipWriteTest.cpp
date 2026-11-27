// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <vector>

#include <gtest/gtest.h>

#include "WW3D2/Backend/DX8TextureMipWrite.h"

namespace
{
struct MockTexture
{
    static constexpr unsigned NoFailure = 3;
    unsigned mip_count = 3;
    unsigned lock_failure = NoFailure;
    unsigned invalid_view = NoFailure;
    unsigned unlock_failure = NoFailure;
    std::array<unsigned char, 3> pixels = {};
    std::vector<unsigned> lock_calls;
    std::vector<unsigned> unlock_calls;

    unsigned GetLevelCount() const { return mip_count; }

    HRESULT LockRect(unsigned mip, D3DLOCKED_RECT * rect, const RECT * region, DWORD flags)
    {
        EXPECT_EQ(nullptr, region);
        EXPECT_EQ(0u, flags);
        lock_calls.push_back(mip);
        if (mip == lock_failure)
            return E_FAIL;
        rect->pBits = &pixels[mip];
        rect->Pitch = mip == invalid_view ? 0 : 32 >> mip;
        return D3D_OK;
    }

    HRESULT UnlockRect(unsigned mip)
    {
        unlock_calls.push_back(mip);
        return mip == unlock_failure ? E_FAIL : D3D_OK;
    }
};

struct MipOutputs
{
    unsigned char * data[3] = {};
    unsigned pitch[3] = {};
};
}

TEST(DX8TextureMipWrite, RejectsInvalidLevelCountBeforeCallingDriver)
{
    MockTexture texture;
    MipOutputs out;
    EXPECT_EQ(0u, DX8TextureMipWrite::Lock(texture, out.data, out.pitch, 2));
    texture.mip_count = 0;
    EXPECT_EQ(0u, DX8TextureMipWrite::Lock(texture, out.data, out.pitch, 3));
    EXPECT_TRUE(texture.lock_calls.empty());
    EXPECT_TRUE(texture.unlock_calls.empty());
}

TEST(DX8TextureMipWrite, ReturnsAllMipPointersAndPitchesInLevelOrder)
{
    MockTexture texture;
    MipOutputs out;
    const unsigned count = DX8TextureMipWrite::Lock(texture, out.data, out.pitch, 3);
    ASSERT_EQ(3u, count);
    const std::vector<unsigned> expected = { 0, 1, 2 };
    EXPECT_EQ(expected, texture.lock_calls);
    for (unsigned level = 0; level < count; ++level)
    {
        EXPECT_EQ(&texture.pixels[level], out.data[level]);
        EXPECT_EQ(32u >> level, out.pitch[level]);
    }
    EXPECT_TRUE(DX8TextureMipWrite::Unlock(texture, out.data, out.pitch, count));
    EXPECT_EQ(expected, texture.unlock_calls);
}

TEST(DX8TextureMipWrite, RollsBackOnlySuccessfulLocksIfADriverCallFails)
{
    MockTexture texture;
    texture.lock_failure = 2;
    MipOutputs out;
    EXPECT_EQ(0u, DX8TextureMipWrite::Lock(texture, out.data, out.pitch, 3));
    EXPECT_EQ((std::vector<unsigned>{ 0, 1, 2 }), texture.lock_calls);
    EXPECT_EQ((std::vector<unsigned>{ 0, 1 }), texture.unlock_calls);
    for (unsigned level = 0; level < 3; ++level)
    {
        EXPECT_EQ(nullptr, out.data[level]);
        EXPECT_EQ(0u, out.pitch[level]);
    }
}

TEST(DX8TextureMipWrite, AlsoUnlocksTheSuccessfulLockThatReturnedAnInvalidView)
{
    MockTexture texture;
    texture.invalid_view = 1;
    MipOutputs out;
    EXPECT_EQ(0u, DX8TextureMipWrite::Lock(texture, out.data, out.pitch, 3));
    EXPECT_EQ((std::vector<unsigned>{ 0, 1 }), texture.lock_calls);
    EXPECT_EQ(texture.lock_calls, texture.unlock_calls);
    for (unsigned level = 0; level < 3; ++level)
    {
        EXPECT_EQ(nullptr, out.data[level]);
        EXPECT_EQ(0u, out.pitch[level]);
    }
}

TEST(DX8TextureMipWrite, ReleasesOtherMipsEvenWhenOneUnlockFails)
{
    MockTexture texture;
    texture.unlock_failure = 0;
    MipOutputs out;
    const unsigned count = DX8TextureMipWrite::Lock(texture, out.data, out.pitch, 3);
    ASSERT_EQ(3u, count);
    EXPECT_FALSE(DX8TextureMipWrite::Unlock(texture, out.data, out.pitch, count));
    EXPECT_EQ((std::vector<unsigned>{ 0, 1, 2 }), texture.unlock_calls);
    for (unsigned level = 0; level < count; ++level)
    {
        EXPECT_EQ(nullptr, out.data[level]);
        EXPECT_EQ(0u, out.pitch[level]);
    }
}
