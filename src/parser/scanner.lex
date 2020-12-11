%option c++
%option yyclass="ExprScanner"
%option nodefault noyywrap
%option never-interactive
%option yylineno

%{
#include "parser/ExprScanner.h"
#include "ExprParser.hpp"

#define YY_USER_ACTION                  \
    yylloc->step();                     \
    yylloc->columns(yyleng);

using TokenType = chaos::ExprParser::token;

static constexpr size_t MAX_STRING = 4096;

%}

OR                          ([Oo][Rr])
AND                         ([Aa][Nn][Dd])
NOT                         ([Nn][Oo][Tt])
XOR                         ([Xx][Oo][Rr])
TRUE                        ([Tt][Rr][Uu][Ee])
FALSE                       ([Ff][Aa][Ll][Ss][Ee])


STRING                      ([a-zA-Z][_a-zA-Z0-9]*)
DEC                         ([0-9])
HEX                         ([0-9a-fA-F])
OCT                         ([0-7])

%%

{OR}                        { return TokenType::KW_OR; }
{AND}                       { return TokenType::KW_AND; }
{NOT}                       { return TokenType::KW_NOT; }
{XOR}                       { return TokenType::KW_XOR; }

"."                         { return TokenType::DOT; }
","                         { return TokenType::COMMA; }
":"                         { return TokenType::COLON; }
";"                         { return TokenType::SEMICOLON; }
"@"                         { return TokenType::AT; }

"+"                         { return TokenType::PLUS; }
"-"                         { return TokenType::MINUS; }
"*"                         { return TokenType::MUL; }
"/"                         { return TokenType::DIV; }
"%"                         { return TokenType::MOD; }
"!"                         { return TokenType::NOT; }
"^"                         { return TokenType::XOR; }

"<"                         { return TokenType::LT; }
"<="                        { return TokenType::LE; }
">"                         { return TokenType::GT; }
">="                        { return TokenType::GE; }
"=="                        { return TokenType::EQ; }
"!="                        { return TokenType::NE; }

"||"                        { return TokenType::OR; }
"&&"                        { return TokenType::AND; }

"="                         { return TokenType::ASSIGN; }

"("                         { return TokenType::L_PAREN; }
")"                         { return TokenType::R_PAREN; }
"["                         { return TokenType::L_BRACKET; }
"]"                         { return TokenType::R_BRACKET; }
"{"                         { return TokenType::L_BRACE; }
"}"                         { return TokenType::R_BRACE; }

"<-"                        { return TokenType::L_ARROW; }
"->"                        { return TokenType::R_ARROW; }

{TRUE}                      { yylval->boolval = true; return TokenType::BOOL; }
{FALSE}                     { yylval->boolval = false; return TokenType::BOOL; }

{STRING}                     {
                                yylval->strval = new std::string(yytext, yyleng);
                                if (yylval->strval->size() > MAX_STRING) {
                                    auto error = "Out of range of the LABEL length, "
                                                  "the  max length of LABEL is " +
                                                  std::to_string(MAX_STRING) + ":";
                                    delete yylval->strval;
                                    throw ExprParser::syntax_error(*yylloc, error);
                                }
                                return TokenType::STRING;
                            }
0[Xx]{HEX}+                 {
                                if (yyleng > 18) {
                                    auto i = 2;
                                    while (i < yyleng && yytext[i] == '0') {
                                        i++;
                                    }
                                    if (yyleng - i > 16) {
                                        throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                    }
                                }
                                uint64_t val = 0;
                                sscanf(yytext, "%lx", &val);
                                if (val > MAX_ABS_INTEGER) {
                                    throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                }
                                yylval->intval = static_cast<int64_t>(val);
                                return TokenType::INTEGER;
                            }
0{OCT}+                     {
                                if (yyleng > 22) {
                                    auto i = 1;
                                    while (i < yyleng && yytext[i] == '0') {
                                        i++;
                                    }
                                    if (yyleng - i > 22 ||
                                            (yyleng - i == 22 && yytext[i] != '1')) {
                                        throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                    }
                                }
                                uint64_t val = 0;
                                sscanf(yytext, "%lo", &val);
                                if (val > MAX_ABS_INTEGER) {
                                    throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                }
                                yylval->intval = static_cast<int64_t>(val);
                                return TokenType::INTEGER;
                            }
{DEC}+                      {
                                try {
                                    folly::StringPiece text(yytext, yyleng);
                                    uint64_t val = folly::to<uint64_t>(text);
                                    if (val > MAX_ABS_INTEGER) {
                                        throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                    }
                                    yylval->intval = val;
                                } catch (...) {
                                    throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                }
                                return TokenType::INTEGER;
                            }
{DEC}+\.{DEC}*              {
                                try {
                                    folly::StringPiece text(yytext, yyleng);
                                    yylval->doubleval = folly::to<double>(text);
                                } catch (...) {
                                    throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                }
                                return TokenType::DOUBLE;
                            }
{DEC}*\.{DEC}+              {
                                try {
                                    folly::StringPiece text(yytext, yyleng);
                                    yylval->doubleval = folly::to<double>(text);
                                } catch (...) {
                                    throw ExprParser::syntax_error(*yylloc, "Out of range:");
                                }
                                return TokenType::DOUBLE;
                            }

\${STRING}                  { yylval->strval = new std::string(yytext + 1, yyleng - 1); return TokenType::VARIABLE; }

[ \r\t]                     { }
\n                          {
                                yylineno++;
                                yylloc->lines(yyleng);
                            }

%%

namespace chaos {

ExprScanner::ExprScanner(std::istream* in,
                         std::ostream* out)
    : yyFlexLexer(in, out) {
}


}

