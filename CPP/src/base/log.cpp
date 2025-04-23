#include "log.h"
#include "Config.h"

LogLevel currentLogLevel = LOG_LEVEL_DEBUG;
// 获取当前时间戳字符串
std::string getTimestamp()
{
    std::time_t now = std::time(nullptr);
    char *dt = std::ctime(&now);
    std::string timestamp(dt);
    // 去掉换行符
    if (!timestamp.empty() && timestamp[timestamp.length() - 1] == '\n')
    {
        timestamp.erase(timestamp.length() - 1);
    }
    return timestamp;
}

// 日志打印函数，定义为内联函数
void logMessage(LogLevel level, const char *file, int line, const char *format, ...)
{
    if (level < currentLogLevel)
    {
        return;
    }

    std::string levelStr;
    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        levelStr = "DEBUG";
        break;
    case LOG_LEVEL_INFO:
        levelStr = "INFO";
        break;
    case LOG_LEVEL_WARN:
        levelStr = "WARN";
        break;
    case LOG_LEVEL_ERROR:
        levelStr = "ERROR";
        break;
    default:
        levelStr = "UNKNOWN";
        break;
    }

    std::string timestamp = getTimestamp();

    // 打开日志文件，以追加模式写入
    std::ofstream logFile(PLATFORM_DEF_LOG_DIR "aliend.log", std::ios::app);
    if (!logFile.is_open())
    {
        std::cerr << "Failed to open log file." << std::endl;
        return;
    }

    logFile << "[" << timestamp << "] [" << levelStr << "] [" << file << ":" << line << "] ";

    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    logFile << buffer << std::endl;
    logFile.close();
}