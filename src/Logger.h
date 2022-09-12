#ifndef NOT_YOUR_SCHEDULER_LOGGER_H
#define NOT_YOUR_SCHEDULER_LOGGER_H

#include <filesystem>
#include <fstream>
#include <utility>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <cstdlib>

class Logger
{
private:
    const char *tag;
    std::ofstream log_file;

public:
    template<typename... Args> void LogTagStream(std::ostream& log_stream, const char *alternate_tag, const char *fmt, Args... args)
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
        char *buffer = (char*)std::malloc(buffer_size);
        std::snprintf(buffer, buffer_size, fmt, args...);
#pragma clang diagnostic pop

        // Calculate timestamp
        auto time_now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(time_now);
        auto timestamp = std::put_time(std::localtime(&time_t), "%F %T");

        // Log
        log_stream      << "[" << alternate_tag << "]\t" << timestamp << ": " << buffer << std::endl;
        this->log_file  << "[" << alternate_tag << "]\t" << timestamp << ": " << buffer << std::endl;

        // Free buffer
        std::free(buffer);
    }

    template<typename... Args> void LogTag(const char *alternate_tag, const char *fmt, Args... args)
    {
        this->template LogTagStream(std::cout, alternate_tag, fmt, args...);
    }

    template<typename... Args> void Log(const char *fmt, Args... args)
    {
        this->LogTagStream(std::cout, this->tag, fmt, args...);
    }

    template<typename... Args> void ErrorTag(const char *alternate_tag, const char *fmt, Args... args)
    {
        this->template LogTagStream(std::cerr, alternate_tag, fmt, args...);
    }

    template<typename... Args> void Error(const char *fmt, Args... args)
    {
        this->LogTagStream(std::cerr, this->tag, fmt, args...);
    }

    Logger() = delete;

    Logger(const char *tag, const std::filesystem::path& log_path) : tag(tag)
    {
        log_file.open(log_path, std::ios::app | std::ios::out);
    }
};


#endif //NOT_YOUR_SCHEDULER_LOGGER_H
