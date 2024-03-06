#ifndef UTILS_H
#define UTILS_H

#include <string>


#define pr_dbg()
#define pr_info()
#define pr_warn()
#define pr_err()


namespace zhengze
{
    //对 POSIX 的 errno做可读化转化
    std::string POSIX_errno();
}


#endif // UTILS_H
