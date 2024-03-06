/*
 * @Description: Copyright (C) SHENZHEN JoyLife Robot Co.
 * @version: v0.01
 * @Author: litaobo
 * @Date: 2023-04-07 12:23:07
 * @LastEditors: litaobo
 * @LastEditTime: 2023-09-15 14:30:58
 */
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <dirent.h>

#include <stdlib.h>
#include <atomic>
#include <exception>
#include <vector>
#include <map>

#include <absl/debugging/failure_signal_handler.h>
#include <absl/debugging/stacktrace.h>
// #include <absl/debugging/symbolize.h>

#include <adk/utils/logger.h>

#include "Application.h"
#include "File.h"

#define DEBUG_FLAG_FILE "/mnt/UDISK/debug.flag"


#undef TAG
#define TAG "common.Application"

namespace {
    constexpr auto EXECABLE_DIR = "/mnt/app/bin/";
    constexpr auto CORE_FILE_DIR = "/mnt/UDISK/";       // 文件目录与文件名格式，查看/proc/sys/kernel/core_pattern

    std::atomic<bool> running(true);

    void exit_func()
    {
        pr_warn("Process has exited.");
    }

    void signal_handler(int sig)
    {
        pr_err("catch signal %d, will stop !", sig);
        running = false;
    }

    std::string toFuncName(void * pc)
    {
        std::string frame_info;

        constexpr int nptrs = 1;
        void *frame[nptrs] = {pc};
        char** strings = ::backtrace_symbols(frame, nptrs);

        for(int i = 0; i < nptrs && strings[i]; i++)
        {
            // pr_err(strings[i]);
            frame_info = strings[i];

            auto symbol_start = frame_info.find("(");
            auto symbol_end = frame_info.find("+");

            if(symbol_start != std::string::npos && symbol_end != std::string::npos)
            {
                symbol_start ++;
                auto symbol = frame_info.substr(symbol_start, symbol_end - symbol_start);

                size_t len = 512;
                int status = 0;
                char* demangled = static_cast<char*>(::malloc(len));
                char* ret = abi::__cxa_demangle(symbol.c_str(), demangled, &len, &status);
                if(status == 0)
                {
                    demangled = ret;  // ret could be realloc()
                    frame_info += " ";
                    frame_info += demangled;
                }
                free(demangled);
            }

        }
        free(strings);

        return frame_info;
    }

    void cxx_unexpected_handler()
    {
        pr_err("throws an exception of the type not listed in its exception specification.");
        std::terminate();
    }

    void cxx_terminate_handler()
    {
        pr_err("The CPP program terminates.");
        std::abort();
    }


    // NOTE: 尝试了下，并不是任何默认行为是终止进程的信号会捕获，
    // 查看failure_signal_handler.h,基本上是可以需要堆栈的信号才会捕获。
    void signalWriteFun(const char *msg)
    {
        // pr_dbg("backtrace frame msg: %s.", msg);
        std::string orig_msg = msg;

        if(orig_msg.find("***") != std::string::npos)
        {
            // 第一行，特殊信息，指出信号名等
            pr_err(msg);
            return;
        }

        auto pc_start = orig_msg.find("0x");
        auto pc_end = orig_msg.find(" ", pc_start);
        if(pc_start != std::string::npos && pc_end != std::string::npos)
        {
            std::string pc = orig_msg.substr(pc_start, pc_end - pc_start);
            auto pc_ull = std::stoull(pc, 0, 16);
            void *pc_ptr = (void *)pc_ull;
            // pr_dbg("pc: %s. pc_ull is %llu. pc_ptr is %p. ", pc.c_str(), pc_ull, pc_ptr);

            std::string frame_info = toFuncName(pc_ptr);

            if(frame_info.empty())
            {
                pr_err(orig_msg.c_str());
            }
            else
            {
                pr_err(frame_info.c_str());
            }
        }
    }

