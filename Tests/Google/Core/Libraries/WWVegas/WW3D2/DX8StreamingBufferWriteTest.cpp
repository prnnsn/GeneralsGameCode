// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "WW3D2/Backend/DX8StreamingBufferWrite.h"
#include "WW3D2/Renderer.h"

namespace
{
struct StreamingBufferMock
{
    unsigned char bytes[64] = {};
    unsigned offset = 0;
    unsigned length = 0;
    unsigned flags = 0;
    unsigned locks = 0;
    unsigned unlocks = 0;
    HRESULT lock_result = D3D_OK;
    HRESULT unlock_result = D3D_OK;
    bool return_data = true;

    HRESULT Lock(unsigned first, unsigned size, unsigned char ** output, unsigned mode)
    {
        ++locks;
        offset = first;
        length = size;
        flags = mode;
        *output = return_data ? bytes + first : nullptr;
        return lock_result;
    }

    HRESULT Unlock()
    {
        ++unlocks;
        return unlock_result;
    }
};
}

TEST(DX8StreamingBufferWrite, PreservesFlagsAndExplicitByteRanges)
{
    StreamingBufferMock vb;
    StreamingBufferMock ib;
    const unsigned char values[] = {1, 2, 3, 4, 5, 6, 7, 8};
    EXPECT_TRUE(DX8StreamingBufferWrite::Upload(&vb, 16, 4, 0, 2, values, D3DLOCK_NOSYSLOCK));
    EXPECT_EQ(0u, vb.offset);
    EXPECT_EQ(8u, vb.length);
    EXPECT_EQ(D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK, vb.flags);
    EXPECT_EQ(0, std::memcmp(vb.bytes, values, sizeof(values)));
    EXPECT_TRUE(DX8StreamingBufferWrite::Upload(&vb, 16, 4, 14, 2, values, D3DLOCK_NOSYSLOCK));
    EXPECT_EQ(56u, vb.offset);
    EXPECT_EQ(8u, vb.length);
    EXPECT_EQ(D3DLOCK_NOOVERWRITE | D3DLOCK_NOSYSLOCK, vb.flags);
    EXPECT_EQ(0, std::memcmp(vb.bytes + 56, values, sizeof(values)));
    EXPECT_EQ(2u, vb.unlocks);

    EXPECT_TRUE(DX8StreamingBufferWrite::Upload(&ib, 32, 2, 0, 4, values, 0));
    EXPECT_EQ(D3DLOCK_DISCARD, ib.flags);
    EXPECT_TRUE(DX8StreamingBufferWrite::Upload(&ib, 32, 2, 7, 4, values, 0));
    EXPECT_EQ(D3DLOCK_NOOVERWRITE, ib.flags);
    EXPECT_EQ(14u, ib.offset);
    EXPECT_EQ(8u, ib.length);
    EXPECT_EQ(0, std::memcmp(ib.bytes + 14, values, sizeof(values)));
    EXPECT_EQ(2u, ib.unlocks);
}

TEST(DX8StreamingBufferWrite, InvalidRangesDoNotReachDriver)
{
    StreamingBufferMock buffer;
    const unsigned char values[8] = {};
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, 4, 0, 0, values, 0));
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, 4, 15, 2, values, 0));
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, 4, UINT32_MAX, 2, values, 0));
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, UINT32_MAX, 0, 2, values, 0));
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, 4, 0, 2, nullptr, 0));
    EXPECT_EQ(0u, buffer.locks);
    EXPECT_EQ(0u, buffer.unlocks);
}

TEST(DX8StreamingBufferWrite, OnlySuccessfullyUnlockedDataCanBeSubmitted)
{
    StreamingBufferMock buffer;
    const unsigned char values[] = {1, 2, 3, 4};
    buffer.lock_result = E_FAIL;
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, 4, 0, 1, values, 0));
    EXPECT_EQ(0u, buffer.unlocks);
    EXPECT_EQ(0u, buffer.bytes[0]);

    buffer.lock_result = D3D_OK;
    buffer.return_data = false;
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, 4, 0, 1, values, 0));
    EXPECT_EQ(1u, buffer.unlocks);
    EXPECT_EQ(0u, buffer.bytes[0]);

    buffer.return_data = true;
    buffer.unlock_result = E_FAIL;
    EXPECT_FALSE(DX8StreamingBufferWrite::Upload(&buffer, 16, 4, 0, 1, values, 0));
    EXPECT_EQ(2u, buffer.unlocks);
}

TEST(DX8StreamingBufferWrite, UnavailableReservationsNeverBind)
{
    DynamicVBAccessClass vertices(8);
    DynamicIBAccessClass indices(6);
    const VertexFormatXYZNDUV2 data[8] = {};
    const unsigned short elements[6] = {};
    EXPECT_FALSE(Renderer::Upload_Dynamic_Vertices(vertices, data, sizeof(data[0])));
    EXPECT_FALSE(Renderer::Upload_Dynamic_Indices(indices, elements));
    EXPECT_FALSE(Renderer::Bind_Dynamic_Triangle_List(vertices, indices));
}
