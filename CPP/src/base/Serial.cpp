
#include <limits.h>
#include <unistd.h>

#include <algorithm>

#include "Serial.h"
#include "log.h"
#include "os.h"

constexpr int INVALID_FD = -1;

Serial::Serial(std::string &&device) : fd_{INVALID_FD}, device_{std::move(device)} {}

Serial::~Serial() { close(); }

Serial::Serial(Serial &&rhd) { *this = std::move(rhd); }

Serial &Serial::operator=(Serial &&rhd)
{
    if (this != &rhd)
    {
        close();

        fd_ = rhd.fd_;
        device_ = std::move(rhd.device_);

        rhd.fd_ = INVALID_FD;
    }
    return *this;
}

bool Serial::open(const int oflag, const speed_t speed, const unsigned char cc_VMIN, const unsigned char cc_VTIME)
{
    if (fd_ != INVALID_FD)
    {
        return false;
    }

    SPDLOG_INFO("open device [{}].", device_);
    struct termios Opt;
    fd_ = ::open(device_.c_str(), oflag);
    if (fd_ == INVALID_FD)
    {
        SPDLOG_ERROR("Open fail.Device is {}.", device_);
        return false;
    }
    tcgetattr(fd_, &Opt);
    cfsetispeed(&Opt, speed);
    Opt.c_lflag &= ~ICANON;
    Opt.c_lflag &= ~IEXTEN;
    Opt.c_lflag &= ~ISIG;
    Opt.c_lflag &= ~ECHO;

    Opt.c_iflag = 0;
    Opt.c_oflag = 0;

    Opt.c_cc[VMIN] = cc_VMIN;
    Opt.c_cc[VTIME] = cc_VTIME;

    tcflush(fd_, TCIFLUSH);
    tcflush(fd_, TCOFLUSH);

    /* 清空缓冲区 */
    if (0 > tcflush(fd_, TCIOFLUSH))
    {
        SPDLOG_ERROR("Device is {}.tcflush error: {}.", device_, os::POSIX_errno());
        close();
        return false;
    }

    if (tcsetattr(fd_, TCSANOW, &Opt) != 0)
    {
        SPDLOG_ERROR("Device is {}.Set uart setting failed!errno:{}.", device_, os::POSIX_errno());
        close();
        return false;
    }

    return true;
}

void Serial::close()
{
    if (fd_ != INVALID_FD)
    {
        ::close(fd_);
        fd_ = INVALID_FD;
    }
}

bool Serial::read(void *buff, const size_t buff_size, size_t &read_len)
{
    if (fd_ == INVALID_FD)
    {
        SPDLOG_ERROR("Invalid fd.Device is {}.", device_);
        return false;
    }

    if (buff_size > SSIZE_MAX)
    {
        SPDLOG_ERROR("buff_size:{}.len is greater than SSIZE_MAX. device is {}. ", buff_size, device_);
        return false;
    }

    bool result = false;
    read_len = 0;

    while (true)
    {
        int ret = ::read(fd_, reinterpret_cast<uint8_t *>(buff) + read_len, buff_size - read_len);
        if (-1 == ret)
        {
            if (errno == EINTR)
            {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                result = true;
                break;
            }

            SPDLOG_ERROR("read error. device is {}.errno:{}.", device_, os::POSIX_errno());
            break;
        }
        else
        {

            read_len += ret;
            result = true;
            break;
        }
    }

    return result;
}

bool Serial::write(const void *data, const size_t data_len, size_t &written)
{
    if (fd_ == INVALID_FD)
    {
        SPDLOG_ERROR("Invalid fd.Device is {}.", device_);
        return false;
    }

    if (data_len > SSIZE_MAX)
    {
        SPDLOG_ERROR("data_len:{}.len is greater than SSIZE_MAX. device is {}. ", data_len, device_);
        return false;
    }

    bool result = false;
    written = 0;
    while (true)
    {
        int ret = ::write(fd_, reinterpret_cast<const uint8_t *>(data) + written, data_len - written);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                result = true;
                break;
            }

            SPDLOG_ERROR("write error. device is {}.errno:{}.", device_, os::POSIX_errno());
            break;
        }
        else
        {
            written += ret;
            if (written < data_len)
            {
                continue;
            }

            result = true;
            break;
        }
    }

    return result;
}

bool Serial::flush(const int action)
{
    bool result = false;
    if (fd_ == INVALID_FD)
    {
        SPDLOG_ERROR("Invalid fd.Device is {}.", device_);
        return result;
    }

    auto ret = tcflush(fd_, action);
    if (ret == -1)
    {
        SPDLOG_ERROR("write error. device is {}.errno:{}.", device_, os::POSIX_errno());
    }
    else
    {
        result = true;
    }

    return result;
}