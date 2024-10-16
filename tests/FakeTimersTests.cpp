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

    std::unique_ptr<FakeTimers> mUnderTest;

    void setup() final
    {
        mUnderTest = std::make_unique<FakeTimers>(DEFAULT_SYS_TICK_PERIOD);
    }

    void teardown() final
    {
        mock().clear();
    }

    static void testCallback(FakeTimers::Handle handle, FakeTimers::Context context)
    {
        (void)context;
        mock().actualCall("testCallback")
            .withParameter("handle", handle);
    }

    FakeTimers::Handle Create(std::chrono::milliseconds period = DEFAULT_TIMER_PERIOD,
                              FakeTimers::Behavior behavior = FakeTimers::Behavior::SingleShot) const
    {
        auto handle = mUnderTest->TimerCreate(
                "TEST", period, behavior,
                &TestContextObject, testCallback);
        return handle;
    }

    FakeTimers::Handle CreateAndStartSingleShot(std::chrono::milliseconds period = DEFAULT_TIMER_PERIOD) const
    {
        auto handle = Create(period);
        CHECK_TRUE(handle != 0);

        bool ok = mUnderTest->TimerStart(handle);
        CHECK_TRUE(ok);
        return handle;
    }

    FakeTimers::Handle CreateAndStartAutoReload(std::chrono::milliseconds period = DEFAULT_TIMER_PERIOD) const
    {
        auto handle = Create(period, FakeTimers::Behavior::AutoReload);
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
    auto handle = Create();
    CHECK_TRUE(handle != 0);
}

TEST(FakeTimersTests, can_create_two_timers)
{
    auto handle1 = Create();
    auto handle2 = Create();
    CHECK_TRUE(handle1 != 0);
    CHECK_TRUE(handle2 != 0);
    CHECK_TRUE(handle1 != handle2);
}

TEST(FakeTimersTests, can_delete_a_timer)
{
    auto handle = Create();
    CHECK_TRUE(handle != 0);

    bool ok = mUnderTest->TimerDelete(handle);
    CHECK_TRUE(ok);
}

TEST(FakeTimersTests, delete_will_error_if_zero_handle)
{
    bool ok = mUnderTest->TimerDelete(0);
    CHECK_FALSE(ok);
}

TEST(FakeTimersTests, move_time_forward_moves_internal_time_point)
{
    using namespace std::chrono_literals;
    //upon creation, should be at 0
    CHECK_TRUE(0ms == mUnderTest->GetCurrentInternalTime());

    //Tick will move time forward one sys tick period
    mUnderTest->Tick();
    CHECK_TRUE(DEFAULT_SYS_TICK_PERIOD == mUnderTest->GetCurrentInternalTime());

    //move time forward moves arbitrary amount, as long as it is modulo the sys tick
    mUnderTest->MoveTimeForward(1s);
    CHECK_TRUE(DEFAULT_SYS_TICK_PERIOD + 1s == mUnderTest->GetCurrentInternalTime());
}

TEST(FakeTimersTests, when_timer_is_started_does_not_fire_if_not_enough_time_has_passed)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
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
    auto handle = Create(TEST_PERIOD);
    CHECK_TRUE(handle != 0);

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(TEST_PERIOD * 3);
    mock().checkExpectations();
}

TEST(FakeTimersTests, timer_period_must_adhere_to_sys_tick_period)
{
    const auto TEST_PERIOD = 3ms;
    auto handle = Create(TEST_PERIOD);
    CHECK_TRUE(handle == 0); //should fail
}

TEST(FakeTimersTests, when_timer_is_started_will_fire_if_enough_time_has_passed)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD);
    mock().checkExpectations();
}

TEST(FakeTimersTests, singleshot_timer_only_fires_once)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD * 100);
    mock().checkExpectations();
}

TEST(FakeTimersTests, tick_convenience_method_moves_time_forward_as_expected)
{
    const auto TEST_PERIOD = DEFAULT_SYS_TICK_PERIOD;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->Tick();
    mock().checkExpectations();
}

