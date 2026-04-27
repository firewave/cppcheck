/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2026 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "timer.h"

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

namespace {
    // TODO: remove and print through (synchronized) ErrorLogger instead
    std::mutex stdCoutLock;
}

Timer::Timer(std::string str, TimerResultsIntf* timerResults)
    : mName(std::move(str))
    , mResults(timerResults)
{
    if (!mResults)
        return;
    mStart = Clock::now();
}

Timer::~Timer()
{
    stop();
}

void Timer::stop()
{
    if (mStart == TimePoint{})
        return;

    const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - mStart);
    mResults->addResults(mName, diff);

    mStart = TimePoint{}; // prevent multiple stops
}

static std::string durationToString(std::chrono::milliseconds duration)
{
    // Extract hours
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours; // Subtract the extracted hours

    // Extract minutes
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes; // Subtract the extracted minutes

    // Extract seconds
    std::chrono::duration<double> seconds = std::chrono::duration_cast<std::chrono::duration<double>>(duration);

    std::string ellapsedTime;
    if (hours.count() > 0)
        ellapsedTime += std::to_string(hours.count()) + "h ";
    if (minutes.count() > 0)
        ellapsedTime += std::to_string(minutes.count()) + "m ";
    std::string secondsStr{std::to_string(seconds.count())};
    auto pos = secondsStr.find_first_of('.');
    if (pos != std::string::npos && (pos + 4) < secondsStr.size())
        secondsStr.resize(pos + 4); // keep three decimal
    return (ellapsedTime + secondsStr + "s");
}

OneShotTimer::OneShotTimer(std::string name)
{
    class MyResults : public TimerResultsIntf
    {
    private:
        void addResults(const std::string &name, std::chrono::milliseconds duration) override
        {
            std::lock_guard<std::mutex> l(stdCoutLock);

            // TODO: do not use std::cout directly
            std::cout << name << ": " << durationToString(duration) << std::endl;
        }
    };

    mResults.reset(new MyResults);
    mTimer.reset(new Timer(std::move(name), mResults.get()));
}
