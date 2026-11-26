/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Lib/Profile.h"

#ifdef PROFILER_ENABLED
#include <cstdint>
#include "WW3D2/dx8wrapper.h"

namespace ShadowPassProfile
{
struct Counters
{
    std::int64_t drawCalls = 0, triangles = 0, stateRequests = 0, bindingRequests = 0;
    std::int64_t lockCalls = 0, lockFailures = 0, discardLocks = 0, noOverwriteLocks = 0;
    std::int64_t lockedBytes = 0;
};

// The volume-shadow pass is synchronous on the render thread. Keep each
// invocation independent, including early returns and hypothetical nesting.
inline thread_local Counters *active = nullptr;
struct Scope
{
    Counters counts;
    Counters *previous = active;
    Scope() { active = &counts; }
    ~Scope()
    {
        active = previous;
        PROFILER_PLOT("ShadowPass/Draw requests", counts.drawCalls);
        PROFILER_PLOT("ShadowPass/Requested triangles", counts.triangles);
        PROFILER_PLOT("ShadowPass/State requests", counts.stateRequests);
        PROFILER_PLOT("ShadowPass/Binding requests", counts.bindingRequests);
        PROFILER_PLOT("ShadowPass/Dynamic lock requests", counts.lockCalls);
        PROFILER_PLOT("ShadowPass/Dynamic lock failures", counts.lockFailures);
        PROFILER_PLOT("ShadowPass/Dynamic discard requests", counts.discardLocks);
        PROFILER_PLOT("ShadowPass/Dynamic no-overwrite requests", counts.noOverwriteLocks);
        PROFILER_PLOT("ShadowPass/Successfully locked dynamic bytes", counts.lockedBytes);
    }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
};

inline void stateRequest() { if (active) ++active->stateRequests; }
inline void bindingRequest() { if (active) ++active->bindingRequests; }

template<class F>
auto draw(unsigned triangles, F &&call)
{
    PROFILER_SECTION_NAME("ShadowPass::DrawCall");
    if (active)
    {
        ++active->drawCalls;
        active->triangles += triangles;
    }
    return call();
}

template<class B, class P>
auto lock(B *buffer, unsigned offset, unsigned bytes, P data, unsigned flags)
{
    PROFILER_SECTION_NAME("ShadowPass::DynamicBufferLock");
    if (active)
    {
        ++active->lockCalls;
        active->discardLocks += (flags & D3DLOCK_DISCARD) != 0;
        active->noOverwriteLocks += (flags & D3DLOCK_NOOVERWRITE) != 0;
    }
    const auto result = buffer->Lock(offset, bytes, data, flags);
    if (active)
    {
        if (result == D3D_OK)
            active->lockedBytes += bytes;
        else
            ++active->lockFailures;
    }
    return result;
}

template<class B>
auto unlock(B *buffer)
{
    PROFILER_SECTION_NAME("ShadowPass::DynamicBufferUnlock");
    return buffer->Unlock();
}
}

#define SHADOW_PASS_SCOPE() ShadowPassProfile::Scope shadowPassProfileScope
#define SHADOW_PASS_STATE(call) (ShadowPassProfile::stateRequest(), (call))
#define SHADOW_PASS_BIND(call) (ShadowPassProfile::bindingRequest(), (call))
#define SHADOW_PASS_DRAW(tris, call) ShadowPassProfile::draw(tris, [&] { return (call); })
#define SHADOW_PASS_LOCK(buffer, offset, bytes, data, flags) ShadowPassProfile::lock(buffer, offset, bytes, data, flags)
#define SHADOW_PASS_UNLOCK(buffer) ShadowPassProfile::unlock(buffer)

#else
#define SHADOW_PASS_SCOPE() ((void)0)
#define SHADOW_PASS_STATE(call) (call)
#define SHADOW_PASS_BIND(call) (call)
#define SHADOW_PASS_DRAW(tris, call) (call)
#define SHADOW_PASS_LOCK(buffer, offset, bytes, data, flags) (buffer)->Lock(offset, bytes, data, flags)
#define SHADOW_PASS_UNLOCK(buffer) (buffer)->Unlock()
#endif