TEST(FakeTimersTests, auto_reload_timer_fires_after_one_period_of_time)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
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
    auto handle = CreateAndStartAutoReload(TEST_PERIOD);

    const int RELOADS = 100;

    mock().expectNCalls(RELOADS, "testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD * RELOADS);
    mock().checkExpectations();
}

TEST(FakeTimersTests, access_user_context_via_handle)
{
    auto handle = Create();
    auto context = mUnderTest->TimerGetContext(handle);
    CHECK_TRUE(context == &TestContextObject);
}

TEST(FakeTimersTests, set_user_context_via_handle)
{
    static int AnotherTestContextObject = 2;
    auto handle = Create();
    auto context = mUnderTest->TimerGetContext(handle);
    CHECK_TRUE(context == &TestContextObject);

    CHECK_TRUE(mUnderTest->TimerSetContext(handle, &AnotherTestContextObject));
    context = mUnderTest->TimerGetContext(handle);
    CHECK_TRUE(context == &AnotherTestContextObject);
}

TEST(FakeTimersTests, access_timer_name_via_handle)
{
    auto handle = Create();
    auto name = mUnderTest->TimerGetName(handle);
    STRCMP_EQUAL("TEST", name);
}

TEST(FakeTimersTests, access_timer_period_via_handle)
{
    auto handle = Create(1s);
    auto period = mUnderTest->TimerGetPeriod(handle);
    CHECK_TRUE(1s == period);
}

TEST(FakeTimersTests, access_timer_behavior_via_handle)
{
    auto handle = Create();
    auto behavior = mUnderTest->TimerGetBehavior(handle);
    CHECK_TRUE(FakeTimers::Behavior::SingleShot == behavior);
}

TEST(FakeTimersTests, set_timer_behavior_via_handle)
{
    auto handle = Create();
    auto behavior = mUnderTest->TimerGetBehavior(handle);
    CHECK_TRUE(FakeTimers::Behavior::SingleShot == behavior);

    mUnderTest->TimerSetBehavior(handle, FakeTimers::Behavior::AutoReload);
    behavior = mUnderTest->TimerGetBehavior(handle);
    CHECK_TRUE(FakeTimers::Behavior::AutoReload == behavior);
}

TEST(FakeTimersTests, is_timer_active_method_works_as_expected)
{
    auto handle = Create();

    //upon creation, the timer is not yet active
    CHECK_FALSE(mUnderTest->TimerIsActive(handle));

    //start (activate) the single shot timer
    mUnderTest->TimerStart(handle);

    //timer is now active
    CHECK_TRUE(mUnderTest->TimerIsActive(handle));

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(DEFAULT_TIMER_PERIOD);
    mock().checkExpectations();

    //the single shot timer has fired, so now inactive
    CHECK_FALSE(mUnderTest->TimerIsActive(handle));
}

TEST(FakeTimersTests, timer_stop_will_stop_the_timer)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);
    CHECK_TRUE(mUnderTest->TimerIsActive(handle));

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(TEST_PERIOD / 2);
    mock().checkExpectations();

    CHECK_TRUE(mUnderTest->TimerIsActive(handle));
    mUnderTest->TimerStop(handle);
    CHECK_FALSE(mUnderTest->TimerIsActive(handle));

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(TEST_PERIOD);
    mock().checkExpectations();
}

TEST(FakeTimersTests, timer_reset_will_restart_a_singleshot_timer)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);
    CHECK_TRUE(mUnderTest->TimerIsActive(handle));

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD);
    mock().checkExpectations();

    //move a bit forward
    mUnderTest->Tick();

    //reset (i.e. re-activate the single shot timer)
    CHECK_TRUE(mUnderTest->TimerReset(handle));

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->MoveTimeForward(TEST_PERIOD);
    mock().checkExpectations();
}

TEST(FakeTimersTests, timer_reset_will_restart_a_repeating_timer)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    auto handle = CreateAndStartAutoReload(TEST_PERIOD);
    CHECK_TRUE(mUnderTest->TimerIsActive(handle));

    //move time a bit forward
    mUnderTest->Tick();

    //reset (i.e. re-activate the timer)
    CHECK_TRUE(mUnderTest->TimerReset(handle));

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(TEST_PERIOD - DEFAULT_SYS_TICK_PERIOD);
    mock().checkExpectations();

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->Tick();
    mock().checkExpectations();
}

