#ifndef OS_H
#define OS_H

#include <string>

namespace os
{
    /****************************************************
     * 函数说明：使用popen，执行shell命令，获取stdout 和 stderr的输出内容
     *
     * shell_cmd: 需要执行的shell cmd.
     *
     * return:
     *         bool:  true:shell cmd以 exit(EXIT_SUCCESS)终止. false:其它
     *  std::string: 命令执行的stdout和stderr
     *****************************************************/
    std::pair<bool, std::string> exec_shell(const std::string &shell_cmd);

    /****************************************************
     * 函数说明：对 POSIX 的 errno做可读化转化
     *
     * return: errno相关描述信息
     *****************************************************/
    std::string POSIX_errno();

    /****************************************************
     * 函数说明：打印堆栈
     *
     * tag：打印堆栈信息的标识
     *****************************************************/
    void printBacktrace(const std::string &tag);

    /****************************************************
     * 函数说明：输入路径，返回路径的上一目录
     *     如:
     *       "/a/b/c" ->   "/a/b"
     *       "/a/b/"  ->   "/a/b"
     *
     * path: linux风格路径
     *
     * return: 目录路径
     *****************************************************/
    std::string dir_name(const std::string &path);

    /****************************************************
     * 函数说明：输入路径，返回不带路径的文件名/目录名
     *     如:
     *       "/a/b/c" ->   "c"
     *       "/a/b/"  ->   ""
     *
     * path: linux风格路径
     *
     * return: 文件名或者目录名
     *****************************************************/
    std::string base_name(const std::string &path);
} // namespace os

#endif // OS_H
