#include "log.h"
#include "Config.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void init_spdlog(const std::string name, const bool to_stdout)
{
  auto stdout_sink_ptr = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  stdout_sink_ptr->set_level(spdlog::level::debug);
  
  constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10M
  constexpr size_t MAX_FILES = 5;                    // 5个
    auto rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(ALIEN_CELL_LOG_RW_DIR + name + ".log",
                                                                                     MAX_FILE_SIZE, MAX_FILES);
    rotating_file_sink->set_level(spdlog::level::info);


    std::shared_ptr<spdlog::logger> logger;
    if(to_stdout)
    {
     logger =
      std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list{stdout_sink_ptr, rotating_file_sink});
    }
    else
    {
     logger =
      std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list{rotating_file_sink});
    }
    logger->flush_on(spdlog::level::info);
    logger->set_pattern("[%-4Y-%2m-%2d %-8T.%-3e] [%=7l] [%5P:%-5t] [%s::%!:%#] %v");

    spdlog::drop_all();
    spdlog::set_default_logger(logger);

    // 参数过少，会触发该打印
    // 其它spdlog内部的C++异常，捕获后以该方式打印。
    // spdlog::info("some invalid message to trigger an error {}{}{}{}", 3);
    spdlog::set_error_handler([](const std::string &msg) { spdlog::warn("*** spdlog error ***:{}.", msg); });
}

void deinit_spdlog()
{
    spdlog::info("spdlog shutdown.");
    spdlog::default_logger_raw()->flush();
    spdlog::shutdown();
}