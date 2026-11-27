// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstring>

#include "WW3D2/Backend/DX8ManagedBufferWrite.h"

namespace DX8StreamingBufferWrite
{
template <typename NativeBuffer>
bool Upload(NativeBuffer * buffer, unsigned capacity, unsigned stride, unsigned first,
            unsigned count, const void * source, unsigned extra_flags)
{
    RenderBuffers::ByteRange range;
    if (!buffer || !source || !RenderBuffers::TryByteRange(capacity, stride, first, count, range))
        return false;

    const unsigned mode = first == 0 ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE;
    unsigned char * data = DX8ManagedBufferWrite::Lock_Native(*buffer, range.offset, range.length, mode | extra_flags);
    if (!data)
        return false;
    std::memcpy(data, source, range.length);
    return DX8ManagedBufferWrite::Unlock(*buffer);
}
}
