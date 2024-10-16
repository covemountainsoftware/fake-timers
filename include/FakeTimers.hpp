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

#ifndef CMS_TEST_FAKE_TIMERS_HPP
#define CMS_TEST_FAKE_TIMERS_HPP

#include <cstdint>
#include <chrono>
#include <functional>
#include <vector>
#include <cassert>

namespace cms {
namespace test {

/**
 * The FakeTimers class implements a software timer functionality loosely equivalent
 * to the Timer API provided by FreeRTOS. The intention, and the reason
 * it is called "Fake", is for this to be used in a unit testing environment
 * to replace RTOS timer functionality, giving unit tests the ability to
 * control "time" during unit testing.
 *
 * @note: Underlying timebase uses std::chrono::nanoseconds.
 *        Sorry, unit testing is limited to 292 years of so of time.
 *        Overflow is not accounted for!
 */
class FakeTimers
{
public:
    using Handle = uint32_t;
    using Context = void*;
    using Callback = std::function<void(Handle, Context)>;
    using Duration = std::chrono::nanoseconds;

    enum class Behavior
    {
        SingleShot,
        AutoReload
    };

    explicit FakeTimers(Duration sysTickPeriod = std::chrono::milliseconds(10)) :
       mTimers(),
       mSysTickPeriod(sysTickPeriod),
       mCurrent(0)
    {
       mTimers.resize(25);
    }

    virtual ~FakeTimers() = default;
    FakeTimers(FakeTimers const &) = delete;
    void operator=(FakeTimers const &x) = delete;

    /**
     * Create a timer.
     * @param timerName - a string
     * @param period - time period in milliseconds for this timer
     * @param behavior - single shot or repeating?
     * @param context - a user provided context
     * @param callback - the callback to execute when the timer fires
     * @return - handle to the created timer.
     *           Zero (0) means an error, for example, the
     *           period must be modulo the configured sys tick
     *           period.
     * @note: Reference FreeRTOS xTimerCreate
     */
    Handle TimerCreate(
            const char * timerName,
            const Duration& period,
            const Behavior behavior,
            Context context,
            const Callback& callback)
    {
        using namespace std::chrono_literals;

        if (period <= 0ms)
        {
            return false;
        }

        if ((period.count() % mSysTickPeriod.count()) != 0)
        {
            return 0;
        }

        auto newTimerIndex = FindAvailableTimer();
        Timer& newTimer = mTimers.at(newTimerIndex);
        newTimer.name = timerName;
        newTimer.period = period;
        newTimer.behavior = behavior;
        newTimer.context = context;
        newTimer.callback = callback;
        newTimer.allocated = true;
        newTimer.handle = newTimerIndex + 1;
        newTimer.next = 0ms;
        return newTimer.handle;
    }

    /**
     * Delete a timer.
     * @param handle
     * @return true, deleted as expected. false, some error.
     */
    bool TimerDelete(Handle handle)
    {
        if (handle > mTimers.size())
        {
            return false;
        }

        if (handle == 0)
        {
            return false;
        }

        uint32_t index = handle - 1;
        mTimers.at(index) = Timer();
        return true;
    }

    /**
     * Start a timer
     * @param handle
     * @return true: started ok. false: some error.
     */
    bool TimerStart(Handle handle)
    {
        if (handle > mTimers.size())
        {
            return false;
        }

        if (handle == 0)
        {
            return false;
        }

        Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);

        timer.next = mCurrent + timer.period;

        return true;
    }

    /**
     * Stop a timer.
     * @param handle
     * @return true: timer was found and stopped.
     *         false: some error, such as non-existent timer.
     */
    bool TimerStop(Handle handle)
    {
        using namespace std::chrono_literals;

        if (handle > mTimers.size())
        {
            return false;
        }

        if (handle == 0)
        {
            return false;
        }

        Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);

        //stop the timer
        timer.next = 0ms;

        return true;
    }

    /**
     * Re-starts a timer that was previously created with TimerCreate() API function.
     * @param handle
     * @return
     * @note: Reference FreeRTOS xTimerReset
     */
    bool TimerReset(Handle handle)
    {
       return TimerStart(handle);
    }

    /**
     * Change a timer's period setting.
     * @param handle
     * @param newPeriod
     * @return: true: changed ok.  false: some error.
     * @note: Reference FreeRTOS xTimerChangePeriod
     */
    bool TimerChangePeriod(Handle handle, Duration newPeriod)
    {
        using namespace std::chrono_literals;

        if (handle > mTimers.size())
        {
            return false;
        }

        if (handle == 0)
        {
            return false;
        }

        if (newPeriod <= 0ms)
        {
            return false;
        }

        Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);

        timer.period = newPeriod;
        timer.next = mCurrent + timer.period;

