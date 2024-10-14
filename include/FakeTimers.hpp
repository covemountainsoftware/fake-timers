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
#include <list>

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
    FakeTimers() = default;
    virtual ~FakeTimers() = default;
    FakeTimers(FakeTimers const &) = delete;
    void operator=(FakeTimers const &x) = delete;

    TimerHandle TimerCreate(
            const char * const timerName,
            const std::chrono::milliseconds& period,
            const TimerBehavior behavior,
            void * const context,
            const TimerCallback& callback)
    {
        Timer newTimer;
        newTimer.name = timerName;
        newTimer.period = period;
        newTimer.behavior = behavior;
        newTimer.context = context;
        newTimer.callback = callback;
        mTimers.push_back(newTimer);

        return mTimers.size();
    }

private:
    struct Timer
    {
        const char * name = nullptr;
        std::chrono::milliseconds period = {};
        TimerBehavior behavior = TimerBehavior::SingleShot;
        void * context = nullptr;
        TimerCallback callback = nullptr;
    };

    std::list<Timer> mTimers;
};

} //namespace test
} //namespace cms

#endif //CMS_TEST_FAKE_TIMERS_HPP
