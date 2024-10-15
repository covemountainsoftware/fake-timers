// MIT License
//
// Copyright (c) 2024 Matthew Eshleman Consulting (covemountainsoftware.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "FakeTimers.hpp"
#include <memory>
#include <chrono>
#include "CppUTestExt/MockSupport.h"

//must be last
#include "CppUTest/TestHarness.h"

using namespace std::chrono_literals;
using namespace cms::test;

TEST_GROUP(FakeTimersTests) {
    std::unique_ptr<cms::test::FakeTimers> mUnderTest;

    void setup() final
    {
        mUnderTest = std::make_unique<cms::test::FakeTimers>();
    }

    void teardown() final
    {
        mock().clear();
    }

    static void testCallback(cms::test::TimerHandle handle)
    {
        mock().actualCall("testCallback").withParameter("handle", handle);
    }

    TimerHandle Create(std::chrono::milliseconds period = 100ms) const
    {
        auto handle = mUnderTest->TimerCreate(
                "TEST", period,
                TimerBehavior::SingleShot, nullptr, testCallback);
        return handle;
    }
};

TEST(FakeTimersTests, can_compile)
{
}

TEST(FakeTimersTests, can_create_a_timer)
{
    using namespace cms::test;
    auto handle = Create();
    CHECK_TRUE(handle != 0);
}

TEST(FakeTimersTests, can_create_two_timers)
{
    using namespace cms::test;
    auto handle1 = Create();
    auto handle2 = Create();
    CHECK_TRUE(handle1 != 0);
    CHECK_TRUE(handle2 != 0);
    CHECK_TRUE(handle1 != handle2);
}

TEST(FakeTimersTests, can_delete_a_timer)
{
    using namespace cms::test;
    auto handle = Create();
    CHECK_TRUE(handle != 0);

    bool ok = mUnderTest->TimerDelete(handle);
    CHECK_TRUE(ok);
}

TEST(FakeTimersTests, delete_will_error_if_zero_handle)
{
    using namespace cms::test;

    bool ok = mUnderTest->TimerDelete(0);
    CHECK_FALSE(ok);
}

TEST(FakeTimersTests, when_timer_is_started_does_not_fire_if_not_enough_time_has_passed)
{
    const auto TEST_PERIOD = 100ms;
    using namespace cms::test;
    auto handle = Create(TEST_PERIOD);
    CHECK_TRUE(handle != 0);

    bool ok = mUnderTest->TimerStart(handle);
    CHECK_TRUE(ok);

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(TEST_PERIOD - 1ms);
    mock().checkExpectations();
}

TEST(FakeTimersTests, when_timer_is_not_started_does_not_fire)
{
    const auto TEST_PERIOD = 100ms;
    using namespace cms::test;
    auto handle = Create(TEST_PERIOD);
    CHECK_TRUE(handle != 0);

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(TEST_PERIOD * 3);
    mock().checkExpectations();
}