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

static const std::chrono::milliseconds DEFAULT_SYS_TICK_PERIOD = 10ms;
static const std::chrono::milliseconds DEFAULT_TIMER_PERIOD = DEFAULT_SYS_TICK_PERIOD * 10;
static int TestContextObject = 1;

TEST_GROUP(FakeTimersTests) {

    std::unique_ptr<cms::test::FakeTimers> mUnderTest;

    void setup() final
    {
        mUnderTest = std::make_unique<cms::test::FakeTimers>(DEFAULT_SYS_TICK_PERIOD);
    }

    void teardown() final
    {
        mock().clear();
    }

    static void testCallback(cms::test::TimerHandle handle)
    {
        mock().actualCall("testCallback").withParameter("handle", handle);
    }

    TimerHandle Create(std::chrono::milliseconds period = DEFAULT_TIMER_PERIOD,
                       TimerBehavior behavior = TimerBehavior::SingleShot) const
    {
        auto handle = mUnderTest->TimerCreate(
                "TEST", period, behavior,
                &TestContextObject, testCallback);
        return handle;
    }

    TimerHandle CreateAndStartSingleShot(std::chrono::milliseconds period = DEFAULT_TIMER_PERIOD) const
    {
        auto handle = Create(period);
        CHECK_TRUE(handle != 0);

        bool ok = mUnderTest->TimerStart(handle);
        CHECK_TRUE(ok);
        return handle;
    }

    TimerHandle CreateAndStartAutoReload(std::chrono::milliseconds period = DEFAULT_TIMER_PERIOD) const
    {
        auto handle = Create(period, cms::test::TimerBehavior::AutoReload);
        CHECK_TRUE(handle != 0);

        bool ok = mUnderTest->TimerStart(handle);
        CHECK_TRUE(ok);
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
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
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
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    using namespace cms::test;
    auto handle = Create(TEST_PERIOD);
    CHECK_TRUE(handle != 0);

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(TEST_PERIOD * 3);
    mock().checkExpectations();
}

TEST(FakeTimersTests, timer_period_must_adhere_to_sys_tick_period)
{
    const auto TEST_PERIOD = 3ms;
    using namespace cms::test;
    auto handle = Create(TEST_PERIOD);
    CHECK_TRUE(handle == 0); //should fail
}

TEST(FakeTimersTests, when_timer_is_started_will_fire_if_enough_time_has_passed)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    using namespace cms::test;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD);
    mock().checkExpectations();
}

TEST(FakeTimersTests, singleshot_timer_only_fires_once)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    using namespace cms::test;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD * 100);
    mock().checkExpectations();
}

TEST(FakeTimersTests, tick_convenience_method_moves_time_forward_as_expected)
{
    const auto TEST_PERIOD = DEFAULT_SYS_TICK_PERIOD;
    using namespace cms::test;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->Tick();
    mock().checkExpectations();
}

TEST(FakeTimersTests, auto_reload_timer_fires_after_one_period_of_time)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    using namespace cms::test;
    auto handle = CreateAndStartAutoReload(TEST_PERIOD);

    // 1.5 * period, should only fire once
    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD + TEST_PERIOD / 2);
    mock().checkExpectations();

    //while here, ensure it fires on the next period, which should be 0.5 * period from now
    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD / 2);
    mock().checkExpectations();
}

TEST(FakeTimersTests, auto_reload_timer_fires_multiple_times)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    using namespace cms::test;
    auto handle = CreateAndStartAutoReload(TEST_PERIOD);

    const int RELOADS = 100;

    mock().expectNCalls(RELOADS, "testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD * RELOADS);
    mock().checkExpectations();
}

TEST(FakeTimersTests, access_user_context_via_handle)
{
    auto handle = Create();
    auto context = mUnderTest->GetTimerContext(handle);
    CHECK_TRUE(context == &TestContextObject);
}