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

using TimerHandle = uint32_t;
using TimerCallback = std::function<void(TimerHandle)>;

enum class TimerBehavior
{
    SingleShot,
    AutoReload
};

class FakeTimers
{
public:
    explicit FakeTimers(std::chrono::milliseconds sysTickPeriod = std::chrono::milliseconds(10)) :
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
     * Create a timer. Modeled after FreeRTOS xTimerCreate
     * @param timerName - a string
     * @param period - time period in milliseconds for this timer
     * @param behavior - single shot or repeating?
     * @param context - a user provided context
     * @param callback - the callback to execute when the timer fires
     * @return - handle to the created timer.
     *           Zero (0) means an error, for example, the
     *           period must be modulo the configured sys tick
     *           period.
     */
    TimerHandle TimerCreate(
            const char * const timerName,
            const std::chrono::milliseconds& period,
            const TimerBehavior behavior,
            void * const context,
            const TimerCallback& callback)
    {
        using namespace std::chrono_literals;

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
    bool TimerDelete(TimerHandle handle)
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
    bool TimerStart(TimerHandle handle)
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
     * Similar to FreeRTOS pvTimerGetTimerID
     * @param handle
     * @return: the user context provided when the timer was created
     */
    void * GetTimerContext(TimerHandle handle)
    {
        Timer& timer = mTimers.at(handle - 1);
        assert(timer.handle == handle);
        assert(timer.allocated);

        return timer.context;
    }

    /**
     * Check if a timer is currently active or not
     * @param handle - the timer handle
     * @return true: timer is active and could fire in the future
     *        false: timer not allocated or not started or
     *               is a single shot timer that has fired
     *               and not yet been restarted.
     */
    bool IsTimerActive(TimerHandle handle)
    {
        using namespace std::chrono_literals;

        Timer& timer = mTimers.at(handle - 1);
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
    void MoveTimeForward(const std::chrono::milliseconds& time)
    {
        using namespace std::chrono_literals;

        std::chrono::milliseconds remaining = time;

        while (remaining > std::chrono::milliseconds(0))
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

private:
    struct Timer
    {
        const char * name = nullptr;
        std::chrono::milliseconds period = {};
        TimerBehavior behavior = TimerBehavior::SingleShot;
        void * context = nullptr;
        TimerCallback callback = nullptr;

        TimerHandle handle = 0;
        bool allocated = false;
        std::chrono::milliseconds next = {};
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
                timer.callback(timer.handle);
            }

            if (timer.behavior == TimerBehavior::AutoReload)
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
    const std::chrono::milliseconds mSysTickPeriod;
    std::chrono::milliseconds mCurrent;
};

} //namespace test
} //namespace cms

#endif //CMS_TEST_FAKE_TIMERS_HPP
