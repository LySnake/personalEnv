#ifndef LOG_H
#define LOG_H

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

#include "Define.h"

void init_spdlog(const std::string name,const bool to_stdout = true);

void deinit_spdlog();

// SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG 成立时，存在SPDLOG_TRACE
// SPDLOG_TRACE
// SPDLOG_DEBUG
// SPDLOG_INFO
// SPDLOG_WARN
// SPDLOG_ERROR
// SPDLOG_CRITICAL
#endif // LOG_H