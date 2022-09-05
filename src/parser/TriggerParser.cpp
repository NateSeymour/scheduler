#include "TriggerParser.h"

TriggerParser::system_time_point TriggerParser::NextTrigger(const std::shared_ptr<Unit> &unit)
{
    return this->Parse(unit->trigger, unit->last_executed);
}

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
 *  : EveryInterval
 *  ;
 */
void TriggerParser::Statement()
{
    switch (this->lookahead->type)
    {
        case KEYWORD_EVERY:
        {
            this->EveryInterval();
            break;
        }

        default:
        {
            throw std::runtime_error("Parsing failed!");
        }
    }
}

/**
 * EveryInterval
 *  : KEYWORD_EVERY integer interval
 *  : KEYWORD_EVERY interval
 *  ;
 */
void TriggerParser::EveryInterval()
{
    this->Eat(KEYWORD_EVERY);

    switch(this->lookahead->type)
    {
        case integer:
        {
            auto interval_duration = this->Eat(integer);
            auto interval_period = this->Eat(interval);

            auto interval_as_seconds = std::chrono::seconds(*interval_duration->As<TOKEN_INTEGER>());
            interval_as_seconds *= *interval_period->As<TOKEN_INTEGER>();

            this->calculated_trigger = this->previous_trigger + interval_as_seconds;
            break;
        }

        case interval:
        {
            auto interval_period = this->Eat(interval);
            auto period_as_seconds = std::chrono::seconds(*interval_period->As<TOKEN_INTEGER>());

            this->calculated_trigger = this->previous_trigger + period_as_seconds;
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