#ifndef NOT_YOUR_SCHEDULER_TOKEN_H
#define NOT_YOUR_SCHEDULER_TOKEN_H

#include <chrono>
#include <vector>
#include <type_traits>
#include <string>

enum TokenType
{
    TOKEN_INVALID,
    TOKEN_WHITESPACE,
    TOKEN_NONE,

    // Interval keywords
    TOKEN_KEYWORD_EVERY,

    TOKEN_integer,
    TOKEN_interval,
    TOKEN_weekday,
    TOKEN_identifier
};

typedef int64_t TOKEN_TYPE_INTEGER;
typedef std::chrono::minutes TOKEN_TYPE_MINUTES;
typedef std::chrono::weekday TOKEN_TYPE_WEEKDAY;

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
