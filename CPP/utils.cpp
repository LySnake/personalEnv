/*
 * @Description: Copyright (C) SHENZHEN JoyLife Robot Co.
 * @version: v0.01
 * @Author: litaobo
 * @Date: 2023-08-17 16:50:06
 * @LastEditors: litaobo
 * @LastEditTime: 2023-11-21 15:57:53
 */
#include <errno.h>

#include <regex>
#include <vector>

#include <adk/utils/logger.h>

#include "utils.h"
#undef TAG
#define TAG "common.utils"

namespace joyrobot
{
    std::string POSIX_errno()
    {
        char errno_buff[1024];
        std::string result = " errno is ";
        result += std::to_string(errno);
        result += " : ";
        result += strerror_r(errno, errno_buff, sizeof(errno_buff));

        return result;
    }

    pid_t getProcessPidByName(const std::string &process_name)
    {
        FILE *fp;
        char buf[100];
        char cmd[200] = {'\0'};
        pid_t pid = -1;
        sprintf(cmd, "pidof %s", process_name.c_str());

        if((fp = popen(cmd, "r")) != NULL)
        {
            if(fgets(buf, 255, fp) != NULL)
            {
                pid = atoi(buf);
            }
        }
        pclose(fp);
        return pid;
    }
}
