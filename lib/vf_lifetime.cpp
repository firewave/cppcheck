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

#include "vf_lifetime.h"

#include "astutils.h"
#include "symboldatabase.h"
#include "token.h"
#include "tokenlist.h"

#include "vf_common.h"
#include "vf_settokenvalue.h"

namespace ValueFlow
{
    struct Lambda {
        explicit Lambda(const Token* tok)
        {
            if (!Token::simpleMatch(tok, "[") || !tok->link())
                return;
            capture = tok;

            if (Token::simpleMatch(capture->link(), "] (")) {
                arguments = capture->link()->next();
            }
            const Token * afterArguments = arguments ? arguments->link()->next() : capture->link()->next();
            if (afterArguments && afterArguments->originalName() == "->") {
                returnTok = afterArguments->next();
                bodyTok = Token::findsimplematch(returnTok, "{");
            } else if (Token::simpleMatch(afterArguments, "{")) {
                bodyTok = afterArguments;
            }
            for (const Token* c:getCaptures()) {
                if (Token::Match(c, "this !!.")) {
                    explicitCaptures[c->variable()] = std::make_pair(c, LifetimeCapture::ByReference);
                } else if (Token::simpleMatch(c, "* this")) {
                    explicitCaptures[c->next()->variable()] = std::make_pair(c->next(), LifetimeCapture::ByValue);
                } else if (c->variable()) {
                    explicitCaptures[c->variable()] = std::make_pair(c, LifetimeCapture::ByValue);
                } else if (c->isUnaryOp("&") && Token::Match(c->astOperand1(), "%var%")) {
                    explicitCaptures[c->astOperand1()->variable()] =
                        std::make_pair(c->astOperand1(), LifetimeCapture::ByReference);
                } else {
                    const std::string& s = c->expressionString();
                    if (s == "=")
                        implicitCapture = LifetimeCapture::ByValue;
                    else if (s == "&")
                        implicitCapture = LifetimeCapture::ByReference;
                }
            }
        }

        const Token* capture{};
        const Token* arguments{};
        const Token* returnTok{};
        const Token* bodyTok{};
        std::unordered_map<const Variable*, std::pair<const Token*, LifetimeCapture>> explicitCaptures;
        LifetimeCapture implicitCapture = LifetimeCapture::Undefined;

        std::vector<const Token*> getCaptures() const {
            return getArguments(capture);
        }

        bool isLambda() const {
            return capture && bodyTok;
        }
    };

    static bool isConvertedToView(const Token* tok, const Settings& settings)
    {
        std::vector<ValueType> vtParents = getParentValueTypes(tok, settings);
        return std::any_of(vtParents.cbegin(), vtParents.cend(), [&](const ValueType& vt) {
            if (!vt.container)
                return false;
            return vt.container->view;
        });
    }

    static bool isContainerOfPointers(const Token* tok, const Settings& settings)
    {
        if (!tok)
        {
            return true;
        }

        ValueType vt = ValueType::parseDecl(tok, settings);
        return vt.pointer > 0;
    }

    static bool isDecayedPointer(const Token *tok)
    {
        if (!tok)
            return false;
        if (!tok->astParent())
            return false;
        if (astIsPointer(tok->astParent()) && !Token::simpleMatch(tok->astParent(), "return"))
            return true;
        if (tok->astParent()->isConstOp())
            return true;
        if (!Token::simpleMatch(tok->astParent(), "return"))
            return false;
        return astIsPointer(tok->astParent()) || astIsContainerView(tok->astParent());
    }

