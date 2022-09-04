#include "TriggerParser.h"

/**
 * Program
 *  : StatementList
 *  ;
 *
 * @param trigger
 * @param previous
 * @return
 */
TriggerParser::system_time_point
TriggerParser::Parse(const std::string &trigger, TriggerParser::system_time_point previous)
{
    this->tokenizer.SetParseData(trigger);
    this->previous_trigger = previous;

    this->lookahead = tokenizer.NextToken();

    this->Statement();

    return this->calculated_trigger;
}

/**
 * Statement
 *  : KEYWORD_EVERY integer interval
 */
void TriggerParser::Statement()
{
    switch (this->lookahead->type)
    {
        case KEYWORD_EVERY:
        {
            this->Eat(KEYWORD_EVERY);
            auto interval_duration = this->Eat(integer);
            auto interval_period = this->Eat(interval);

            auto interval_as_seconds = std::chrono::seconds(*interval_duration->As<TOKEN_INTEGER>());
            interval_as_seconds *= *interval_period->As<TOKEN_INTEGER>();

            this->calculated_trigger = this->previous_trigger + interval_as_seconds;
            break;
        }

        default:
        {
            throw std::runtime_error("Parsing failed!");
        }
    }
}

std::unique_ptr<Token> TriggerParser::Eat(TokenType type)
{
    auto token = std::move(this->lookahead);
    this->lookahead = this->tokenizer.NextToken();

    if(token->type == type)
    {
        return std::move(token);
    }

    throw std::runtime_error("Unexpected token!");
}