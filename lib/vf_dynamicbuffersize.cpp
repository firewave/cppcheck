/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2024 Cppcheck team.
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

#include "vf_dynamicbuffersize.h"

#include "astutils.h"
#include "library.h"
#include "mathlib.h"
#include "settings.h"
#include "symboldatabase.h"
#include "token.h"
#include "vfvalue.h"

#include "vf_forward.h"

#include <list>
#include <string>
#include <utility>
#include <vector>

namespace ValueFlow
{
    void analyzeDynamicBufferSize(const TokenList& tokenlist, const SymbolDatabase& symboldatabase, ErrorLogger& errorLogger, const Settings& settings)
    {
        auto getBufferSizeFromAllocFunc = [&](const Token* funcTok) -> MathLib::bigint {
            MathLib::bigint sizeValue = -1;
            const Library::AllocFunc* allocFunc = settings.library.getAllocFuncInfo(funcTok);
            if (!allocFunc)
                allocFunc = settings.library.getReallocFuncInfo(funcTok);
            if (!allocFunc || allocFunc->bufferSize == Library::AllocFunc::BufferSize::none)
                return sizeValue;

            const std::vector<const Token*> args = getArguments(funcTok);

            const Token* const arg1 = (args.size() >= allocFunc->bufferSizeArg1) ? args[allocFunc->bufferSizeArg1 - 1] : nullptr;
            const Token* const arg2 = (args.size() >= allocFunc->bufferSizeArg2) ? args[allocFunc->bufferSizeArg2 - 1] : nullptr;

            switch (allocFunc->bufferSize) {
            case Library::AllocFunc::BufferSize::none:
                break;
            case Library::AllocFunc::BufferSize::malloc:
                if (arg1 && arg1->hasKnownIntValue())
                    sizeValue = arg1->getKnownIntValue();
                break;
            case Library::AllocFunc::BufferSize::calloc:
                if (arg1 && arg2 && arg1->hasKnownIntValue() && arg2->hasKnownIntValue())
                    sizeValue = arg1->getKnownIntValue() * arg2->getKnownIntValue();
                break;
            case Library::AllocFunc::BufferSize::strdup:
                if (arg1 && arg1->hasKnownValue()) {
                    const ValueFlow::Value& value = arg1->values().back();
                    if (value.isTokValue() && value.tokvalue->tokType() == Token::eString)
                        sizeValue = Token::getStrLength(value.tokvalue) + 1; // Add one for the null terminator
                }
                break;
            }
            return sizeValue;
        };

        auto getBufferSizeFromNew = [&](const Token* newTok) -> MathLib::bigint {
            MathLib::bigint sizeValue = -1, numElem = -1;

            if (newTok && newTok->astOperand1()) { // number of elements
                const Token* bracTok = nullptr, *typeTok = nullptr;
                if (newTok->astOperand1()->str() == "[")
                    bracTok = newTok->astOperand1();
                else if (Token::Match(newTok->astOperand1(), "(|{")) {
                    if (newTok->astOperand1()->astOperand1() && newTok->astOperand1()->astOperand1()->str() == "[")
                        bracTok = newTok->astOperand1()->astOperand1();
                    else
                        typeTok = newTok->astOperand1()->astOperand1();
                }
                else
                    typeTok = newTok->astOperand1();
                if (bracTok && bracTok->astOperand2() && bracTok->astOperand2()->hasKnownIntValue())
                    numElem = bracTok->astOperand2()->getKnownIntValue();
                else if (Token::Match(typeTok, "%type%"))
                    numElem = 1;
            }

            if (numElem >= 0 && newTok->astParent() && newTok->astParent()->isAssignmentOp()) { // size of the allocated type
                const Token* typeTok = newTok->astParent()->astOperand1(); // TODO: implement fallback for e.g. "auto p = new Type;"
                if (!typeTok || !typeTok->varId())
                    typeTok = newTok->astParent()->previous(); // hack for "int** z = ..."
                if (typeTok && typeTok->valueType()) {
                    const MathLib::bigint typeSize = typeTok->valueType()->typeSize(settings.platform, typeTok->valueType()->pointer > 1);
                    if (typeSize >= 0)
                        sizeValue = numElem * typeSize;
                }
            }
            return sizeValue;
        };

        for (const Scope *functionScope : symboldatabase.functionScopes) {
            for (const Token *tok = functionScope->bodyStart; tok != functionScope->bodyEnd; tok = tok->next()) {
                if (!Token::Match(tok, "[;{}] %var% ="))
                    continue;

                if (!tok->next()->variable())
                    continue;

                const Token *rhs = tok->tokAt(2)->astOperand2();
                while (rhs && rhs->isCast())
                    rhs = rhs->astOperand2() ? rhs->astOperand2() : rhs->astOperand1();
                if (!rhs)
                    continue;

                const bool isNew = rhs->isCpp() && rhs->str() == "new";
                if (!isNew && !Token::Match(rhs->previous(), "%name% ("))
                    continue;

                const MathLib::bigint sizeValue = isNew ? getBufferSizeFromNew(rhs) : getBufferSizeFromAllocFunc(rhs->previous());
                if (sizeValue < 0)
                    continue;

                ValueFlow::Value value(sizeValue);
                value.errorPath.emplace_back(tok->tokAt(2), "Assign " + tok->strAt(1) + ", buffer with size " + std::to_string(sizeValue));
                value.valueType = ValueFlow::Value::ValueType::BUFFER_SIZE;
                value.setKnown();
                valueFlowForward(const_cast<Token*>(rhs), functionScope->bodyEnd, tok->next(), std::move(value), tokenlist, errorLogger, settings);
            }
        }
    }
}
