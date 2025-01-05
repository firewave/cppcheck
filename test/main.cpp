/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2023 Cppcheck team.
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

#include "color.h"
#include "options.h"
#include "preprocessor.h"
#include "fixture.h"

#include <cstdlib>

#include <cfenv>
#include <iostream>

#ifdef _MSC_VER
#include <float.h>
#endif

int main(int argc, char *argv[])
{
    // MS Visual C++ memory leak debug tracing
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef __GNUC__
    if (feenableexcept(FE_ALL_EXCEPT) != 0)
    {
        std::cerr << "failed to enable traps for floating-point exceptions - exiting" << std::endl;
        std::exit(EXIT_FAILURE);
    }
#elif defined(_MSC_VER)
    {
        const unsigned int cw_flags = _EM_INVALID|_EM_DENORMAL|_EM_ZERODIVIDE|_EM_OVERFLOW|_EM_UNDERFLOW|_EM_INEXACT;
        if (_controlfp(cw_flags, _MCW_EM) != cw_flags)
        {
            std::cerr << "failed to set hardware floating-point exception mask - exiting" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
#endif
    if (std::feraiseexcept(FE_ALL_EXCEPT) != 0)
    {
        std::cerr << "failed to raise floating-point exceptions - exiting" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Preprocessor::macroChar = '$';     // While macroChar is char(1) per default outside test suite, we require it to be a human-readable character here.
    gDisableColors = true;

    options args(argc, argv);

    if (args.help()) {
        TestFixture::printHelp();
        return EXIT_SUCCESS;
    }
    const std::size_t failedTestsCount = TestFixture::runTests(args);
    return (failedTestsCount == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
