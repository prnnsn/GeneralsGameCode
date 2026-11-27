// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "WW3D2/dx8wrapper.h"

namespace DX8TextureMipWrite
{
template <typename NativeTexture>
bool Unlock(NativeTexture & texture, unsigned char ** data, unsigned * pitch, unsigned mip_count)
{
    bool all_unlocked = true;
    for (unsigned level = 0; level < mip_count; ++level)
    {
        const HRESULT result = texture.UnlockRect(level);
        if (FAILED(result))
        {
            Non_Fatal_Log_DX8_ErrorCode(result, __FILE__, __LINE__);
            all_unlocked = false;
        }
        data[level] = nullptr;
        pitch[level] = 0;
    }
    return all_unlocked;
}

template <typename NativeTexture>
unsigned Lock(NativeTexture & texture, unsigned char ** data, unsigned * pitch, unsigned capacity)
{
    const unsigned mip_count = texture.GetLevelCount();
    if (!mip_count || mip_count > capacity)
        return 0;

    for (unsigned level = 0; level < mip_count; ++level)
    {
        D3DLOCKED_RECT rect = {};
        const HRESULT result = texture.LockRect(level, &rect, nullptr, 0);
        if (FAILED(result))
        {
            Non_Fatal_Log_DX8_ErrorCode(result, __FILE__, __LINE__);
            Unlock(texture, data, pitch, level);
            return 0;
        }

        if (!rect.pBits || rect.Pitch <= 0)
        {
            Non_Fatal_Log_DX8_ErrorCode(E_FAIL, __FILE__, __LINE__);
            Unlock(texture, data, pitch, level + 1);
            return 0;
        }

        data[level] = static_cast<unsigned char *>(rect.pBits);
        pitch[level] = static_cast<unsigned>(rect.Pitch);
    }

    return mip_count;
}
}
