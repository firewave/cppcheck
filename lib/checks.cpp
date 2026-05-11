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

#include "checks.h"

#include "check64bit.h"
#include "checkassert.h"
#include "checkautovariables.h"
#include "checkbool.h"
#include "checkbufferoverrun.h"
#include "checkclass.h"
#include "checkcondition.h"
#include "checkexceptionsafety.h"
#include "checkfunctions.h"
#include "checkinternal.h"
#include "checkio.h"
#include "checkleakautovar.h"
#include "checkmemoryleak.h"
#include "checknullpointer.h"
#include "checkother.h"
#include "checkpostfixoperator.h"
#include "checksizeof.h"
#include "checkstl.h"
#include "checkstring.h"
#include "checktype.h"
#include "checkuninitvar.h"
#include "checkunusedvar.h"
#include "checkvaarg.h"

const std::list<Check *>& CheckInstances::get()
{
    static std::list<Check *> s_checks{
        new Check64BitPortability,
        new CheckAssert,
        new CheckAutoVariables,
        new CheckBool,
        new CheckBufferOverrun,
        new CheckClass,
        new CheckCondition,
        new CheckExceptionSafety,
        new CheckFunctions,
#ifdef CHECK_INTERNAL
        new CheckInternal,
#endif
        new CheckIO,
        new CheckLeakAutoVar,
        new CheckMemoryLeakInFunction,
        new CheckMemoryLeakInClass,
        new CheckMemoryLeakStructMember,
        new CheckMemoryLeakNoVar,
        new CheckNullPointer,
        new CheckOther,
        new CheckPostfixOperator,
        new CheckSizeof,
        new CheckStl,
        new CheckString,
        new CheckType,
        new CheckUninitVar,
        new CheckUnusedVar,
        new CheckVaarg
    };

    return s_checks;
}
