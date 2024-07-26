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

#ifndef conditionHandlerH
#define conditionHandlerH

#include "analyzer.h"
#include "astutils.h"
#include "programmemory.h"
#include "settings.h"
#include "valueflow.h"
#include "vfvalue.h"

#include "vf_bailout.h"
#include "vf_common.h"

#include <algorithm>
#include <list>

class Token;

struct ConditionHandler {
    static bool isConditionKnown(const Token* tok, bool then)
    {
        const char* op = "||";
        if (then)
            op = "&&";
        const Token* parent = tok->astParent();
        while (parent && (parent->str() == op || parent->str() == "!" || parent->isCast()))
            parent = parent->astParent();
        const Token* top = tok->astTop();
        if (top && Token::Match(top->previous(), "if|while|for ("))
            return parent == top || Token::simpleMatch(parent, ";");
        return parent && parent->str() != op;
    }

    struct Condition {
        const Token* vartok{};
        std::list<ValueFlow::Value> true_values;
        std::list<ValueFlow::Value> false_values;
        bool inverted = false;
        // Whether to insert impossible values for the condition or only use possible values
        bool impossible = true;

        bool isBool() const {
            return astIsBool(vartok);
        }

        static MathLib::bigint findPath(const std::list<ValueFlow::Value>& values)
        {
            auto it = std::find_if(values.cbegin(), values.cend(), [](const ValueFlow::Value& v) {
                return v.path > 0;
            });
            if (it == values.end())
                return 0;
            assert(std::all_of(it, values.end(), [&](const ValueFlow::Value& v) {
                return v.path == 0 || v.path == it->path;
            }));
            return it->path;
        }

        MathLib::bigint getPath() const
        {
            assert(std::abs(findPath(true_values) - findPath(false_values)) == 0);
            return findPath(true_values) | findPath(false_values);
        }

        static void insertImpossible(std::list<ValueFlow::Value>& values, const std::list<ValueFlow::Value>& input)
        {
            std::transform(input.cbegin(), input.cend(), std::back_inserter(values), &ValueFlow::asImpossible);
        }

        static void insertNegateKnown(std::list<ValueFlow::Value>& values, const std::list<ValueFlow::Value>& input)
        {
            for (ValueFlow::Value value:input) {
                if (!value.isIntValue() && !value.isContainerSizeValue())
                    continue;
                value.intvalue = !value.intvalue;
                value.setKnown();
                values.push_back(std::move(value));
            }
        }

        Token* getContextAndValues(Token* condTok,
                                   std::list<ValueFlow::Value>& thenValues,
                                   std::list<ValueFlow::Value>& elseValues,
                                   bool known = false) const
        {
            const MathLib::bigint path = getPath();
            const bool allowKnown = path == 0;
            const bool allowImpossible = impossible && allowKnown;

            bool inverted2 = inverted;
            Token* ctx = skipNotAndCasts(condTok, &inverted2);
            bool then = !inverted || !inverted2;

            if (!Token::Match(condTok, "!=|=|(|.") && condTok != vartok) {
                thenValues.insert(thenValues.end(), true_values.cbegin(), true_values.cend());
                if (allowImpossible && (known || isConditionKnown(ctx, !then)))
                    insertImpossible(elseValues, false_values);
            }
            if (!Token::Match(condTok, "==|!")) {
                elseValues.insert(elseValues.end(), false_values.cbegin(), false_values.cend());
                if (allowImpossible && (known || isConditionKnown(ctx, then))) {
                    insertImpossible(thenValues, true_values);
                    if (isBool())
                        insertNegateKnown(thenValues, true_values);
                }
            }

            if (inverted2)
                std::swap(thenValues, elseValues);

            return ctx;
        }
    };

    virtual std::vector<Condition> parse(const Token* tok, const Settings& settings) const = 0;

    virtual Analyzer::Result forward(Token* start,
                                     const Token* stop,
                                     const Token* exprTok,
                                     const std::list<ValueFlow::Value>& values,
                                     TokenList& tokenlist,
                                     ErrorLogger& errorLogger,
                                     const Settings& settings,
                                     SourceLocation loc = SourceLocation::current()) const
    {
        return ValueFlow::valueFlowForward(start->next(), stop, exprTok, values, tokenlist, errorLogger, settings, loc);
    }

