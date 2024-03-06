#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <limits.h>



#include "utils.h"
#include "File.h"


namespace zhengze
{
    File::File(const std::string &file_name):fd_{-1}, file_name_{file_name}
    {
    }

    File::~File()
    {
        close();
    }

    File::File(File&& rh)
    {
        *this = std::move(rh);
    }

    File &File::operator=(File&& rh)
    {
        close();

        fd_ = rh.fd_;
        file_name_ = rh.file_name_;

        rh.fd_ = -1;
        rh.file_name_.clear();

        return *this;
    }

    bool File::isExist(const std::string &file_name)
    {
        struct stat statbuf;
        auto ret = ::stat(file_name.c_str(), &statbuf);
        if(-1 == ret)
        {
            pr_warn("function: %s.filename is %s. %s", __FUNCTION__, file_name.c_str(), POSIX_errno().c_str());
            return false;
        }

        return ret == 0;
    }

    bool File::open(const bool enable_read, const bool enable_write)
    {
        if(!enable_read && !enable_write)
        {
            pr_err("function: %s. filename is %s.Invalid parameter. enable_read is false and enable_write is false.", __FUNCTION__, file_name_.c_str());
            return false;
        }

        if(fd_ != -1)
        {
            pr_err("File has been opened. filename is %s.", file_name_.c_str());
            return false;
        }

        int flags = O_CLOEXEC;
        int mode = 0;
        if(enable_read && enable_write)
        {
            flags |= O_CREAT;         // 文件不存在时，创建。
            flags |= O_RDWR;
            flags |= O_SYNC;

            mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        }
        else
        {
            if(enable_write)
            {
                flags |= O_CREAT;     // 文件不存在时，创建。
                flags |= O_WRONLY;
                flags |= O_SYNC;

                mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            }
            else if(enable_read)
            {
                flags |= O_RDONLY;
            }
        }

        fd_ = ::open(file_name_.c_str(), flags, mode);
        if(fd_ == -1)
        {
            pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
            return false;
        }

        struct stat statbuf;
        auto ret = ::fstat(fd_, &statbuf);
        if(-1 == ret)
        {
            pr_err("function: %s. fd is %d. filename is %s. %s", __FUNCTION__, fd_, file_name_.c_str(), POSIX_errno().c_str());
            return false;
        }
        auto file_type = statbuf.st_mode & S_IFMT;
        if(file_type != S_IFREG) //是否是普通文件
        {
            pr_err("function: %s. file_type: %d. filename is %s. ", __FUNCTION__, (int)file_type, file_name_.c_str());
            return false;
        }

        return true;
    }

    void File::close()
    {
        if(-1 != fd_)
        {
            auto ret = ::close(fd_);
            if(-1 == ret)
            {
                pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
            }
            fd_ = -1;
            file_name_.clear();
        }
    }

    bool File::write(const void *buff, const size_t len)
    {
        if(len > SSIZE_MAX)
        {
            pr_err("len:%llu.len is greater than SSIZE_MAX. filename is %s. ", len, file_name_.c_str());
            return false;
        }

        size_t writed_count = 0;
        do
        {
            const auto ret = ::write(fd_, (char *)buff + writed_count, len - writed_count);
            if(ret == -1)
            {
                if(errno != EINTR)
                {
                    pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
                    return false;
                }
            }
            else
            {
                writed_count += ret;
            }

            if(writed_count == len || ret == 0)
            {
                break;
            }
        } while (true);

        return true;
    }

    bool File::append(const void *buff, const size_t len)
    {
        if(len > SSIZE_MAX)
        {
            pr_err("len:%llu.len is greater than SSIZE_MAX. filename is %s. ", len, file_name_.c_str());
            return false;
        }

        return tail() && write(buff, len);
    }

    bool File::read(uint8_t *buff, const size_t max_len, size_t &len)
    {
        if(max_len > SSIZE_MAX)
        {
            pr_err("len:%llu.len is greater than SSIZE_MAX. filename is %s. ", len, file_name_.c_str());
            return false;
        }

        size_t read_count = 0;
        do
        {
            const auto ret = ::read(fd_, buff + read_count, max_len - read_count);
            if(ret == -1)
            {
                if(errno != EINTR)
                {
                    pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
                    return false;
                }
            }
            else
            {
                read_count += ret;
            }

            if(read_count == max_len || ret == 0)
            {
                len = read_count;
                break;
            }
        } while (true);

        return true;
    }

    bool File::read(std::string & content)
    {
        std::vector<uint8_t> vecContent;
        if(read(vecContent))
        {
            content.clear();
            content.reserve(vecContent.size() + 1);

            content.insert(content.end(), vecContent.begin(), vecContent.end());
            return true;
        }
        return false;
    }

    bool File::read(std::vector<uint8_t> &content)
    {
        uint8_t buff[1024];
        size_t len = 0;

        content.clear();
        content.reserve(sizeof(buff));

        do
        {
            if(read(buff, sizeof(buff), len))
            {
                if(len == 0)
                {
                    break;
                }
                else
                {
                    content.insert(content.end(), buff, buff + len);
                }
            }
            else
            {
                return false;
            }
        }while(true);

        return true;
    }

    bool File::truncate(const size_t size)
    {
        do
        {
            auto ret = ::ftruncate(fd_, (off_t)size);
            if(ret == -1)
            {
                if(errno != EINTR)
                {
                    pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
                    return false;
                }
                continue;
            }
            break;
        } while (true);
        return true;
    }

    bool File::size(size_t &size)
    {
        auto ret = ::lseek(fd_, 0, SEEK_END);
        if(ret == -1)
        {
            pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
            return false;
        }

        size = ret;
        return true;
    }

    bool File::getPos(size_t &pos)
    {
        auto ret = ::lseek(fd_, 0, SEEK_CUR);
        if(ret == -1)
        {
            pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
            return false;
        }

        pos = ret;
        return true;
    }

    bool File::setPos(const size_t pos)
    {
        auto ret = ::lseek(fd_, (off_t)pos, SEEK_SET);
        if(ret == -1)
        {
            pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
            return false;
        }

        return true;
    }

    bool File::move(const long offset)
    {
        auto ret = ::lseek(fd_, (off_t)offset, SEEK_CUR);
        if(ret == -1)
        {
            pr_err("function: %s. filename is %s. %s", __FUNCTION__, file_name_.c_str(), POSIX_errno().c_str());
            return false;
        }

        return true;
    }
}


