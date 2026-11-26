// Client-only wheel zoom math; no dependencies on the simulation.
#pragma once
#include <algorithm>
#include <cmath>

namespace StrategicCameraZoom
{
inline float limit(float x, float lo, float hi)
{
    return (std::max)(lo, (std::min)(x, hi));
}
inline float boundedStep(float dt) { return limit(dt, 0.0f, 1.0f / 15.0f); }
inline float wheelTarget(float current, float amount, float reference, float lo, float hi)
{
    if (amount == 0.0f)
        return limit(current, lo, hi);
    const float logTarget = std::log(current) + amount / reference;
    // Return exact bounds rather than round-tripping log/exp at a limit.
    if (logTarget <= std::log(lo)) return lo;
    if (logTarget >= std::log(hi)) return hi;
    return limit(std::exp(logTarget), lo, hi);
}
inline float smoothStep(float t)
{
    t = limit(t, 0.0f, 1.0f);
    return t*t*(3.0f-2.0f*t);
}
// Exact critically damped solution for a fixed target during dt. Velocity
// survives retargeting; crossing the target ends the transition.
inline float advance(float current, float target, float &velocity, float dt)
{
    constexpr float omega = 22.0f;
    const float error = current - target;
    const float j = velocity + omega*error;
    const float decay = std::exp(-omega*dt);
    const float nextError = (error+j*dt)*decay;
    const float nextVelocity = (velocity-omega*j*dt)*decay;
    if (error == 0.0f || error*nextError <= 0.0f
        || (std::fabs(nextError) < 0.0001f && std::fabs(nextVelocity) < 0.002f))
    {
        velocity = 0.0f;
        return target;
    }
    velocity = nextVelocity;
    return target + nextError;
}
inline bool insideMap(float x, float y, float minX, float minY, float maxX, float maxY)
{
    return x >= minX && x <= maxX && y >= minY && y <= maxY;
}
}