    virtual Analyzer::Result forward(Token* top,
                                     const Token* exprTok,
                                     const std::list<ValueFlow::Value>& values,
                                     TokenList& tokenlist,
                                     ErrorLogger& errorLogger,
                                     const Settings& settings,
                                     SourceLocation loc = SourceLocation::current()) const
    {
        return valueFlowForwardRecursive(top, exprTok, values, tokenlist, errorLogger, settings, loc);
    }

    virtual void reverse(Token* start,
                         const Token* endToken,
                         const Token* exprTok,
                         const std::list<ValueFlow::Value>& values,
                         TokenList& tokenlist,
                         ErrorLogger& errorLogger,
                         const Settings& settings,
                         SourceLocation loc = SourceLocation::current()) const
    {
        ValueFlow::valueFlowReverse(start, endToken, exprTok, values, tokenlist, errorLogger, settings, loc);
    }

    void traverseCondition(const SymbolDatabase& symboldatabase,
                           const Settings& settings,
                           const std::set<const Scope*>& skippedFunctions,
                           const std::function<void(const Condition& cond, Token* tok, const Scope* scope)>& f) const
    {
        for (const Scope *scope : symboldatabase.functionScopes) {
            if (skippedFunctions.count(scope))
                continue;
            for (auto *tok = const_cast<Token *>(scope->bodyStart); tok != scope->bodyEnd; tok = tok->next()) {
                if (Token::Match(tok, "if|while|for ("))
                    continue;
                if (Token::Match(tok, ":|;|,"))
                    continue;

                const Token* top = tok->astTop();
                if (!top)
                    continue;

                if (!Token::Match(top->previous(), "if|while|for (") && !Token::Match(tok->astParent(), "&&|%oror%|?|!"))
                    continue;
                for (const Condition& cond : parse(tok, settings)) {
                    if (!cond.vartok)
                        continue;
                    if (cond.vartok->exprId() == 0)
                        continue;
                    if (cond.vartok->hasKnownIntValue())
                        continue;
                    if (cond.true_values.empty() || cond.false_values.empty())
                        continue;
                    if (!isConstExpression(cond.vartok, settings.library))
                        continue;
                    f(cond, tok, scope);
                }
            }
        }
    }

