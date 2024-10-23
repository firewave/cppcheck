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

#include "vf_symbolic.h"

#include "astutils.h"
#include "config.h"
#include "settings.h"
#include "symboldatabase.h"
#include "token.h"
#include "valueflow.h"
#include "vfvalue.h"

#include "vf_common.h"
#include "vf_forward.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ValueFlow
{
    static bool isTruncated(const ValueType* src, const ValueType* dst, const Settings& settings)
    {
        if (src->pointer > 0 || dst->pointer > 0)
            return src->pointer != dst->pointer;
        if (src->smartPointer && dst->smartPointer)
            return false;
        if ((src->isIntegral() && dst->isIntegral()) || (src->isFloat() && dst->isFloat())) {
            const size_t srcSize = getSizeOf(*src, settings);
            const size_t dstSize = getSizeOf(*dst, settings);
            if (srcSize > dstSize)
                return true;
            if (srcSize == dstSize && src->sign != dst->sign)
                return true;
        } else if (src->type == dst->type) {
            if (src->type == ValueType::Type::RECORD)
                return src->typeScope != dst->typeScope;
        } else {
            return true;
        }
        return false;
    }

    static std::set<nonneg int> getVarIds(const Token* tok)
    {
        std::set<nonneg int> result;
        visitAstNodes(tok, [&](const Token* child) {
            if (child->varId() > 0)
                result.insert(child->varId());
            return ChildrenToVisit::op1_and_op2;
        });
        return result;
    }

    void analyzeSymbolic(const TokenList& tokenlist, const SymbolDatabase& symboldatabase, ErrorLogger& errorLogger, const Settings& settings)
    {
        for (const Scope* scope : symboldatabase.functionScopes) {
            for (auto* tok = const_cast<Token*>(scope->bodyStart); tok != scope->bodyEnd; tok = tok->next()) {
                if (!Token::simpleMatch(tok, "="))
                    continue;
                if (tok->astParent())
                    continue;
                if (!tok->astOperand1())
                    continue;
                if (!tok->astOperand2())
                    continue;
                if (tok->astOperand1()->hasKnownIntValue())
                    continue;
                if (tok->astOperand2()->hasKnownIntValue())
                    continue;
                if (tok->astOperand1()->exprId() == 0)
                    continue;
                if (tok->astOperand2()->exprId() == 0)
                    continue;
                if (!isConstExpression(tok->astOperand2(), settings.library))
                    continue;
                if (tok->astOperand1()->valueType() && tok->astOperand2()->valueType()) {
                    if (isTruncated(
                            tok->astOperand2()->valueType(), tok->astOperand1()->valueType(), settings))
                        continue;
                } else if (isDifferentType(tok->astOperand2(), tok->astOperand1())) {
                    continue;
                }
                const std::set<nonneg int> rhsVarIds = getVarIds(tok->astOperand2());
                const std::vector<const Variable*> vars = getLHSVariables(tok);
                if (std::any_of(vars.cbegin(), vars.cend(), [&](const Variable* var) {
                    if (rhsVarIds.count(var->declarationId()) > 0)
                        return true;
                    if (var->isLocal())
                        return var->isStatic();
                    return !var->isArgument();
                }))
                    continue;

                if (findAstNode(tok, [](const Token* child) {
                    return child->isIncompleteVar();
                }))
                    continue;

                Token* start = nextAfterAstRightmostLeaf(tok);
                const Token* end = getEndOfExprScope(tok->astOperand1(), scope);

                Value rhs = makeSymbolic(tok->astOperand2());
                rhs.errorPath.emplace_back(tok,
                                           tok->astOperand1()->expressionString() + " is assigned '" +
                                           tok->astOperand2()->expressionString() + "' here.");
                valueFlowForward(start, end, tok->astOperand1(), std::move(rhs), tokenlist, errorLogger, settings);

                Value lhs = makeSymbolic(tok->astOperand1());
                lhs.errorPath.emplace_back(tok,
                                           tok->astOperand1()->expressionString() + " is assigned '" +
                                           tok->astOperand2()->expressionString() + "' here.");
                valueFlowForward(start, end, tok->astOperand2(), std::move(lhs), tokenlist, errorLogger, settings);
            }
        }
    }
}
