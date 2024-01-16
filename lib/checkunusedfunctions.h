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


//---------------------------------------------------------------------------
#ifndef checkunusedfunctionsH
#define checkunusedfunctionsH
//---------------------------------------------------------------------------

#include "config.h"

#include <string>

class ErrorLogger;
class Function;
class Settings;
class Tokenizer;

/** @brief Check for functions never called */
/// @{

class CPPCHECKLIB CheckUnusedFunctions {
    friend class TestUnusedFunctions;

public:
    CheckUnusedFunctions();

    // Parse current tokens and determine..
    // * Check what functions are used
    // * What functions are declared
    void parseTokens(const Tokenizer &tokenizer, const char FileName[], const Settings &settings);
    std::string analyzerInfo() const;

    static void parseTokens(const Tokenizer &tokenizer, const Settings &settings);

    static void analyseWholeProgram(const Settings &settings, ErrorLogger& errorLogger, const std::string &buildDir);

    static void getErrorMessages(ErrorLogger &errorLogger);

    static bool check(const Settings& settings, ErrorLogger &errorLogger);

private:
    // Return true if an error is reported.
    bool check(ErrorLogger& errorLogger, const Settings& settings) const;

    void* mImpl;
};
/// @}
//---------------------------------------------------------------------------
#endif // checkunusedfunctionsH
