/*
 * @Descripttion: Copyright (C) SHENZHEN JoyLife Robot Co.
 * @version: v0.01
 * @Author: weiqi
 * @Date: 2022-11-30 20:04:59
 * @LastEditors: litaobo
 * @LastEditTime: 2024-01-29 12:03:59
 */

#ifndef UTILS_H
#define UTILS_H

#include <string>

#define PR_THR_VAR_BASE(PR_TYPE, TIME_INTERVAL, FORMAT, ...)        \
    do{                                                              \
        static auto __time_stamp__ = 0;                             \
        static auto __interval__ = (uint64_t)(TIME_INTERVAL * 1e9); \
        if (get_time_of_ns() - __time_stamp__ > __interval__)       \
        {                                                           \
            __time_stamp__ = get_time_of_ns();                      \
            PR_TYPE(FORMAT, __VA_ARGS__);                           \
        }                                                           \
    }while(0);

#define PR_THR_BASE(PR_TYPE, TIME_INTERVAL, CONTENT)                \
    do{                                                             \
        static auto __time_stamp__ = 0;                             \
        static auto __interval__ = (uint64_t)(TIME_INTERVAL * 1e9); \
        if (get_time_of_ns() - __time_stamp__ > __interval__)       \
        {                                                           \
            __time_stamp__ = get_time_of_ns();                      \
            PR_TYPE(CONTENT);                                       \
        }                                                           \
    }while(0);

//打印间隔不能太密集，间隔时间太短的不打印，TIME_INTERVAL的单位为秒
#define pr_info_thr(TIME_INTERVAL, FORMAT, ...) PR_THR_VAR_BASE(pr_info, TIME_INTERVAL, FORMAT, __VA_ARGS__);
#define pr_warn_thr(TIME_INTERVAL, FORMAT, ...) PR_THR_VAR_BASE(pr_warn, TIME_INTERVAL, FORMAT, __VA_ARGS__);
#define pr_err_thr(TIME_INTERVAL, FORMAT, ...) PR_THR_VAR_BASE(pr_err, TIME_INTERVAL, FORMAT, __VA_ARGS__);

#define pr_info_thr_pure(TIME_INTERVAL, CONTENT) PR_THR_BASE(pr_info, TIME_INTERVAL, CONTENT);
#define pr_warn_thr_pure(TIME_INTERVAL, CONTENT) PR_THR_BASE(pr_warn, TIME_INTERVAL, CONTENT);
#define pr_err_thr_pure(TIME_INTERVAL, CONTENT) PR_THR_BASE(pr_err, TIME_INTERVAL, CONTENT);

// 证书文件
#define LIBCURL_CAPATH "/mnt/app/cacert.pem"

namespace joyrobot
{
    //对 POSIX 的 errno做可读化转化
    std::string POSIX_errno();

    pid_t getProcessPidByName(const std::string &process_name);
}


#endif // UTILS_H
