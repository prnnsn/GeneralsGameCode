// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "WW3D2/Backend/DX8ManagedBufferWrite.h"
#include "WW3D2/Renderer.h"

namespace
{
struct TestBuffer
{
    unsigned char bytes[16] = {};
    unsigned offset = 0;
    unsigned length = 0;
    unsigned flags = 1;
    unsigned locks = 0;
    unsigned unlocks = 0;
    HRESULT lockResult = D3D_OK;
    HRESULT unlockResult = D3D_OK;
    bool returnData = true;

    HRESULT Lock(unsigned position, unsigned size, unsigned char ** data, unsigned mode)
    {
        ++locks;
        offset = position;
        length = size;
        flags = mode;
        *data = returnData ? bytes : nullptr;
        return lockResult;
    }

    HRESULT Unlock()
    {
        ++unlocks;
        return unlockResult;
    }
};
}

TEST(DX8ManagedBufferWrite, PreservesScorchLockArguments)
{
    TestBuffer vb;
    EXPECT_EQ(vb.bytes, DX8ManagedBufferWrite::Lock(&vb, 32768, 24, 0, 32768, true));
    EXPECT_EQ(0u, vb.offset);
    EXPECT_EQ(0u, vb.length);
    EXPECT_EQ(0u, vb.flags);
    EXPECT_TRUE(DX8ManagedBufferWrite::Unlock(vb));
    EXPECT_EQ(1u, vb.unlocks);

    TestBuffer ib;
    EXPECT_EQ(ib.bytes, DX8ManagedBufferWrite::Lock(&ib, 65535, 2, 0, 65535, false));
    EXPECT_EQ(0u, ib.offset);
    EXPECT_EQ(131070u, ib.length);
    EXPECT_EQ(0u, ib.flags);
    EXPECT_TRUE(DX8ManagedBufferWrite::Unlock(ib));
    EXPECT_EQ(1u, ib.unlocks);
}

TEST(DX8ManagedBufferWrite, ValidatesNonemptyElementRange)
{
    TestBuffer buffer;
    EXPECT_EQ(nullptr, DX8ManagedBufferWrite::Lock(&buffer, 64, 24, 0, 0, true));
    EXPECT_EQ(nullptr, DX8ManagedBufferWrite::Lock(&buffer, 64, 24, 63, 2, true));
    EXPECT_EQ(nullptr, DX8ManagedBufferWrite::Lock(&buffer, 64, 0, 1, 1, true));
    EXPECT_EQ(0u, buffer.locks);
    EXPECT_EQ(buffer.bytes, DX8ManagedBufferWrite::Lock(&buffer, 64, 24, 63, 1, true));
    EXPECT_EQ(1512u, buffer.offset);
    EXPECT_EQ(24u, buffer.length);
    EXPECT_TRUE(DX8ManagedBufferWrite::Unlock(buffer));
}

TEST(DX8ManagedBufferWrite, HandlesNativeFailures)
{
    TestBuffer buffer;
    buffer.lockResult = E_FAIL;
    EXPECT_EQ(nullptr, DX8ManagedBufferWrite::Lock(&buffer, 64, 24, 0, 64, true));
    EXPECT_EQ(1u, buffer.locks);
    EXPECT_EQ(0u, buffer.unlocks);

    buffer.lockResult = D3D_OK;
    buffer.returnData = false;
    EXPECT_EQ(nullptr, DX8ManagedBufferWrite::Lock(&buffer, 64, 24, 0, 64, true));
    EXPECT_EQ(1u, buffer.unlocks);

    buffer.returnData = true;
    EXPECT_EQ(buffer.bytes, DX8ManagedBufferWrite::Lock(&buffer, 64, 24, 0, 64, true));
    buffer.unlockResult = E_FAIL;
    EXPECT_FALSE(DX8ManagedBufferWrite::Unlock(buffer));
    EXPECT_EQ(2u, buffer.unlocks);
}

TEST(DX8ManagedBufferWrite, NoDeviceDoesNotAllocateOrMap)
{
    using namespace RenderBuffers;
    const BufferDesc desc{Role::Index, Storage::Gpu, Update::Rewritable, Recreation::Regenerate,
                          16, 2, IndexFormat::UInt16, {}};
    EXPECT_EQ(nullptr, Renderer::Create_Rewritable_Index_Buffer(desc));
    Renderer::BufferWrite vertices(static_cast<VertexBufferClass *>(nullptr), 0, 16);
    EXPECT_EQ(nullptr, vertices.Data());
    EXPECT_FALSE(vertices.Finish());
    Renderer::BufferWrite indices(static_cast<IndexBufferClass *>(nullptr), 0, 16);
    EXPECT_EQ(nullptr, indices.Data());
    EXPECT_FALSE(indices.Finish());
}
