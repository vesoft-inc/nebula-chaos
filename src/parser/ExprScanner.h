/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#ifndef PARSER_EXPRSCANNER_H_
#define PARSER_EXPRSCANNER_H_

#include <string>
#include <memory>
#include <functional>

// Only include FlexLexer.h if it hasn't been already included
#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

// Override the interface for yylex since we namespaced it
#undef YY_DECL
#define YY_DECL int chaos::ExprScanner::yylex()

#include "ExprParser.hpp"

namespace chaos {

class ExprScanner : public yyFlexLexer {
public:
    ExprScanner(std::istream* arg_yyin = 0,
                std::ostream* arg_yyout = 0);

    virtual ~ExprScanner() = default;

    int yylex(ExprParser::semantic_type * lval,
              ExprParser::location_type *loc) {
        yylval = lval;
        yylloc = loc;
        return yylex();
    }

private:
    int yylex() override;

private:
    ExprParser::semantic_type* yylval{nullptr};
    ExprParser::location_type* yylloc{nullptr};
};

}   // namespace chaos
#endif  // PARSER_EXPRSCANNER_H_
