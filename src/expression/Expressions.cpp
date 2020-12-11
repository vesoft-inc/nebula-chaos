/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "expression/Expressions.h"

namespace chaos {

std::string ConstantExpression::toString() const {
    char buf[1024];
    switch (operand_.which()) {
        case VAR_INT64:
            snprintf(buf, sizeof(buf), "%ld", boost::get<int64_t>(operand_));
            break;
        case VAR_DOUBLE: {
            int digits10 = std::numeric_limits<double>::digits10;
            std::string fmt = folly::sformat("%.{}lf", digits10);
            snprintf(buf, sizeof(buf), fmt.c_str(), boost::get<double>(operand_));
            break;
        }
        case VAR_BOOL:
            snprintf(buf, sizeof(buf), "%s", boost::get<bool>(operand_) ? "true" : "false");
            break;
        case VAR_STR:
            return boost::get<std::string>(operand_);
    }
    return buf;
}

ValueOrErr ConstantExpression::eval(ExprContext*) const {
    switch (operand_.which()) {
        case VAR_INT64:
            return boost::get<int64_t>(operand_);
        case VAR_DOUBLE:
            return boost::get<double>(operand_);
        case VAR_BOOL:
            return boost::get<bool>(operand_);
        case VAR_STR:
            return boost::get<std::string>(operand_);
    }
    return folly::makeUnexpected(ErrorCode::ERR_UNKNOWN_TYPE);
}

ValueOrErr VariableExpression::eval(ExprContext* ctx) const {
    if (ctx == nullptr) {
        LOG(INFO) << "The expr context is nullptr";
        return folly::makeUnexpected(ErrorCode::ERR_BAD_PARAMS);
    }
    return ctx->getVar(varName_);
}

std::string UnaryExpression::toString() const {
    std::string buf;
    buf.reserve(256);
    switch (op_) {
        case PLUS:
            buf += '+';
            break;
        case NEGATE:
            buf += '-';
            break;
        case NOT:
            buf += '!';
            break;
    }
    buf += '(';
    buf.append(operand_->toString());
    buf += ')';
    return buf;
}

ValueOrErr UnaryExpression::eval(ExprContext* ctx) const {
    auto valOrErr = operand_->eval(ctx);
    if (!valOrErr) {
        return valOrErr;
    }
    auto value = valOrErr.value();
    if (op_ == PLUS) {
        return value;
    } else if (op_ == NEGATE) {
        if (ExprUtils::isInt(value)) {
            return -ExprUtils::asInt(value);
        } else if (ExprUtils::isDouble(value)) {
            return -ExprUtils::asDouble(value);
        }
    } else {
        return !ExprUtils::asBool(value);
    }
    return folly::makeUnexpected(ErrorCode::ERR_UNKNOWN_OP);
}

std::string ArithmeticExpression::toString() const {
    std::string buf;
    buf.reserve(256);
    buf += '(';
    buf.append(left_->toString());
    switch (op_) {
        case ADD:
            buf += '+';
            break;
        case SUB:
            buf += '-';
            break;
        case MUL:
            buf += '*';
            break;
        case DIV:
            buf += '/';
            break;
        case MOD:
            buf += '%';
            break;
    }
    buf.append(right_->toString());
    buf += ')';
    return buf;
}

ValueOrErr ArithmeticExpression::eval(ExprContext* ctx) const {
    auto lValOrErr = left_->eval(ctx);
    if (!lValOrErr) {
        return lValOrErr;
    }
    auto rValOrErr = right_->eval(ctx);
    if (!rValOrErr) {
        return rValOrErr;
    }
    auto l = lValOrErr.value();
    auto r = rValOrErr.value();

    static constexpr int64_t maxInt = std::numeric_limits<int64_t>::max();
    static constexpr int64_t minInt = std::numeric_limits<int64_t>::min();

    auto isAddOverflow = [] (int64_t lv, int64_t rv) -> bool {
        if (lv >= 0 && rv >= 0) {
            return maxInt - lv < rv;
        } else if (lv < 0 && rv < 0) {
            return minInt - lv > rv;
        } else {
            return false;
        }
    };

    auto isSubOverflow = [] (int64_t lv, int64_t rv) -> bool {
        if (lv > 0 && rv < 0) {
            return maxInt - lv < -rv;
        } else if (lv < 0 && rv > 0) {
            return minInt - lv > -rv;
        } else {
            return false;
        }
    };

    auto isMulOverflow = [] (int64_t lv, int64_t rv) -> bool {
        if (lv > 0 && rv > 0) {
            return maxInt / lv < rv;
        } else if (lv < 0 && rv < 0) {
            return maxInt / lv > rv;
        } else if (lv > 0 && rv < 0) {
            return minInt / lv > rv;
        } else if (lv < 0 && rv > 0) {
            return minInt / lv < rv;
        } else {
            return false;
        }
    };

    switch (op_) {
        case ADD:
            if (ExprUtils::isArithmetic(l) && ExprUtils::isArithmetic(r)) {
                if (ExprUtils::isDouble(l) || ExprUtils::isDouble(r)) {
                    return ExprUtils::asDouble(l) + ExprUtils::asDouble(r);
                }
                int64_t lValue = ExprUtils::asInt(l);
                int64_t rValue = ExprUtils::asInt(r);
                if (isAddOverflow(lValue, rValue)) {
                    LOG(FATAL) << "Error";
                }
                return lValue + rValue;
            }

            if (ExprUtils::isString(l) && ExprUtils::isString(r)) {
                return ExprUtils::asString(l) + ExprUtils::asString(r);
            }
            break;
        case SUB:
            if (ExprUtils::isArithmetic(l) && ExprUtils::isArithmetic(r)) {
                if (ExprUtils::isDouble(l) || ExprUtils::isDouble(r)) {
                    return ExprUtils::asDouble(l) - ExprUtils::asDouble(r);
                }
                int64_t lValue = ExprUtils::asInt(l);
                int64_t rValue = ExprUtils::asInt(r);
                if (isSubOverflow(lValue, rValue)) {
                    LOG(FATAL) << "Error";
                }
                return lValue - rValue;
            }
            break;
        case MUL:
            if (ExprUtils::isArithmetic(l) && ExprUtils::isArithmetic(r)) {
                if (ExprUtils::isDouble(l) || ExprUtils::isDouble(r)) {
                    return ExprUtils::asDouble(l) * ExprUtils::asDouble(r);
                }
                int64_t lValue = ExprUtils::asInt(l);
                int64_t rValue = ExprUtils::asInt(r);
                if (isMulOverflow(lValue, rValue)) {
                    LOG(FATAL) << "Error";
                }
                return lValue * rValue;
            }
            break;
        case DIV:
            if (ExprUtils::isArithmetic(l) && ExprUtils::isArithmetic(r)) {
                if (ExprUtils::isDouble(l) || ExprUtils::isDouble(r)) {
                    if (abs(ExprUtils::asDouble(r)) < 1e-8) {
                        LOG(FATAL) << "Error";
                    }
                    return ExprUtils::asDouble(l) / ExprUtils::asDouble(r);
                }

                if (abs(ExprUtils::asInt(r)) == 0) {
                    LOG(FATAL) << "Error";
                }
                return ExprUtils::asInt(l) / ExprUtils::asInt(r);
            }
            break;
        case MOD:
            if (ExprUtils::isArithmetic(l) && ExprUtils::isArithmetic(r)) {
                if (ExprUtils::isDouble(l) || ExprUtils::isDouble(r)) {
                    if (abs(ExprUtils::asDouble(r)) < 1e-8) {
                        LOG(FATAL) << "Error";
                    }
                    return fmod(ExprUtils::asDouble(l), ExprUtils::asDouble(r));
                }
                if (abs(ExprUtils::asInt(r)) == 0) {
                    LOG(FATAL) << "Error";
                }
                return ExprUtils::asInt(l) % ExprUtils::asInt(r);
            }
            break;
        default:
            DCHECK(false);
    }
    return folly::makeUnexpected(ErrorCode::ERR_UNKNOWN_OP);
}

std::string RelationalExpression::toString() const {
    std::string buf;
    buf.reserve(256);
    buf += '(';
    buf.append(left_->toString());
    switch (op_) {
        case LT:
            buf += '<';
            break;
        case LE:
            buf += "<=";
            break;
        case GT:
            buf += '>';
            break;
        case GE:
            buf += ">=";
            break;
        case EQ:
            buf += "==";
            break;
        case NE:
            buf += "!=";
            break;
    }
    buf.append(right_->toString());
    buf += ')';
    return buf;
}

ValueOrErr RelationalExpression::eval(ExprContext* ctx) const {
    auto lValOrErr = left_->eval(ctx);
    if (!lValOrErr) {
        return lValOrErr;
    }
    auto rValOrErr = right_->eval(ctx);
    if (!rValOrErr) {
        return rValOrErr;
    }
    auto l = lValOrErr.value();
    auto r = rValOrErr.value();

    switch (op_) {
        case LT:
            return l < r;
        case LE:
            return l <= r;
        case GT:
            return l > r;
        case GE:
            return l >= r;
        case EQ:
            if (ExprUtils::isArithmetic(l) && ExprUtils::isArithmetic(r)) {
                if (ExprUtils::isDouble(l) || ExprUtils::isDouble(r)) {
                    return ExprUtils::almostEqual(ExprUtils::asDouble(l),
                                                  ExprUtils::asDouble(r));
                }
            }
            return l == r;
        case NE:
            if (ExprUtils::isArithmetic(l) && ExprUtils::isArithmetic(r)) {
                if (ExprUtils::isDouble(l) || ExprUtils::isDouble(r)) {
                    return !ExprUtils::almostEqual(ExprUtils::asDouble(l),
                                                   ExprUtils::asDouble(r));
                }
            }
            return l != r;
    }
    return folly::makeUnexpected(ErrorCode::ERR_UNKNOWN_OP);
}

std::string LogicalExpression::toString() const {
    std::string buf;
    buf.reserve(256);
    buf += '(';
    buf.append(left_->toString());
    switch (op_) {
        case AND:
            buf += "&&";
            break;
        case OR:
            buf += "||";
            break;
        case XOR:
            buf += "^";
            break;
    }
    buf.append(right_->toString());
    buf += ')';
    return buf;
}

ValueOrErr LogicalExpression::eval(ExprContext* ctx) const {
    auto lValOrErr = left_->eval(ctx);
    if (!lValOrErr) {
        return lValOrErr;
    }
    auto rValOrErr = right_->eval(ctx);
    if (!rValOrErr) {
        return rValOrErr;
    }
    auto l = lValOrErr.value();
    auto r = rValOrErr.value();
    VLOG(3) << "left: " << ExprUtils::asBool(l)
            << ", right:" << ExprUtils::asBool(r)
            << ", op " << static_cast<int32_t>(op_);
    if (op_ == AND) {
        if (!ExprUtils::asBool(l)) {
            return false;
        }
        return ExprUtils::asBool(r);
    } else if (op_ == OR) {
        if (ExprUtils::asBool(l)) {
            return true;
        }
        return ExprUtils::asBool(r);
    } else {
        // op_ == XOR
        return ExprUtils::asBool(l) != ExprUtils::asBool(r);
    }
}

}   // namespace chaos
