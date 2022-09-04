#ifndef NOT_YOUR_SCHEDULER_TOKENIZER_H
#define NOT_YOUR_SCHEDULER_TOKENIZER_H

#include <cstdint>
#include <memory>
#include <string>
#include <regex>
#include <map>
#include "Token.h"

class Tokenizer
{
private:
    static const std::map<TokenType, std::regex> token_map;

    uint64_t cursor = 0;
    std::string parse_data;

public:
    std::unique_ptr<Token> NextToken();

    void SetParseData(std::string data)
    {
        this->cursor = 0;
        this->parse_data = std::move(data);
    }
};


#endif //NOT_YOUR_SCHEDULER_TOKENIZER_H
