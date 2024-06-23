//
// Created by lang liu on 2024/6/22.
//

#pragma once

#include "help.hpp"

namespace MyMQ
{
    class Router {
    public:
        static bool IsLegalRoutingKey(const std::string &routingKey) {
            for (auto &it: routingKey) {
                if (std::isalpha(it) ||
                    std::isdigit(it) ||
                    it == '_' || it == '.') {
                    continue;
                }
                return false;
            }
            return true;
        }

        static bool IsLegalBindingKey(const std::string &bindingKey) {
            for (auto &ch: bindingKey) {
                if (std::isdigit(ch) ||
                    std::isalpha(ch) ||
                    ch == '.' || ch == '_' ||
                    ch == '*' || ch == '#') {
                    continue;
                }
                return false;
            }

            std::vector<std::string> sub;
            StrHelper::Split(sub, bindingKey, ".");

            for(auto& str : sub)
            {
                if(str.size() > 1 && (str.find('*') != std::string::npos || str.find('#') != std::string::npos))
                    return false;
            }

            const static std::unordered_set<std::string> invalid = {"*#", "##", "#*"};
            // new.sport#.*.#
            for (int i = 1; i < sub.size(); ++i) {
                if (invalid.find(sub[i]) != invalid.end()) return false;
            }

            return true;
        }

        static bool Route(ExchangeType type, const std::string &routingKey, const std::string &bindingKey) {
            if (type == ExchangeType::DIRECT)
                return routingKey == bindingKey;
            else if (type == ExchangeType::FANOUT)   //广播
                return true;

            std::vector<std::string> bkeys, rkeys;
            StrHelper::Split(bkeys, bindingKey, ".");
            StrHelper::Split(rkeys, routingKey, ".");

            int n = bkeys.size();
            int m = rkeys.size();

            std::vector<std::vector<bool>> dp(n + 1, std::vector<bool>(m + 1, false));
            dp[0][0] = true;

            for (int i = 1; i <= n; ++i) {
                if (bkeys[i - 1] == "#") {
                    dp[i][0] = true;
                    continue;
                }
                break;
            }

            for (int i = 1; i <= n; ++i) {
                for (int j = 1; j <= m; ++j) {
                    if (bkeys[i - 1] == rkeys[j - 1] || bkeys[i - 1] == "*")
                        dp[i][j] = true;
                    else if (bkeys[i - 1] == "#")
                        dp[i][j] = dp[i - 1][j] | dp[i - 1][j - 1] | dp[i][j - 1];
                }
            }

            return dp[n][m];
        };
    };
}