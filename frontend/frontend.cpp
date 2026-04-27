/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2025 Cppcheck team.
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

#include "frontend.h"

#include "filesettings.h"
#include "library.h"
#include "path.h"
#include "settings.h"

#include <cassert>
#include <iostream>

namespace frontend {
    void applyLang(std::list<FileSettings>& fileSettings, const Settings& settings, Standards::Language enforcedLang)
    {
        if (enforcedLang != Standards::Language::None)
        {
            // apply enforced language
            for (auto& fs : fileSettings)
            {
                if (settings.library.markupFile(fs.filename()))
                    continue;
                fs.file.setLang(enforcedLang);
            }
        }
        else
        {
            // identify files
            for (auto& fs : fileSettings)
            {
                if (settings.library.markupFile(fs.filename()))
                    continue;
                assert(fs.file.lang() == Standards::Language::None);
                bool header = false;
                fs.file.setLang(Path::identify(fs.filename(), settings.cppHeaderProbe, &header));
                // unknown extensions default to C++
                if (!header && fs.file.lang() == Standards::Language::None)
                    fs.file.setLang(Standards::Language::CPP);
            }
        }

        // enforce the language since markup files are special and do not adhere to the enforced language
        for (auto& fs : fileSettings)
        {
            if (settings.library.markupFile(fs.filename())) {
                assert(fs.file.lang() == Standards::Language::None);
                fs.file.setLang(Standards::Language::C);
            }
        }
    }

    void applyLang(std::list<FileWithDetails>& files, const Settings& settings, Standards::Language enforcedLang)
    {
        if (enforcedLang != Standards::Language::None)
        {
            // apply enforced language
            for (auto& f : files)
            {
                if (settings.library.markupFile(f.path()))
                    continue;
                f.setLang(enforcedLang);
            }
        }
        else
        {
            // identify remaining files
            for (auto& f : files)
            {
                if (f.lang() != Standards::Language::None)
                    continue;
                if (settings.library.markupFile(f.path()))
                    continue;
                bool header = false;
                f.setLang(Path::identify(f.path(), settings.cppHeaderProbe, &header));
                // unknown extensions default to C++
                if (!header && f.lang() == Standards::Language::None)
                    f.setLang(Standards::Language::CPP);
            }
        }

        // enforce the language since markup files are special and do not adhere to the enforced language
        for (auto& f : files)
        {
            if (settings.library.markupFile(f.path())) {
                f.setLang(Standards::Language::C);
            }
        }
    }
}

namespace {
    using dataElementType = std::pair<std::string, TimerResultsData>;
    bool more_second_sec(const dataElementType& lhs, const dataElementType& rhs)
    {
        return lhs.second.getSeconds() > rhs.second.getSeconds();
    }
    // TODO: remove and print through (synchronized) ErrorLogger instead
    std::mutex stdCoutLock;
}

// TODO: this does not include any file context when SHOWTIME_FILE thus rendering it useless - should we include the logging with the progress logging?
// that could also get rid of the broader locking
void TimerResults::showResults(size_t max_results, bool metrics) const
{
    std::vector<dataElementType> data;
    {
        std::lock_guard<std::mutex> l(mResultsSync);

        data.reserve(mResults.size());
        data.insert(data.begin(), mResults.cbegin(), mResults.cend());
    }
    std::sort(data.begin(), data.end(), more_second_sec);

    // lock the whole logging operation to avoid multiple threads printing their results at the same time
    std::lock_guard<std::mutex> l(stdCoutLock);

    size_t ordinal = 1; // maybe it would be nice to have an ordinal in output later!
    for (auto iter=data.cbegin(); iter!=data.cend(); ++iter) {
        if (ordinal <= max_results) {
            const double sec = iter->second.getSeconds().count();
            std::cout << iter->first << ": " << sec << "s";
            if (metrics) {
                const double secAverage = sec / static_cast<double>(iter->second.mResults.size());
                const double secMin = asSeconds(*std::min_element(iter->second.mResults.cbegin(), iter->second.mResults.cend())).count();
                const double secMax = asSeconds(*std::max_element(iter->second.mResults.cbegin(), iter->second.mResults.cend())).count();
                std::cout << " (avg. " << secAverage << "s / min " << secMin << "s / max " << secMax << "s - " << iter->second.mResults.size() << " result(s))";
            }
            std::cout << std::endl;
        }
        ++ordinal;
    }
}

void TimerResults::addResults(const std::string& name, std::chrono::milliseconds duration)
{
    std::lock_guard<std::mutex> l(mResultsSync);

    mResults[name].mResults.push_back(duration);
}

void TimerResults::reset()
{
    std::lock_guard<std::mutex> l(mResultsSync);
    mResults.clear();
}
