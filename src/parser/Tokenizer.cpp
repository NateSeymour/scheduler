#include "Tokenizer.h"

const std::map<TokenType, std::regex> Tokenizer::token_map = {
        /**
         * Literals
         */
        { integer, std::regex("^(\\d+)") },
        { KEYWORD_EVERY, std::regex("^(every)") },
        { interval, std::regex("(^seconds?|^minutes?|^hours?|^days?)") },
        //{ identifier, std::regex("^([a-z]+)") },

        /**
         * Misc.
         */
        { WHITESPACE, std::regex("^\\s+") }
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
                case WHITESPACE:
                    continue;

                case integer:
                {
                    TOKEN_INTEGER value = std::stoll(match.str());
                    return std::make_unique<Token>(integer, value);
                }

                case interval:
                {
                    if(match.str().starts_with("second"))
                        return std::make_unique<Token>(interval, static_cast<TOKEN_INTEGER>(1));
                    if(match.str().starts_with("minute"))
                        return std::make_unique<Token>(interval, static_cast<TOKEN_INTEGER>(60));
                    if(match.str().starts_with("hour"))
                        return std::make_unique<Token>(interval, static_cast<TOKEN_INTEGER>(60 * 60));
                    if(match.str().starts_with("day"))
                        return std::make_unique<Token>(interval, static_cast<TOKEN_INTEGER>(60 * 60 * 24));
                }

                case KEYWORD_EVERY:
                {
                    return std::make_unique<Token>(KEYWORD_EVERY);
                }
            }
        }
    }

    return std::make_unique<Token>(NONE);
}