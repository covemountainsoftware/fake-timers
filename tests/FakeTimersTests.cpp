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

//must be last
#include "CppUTest/TestHarness.h"

using namespace std::chrono_literals;

TEST_GROUP(FakeTimersTests) {
    std::unique_ptr<cms::test::FakeTimers> mUnderTest;

    void setup() final
    {
        mUnderTest = std::make_unique<cms::test::FakeTimers>();
    }

    void teardown() final
    {
    }

    static void dummyCallback(cms::test::TimerHandle handle)
    {
        (void)handle;
    }
};

TEST(FakeTimersTests, can_compile)
{
}

TEST(FakeTimersTests, can_create_a_timer)
{
    using namespace cms::test;
    auto handle = mUnderTest->TimerCreate("TEST", 100ms, TimerBehavior::SingleShot, nullptr, dummyCallback);
    CHECK_TRUE(handle != 0);
}