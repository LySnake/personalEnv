#ifndef SERIAL_H
#define SERIAL_H
#include <fcntl.h>
#include <string>
#include <termios.h>

#include "Define.h"

// 串口工具类
class Serial
{
  public:
    /****************************************************
     * 函数说明:构造函数
     * deivce: 需要操作的串口设备
     *****************************************************/
    Serial(std::string &&deivce);
    ~Serial();

    DISALLOW_COPY_OPERATOR(Serial);

    /****************************************************
     * 函数说明:移动函数
     * rhd: 被移动的对象。被移动后不可以再使用。
     *****************************************************/
    Serial(Serial &&rhd);
    Serial &operator=(Serial &&rhd);

    int fd() noexcept { return fd_; }
    const std::string &device() noexcept { return device_; }

  public:
    /****************************************************
     * 函数说明:打开串口设备
     *
     * oflag: 调用系统调用open时，使用的oflag参数
     * speed: 波特率
     * cc_VMIN:串口属性struct termios中的c_cc数组，VMIN成员的值:每次读期待字符个数。
     * cc_VTIME:串口属性struct termios中的c_cc数组，VTIME成员的值:阻塞等待时间，单位1/10s。
     *
     * return: true:打开且配置成功     false:打开或者配置失败，保持关闭
     *****************************************************/
    bool open(const int oflag = O_RDWR | O_NOCTTY, const speed_t speed = B115200, const unsigned char cc_VMIN = 1,
              const unsigned char cc_VTIME = 5);

    /****************************************************
     * 函数说明:关闭串口
     *****************************************************/
    void close();

    /****************************************************
     * 函数说明:向串口读数据。
     *
     * buff:读数据存放buff
     * buff_size:buff长度
     * read_len:成功读数据时，存放buff中有效数据长度.满足 read_len <= buff_size
     *
     * return:  true:成功读        false:读失败
     *****************************************************/
    bool read(void *buff, const size_t buff_size, size_t &read_len);

    /****************************************************
     * 函数说明:向串口写数据。
     *
     * data:写数据地址
     * data_len:data数据长度
     * written:已写入数据。如果设备标识O_NONBLOCK，那么可能存在成功返回，但written < data_len
     *
     * return:  true:成功写        false:写失败,不保证已写数据情况
     *****************************************************/
    bool write(const void *data, const size_t data_len, size_t &written);

    /****************************************************
     * 函数说明:把串口中缓冲的数据flush掉
     *
     * action:flush方式。
     *         TCIFLUSH：清空输入缓冲区（未读取的数据）。
     *         TCOFLUSH：清空输出缓冲区（未发送的数据）。
     *         TCIOFLUSH：同时清空输入和输出缓冲区
     *
     * return:  true:成功写        false:写失败,不保证已写数据情况
     *****************************************************/
    bool flush(const int action = TCIOFLUSH);

  private:
    int fd_;
    std::string device_;
};

#endif // SERIAL_H