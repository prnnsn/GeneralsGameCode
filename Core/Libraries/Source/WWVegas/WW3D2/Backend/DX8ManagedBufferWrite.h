// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "WW3D2/dx8wrapper.h"
#include "WW3D2/RenderBuffer.h"

namespace DX8ManagedBufferWrite
{
template <typename NativeBuffer>
bool Unlock(NativeBuffer & buffer)
{
    const HRESULT result = buffer.Unlock();
    if (FAILED(result))
        Non_Fatal_Log_DX8_ErrorCode(result, __FILE__, __LINE__);
    return SUCCEEDED(result);
}

template <typename NativeBuffer>
unsigned char * Lock(NativeBuffer * buffer, unsigned capacity, unsigned stride,
                     unsigned first, unsigned count, bool legacy_whole_lock)
{
    RenderBuffers::ByteRange range;
    if (!buffer || !RenderBuffers::TryByteRange(capacity, stride, first, count, range))
        return nullptr;

    // DX8 used a zero byte count to lock the entire vertex allocation.
    // Only use it for a verified full span. Index locks retain explicit sizes.
    const bool whole = legacy_whole_lock && first == 0 && count == capacity;
    unsigned char * data = nullptr;
    const HRESULT result = buffer->Lock(range.offset, whole ? 0 : range.length, &data, 0);
    if (FAILED(result))
    {
        Non_Fatal_Log_DX8_ErrorCode(result, __FILE__, __LINE__);
        return nullptr;
    }
    if (!data)
    {
        Unlock(*buffer);
        Non_Fatal_Log_DX8_ErrorCode(E_FAIL, __FILE__, __LINE__);
    }
    return data;
}
}
