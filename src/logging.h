#pragma once

#include <string>

enum class Level {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    LevelCount, // number of levels
};

class Logger {
public:
    void Log(Level level, const char* format, ...);

    std::string Format(const char* format, va_list args);
    std::string Format(const char* format, ...);
};

extern Logger logger;

#define LTRACE(format, ...) logger.Log(Level::Trace, format, ##__VA_ARGS__)
#define LDEBUG(format, ...) logger.Log(Level::Debug, format, ##__VA_ARGS__)
#define LINFO(format, ...) logger.Log(Level::Info, format, ##__VA_ARGS__)
#define LWARN(format, ...) logger.Log(Level::Warning, format, ##__VA_ARGS__)
#define LERROR(format, ...) logger.Log(Level::Error, format, ##__VA_ARGS__)
#define LFATAL(format, ...) logger.Log(Level::Fatal, format, ##__VA_ARGS__)
