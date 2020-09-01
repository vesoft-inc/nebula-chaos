/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */
#ifndef EXPRESSION_EXPRUTILS_H_
#define EXPRESSION_EXPRUTILS_H_

#include <glog/logging.h>
#include <folly/Range.h>
#include <boost/variant.hpp>
#include <folly/futures/Future.h>
#include <folly/String.h>
#include <folly/Format.h>
#include <folly/Expected.h>
#include "common/Base.h"

namespace nebula_chaos {

#ifndef VAR_INT64
#define VAR_INT64 0
#endif

#ifndef VAR_DOUBLE
#define VAR_DOUBLE 1
#endif

#ifndef VAR_BOOL
#define VAR_BOOL 2
#endif

#ifndef VAR_STR
#define VAR_STR 3
#endif

using Value = boost::variant<int64_t, double, bool, std::string>;
enum class ErrorCode {
    ERR_NULL,
    ERR_UNKNOWN_TYPE,
    ERR_UNKNOWN_OP,
    ERR_UNSUPPORTED_OP,
    ERR_BAD_PARAMS,
};

using ValueOrErr = folly::Expected<Value, ErrorCode>;

static std::string indent(uint32_t d) {
    std::string indent(d * 4, '-');
    return indent;
}

static int64_t asInt(const Value& value) {
    return boost::get<int64_t>(value);
}

static double asDouble(const Value& value) {
    if (value.which() == 0) {
        return static_cast<double>(boost::get<int64_t>(value));
    }
    return boost::get<double>(value);
}

static const std::string& asString(const Value &value) {
    return boost::get<std::string>(value);
}

static bool asBool(const Value& value) {
    switch (value.which()) {
        case 0:
            return asInt(value) != 0;
        case 1:
            return asDouble(value) != 0.0;
        case 2:
            return boost::get<bool>(value);
        case 3:
            return asString(value).empty();
        default:
            DCHECK(false);
    }
    return false;
}

static bool isInt(const Value &value) {
    return value.which() == 0;
}

static bool isDouble(const Value &value) {
    return value.which() == 1;
}

static bool isBool(const Value &value) {
    return value.which() == 2;
}

static bool isString(const Value &value) {
    return value.which() == 3;
}

static bool isArithmetic(const Value &value) {
    return isInt(value) || isDouble(value);
}

static bool almostEqual(double left, double right) {
    constexpr auto EPSILON = 1e-8;
    return std::abs(left - right) < EPSILON;
}

}   // namespace nebula_chaos

#endif  // EXPRESSION_EXPRUTILS_H_
