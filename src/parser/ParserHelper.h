/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#ifndef PARSER_CHAOSEXPRPARSER_H_
#define PARSER_CHAOSEXPRPARSER_H_

#include <glog/logging.h>
#include "ExprParser.hpp"
#include "ExprScanner.h"

namespace chaos {

class ParserHelper {
public:
    static std::unique_ptr<Expression> parse(const std::string& query) {
        std::istringstream iss(query);
        ExprScanner scanner(&iss);
        std::string errMsg;
        Expression* expr;
        ExprParser parser(scanner, errMsg, &expr);
        auto ret = parser.parse();
        if (ret == 0) {
            LOG(INFO) << "Parse " << query << " succeeded!";
            return std::unique_ptr<Expression>(expr);
        }
        return nullptr;
    }

private:
    ParserHelper() = default;
};

}   // namespace chaos
#endif  // PARSER_CHAOSEXPRPARSER_H_
