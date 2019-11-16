#include <cstdio>
#include <cstdarg>
#include "logging.h"

#define TEAL "\033[36m"
#define WHITE "\033[1;37m"
#define YELLOW "\033[1;33m"
#define RED "\033[1;31m"
#define PURPLE "\033[1;35m"

Logger logger;

void Logger::Log(Level level, const char* format, ...) {
    switch (level) {
        case Level::Trace:
            fprintf(stdout, "trace: ");
            break;
        case Level::Debug:
            fprintf(stdout, TEAL "debug: ");
            break;
        case Level::Info:
            fprintf(stdout, WHITE "info: ");
            break;
        case Level::Warning:
            fprintf(stdout, YELLOW "warning: ");
            break;
        case Level::Error:
            fprintf(stdout, RED "error: ");
            break;
        case Level::Fatal:
            fprintf(stdout, PURPLE "fatal: ");
            break;
        case Level::LevelCount:
        default:
            break;
    }

    va_list args;
    va_start(args, format);
    std::string msg = Format(format, args);
    va_end(args);

    fprintf(stdout, "%s\033[0m\n", msg.c_str());
}

std::string Logger::Format(const char* format, va_list args) {
    char buffer[8192];
    
    vsnprintf(buffer, 8192, format, args);
    
    return std::string(buffer);
}

std::string Logger::Format(const char* format, ...) {
    char buffer[8192];
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 8192, format, args);
    va_end(args);

    return std::string(buffer);
}