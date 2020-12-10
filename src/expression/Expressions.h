/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#ifndef EXPRESSION_EXPRESSIONS_H_
#define EXPRESSION_EXPRESSIONS_H_

#include "common/base/Base.h"
#include "expression/ExprUtils.h"

namespace nebula_chaos {

class ExprContext {
public:
    virtual ValueOrErr getVar(const std::string& name) const {
        auto it = variables_.find(name);
        if (it == variables_.end()) {
            return folly::makeUnexpected(ErrorCode::ERR_NULL);
        }
        return it->second;
    }

    // insert or overwrite the variable named "name"
    virtual void setVar(const std::string& name, Value val) {
        variables_[name] = std::move(val);
    }

    virtual ~ExprContext() = default;

private:
    std::unordered_map<std::string, Value> variables_;
};

class Expression {
public:
    enum Type : uint8_t {
        kUnknown = 0,
        kConstant,
        kVariable,
        kUnary,
        kArithmetic,
        kRelational,
        kLogical,
    };
    static_assert(sizeof(Type) == sizeof(uint8_t), "");

    virtual ~Expression() {}

    virtual std::string toString() const = 0;

    virtual ValueOrErr eval(ExprContext* ctx) const = 0;

    Type type() const {
        return type_;
    }

    std::string typeStr() const {
        switch (type_) {
            case kConstant:
                return "Primary";
            case kUnary:
                return "Unary";
            case kArithmetic:
                return "Arithmetic";
            case kRelational:
                return "Relational";
            case kLogical:
                return "Logical";
            case kVariable:
                return "Variable";
            default:
                return "Unknown";
        }
    }

protected:
    Type                                        type_{kUnknown};
};

// literal constants: bool, integer, double, string
class ConstantExpression final : public Expression {
public:
    explicit ConstantExpression(bool val) {
        type_ = kConstant;
        operand_ = val;
    }

    explicit ConstantExpression(int64_t val) {
        type_ = kConstant;
        operand_ = val;
    }

    explicit ConstantExpression(double val) {
        type_ = kConstant;
        operand_ = val;
    }

    explicit ConstantExpression(std::string val) {
        type_ = kConstant;
        operand_ = std::move(val);
    }

    std::string toString() const override;

    ValueOrErr eval(ExprContext*) const override;

private:
    Value                                 operand_;
};

class VariableExpression final : public Expression {
public:
    explicit VariableExpression(std::string valName) {
        type_ = kVariable;
        varName_ = std::move(valName);
    }

    std::string toString() const override {
        return "$" + varName_;
    }

    ValueOrErr eval(ExprContext* ctx) const override;

private:
    std::string varName_;
};

// +expr, -expr, !expr
class UnaryExpression final : public Expression {
public:
    enum Operator : uint8_t {
        PLUS, NEGATE, NOT
    };
    static_assert(sizeof(Operator) == sizeof(uint8_t), "");

    UnaryExpression(Operator op, Expression* operand) {
        type_ = kUnary;
        op_ = op;
        operand_.reset(operand);
    }

    std::string toString() const override;

    ValueOrErr eval(ExprContext*) const override;

private:
    std::string opStr() const {
        switch (op_) {
            case PLUS:
                return "+";
            case NEGATE:
                return "-";
            case NOT:
                return "!";
            default:
                return "Unknown";
        }
    }

private:
    Operator                                    op_;
    std::unique_ptr<Expression>                 operand_;
};

// +, -, *, /, %
class ArithmeticExpression final : public Expression {
public:
    enum Operator : uint8_t {
        ADD, SUB, MUL, DIV, MOD
    };
    static_assert(sizeof(Operator) == sizeof(uint8_t), "");

    ArithmeticExpression(Expression *left, Operator op, Expression *right) {
        type_ = kArithmetic;
        op_ = op;
        left_.reset(left);
        right_.reset(right);
    }

    std::string toString() const override;

    ValueOrErr eval(ExprContext*) const override;

private:
    std::string opStr() const {
        switch (op_) {
            case ADD:
                return "+";
            case SUB:
                return "-";
            case MUL:
                return "*";
            case DIV:
                return "/";
            case MOD:
                return "%";
            default:
                return "Unknown";
        }
    }

private:
    Operator                                    op_;
    std::unique_ptr<Expression>                 left_;
    std::unique_ptr<Expression>                 right_;
};

// <, <=, >, >=, ==, !=
class RelationalExpression final : public Expression {
public:
    enum Operator : uint8_t {
        LT, LE, GT, GE, EQ, NE
    };
    static_assert(sizeof(Operator) == sizeof(uint8_t), "");

    RelationalExpression(Expression *left, Operator op, Expression *right) {
        type_ = kRelational;
        op_ = op;
        left_.reset(left);
        right_.reset(right);
    }

    ValueOrErr eval(ExprContext*) const override;

    std::string toString() const override;

    std::string opStr() const {
        switch (op_) {
            case LT:
                return "<";
            case LE:
                return "<=";
            case GE:
                return ">=";
            case EQ:
                return "==";
            case NE:
                return "!=";
            default:
                return "Unknown";
        }
    }

private:
    ErrorCode implicitCasting(Value &lhs, Value &rhs) const;

private:
    Operator                                    op_;
    std::unique_ptr<Expression>                 left_;
    std::unique_ptr<Expression>                 right_;
};

// &&, ||, ^
class LogicalExpression final : public Expression {
public:
    enum Operator : uint8_t {
        AND, OR, XOR
    };
    static_assert(sizeof(Operator) == sizeof(uint8_t), "");

    LogicalExpression(Expression *left, Operator op, Expression *right) {
        type_ = kLogical;
        op_ = op;
        left_.reset(left);
        right_.reset(right);
    }

    ValueOrErr eval(ExprContext*) const override;

    std::string toString() const override;

    std::string opStr() const {
        switch (op_) {
            case AND:
                return "&&";
            case OR:
                return "||";
            case XOR:
                return "^";
            default:
                return "Unknown";
        }
    }

private:
    Operator                                    op_;
    std::unique_ptr<Expression>                 left_;
    std::unique_ptr<Expression>                 right_;
};
}   // namespace nebula_chaos

#endif  // EXPRESSION_EXPRESSIONS_H_