        return true;
    }

    /**
     * Change a timer's behavior setting.
     * Does not impact timer's active status.
     * @param handle
     * @param behavior
     * @return: true: changed ok.  false: some error.
     * @note: Reference FreeRTOS vTimerSetReloadMode
     */
    bool TimerSetBehavior(Handle handle, Behavior behavior)
    {
        using namespace std::chrono_literals;

        if (handle > mTimers.size())
        {
            return false;
        }

        if (handle == 0)
        {
            return false;
        }

        Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);

        timer.behavior = behavior;
        return true;
    }

    /**
     * Get the timer's user context.
     * @param handle
     * @return: the user context provided when the timer was created
     * @note: Reference FreeRTOS pvTimerGetTimerID
     */
    Context TimerGetContext(Handle handle) const
    {
        const Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);
        assert(timer.allocated);

        return timer.context;
    }

    /**
     * Get the timer's name.
     * @param handle
     * @return the timer name provided when created.
     * @note: Reference FreeRTOS pcTimerGetName
     */
    const char * TimerGetName(Handle handle) const
    {
        const Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);
        assert(timer.allocated);

        return timer.name;
    }

    /**
     * Get the timer's period setting.
     * @param handle
     * @return
     * @note: Reference FreeRTOS xTimerGetPeriod
     */
    Duration TimerGetPeriod(Handle handle) const
    {
        const Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);
        assert(timer.allocated);

        return timer.period;
    }

    /**
     * Get the timer's behavior setting.
     * @param handle
     * @return
     * @note: Reference FreeRTOS xTimerGetReloadMode
     */
    Behavior TimerGetBehavior(Handle handle) const
    {
        const Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);
        assert(timer.allocated);

        return timer.behavior;
    }

    /**
     * Get the next time tick on the internal time base
     * when this timer will fire/expire next.
     * @param handle
     * @return the tick.  Will be negative if the timer is not active.
     */
    Duration TimerGetExpiryTime(Handle handle) const
    {
        const Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);
        assert(timer.allocated);

        if (TimerIsActive(handle))
        {
            return timer.next;
        }
        else
        {
            return Duration(-1);
        }
    }


    /**
     * Check if a timer is currently active or not
     * @param handle - the timer handle
     * @return true: timer is active and could fire in the future
     *        false: timer not allocated or not started or
     *               is a single shot timer that has fired
     *               and not yet been restarted.
     * @note: Reference FreeRTOS xTimerIsTimerActive
     */
    bool TimerIsActive(Handle handle) const
    {
        using namespace std::chrono_literals;

        const Timer& timer = mTimers.at(handle - 1);
        if (!timer.allocated)
        {
            return false;
        }

        return timer.next != 0ms;
    }

    /**
     * Move time forward. Timers only have an opportunity
     * to fire based on sys tick period.
     * @param time
     */
    void MoveTimeForward(const Duration& time)
    {
        using namespace std::chrono_literals;

        Duration remaining = time;

        while (remaining > Duration(0))
        {
            auto this_delta = std::min(remaining, mSysTickPeriod);
            mCurrent += this_delta;
            remaining -= this_delta;

            for (auto& timer : mTimers)
            {
                ConsiderFiringTimer(timer);
            }
        }
    }

    /**
     * Convenience method which moves time forward
     * one sys tick period.
     */
    void Tick()
    {
        MoveTimeForward(mSysTickPeriod);
    }

    /**
     * Get the internal current time base
     */
    Duration GetCurrentInternalTime() const
    {
        return mCurrent;
    }

private:
    struct Timer
    {
        const char * name = nullptr;
        Duration period = {};
        Behavior behavior = Behavior::SingleShot;
        Context context = nullptr;
        Callback callback = nullptr;

        Handle handle = 0;
        bool allocated = false;
        Duration next = {};
    };

    uint32_t FindAvailableTimer()
    {
        for (uint32_t i = 0; i < mTimers.size(); ++i)
        {
            if (!mTimers[i].allocated)
            {
                return i;
            }
        }

        //allow growth, this isn't embedded after all
        mTimers.emplace_back();
        return mTimers.size() - 1;
    }

    void ConsiderFiringTimer(Timer& timer)
    {
        using namespace std::chrono_literals;

        if (!timer.allocated)
        {
            return;
        }

        if (timer.period <= 0ms)
        {
            return;
        }

        if (timer.next <= 0ms)
        {
            return;
        }

        if (mCurrent >= timer.next)
        {
            //fire away
            if (timer.callback != nullptr)
            {
                timer.callback(timer.handle, timer.context);
            }

            if (timer.behavior == Behavior::AutoReload)
            {
                timer.next = mCurrent + timer.period;
            }
            else
            {
                timer.next = 0ms;
            }
        }
    }

    std::vector<Timer> mTimers;
    const Duration mSysTickPeriod;
    Duration mCurrent;
};

} //namespace test
} //namespace cms

#endif //CMS_TEST_FAKE_TIMERS_HPP