    void processSignal(const std::string &comm_name)
    {
        // std::string execFile = std::string(EXECABLE_DIR) + comm_name;
        // absl::InitializeSymbolizer(execFile.c_str());

        absl::FailureSignalHandlerOptions options;
        options.writerfn = signalWriteFun;
        absl::InstallFailureSignalHandler(options);

        struct sigaction sa;
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGHUP, &sa, NULL);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGALRM, &sa, NULL);
        sigaction(SIGSTKFLT, &sa, NULL);
        sigaction(SIGVTALRM, &sa, NULL);
        sigaction(SIGPROF, &sa, NULL);
        sigaction(SIGPWR, &sa, NULL);
        sigaction(SIGSYS, &sa, NULL);
        sigaction(SIGPOLL, &sa, NULL);
        sigaction(SIGUSR1, &sa, NULL);
        sigaction(SIGUSR2, &sa, NULL);
        sigaction(SIGXCPU, &sa, NULL);
        sigaction(SIGXFSZ, &sa, NULL);

        sigaction(SIGPIPE, &sa, NULL);      // 信号默认终止，但libcurl要求，自己处理,所以会被覆盖。
        // sigaction(SIGQUIT, &sa, NULL);   // 快捷键操作。信号默认终止且生成core，但被忽略
    }

    void processLoger(const bool is_debug_server)
    {
        // 日志支持
        if(is_debug_server){
            joyrobot::Logger::set_debug_level(joyrobot::Logger::kDebugLevel);
        } else{
            joyrobot::Logger::set_debug_level(joyrobot::Logger::kInfoLevel);
        }
        // EasyLogger默认情况下只有debug等级有函数名和行号的打印，此处设置成所有打印等级都打印函数名和行号
        size_t elog_flag = ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_FUNC | ELOG_FMT_LINE;
        elog_set_fmt(ELOG_LVL_INFO, elog_flag);
        elog_set_fmt(ELOG_LVL_WARN, elog_flag);
        elog_set_fmt(ELOG_LVL_ERROR, elog_flag);
    }

    void processCxxRuntimeError()
    {
        std::set_unexpected(cxx_unexpected_handler);
        std::set_terminate(cxx_terminate_handler);
    }

    void processCoreFiles(const std::string &comm_name, const bool is_debug_server)
    {
        if(is_debug_server)
        {
            // debug 条件下，启用core文件。
            struct rlimit rlim;
            rlim.rlim_cur = RLIM_INFINITY;
            rlim.rlim_max = RLIM_INFINITY;
            if(!setrlimit(RLIMIT_CORE, &rlim))
            {
                pr_info("Dump core file size is RLIM_INFINITY.");
            }

            DIR *dir = opendir(CORE_FILE_DIR);
            struct dirent *ptr;
            if(dir == nullptr){
                pr_err("Directory opening failure. Directory is %s.", CORE_FILE_DIR);
                return;
            }

            std::vector<std::string> coreFiles;
            while ((ptr = readdir(dir)) != nullptr)
            {
                if (ptr->d_type == DT_REG)
                {
                    std::string file_name = ptr->d_name;
                    if(file_name.find(comm_name) == 0)
                    {
                        coreFiles.emplace_back(std::move(file_name));
                    }
                }
            }
            closedir(dir);

            // 根据SIGNAL类型，每种SIGNAL保留最新的
            if(coreFiles.size() > 1)
            {
                // < SIGNAL, <timestamp, file_name> >
                std::map<int, std::pair<size_t, std::string>>  map_core_infos;
                auto fileNameAnalyse = [](const std::string &file_name)
                {
                    auto comm_name_end = file_name.find('.');
                    auto timestamp_begin = comm_name_end + 1;
                    auto timestamp_end = file_name.find('.', timestamp_begin);
                    auto pid_begin = timestamp_end + 1;
                    auto pid_end = file_name.find('.', pid_begin);
                    auto sig_begin = pid_end + 1;
                    auto sig_end = file_name.find('.', sig_begin);
                    auto file_ext = file_name.find(".core");

                    if(comm_name_end != std::string::npos &&
                    timestamp_begin != std::string::npos &&
                    timestamp_end != std::string::npos &&
                    pid_begin != std::string::npos &&
                    pid_end != std::string::npos &&
                    sig_begin != std::string::npos &&
                    sig_end != std::string::npos &&
                    file_ext != std::string::npos )
                    {
                        std::string timestamp = file_name.substr(timestamp_begin, timestamp_end - timestamp_begin);
                        std::string sig = file_name.substr(sig_begin, sig_end - sig_begin);

                        return std::pair<const int, std::pair<size_t, std::string>> {std::stoi(sig), {std::stoull(timestamp), file_name}};
                    }
                    else
                    {
                        pr_err("core file name parsing failed. file name is %s.", file_name.c_str());
                    }

                    return std::pair<const int, std::pair<size_t, std::string>> {0, {0, ""}};
                };

                for(const auto &file_name: coreFiles)
                {
                    auto info = fileNameAnalyse(file_name);
                    if(info.first > 0)
                    {
                        auto iter = map_core_infos.find(info.first);
                        if(iter == map_core_infos.end())
                        {
                            map_core_infos.insert(info);
                        }
                        else
                        {
                            std::string cmd = "rm ";
                            cmd += CORE_FILE_DIR;
                            if(info.second.first > iter->second.first)
                            {
                                cmd += iter->second.second;
                                map_core_infos[info.first] = info.second;
                            }
                            else
                            {
                                cmd += info.second.second;
                            }
                            system(cmd.c_str());
                            pr_info("exec system(%s).", cmd.c_str());
                        }
                    }
                }
            }
        }
        else
        {
            std::string cmd = "rm ";
            cmd += CORE_FILE_DIR;
            cmd += comm_name;
            cmd += ".*.core";
            system(cmd.c_str());
        }
    }

    std::string getCommName()
    {
        std::string comm_name = "unknow";
        joyrobot::File comm_file("/proc/self/comm");
        comm_file.open(true, false);

        comm_file.readAll(comm_name);
        return comm_name;
    }
}


namespace joyrobot
{
    namespace Application
    {
        void init(const bool is_debug_server)
        {
            auto comm_name = getCommName();
            atexit(exit_func);
            processLoger(is_debug_server);
            processSignal(comm_name);
            processCxxRuntimeError();

            processCoreFiles(comm_name, File::isExist(DEBUG_FLAG_FILE));

            pr_info("comm_name:%s", comm_name.c_str());
        }

        void waitExit()
        {
            while(running)
            {
                sleep(1);
            }
        }

        void printBacktrace(const std::string &tag)
        {
            constexpr int max_depth = 16;
            void* result[max_depth];
            int depth = absl::GetStackTrace(result, max_depth, 1);

            for(int i = 0; i < depth; ++i)
            {
                std::string frame_info = toFuncName(result[i]);
                if(frame_info.empty())
                {
                    pr_info("%s:[%d] %p.", tag.c_str(), i, result[i]);
                }
                else
                {
                    pr_info("%s:[%d] %s.", tag.c_str(), i, frame_info.c_str());
                }
            }
        }
    };
}
