#ifndef NOT_YOUR_SCHEDULER_TOKEN_H
#define NOT_YOUR_SCHEDULER_TOKEN_H

#include <chrono>
#include <vector>
#include <type_traits>
#include <string>

enum TokenType
{
    INVALID,
    WHITESPACE,
    NONE,

    // Interval keywords
    KEYWORD_EVERY,

    integer,
    interval,
    weekday,
    identifier
};

typedef int64_t TOKEN_INTEGER;
typedef std::chrono::minutes TOKEN_MINUTES;
typedef std::chrono::weekday TOKEN_WEEKDAY;

class Token
{
private:
    void *value = nullptr;

public:
    TokenType type;

    template <typename T>
    T* As()
    {
        return static_cast<T*>(this->value);
    }

    Token() = delete;

    template <typename T>
    Token(TokenType token_type, T token_value) : type(token_type)
    {
        this->value = (T*)malloc(sizeof(T));
        *(T*)this->value = std::move(token_value);
    }

    explicit Token(TokenType token_type) : type(token_type) {}

    ~Token()
    {
        if(this->value != nullptr) free(this->value);
    }
};


#endif //NOT_YOUR_SCHEDULER_TOKEN_H