    void beforeCondition(TokenList& tokenlist,
                         const SymbolDatabase& symboldatabase,
                         ErrorLogger& errorLogger,
                         const Settings& settings,
                         const std::set<const Scope*>& skippedFunctions) const {
        traverseCondition(symboldatabase, settings, skippedFunctions, [&](const Condition& cond, Token* tok, const Scope*) {
            if (cond.vartok->exprId() == 0)
                return;

            // If condition is known then don't propagate value
            if (tok->hasKnownIntValue())
                return;

            Token* top = tok->astTop();

            if (Token::Match(top, "%assign%"))
                return;
            if (Token::Match(cond.vartok->astParent(), "%assign%|++|--"))
                return;

            if (Token::simpleMatch(tok->astParent(), "?") && tok->astParent()->isExpandedMacro()) {
                if (settings.debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            tok,
                            "variable '" + cond.vartok->expressionString() + "', condition is defined in macro");
                return;
            }

            // if,macro => bailout
            if (Token::simpleMatch(top->previous(), "if (") && top->previous()->isExpandedMacro()) {
                if (settings.debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            tok,
                            "variable '" + cond.vartok->expressionString() + "', condition is defined in macro");
                return;
            }

            std::list<ValueFlow::Value> values = cond.true_values;
            if (cond.true_values != cond.false_values)
                values.insert(values.end(), cond.false_values.cbegin(), cond.false_values.cend());

            // extra logic for unsigned variables 'i>=1' => possible value can also be 0
            if (Token::Match(tok, "<|>|<=|>=")) {
                values.remove_if([](const ValueFlow::Value& v) {
                    if (v.isIntValue())
                        return v.intvalue != 0;
                    return false;
                });
                if (cond.vartok->valueType() && cond.vartok->valueType()->sign != ValueType::Sign::UNSIGNED)
                    return;
            }
            if (values.empty())
                return;

            // bailout: for/while-condition, variable is changed in while loop
            if (Token::Match(top->previous(), "for|while (") && Token::simpleMatch(top->link(), ") {")) {

                // Variable changed in 3rd for-expression
                if (Token::simpleMatch(top->previous(), "for (")) {
                    if (top->astOperand2() && top->astOperand2()->astOperand2() &&
                        findExpressionChanged(
                                cond.vartok, top->astOperand2()->astOperand2(), top->link(), settings)) {
                        if (settings.debugwarnings)
                            bailout(tokenlist,
                                    errorLogger,
                                    tok,
                                    "variable '" + cond.vartok->expressionString() + "' used in loop");
                        return;
                    }
                }

                // Variable changed in loop code
                const Token* const start = top;
                const Token* const block = top->link()->next();
                const Token* const end = block->link();

                if (findExpressionChanged(cond.vartok, start, end, settings)) {
                    // If its reassigned in loop then analyze from the end
                    if (!Token::Match(tok, "%assign%|++|--") &&
                        findExpression(cond.vartok->exprId(), start, end, [&](const Token* tok2) {
                            return Token::Match(tok2->astParent(), "%assign%") && astIsLHS(tok2);
                        })) {
                        // Start at the end of the loop body
                        Token* bodyTok = top->link()->next();
                        reverse(bodyTok->link(), bodyTok, cond.vartok, values, tokenlist, errorLogger, settings);
                    }
                    if (settings.debugwarnings)
                        bailout(tokenlist,
                                errorLogger,
                                tok,
                                "variable '" + cond.vartok->expressionString() + "' used in loop");
                    return;
                }
            }

            Token* startTok = nullptr;
            if (astIsRHS(tok))
                startTok = tok->astParent();
            else if (astIsLHS(tok))
                startTok = previousBeforeAstLeftmostLeaf(tok->astParent());
            if (!startTok)
                startTok = tok->previous();

            reverse(startTok, nullptr, cond.vartok, values, tokenlist, errorLogger, settings);
        });
    }

    static Token* skipNotAndCasts(Token* tok, bool* inverted = nullptr)
    {
        for (; tok->astParent(); tok = tok->astParent()) {
            if (Token::simpleMatch(tok->astParent(), "!")) {
                if (inverted)
                    *inverted ^= true;
                continue;
            }
            if (Token::Match(tok->astParent(), "==|!=")) {
                const Token* sibling = tok->astSibling();
                if (sibling->hasKnownIntValue() && (astIsBool(tok) || astIsBool(sibling))) {
                    const bool value = sibling->values().front().intvalue;
                    if (inverted)
                        *inverted ^= value == Token::simpleMatch(tok->astParent(), "!=");
                    continue;
                }
            }
            if (tok->astParent()->isCast() && astIsBool(tok->astParent()))
                continue;
            return tok;
        }
        return tok;
    }

    static void fillFromPath(ProgramMemory& pm, const Token* top, MathLib::bigint path, const Settings& settings)
    {
        if (path < 1)
            return;
        visitAstNodes(top, [&](const Token* tok) {
            const ValueFlow::Value* v = ValueFlow::findValue(tok->values(), settings, [&](const ValueFlow::Value& v) {
                return v.path == path && isNonConditionalPossibleIntValue(v);
            });
            if (v == nullptr)
                return ChildrenToVisit::op1_and_op2;
            pm.setValue(tok, *v);
            return ChildrenToVisit::op1_and_op2;
        });
    }

    static void changePossibleToKnown(std::list<ValueFlow::Value>& values, int indirect = -1)
    {
        for (ValueFlow::Value& v : values) {
            if (indirect >= 0 && v.indirect != indirect)
                continue;
            if (!v.isPossible())
                continue;
            if (v.bound != ValueFlow::Value::Bound::Point)
                continue;
            v.setKnown();
        }
    }

    static void valueFlowSetConditionToKnown(const Token* tok, std::list<ValueFlow::Value>& values, bool then)
    {
        if (values.empty())
            return;
        if (then && !Token::Match(tok, "==|!|("))
            return;
        if (!then && !Token::Match(tok, "!=|%var%|("))
            return;
        if (isConditionKnown(tok, then))
            changePossibleToKnown(values);
    }

