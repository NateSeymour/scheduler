#ifndef NOT_YOUR_SCHEDULER_TRIGGERPARSER_H
#define NOT_YOUR_SCHEDULER_TRIGGERPARSER_H

#include <chrono>
#include <vector>
#include <type_traits>
#include <string>
#include "Tokenizer.h"

class TriggerParser
{
private:
    using system_time_point = std::chrono::time_point<std::chrono::system_clock>;

    Tokenizer tokenizer;
    std::unique_ptr<Token> lookahead;

    system_time_point previous_trigger;
    system_time_point calculated_trigger;

    std::unique_ptr<Token> Eat(TokenType type);

    void StatementList();
    void Statement();

public:
    system_time_point Parse(const std::string& parse_trigger, system_time_point previous);
};

#endif //NOT_YOUR_SCHEDULER_TRIGGERPARSER_H