    void analyzeLifetime(TokenList &tokenlist, ErrorLogger &errorLogger, const Settings &settings)
    {
        for (Token *tok = tokenlist.front(); tok; tok = tok->next()) {
            if (!tok->scope())
                continue;
            if (tok->scope()->type == Scope::eGlobal)
                continue;
            Lambda lam(tok);
            // Lambdas
            if (lam.isLambda()) {
                const Scope * bodyScope = lam.bodyTok->scope();

                std::set<const Scope *> scopes;
                // Avoid capturing a variable twice
                std::set<nonneg int> varids;
                bool capturedThis = false;

                auto isImplicitCapturingVariable = [&](const Token *varTok) {
                    const Variable *var = varTok->variable();
                    if (!var)
                        return false;
                    if (varids.count(var->declarationId()) > 0)
                        return false;
                    if (!var->isLocal() && !var->isArgument())
                        return false;
                    const Scope *scope = var->scope();
                    if (!scope)
                        return false;
                    if (scopes.count(scope) > 0)
                        return false;
                    if (scope->isNestedIn(bodyScope))
                        return false;
                    scopes.insert(scope);
                    varids.insert(var->declarationId());
                    return true;
                };

                bool update = false;
                auto captureVariable = [&](const Token* tok2, LifetimeCapture c, const std::function<bool(const Token*)> &pred) {
                    if (varids.count(tok->varId()) > 0)
                        return;
                    if (c == LifetimeCapture::ByReference) {
                        LifetimeStore ls{
                            tok2, "Lambda captures variable by reference here.", ValueFlow::Value::LifetimeKind::Lambda};
                        ls.forward = false;
                        update |= ls.byRef(tok, tokenlist, errorLogger, settings, pred);
                    } else if (c == LifetimeCapture::ByValue) {
                        LifetimeStore ls{
                            tok2, "Lambda captures variable by value here.", ValueFlow::Value::LifetimeKind::Lambda};
                        ls.forward = false;
                        update |= ls.byVal(tok, tokenlist, errorLogger, settings, pred);
                        pred(tok2);
                    }
                };

                auto captureThisVariable = [&](const Token* tok2, LifetimeCapture c) {
                    ValueFlow::Value value;
                    value.valueType = ValueFlow::Value::ValueType::LIFETIME;
                    if (c == LifetimeCapture::ByReference)
                        value.lifetimeScope = ValueFlow::Value::LifetimeScope::ThisPointer;
                    else if (c == LifetimeCapture::ByValue)
                        value.lifetimeScope = ValueFlow::Value::LifetimeScope::ThisValue;
                    value.tokvalue = tok2;
                    value.errorPath.emplace_back(tok2, "Lambda captures the 'this' variable here.");
                    value.lifetimeKind = ValueFlow::Value::LifetimeKind::Lambda;
                    capturedThis = true;
                    // Don't add the value a second time
                    if (std::find(tok->values().cbegin(), tok->values().cend(), value) != tok->values().cend())
                        return;
                    setTokenValue(tok, std::move(value), settings);
                    update |= true;
                };

                // Handle explicit capture
                for (const auto& p:lam.explicitCaptures) {
                    const Variable* var = p.first;
                    const Token* tok2 = p.second.first;
                    const LifetimeCapture c = p.second.second;
                    if (Token::Match(tok2, "this !!.")) {
                        captureThisVariable(tok2, c);
                    } else if (var) {
                        captureVariable(tok2, c, [](const Token*) {
                            return true;
                        });
                        varids.insert(var->declarationId());
                    }
                }

                auto isImplicitCapturingThis = [&](const Token* tok2) {
                    if (capturedThis)
                        return false;
                    if (Token::simpleMatch(tok2, "this"))
                        return true;
                    if (tok2->variable()) {
                        if (Token::simpleMatch(tok2->previous(), "."))
                            return false;
                        const Variable* var = tok2->variable();
                        if (var->isLocal())
                            return false;
                        if (var->isArgument())
                            return false;
                        return exprDependsOnThis(tok2);
                    }
                    if (Token::simpleMatch(tok2, "("))
                        return exprDependsOnThis(tok2);
                    return false;
                };

                for (const Token * tok2 = lam.bodyTok; tok2 != lam.bodyTok->link(); tok2 = tok2->next()) {
                    if (isImplicitCapturingThis(tok2)) {
                        captureThisVariable(tok2, LifetimeCapture::ByReference);
                    } else if (tok2->variable()) {
                        captureVariable(tok2, lam.implicitCapture, isImplicitCapturingVariable);
                    }
                }
                if (update)
                    valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
            }
            // address of
            else if (tok->isUnaryOp("&")) {
                if (Token::simpleMatch(tok->astParent(), "*"))
                    continue;
                for (const ValueFlow::LifetimeToken& lt : ValueFlow::getLifetimeTokens(tok->astOperand1(), settings)) {
                    if (!settings.certainty.isEnabled(Certainty::inconclusive) && lt.inconclusive)
                        continue;
                    ErrorPath errorPath = lt.errorPath;
                    errorPath.emplace_back(tok, "Address of variable taken here.");

                    ValueFlow::Value value;
                    value.valueType = ValueFlow::Value::ValueType::LIFETIME;
                    value.lifetimeScope = ValueFlow::Value::LifetimeScope::Local;
                    value.tokvalue = lt.token;
                    value.errorPath = std::move(errorPath);
                    if (lt.addressOf || astIsPointer(lt.token) || !Token::Match(lt.token->astParent(), ".|["))
                        value.lifetimeKind = ValueFlow::Value::LifetimeKind::Address;
                    value.setInconclusive(lt.inconclusive);
                    setTokenValue(tok, std::move(value), settings);

                    valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
                }
            }
            // Converting to container view
            else if (astIsContainerOwned(tok) && isConvertedToView(tok, settings)) {
                LifetimeStore ls =
                    LifetimeStore{tok, "Converted to container view", ValueFlow::Value::LifetimeKind::SubObject};
                ls.byRef(tok, tokenlist, errorLogger, settings);
                valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
            }
            // container lifetimes
            else if (astIsContainer(tok)) {
                Token * parent = astParentSkipParens(tok);
                if (!parent)
                    continue;
                if (!Token::Match(parent, ". %name% (") && !Token::Match(parent->previous(), "%name% ("))
                    continue;

                // Skip if its a free function that doesnt yield an iterator to the container
                if (Token::Match(parent->previous(), "%name% (") &&
                    !contains({Library::Container::Yield::START_ITERATOR, Library::Container::Yield::END_ITERATOR},
                              astFunctionYield(parent->previous(), settings)))
                    continue;

                ValueFlow::Value master;
                master.valueType = ValueFlow::Value::ValueType::LIFETIME;
                master.lifetimeScope = ValueFlow::Value::LifetimeScope::Local;

                if (astIsIterator(parent->tokAt(2))) {
                    master.errorPath.emplace_back(parent->tokAt(2), "Iterator to container is created here.");
                    master.lifetimeKind = ValueFlow::Value::LifetimeKind::Iterator;
                } else if (astIsIterator(parent) && Token::Match(parent->previous(), "%name% (") &&
                           contains({Library::Container::Yield::START_ITERATOR, Library::Container::Yield::END_ITERATOR},
                                    astFunctionYield(parent->previous(), settings))) {
                    master.errorPath.emplace_back(parent, "Iterator to container is created here.");
                    master.lifetimeKind = ValueFlow::Value::LifetimeKind::Iterator;
                } else if ((astIsPointer(parent->tokAt(2)) &&
                            !isContainerOfPointers(tok->valueType()->containerTypeToken, settings)) ||
                           Token::Match(parent->next(), "data|c_str")) {
                    master.errorPath.emplace_back(parent->tokAt(2), "Pointer to container is created here.");
                    master.lifetimeKind = ValueFlow::Value::LifetimeKind::Object;
                } else {
                    continue;
                }

                std::vector<const Token*> toks;
                if (tok->isUnaryOp("*") || parent->originalName() == "->") {
                    for (const ValueFlow::Value& v : tok->values()) {
                        if (!v.isLocalLifetimeValue())
                            continue;
                        if (v.lifetimeKind != ValueFlow::Value::LifetimeKind::Address)
                            continue;
                        if (!v.tokvalue)
                            continue;
                        toks.push_back(v.tokvalue);
                    }
                } else if (astIsContainerView(tok)) {
                    for (const ValueFlow::Value& v : tok->values()) {
                        if (!v.isLocalLifetimeValue())
                            continue;
                        if (!v.tokvalue)
                            continue;
                        if (!astIsContainerOwned(v.tokvalue))
                            continue;
                        toks.push_back(v.tokvalue);
                    }
                } else {
                    toks = {tok};
                }

                for (const Token* tok2 : toks) {
                    for (const ReferenceToken& rt : followAllReferences(tok2, false)) {
                        ValueFlow::Value value = master;
                        value.tokvalue = rt.token;
                        value.errorPath.insert(value.errorPath.begin(), rt.errors.cbegin(), rt.errors.cend());
                        if (Token::simpleMatch(parent, "("))
                            setTokenValue(parent, std::move(value), settings);
                        else
                            setTokenValue(parent->tokAt(2), std::move(value), settings);

                        if (!rt.token->variable()) {
                            LifetimeStore ls = LifetimeStore{
                                rt.token, master.errorPath.back().second, ValueFlow::Value::LifetimeKind::Object};
                            ls.byRef(parent->tokAt(2), tokenlist, errorLogger, settings);
                        }
                    }
                }
                valueFlowForwardLifetime(parent->tokAt(2), tokenlist, errorLogger, settings);
            }
            // Check constructors
            else if (Token::Match(tok, "=|return|%name%|{|,|> {") && !isScope(tok->next())) {
                valueFlowLifetimeConstructor(tok->next(), tokenlist, errorLogger, settings);
            }
            // Check function calls
            else if (Token::Match(tok, "%name% (") && !Token::simpleMatch(tok->linkAt(1), ") {")) {
                valueFlowLifetimeFunction(tok, tokenlist, errorLogger, settings);
            }
            // Unique pointer lifetimes
            else if (astIsUniqueSmartPointer(tok) && astIsLHS(tok) && Token::simpleMatch(tok->astParent(), ". get ( )")) {
                Token* ptok = tok->astParent()->tokAt(2);
                ErrorPath errorPath = {{ptok, "Raw pointer to smart pointer created here."}};
                ValueFlow::Value value;
                value.valueType = ValueFlow::Value::ValueType::LIFETIME;
                value.lifetimeScope = ValueFlow::Value::LifetimeScope::Local;
                value.lifetimeKind = ValueFlow::Value::LifetimeKind::SubObject;
                value.tokvalue = tok;
                value.errorPath = std::move(errorPath);
                setTokenValue(ptok, std::move(value), settings);
                valueFlowForwardLifetime(ptok, tokenlist, errorLogger, settings);
            }
            // Check variables
            else if (tok->variable()) {
                ErrorPath errorPath;
                const Variable * var = ValueFlow::getLifetimeVariable(tok, errorPath, settings);
                if (!var)
                    continue;
                if (var->nameToken() == tok)
                    continue;
                if (var->isArray() && !var->isStlType() && !var->isArgument() && isDecayedPointer(tok)) {
                    errorPath.emplace_back(tok, "Array decayed to pointer here.");

                    ValueFlow::Value value;
                    value.valueType = ValueFlow::Value::ValueType::LIFETIME;
                    value.lifetimeScope = ValueFlow::Value::LifetimeScope::Local;
                    value.tokvalue = var->nameToken();
                    value.errorPath = std::move(errorPath);
                    setTokenValue(tok, std::move(value), settings);

                    valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
                }
            }
            // Forward any lifetimes
            else if (std::any_of(tok->values().cbegin(), tok->values().cend(), std::mem_fn(&ValueFlow::Value::isLifetimeValue))) {
                valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
            }
        }
    }
}
