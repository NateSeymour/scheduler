#include "Tokenizer.h"

const std::map<TokenType, std::regex> Tokenizer::token_map = {
        /**
         * Literals
         */
        {TOKEN_integer,       std::regex("^(\\d+)") },
        {TOKEN_KEYWORD_EVERY, std::regex("^(every)") },
        {TOKEN_interval,      std::regex("(^seconds?|^minutes?|^hours?|^days?)") },
        //{ identifier, std::regex("^([a-z]+)") },

        /**
         * Misc.
         */
        {TOKEN_WHITESPACE,    std::regex("^\\s+") }
};

std::unique_ptr<Token> Tokenizer::NextToken()
{
    for(const auto& [type, regex] : Tokenizer::token_map)
    {
        std::string parse_sub = this->parse_data.substr(this->cursor);

        if(parse_sub.length() == 0) break;

        std::smatch match;
        if(std::regex_search(parse_sub, match, regex))
        {
            this->cursor += match.length();

            switch(type) {
                case TOKEN_WHITESPACE:
                    continue;

                case TOKEN_integer:
                {
                    TOKEN_TYPE_INTEGER value = std::stoll(match.str());
                    return std::make_unique<Token>(TOKEN_integer, value);
                }

                case TOKEN_interval:
                {
                    if(match.str().starts_with("second"))
                        return std::make_unique<Token>(TOKEN_interval, static_cast<TOKEN_TYPE_INTEGER>(1));
                    if(match.str().starts_with("minute"))
                        return std::make_unique<Token>(TOKEN_interval, static_cast<TOKEN_TYPE_INTEGER>(60));
                    if(match.str().starts_with("hour"))
                        return std::make_unique<Token>(TOKEN_interval, static_cast<TOKEN_TYPE_INTEGER>(60 * 60));
                    if(match.str().starts_with("day"))
                        return std::make_unique<Token>(TOKEN_interval, static_cast<TOKEN_TYPE_INTEGER>(60 * 60 * 24));
                }

                case TOKEN_KEYWORD_EVERY:
                {
                    return std::make_unique<Token>(TOKEN_KEYWORD_EVERY);
                }
            }
        }
    }

    return std::make_unique<Token>(TOKEN_NONE);
}