#ifndef NOT_YOUR_SCHEDULER_FORMAT_H
#define NOT_YOUR_SCHEDULER_FORMAT_H

#include <string>

namespace nys
{
    template<typename... Args>
    std::string format(const char *fmt, Args... args)
    {
        /*
         * Clang's warnings are annoying here, and there is no reasonable assumption
         * of a format security risk, as these methods are only called with string
         * literals.
         */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
        // Add 1 to include the null terminator
        size_t buffer_size = std::snprintf(nullptr, 0, fmt, args...) + 1;
        std::string formatted_string(buffer_size, '\0');
        std::snprintf(formatted_string.data(), buffer_size, fmt, args...);
#pragma clang diagnostic pop

        return formatted_string;
    }
}

#endif //NOT_YOUR_SCHEDULER_FORMAT_H