TEST(FakeTimersTests, timer_change_period_changes_the_period)
{
    const auto TEST_PERIOD = DEFAULT_TIMER_PERIOD;
    auto handle = CreateAndStartAutoReload(TEST_PERIOD);

    //move time a bit
    mUnderTest->Tick();

    CHECK_TRUE(mUnderTest->TimerChangePeriod(handle, 1s));

    mock().expectNoCall("testCallback");
    mUnderTest->MoveTimeForward(1s - DEFAULT_SYS_TICK_PERIOD);
    mock().checkExpectations();

    mock().expectOneCall("testCallback").withParameter("handle", handle);
    mUnderTest->Tick();
    mock().checkExpectations();
}

TEST(FakeTimersTests, get_expiry_time_method)
{
    mock().ignoreOtherCalls();

    const auto TEST_PERIOD = 2s;
    auto handle = CreateAndStartAutoReload(TEST_PERIOD);
    mUnderTest->MoveTimeForward(1s);

    auto expiryTime = mUnderTest->TimerGetExpiryTime(handle);
    CHECK_TRUE(2s == expiryTime);

    mUnderTest->MoveTimeForward(2s);
    expiryTime = mUnderTest->TimerGetExpiryTime(handle);
    CHECK_TRUE(4s == expiryTime);
}

TEST(FakeTimersTests, get_expiry_time_method_returns_negative_value_if_timer_is_expired)
{
    mock().ignoreOtherCalls();

    const auto TEST_PERIOD = 1s;
    auto handle = CreateAndStartSingleShot(TEST_PERIOD);
    mUnderTest->MoveTimeForward(2s);

    auto expiryTime = mUnderTest->TimerGetExpiryTime(handle);
    CHECK_TRUE(expiryTime < 0ns);
}

static void PendTestFunc(FakeTimers::Context context, uint32_t param2)
{
    mock().actualCall("PendTestFunc")
    .withParameter("context", context)
    .withParameter("param2", param2);
}

TEST(FakeTimersTests, supports_pend_function_call_on_next_tick)
{
    static int SomeContextObject = 3;
    const uint32_t MyTestParam2 = 22;
    const auto TEST_PERIOD = 1s;
    CreateAndStartSingleShot(TEST_PERIOD);

    CHECK_TRUE(mUnderTest->PendFunctionCall(PendTestFunc, &SomeContextObject, MyTestParam2));

    mock().expectOneCall("PendTestFunc")
             .withParameter("context", &SomeContextObject)
             .withParameter("param2", MyTestParam2);
    mock().expectOneCall("testCallback").ignoreOtherParameters();
    mUnderTest->MoveTimeForward(2s);
    mock().checkExpectations();

    mock().expectNoCall("PendTestFunc");
    mUnderTest->Tick();
    mock().checkExpectations();
}

TEST(FakeTimersTests, supports_pend_function_call_on_next_tick_and_pendables_executed_fifo)
{
    static int SomeContextObject = 3;
    const uint32_t MyTestParam2a = 22;
    const uint32_t MyTestParam2b = 33;
    const auto TEST_PERIOD = 1s;

    CHECK_TRUE(mUnderTest->PendFunctionCall(PendTestFunc, &SomeContextObject, MyTestParam2a));
    CHECK_TRUE(mUnderTest->PendFunctionCall(PendTestFunc, &SomeContextObject, MyTestParam2b));

    mock().expectOneCall("PendTestFunc")
            .withParameter("context", &SomeContextObject)
            .withParameter("param2", MyTestParam2a);
    mock().expectOneCall("PendTestFunc")
            .withParameter("context", &SomeContextObject)
            .withParameter("param2", MyTestParam2b);
    mUnderTest->Tick();
    mock().checkExpectations();

    mock().expectNoCall("PendTestFunc");
    mUnderTest->Tick();
    mock().checkExpectations();
}
