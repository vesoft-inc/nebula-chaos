%language "C++"
%skeleton "lalr1.cc"
%no-lines
%locations
%define api.namespace { chaos }
%define parser_class_name { ExprParser }
%lex-param { chaos::ExprScanner& scanner }
%parse-param { chaos::ExprScanner& scanner }
%parse-param { std::string& errmsg }
%parse-param { Expression** expr }

%code requires {
#include <iostream>
#include <sstream>
#include <string>
#include <cstddef>
#include "expression/Expressions.h"

#define YYDEBUG 1

namespace chaos {

class ExprScanner;

}

static constexpr size_t MAX_ABS_INTEGER = 9223372036854775808ULL;

}

%code {
    static int yylex(chaos::ExprParser::semantic_type* yylval,
                     chaos::ExprParser::location_type *yylloc,
                     chaos::ExprScanner& scanner);

    void ifOutOfRange(const int64_t input,
                      const chaos::ExprParser::location_type& loc);
}

%union {
    bool                                    boolval;
    int64_t                                 intval;
    double                                  doubleval;
    std::string*                            strval;
    chaos::Expression*                      expr;
}

/* destructors */
%destructor {} <boolval> <intval> <doubleval> <expr>
%destructor { delete $$; } <*>

%token KW_AND KW_OR KW_NOT KW_XOR L_PAREN R_PAREN L_BRACKET R_BRACKET L_BRACE R_BRACE COMMA LT LE GT GE EQ NE PLUS MINUS MUL DIV MOD NOT NEG ASSIGN
%token OR AND XOR DOT COLON SEMICOLON L_ARROW R_ARROW AT

/* token type specification */
%token <boolval> BOOL
%token <intval> INTEGER
%token <doubleval> DOUBLE
%token <strval> VARIABLE STRING

%type <expr> expression logic_xor_expression logic_or_expression logic_and_expression
%type <expr> relational_expression multiplicative_expression additive_expression
%type <expr> unary_expression equality_expression const_expression var_expression base_expression



%start expression

%%

const_expression
    : DOUBLE {
        $$ = new ConstantExpression($1);
    }
    | STRING {
        $$ = new ConstantExpression(*$1);
        delete $1;
    }
    | BOOL {
        $$ = new ConstantExpression($1);
    }
    | INTEGER {
        ifOutOfRange($1, @1);
        $$ = new ConstantExpression($1);
    }
    ;

var_expression
    : VARIABLE {
        $$ = new VariableExpression(*$1);
        delete $1;
    }
    ;

base_expression
    : const_expression {
        $$ = $1;
    }
    | var_expression {
        $$ = $1;
    }
    | L_PAREN expression R_PAREN {
        $$ = $2;
    }
    ;

unary_expression
    : base_expression { $$ = $1; }
    | PLUS unary_expression {
        $$ = new UnaryExpression(UnaryExpression::PLUS, $2);
    }
    | MINUS unary_expression {
        $$ = new UnaryExpression(UnaryExpression::NEGATE, $2);
    }
    | KW_NOT unary_expression {
        $$ = new UnaryExpression(UnaryExpression::NOT, $2);
    }
    ;

multiplicative_expression
    : unary_expression { $$ = $1; }
    | multiplicative_expression MUL unary_expression {
        $$ = new ArithmeticExpression($1, ArithmeticExpression::MUL, $3);
    }
    | multiplicative_expression DIV unary_expression {
        $$ = new ArithmeticExpression($1, ArithmeticExpression::DIV, $3);
    }
    | multiplicative_expression MOD unary_expression {
        $$ = new ArithmeticExpression($1, ArithmeticExpression::MOD, $3);
    }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression PLUS multiplicative_expression {
        $$ = new ArithmeticExpression($1, ArithmeticExpression::ADD, $3);
    }
    | additive_expression MINUS multiplicative_expression {
        $$ = new ArithmeticExpression($1, ArithmeticExpression::SUB, $3);
    }
    ;

relational_expression
    : additive_expression { $$ = $1; }
    | relational_expression LT additive_expression {
        $$ = new RelationalExpression($1, RelationalExpression::LT, $3);
    }
    | relational_expression GT additive_expression {
        $$ = new RelationalExpression($1, RelationalExpression::GT, $3);
    }
    | relational_expression LE additive_expression {
        $$ = new RelationalExpression($1, RelationalExpression::LE, $3);
    }
    | relational_expression GE additive_expression {
        $$ = new RelationalExpression($1, RelationalExpression::GE, $3);
    }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQ relational_expression {
        $$ = new RelationalExpression($1, RelationalExpression::EQ, $3);
    }
    | equality_expression NE relational_expression {
        $$ = new RelationalExpression($1, RelationalExpression::NE, $3);
    }
    ;

logic_and_expression
    : equality_expression { $$ = $1; }
    | logic_and_expression AND equality_expression {
        $$ = new LogicalExpression($1, LogicalExpression::AND, $3);
    }
    | logic_and_expression KW_AND equality_expression {
        $$ = new LogicalExpression($1, LogicalExpression::AND, $3);
    }
    ;

logic_or_expression
    : logic_and_expression { $$ = $1; }
    | logic_or_expression OR logic_and_expression {
        $$ = new LogicalExpression($1, LogicalExpression::OR, $3);
    }
    | logic_or_expression KW_OR logic_and_expression {
        $$ = new LogicalExpression($1, LogicalExpression::OR, $3);
    }
    ;

logic_xor_expression
    : logic_or_expression { $$ = $1; }
    | logic_xor_expression XOR logic_or_expression {
        $$ = new LogicalExpression($1, LogicalExpression::XOR, $3);
    }
    | logic_xor_expression KW_XOR logic_or_expression {
        $$ = new LogicalExpression($1, LogicalExpression::XOR, $3);
    }
    ;

expression
    : logic_xor_expression {
        $$ = $1;
        *expr = $$;
    }
    ;

%%

#include "ExprScanner.h"

void chaos::ExprParser::error(const chaos::ExprParser::location_type& loc,
                              const std::string &msg) {
    std::ostringstream os;
    if (msg.empty()) {
        os << "syntax error";
    } else {
        os << msg;
    }
    os << " at " << loc;
    errmsg = os.str();
}

// check the positive integer boundary
// parameter input accept the INTEGER value
// which filled as uint64_t
// so the conversion is expected
void ifOutOfRange(const int64_t input,
                  const chaos::ExprParser::location_type& loc) {
    if ((uint64_t)input >= MAX_ABS_INTEGER) {
        throw chaos::ExprParser::syntax_error(loc, "Out of range:");
    }
}

static int yylex(chaos::ExprParser::semantic_type* yylval,
                 chaos::ExprParser::location_type *yylloc,
                 chaos::ExprScanner& scanner) {
    return scanner.yylex(yylval, yylloc);
}
