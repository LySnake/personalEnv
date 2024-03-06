#include <errno.h>

#include <regex>
#include <vector>

#include "utils.h"

namespace zhengze
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
}
