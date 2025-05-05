#include <cxxabi.h>
#include <errno.h>
#include <execinfo.h>

#include <array>
#include <regex>
#include <vector>

#include "log.h"
#include "os.h"
#include "File.h"

namespace os
{

    std::pair<bool, std::string> exec_shell(const std::string &shell_cmd)
    {
        std::array<char, 1024> buffer;
        std::string result;

        LOG_INFO("Exec shell cmd: %s.", shell_cmd.c_str());

        // 使用 popen 执行命令并将 stderr 重定向到 stdout
        FILE *pipe = popen((shell_cmd + " 2>&1").c_str(), "r");
        if (!pipe)
        {
            throw std::runtime_error("can not open the pipe");
        }

        // 读取命令输出
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            result += buffer.data();
        }

        // 获取子进程的退出状态
        int status = pclose(pipe);
        int exit_code = WEXITSTATUS(status); // 获取退出码

        return {exit_code == EXIT_SUCCESS, std::move(result)};
    }

    std::string POSIX_errno()
    {
        char errno_buff[1024];
        std::string result = " errno is ";
        result += std::to_string(errno);
        result += " : ";
        result += strerror_r(errno, errno_buff, sizeof(errno_buff));

        return result;
    }

    void printBacktrace(const std::string &tag)
    {
        constexpr int max_depth = 16;
        void *result[max_depth];
        int depth = ::backtrace(result, max_depth);

        char **strings = ::backtrace_symbols(result, depth);

        std::string frame_info;
        size_t len = 512;
        char *demangled = static_cast<char *>(::malloc(len));
        for (int i = 0; i < depth && strings[i]; i++)
        {
            frame_info = strings[i];

            // LOG_INFO("frame_info: %s.", frame_info.c_str());
            auto symbol_start = frame_info.find("(");

            if (symbol_start != std::string::npos)
            {
                auto symbol_end = frame_info.find("+", symbol_start);
                if (symbol_end != std::string::npos)
                {
                    symbol_start++;
                    auto symbol = frame_info.substr(symbol_start, symbol_end - symbol_start);

                    int status = 0;
                    char *ret = abi::__cxa_demangle(symbol.c_str(), demangled, &len, &status);
                    if (status == 0)
                    {
                        demangled = ret; // ret could be realloc()
                        frame_info += " ";
                        frame_info += demangled;
                    }
                }
            }

            if (frame_info.empty())
            {
                LOG_INFO("%s:[%d] %p.", tag.c_str(), i, result[i]);
            }
            else
            {
                LOG_INFO("%s:[%d] %s.", tag.c_str(), i, frame_info.c_str());
            }
        }
        free(demangled);
        free(strings);
    }

    std::string dir_name(const std::string &path)
    {
        auto pos = path.find_last_of("/");
        return pos != std::string::npos ? path.substr(0, pos) : std::string{};
    }

    std::string base_name(const std::string &path)
    {
        auto pos = path.find_last_of("/");
        return pos != std::string::npos ? path.substr(pos + 1) : std::string{};
    }

    std::string getCommName()
    {
        std::string comm_name = "unknow";
        File comm_file("/proc/self/comm");
        comm_file.open(true, false);

        comm_file.readAll(comm_name);
        return comm_name;
    }
} // namespace os
