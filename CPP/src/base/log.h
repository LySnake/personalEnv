#ifndef LOG_H
#define LOG_H

#include <cstdarg>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

// 定义日志等级
enum LogLevel
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
};

// 当前日志等级，可根据需要修改

void logMessage(LogLevel level, const char *file, int line, const char *format, ...);
// 定义日志宏
#define LOG_DEBUG(format, ...) logMessage(LOG_LEVEL_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) logMessage(LOG_LEVEL_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) logMessage(LOG_LEVEL_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) logMessage(LOG_LEVEL_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)

#endif // LOG_H