    static bool isBreakScope(const Token* const endToken)
    {
        if (!Token::simpleMatch(endToken, "}"))
            return false;
        if (!Token::simpleMatch(endToken->link(), "{"))
            return false;
        return Token::findmatch(endToken->link(), "break|goto", endToken);
    }

    void afterCondition(TokenList& tokenlist,
                        const SymbolDatabase& symboldatabase,
                        ErrorLogger& errorLogger,
                        const Settings& settings,
                        const std::set<const Scope*>& skippedFunctions) const {
        traverseCondition(symboldatabase, settings, skippedFunctions, [&](const Condition& cond, Token* condTok, const Scope* scope) {
            Token* top = condTok->astTop();

            const MathLib::bigint path = cond.getPath();
            const bool allowKnown = path == 0;

            std::list<ValueFlow::Value> thenValues;
            std::list<ValueFlow::Value> elseValues;

            Token* ctx = cond.getContextAndValues(condTok, thenValues, elseValues);

            if (Token::Match(ctx->astParent(), "%oror%|&&")) {
                Token* parent = ctx->astParent();
                if (astIsRHS(ctx) && astIsLHS(parent) && parent->astParent() &&
                    parent->str() == parent->astParent()->str())
                    parent = parent->astParent();
                else if (!astIsLHS(ctx)) {
                    parent = nullptr;
                }
                if (parent) {
                    std::vector<Token*> nextExprs = {parent->astOperand2()};
                    if (astIsLHS(parent) && parent->astParent() && parent->astParent()->str() == parent->str()) {
                        nextExprs.push_back(parent->astParent()->astOperand2());
                    }
                    std::list<ValueFlow::Value> andValues;
                    std::list<ValueFlow::Value> orValues;
                    cond.getContextAndValues(condTok, andValues, orValues, true);

                    const std::string& op(parent->str());
                    std::list<ValueFlow::Value> values;
                    if (op == "&&")
                        values = std::move(andValues);
                    else if (op == "||")
                        values = std::move(orValues);
                    if (allowKnown && (Token::Match(condTok, "==|!=") || cond.isBool()))
                        changePossibleToKnown(values);
                    if (astIsFloat(cond.vartok, false) ||
                        (!cond.vartok->valueType() &&
                         std::all_of(values.cbegin(), values.cend(), [](const ValueFlow::Value& v) {
                             return v.isIntValue() || v.isFloatValue();
                         })))
                        values.remove_if([&](const ValueFlow::Value& v) {
                            return v.isImpossible();
                        });
                    for (Token* start:nextExprs) {
                        Analyzer::Result r = forward(start, cond.vartok, values, tokenlist, errorLogger, settings);
                        if (r.terminate != Analyzer::Terminate::None || r.action.isModified())
                            return;
                    }
                }
            }

            {
                const Token* tok2 = condTok;
                std::string op;
                bool mixedOperators = false;
                while (tok2->astParent()) {
                    const Token* parent = tok2->astParent();
                    if (Token::Match(parent, "%oror%|&&")) {
                        if (op.empty()) {
                            op = parent->str();
                        } else if (op != parent->str()) {
                            mixedOperators = true;
                            break;
                        }
                    }
                    if (parent->str() == "!") {
                        op = (op == "&&" ? "||" : "&&");
                    }
                    tok2 = parent;
                }

                if (mixedOperators) {
                    return;
                }
            }

            if (!top)
                return;

            if (top->previous()->isExpandedMacro()) {
                for (std::list<ValueFlow::Value>* values : {&thenValues, &elseValues}) {
                    for (ValueFlow::Value& v : *values)
                        v.macro = true;
                }
            }

            Token* condTop = ctx->astParent();
            {
                bool inverted2 = false;
                while (Token::Match(condTop, "%oror%|&&")) {
                    Token* parent = skipNotAndCasts(condTop, &inverted2)->astParent();
                    if (!parent)
                        break;
                    condTop = parent;
                }
                if (inverted2)
                    std::swap(thenValues, elseValues);
            }

            if (!condTop)
                return;

            if (Token::simpleMatch(condTop, "?")) {
                Token* colon = condTop->astOperand2();
                forward(colon->astOperand1(), cond.vartok, thenValues, tokenlist, errorLogger, settings);
                forward(colon->astOperand2(), cond.vartok, elseValues, tokenlist, errorLogger, settings);
                // TODO: Handle after condition
                return;
            }

            if (condTop != top && condTop->str() != ";")
                return;

            if (!Token::Match(top->previous(), "if|while|for ("))
                return;

            if (top->strAt(-1) == "for") {
                if (!Token::Match(condTok, "%comp%"))
                    return;
                if (!Token::simpleMatch(condTok->astParent(), ";"))
                    return;
                const Token* stepTok = getStepTok(top);
                if (cond.vartok->varId() == 0)
                    return;
                if (!cond.vartok->variable())
                    return;
                if (!Token::Match(stepTok, "++|--"))
                    return;
                std::set<ValueFlow::Value::Bound> bounds;
                for (const ValueFlow::Value& v : thenValues) {
                    if (v.bound != ValueFlow::Value::Bound::Point && v.isImpossible())
                        continue;
                    bounds.insert(v.bound);
                }
                if (Token::simpleMatch(stepTok, "++") && bounds.count(ValueFlow::Value::Bound::Lower) > 0)
                    return;
                if (Token::simpleMatch(stepTok, "--") && bounds.count(ValueFlow::Value::Bound::Upper) > 0)
                    return;
                const Token* childTok = condTok->astOperand1();
                if (!childTok)
                    childTok = condTok->astOperand2();
                if (!childTok)
                    return;
                if (childTok->varId() != cond.vartok->varId())
                    return;
                const Token* startBlock = top->link()->next();
                if (isVariableChanged(startBlock,
                                      startBlock->link(),
                                      cond.vartok->varId(),
                                      cond.vartok->variable()->isGlobal(),
                                      settings))
                    return;
                // Check if condition in for loop is always false
                const Token* initTok = getInitTok(top);
                ProgramMemory pm;
                fillFromPath(pm, initTok, path, settings);
                fillFromPath(pm, condTok, path, settings);
                execute(initTok, pm, nullptr, nullptr, settings);
                MathLib::bigint result = 1;
                execute(condTok, pm, &result, nullptr, settings);
                if (result == 0)
                    return;
                // Remove condition since for condition is not redundant
                for (std::list<ValueFlow::Value>* values : {&thenValues, &elseValues}) {
                    for (ValueFlow::Value& v : *values) {
                        v.condition = nullptr;
                        v.conditional = true;
                    }
                }
            }

            bool deadBranch[] = {false, false};
            // start token of conditional code
            Token* startTokens[] = {nullptr, nullptr};
            // determine startToken(s)
            if (Token::simpleMatch(top->link(), ") {"))
                startTokens[0] = top->link()->next();
            if (Token::simpleMatch(top->link()->linkAt(1), "} else {"))
                startTokens[1] = top->link()->linkAt(1)->tokAt(2);

            int changeBlock = -1;
            int bailBlock = -1;

            for (int i = 0; i < 2; i++) {
                const Token* const startToken = startTokens[i];
                if (!startToken)
                    continue;
                std::list<ValueFlow::Value>& values = (i == 0 ? thenValues : elseValues);
                if (allowKnown)
                    valueFlowSetConditionToKnown(condTok, values, i == 0);

                Analyzer::Result r = forward(startTokens[i], startTokens[i]->link(), cond.vartok, values, tokenlist, errorLogger, settings);
                deadBranch[i] = r.terminate == Analyzer::Terminate::Escape;
                if (r.action.isModified() && !deadBranch[i])
                    changeBlock = i;
                if (r.terminate != Analyzer::Terminate::None && r.terminate != Analyzer::Terminate::Escape &&
                    r.terminate != Analyzer::Terminate::Modified)
                    bailBlock = i;
                changeKnownToPossible(values);
            }
            if (changeBlock >= 0 && !Token::simpleMatch(top->previous(), "while (")) {
                if (settings.debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            startTokens[changeBlock]->link(),
                            "valueFlowAfterCondition: " + cond.vartok->expressionString() +
                            " is changed in conditional block");
                return;
            }
            if (bailBlock >= 0) {
                if (settings.debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            startTokens[bailBlock]->link(),
                            "valueFlowAfterCondition: bailing in conditional block");
                return;
            }

            // After conditional code..
            if (Token::simpleMatch(top->link(), ") {")) {
                Token* after = top->link()->linkAt(1);
                bool dead_if = deadBranch[0];
                bool dead_else = deadBranch[1];
                const Token* unknownFunction = nullptr;
                if (condTok->astParent() && Token::Match(top->previous(), "while|for ("))
                    dead_if = !isBreakScope(after);
                else if (!dead_if)
                    dead_if = isReturnScope(after, settings.library, &unknownFunction);

                if (!dead_if && unknownFunction) {
                    if (settings.debugwarnings)
                        bailout(tokenlist, errorLogger, unknownFunction, "possible noreturn scope");
                    return;
                }

                if (Token::simpleMatch(after, "} else {")) {
                    after = after->linkAt(2);
                    unknownFunction = nullptr;
                    if (!dead_else)
                        dead_else = isReturnScope(after, settings.library, &unknownFunction);
                    if (!dead_else && unknownFunction) {
                        if (settings.debugwarnings)
                            bailout(tokenlist, errorLogger, unknownFunction, "possible noreturn scope");
                        return;
                    }
                }

                if (dead_if && dead_else)
                    return;

                std::list<ValueFlow::Value> values;
                if (dead_if) {
                    values = std::move(elseValues);
                } else if (dead_else) {
                    values = std::move(thenValues);
                } else {
                    std::copy_if(thenValues.cbegin(),
                                 thenValues.cend(),
                                 std::back_inserter(values),
                                 std::mem_fn(&ValueFlow::Value::isPossible));
                    std::copy_if(elseValues.cbegin(),
                                 elseValues.cend(),
                                 std::back_inserter(values),
                                 std::mem_fn(&ValueFlow::Value::isPossible));
                }

                if (values.empty())
                    return;

                if (dead_if || dead_else) {
                    const Token* parent = condTok->astParent();
                    // Skip the not operator
                    while (Token::simpleMatch(parent, "!"))
                        parent = parent->astParent();
                    bool possible = false;
                    if (Token::Match(parent, "&&|%oror%")) {
                        const std::string& op(parent->str());
                        while (parent && parent->str() == op)
                            parent = parent->astParent();
                        if (Token::simpleMatch(parent, "!") || Token::simpleMatch(parent, "== false"))
                            possible = op == "||";
                        else
                            possible = op == "&&";
                    }
                    if (possible) {
                        values.remove_if(std::mem_fn(&ValueFlow::Value::isImpossible));
                        changeKnownToPossible(values);
                    } else if (allowKnown) {
                        valueFlowSetConditionToKnown(condTok, values, true);
                        valueFlowSetConditionToKnown(condTok, values, false);
                    }
                }
                if (values.empty())
                    return;
                const bool isKnown = std::any_of(values.cbegin(), values.cend(), [&](const ValueFlow::Value& v) {
                    return v.isKnown() || v.isImpossible();
                });
                if (isKnown && ValueFlow::isBreakOrContinueScope(after)) {
                    const Scope* loopScope = ValueFlow::getLoopScope(cond.vartok);
                    if (loopScope) {
                        Analyzer::Result r = forward(after, loopScope->bodyEnd, cond.vartok, values, tokenlist, errorLogger, settings);
                        if (r.terminate != Analyzer::Terminate::None)
                            return;
                        if (r.action.isModified())
                            return;
                        auto* start = const_cast<Token*>(loopScope->bodyEnd);
                        if (Token::simpleMatch(start, "} while (")) {
                            start = start->tokAt(2);
                            forward(start, start->link(), cond.vartok, values, tokenlist, errorLogger, settings);
                            start = start->link();
                        }
                        values.remove_if(std::mem_fn(&ValueFlow::Value::isImpossible));
                        changeKnownToPossible(values);
                    }
                }
                forward(after, ValueFlow::getEndOfExprScope(cond.vartok, scope), cond.vartok, values, tokenlist, errorLogger, settings);
            }
        });
    }
    virtual ~ConditionHandler() = default;
    ConditionHandler(const ConditionHandler&) = default;
protected:
    ConditionHandler() = default;
};

#endif // conditionHandlerH
