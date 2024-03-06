

#ifndef APPLICATION_H
#define APPLICATION_H

/****************************************************
 * 工具说明：
 *     用于设置进程相关的环境，比如日志初始化、进程终止信息、
 * 通过信号特殊处理等。
 *
 * NOTE:对于现有的openwrt环境，很有必要说明一些情况，那
 *      就是：此工具并不能按理论上运行。并不是实现不行，而是
 *      可能我们的openwrt是嵌入式系统以及构造镜像的原因，导致不能
 *      像在PC linux中那样完善的debug环境。以下是发生的案例：
 *
 *      案例1:
 *         米家OTA压测，升级与降级共100次，在60多次时，突然OTA失败。
 *         查看日志，发现iot_server在获取到libcurl库使用PUT url时，
 *         进程就没了，然后brain_hub再把iot_server拉起。导致OTA流程无法继续。
 *
 *         现象：
 *         1. libcurl.so库文件的MD5值，与近期同一天出的镜象中的MD5值不一样。
 *         2. 拿另一台正常设备中的libcurl.so替换，即可恢复。
 *         3. 当使用错误的curl库时，该类也没捕获到SIGSEGV.
 *         4. 当使用错误的curl库时，使用系统调用的signal也无法捕获SIGSEGV.
 *         5. 当使用错误的curl库时，在未触发该SIGSEGV时，主动发送SIGSEGV，是能捕获的。
 *         6. 启用core文件，在core文件中，能够看到是SIGSEGV造成的进程终止。
 *
 *         结论:
 *         1. OTA时，存在万次OTA中，可能有一次，使得OTA后的某些文件被更改。
 *         2. 特殊情况下，可能信号无法捕获，只能信任core能提供更多信息。因此，可能日志没法记录进程终止信息。
 *         3. 猜测的原因，可能是因为系统是嵌入式系统，裁剪过多东西，所以没有PC linxu上那么全。
 *
 *     案例2:
 *        米家使用libcurl.so时，因为该库在多线程环境下需要特殊设置的原因，未设置的情况下，当网络不好时，会异常终止在libc库中。
 *        在使用该工具类时或生成core文件，能看到libc调用abort产生的SIGABRT信号及堆栈，但是只有libc的堆栈，最上层堆栈不可见。
 *        但是，使用之前的一个镜像，用同样的程序，使用该工具类或生成的core文件，能看到libc调用abort产生的SIGABRT信号及堆栈，
 *        以及最上层堆栈为来自libcurl.so库。
 *
 *
 *        现象：
 *          1. 在A镜像下，core中的堆栈，能看到最上层堆栈所属库。
 *          2. 在B镜像下，core中的堆栈，不能看到最上层堆栈所属库。
 *          3. 相同程序，分别在A、B镜像下生成core文件a.core和b.core。然后分别在A镜像
 *             或B镜像中，通过gdb查看堆栈，会发现：
 *             i.  a.core和b.core在A镜像中，使用gdb查看堆栈，能看到最深栈帧来自libcurl.so.
 *             ii. a.core和b.core在B镜像中，使用gdb查看堆栈，不能看到最深栈帧来自libcurl.so.
 *          4. 在该工具类中，捕获后打印的堆栈与core中不一样,无论是使用absl::debug还是系统调用直接捕获打印。
 *          5. 存在一些ERROR级日志，输出在标准输出，而不在我们的日志文件中。
 *
 *        结论:
 *          1. 应该是镜像构建时，有类似"支持debug"的功能，所以A、B镜像不一样。
 *          2. 日志系统使用的是syslog收集日志，但libc中，有错误日志输出到标准输出。
 *
 * NOTE:所以，本工具类能在一定程度上记录进程终止时的信息，但当失效时则需要core文件，甚至需要一个能提供更多debug信号的镜像。
 *
*****************************************************/

namespace joyrobot
{
    namespace Application
    {
        // 初始化进程相关的环境配置
        void init(const bool is_debug_server = true);
        // 阻塞等待退出
        void waitExit();
        // 打印函数栈
        void printBacktrace(const std::string &tag = "unknow");
    };
}


#endif // APPLICATION_H
