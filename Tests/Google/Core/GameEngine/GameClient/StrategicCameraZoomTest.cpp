#include <gtest/gtest.h>
#include "../../../../../Core/GameEngineDevice/Include/W3DDevice/GameClient/StrategicCameraZoom.h"

using namespace StrategicCameraZoom;

TEST(StrategicCameraZoom, ReciprocalWheelSteps)
{
    const float out = wheelTarget(200.0f, 75.0f, 300.0f, 50.0f, 2000.0f);
    EXPECT_NEAR(wheelTarget(out, -75.0f, 300.0f, 50.0f, 2000.0f), 200.0f, 0.001f);
    EXPECT_NEAR(wheelTarget(200.0f, 100000.0f, 300.0f, 50.0f, 2000.0f), 2000.0f, 0.001f);
    EXPECT_NEAR(wheelTarget(200.0f, -100000.0f, 300.0f, 50.0f, 2000.0f), 50.0f, 0.001f);
}
TEST(StrategicCameraZoom, FrameSubdivision)
{
    float v30 = 0.0f, v120 = 0.0f, a = 0.0f, b = 0.0f;
    for (int i = 0; i < 6; ++i) a = advance(a, 1.0f, v30, 1.0f/30.0f);
    for (int i = 0; i < 24; ++i) b = advance(b, 1.0f, v120, 1.0f/120.0f);
    EXPECT_NEAR(a, b, 0.00001f);
    EXPECT_NEAR(v30, v120, 0.0001f);
}
TEST(StrategicCameraZoom, RetargetAndSettle)
{
    float v = 0.0f;
    float p = advance(0.0f, 1.0f, v, 1.0f/60.0f);
    EXPECT_GT(v, 0.0f);
    const float before = p;
    p = advance(p, -1.0f, v, 0.00001f);
    EXPECT_GT(p, before);
    for (int i = 0; i < 120; ++i)
    {
        p = advance(p, -1.0f, v, 1.0f/60.0f);
        EXPECT_GE(p, -1.0f);
    }
    EXPECT_FLOAT_EQ(p, -1.0f);
    EXPECT_FLOAT_EQ(v, 0.0f);
}
TEST(StrategicCameraZoom, BoundsAndStalls)
{
    EXPECT_FLOAT_EQ(boundedStep(0.5f), 1.0f/15.0f);
    EXPECT_FLOAT_EQ(boundedStep(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(smoothStep(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(smoothStep(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(smoothStep(1.0f), 1.0f);
    EXPECT_TRUE(insideMap(10,20,10,20,100,200));
    EXPECT_TRUE(insideMap(100,200,10,20,100,200));
    EXPECT_FALSE(insideMap(9.99f,20,10,20,100,200));
    EXPECT_FALSE(insideMap(100,200.01f,10,20,100,200));
}
