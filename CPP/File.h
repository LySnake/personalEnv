/*
 * @Descripttion: Copyright (C) SHENZHEN JoyLife Robot Co.
 * @version: v0.01
 * @Author: litaobo
 * @Date: 2022-11-30 20:04:59
 * @LastEditors: litaobo
 * @LastEditTime: 2023-05-08 10:17:48
 */

#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>

/****************************************************
 * 类说明：对普通文件的读写等操作，包加少量的不局限于普通文件的操作
*****************************************************/

namespace joyrobot
{
    class File
    {
        public:
            /****************************************************
             * 函数说明：构造需要操作文件对象，但在构造期不做任何文件操作(包括打开文件)
             *
             * file_name: 需要打开文件的文件名
            *****************************************************/
            explicit File(const std::string &file_name);
            ~File();

            /****************************************************
             * 函数说明：禁用copy功能
            *****************************************************/
            File(const File&) = delete;
            File &operator=(const File&) = delete;

            /****************************************************
             * 函数说明：启用移动说语义
            *****************************************************/
            File(File&&);
            File &operator=(File&&);

        public:
            /****************************************************
             * 函数说明：判定文件是否存在，不区分文件类型(目录、普通文件、块设备等，但不支持符号链接文件类型。
			 *           当为符号链接时，会判定符号链接指向的文件是否存在)。
             *
             *  NOTE: 但是，该类是针对普通文件操作的类。
             *
             * file_name: 需要判定文件的文件名
            *****************************************************/
            static bool isExist(const std::string &file_name);

        public:
            /****************************************************
             * 函数说明：根据构造时的文件名，打开文件，准备开始读写.如果文件不存在则创建。仅限普通文件。
             *          初始化pos为文件头，即为0.
             *
             * enable_read：是否打开文件时允许读操作
             * enable_write：是否打开文件时允许写操作
             *
             * return:
             *     true: 是普通文件，打开成功。
             *     false:打开失败，可能是目录这类非普通文件，也可能是权限等。
            *****************************************************/
            bool open(const bool enable_read = true, const bool enable_write = true);

            /****************************************************
             * 函数说明：关闭当前打开的文件
            *****************************************************/
            void close();
        public:
            /****************************************************
             * 函数说明：根据当前的pos,写入文件数据(NOTE：如果不是尾端，会覆盖旧数据。且随着写入，修改pos)
             *
             * buff：需要写入文件内容的BUFF
             * len：buff的大小
             *
             * return:
             *     true:写入成功
             *     false:写入失败
            *****************************************************/
            bool write(const void *buff, const size_t len);
            bool write(const std::string &content) { return write(content.c_str(), content.length()); }
            bool write(const std::vector<uint8_t> &content) { return write(content.data(), content.size()); }

            /****************************************************
             * 函数说明：清空文件并重写,写入文件数据(NOTE：先清空内容，再写入。且随着写入，修改pos)
             *
             * content:需要写入的内容
             *
             * return:
             *     true:写入成功
             *     false:写入失败
            *****************************************************/
            bool writeAll(const std::string &content){return clear() && write(content);};
            bool writeAll(const std::vector<uint8_t> &content){return clear() && write(content);};

            /****************************************************
             * 函数说明：在文件尾附加数据。(NOTE：强制把pos设置为尾端，并且不恢复)
             *
             * buff: 需要写入的数据
             * len: buff的大小
             * return:
             *     true:写入成功
             *     false:写入失败
            *****************************************************/
            bool append(const void *buff, const size_t len);
            bool append(const std::string &content) { return append(content.c_str(), content.length()); }
            bool append(const std::vector<uint8_t> &content) { return append(content.data(), content.size()); }

            /****************************************************
             * 函数说明：根据当前的pos,从文件中读数据。(NOTE：pos随读随移)
             *
             * buff:缓存数据的Buff
             * max_len： buff的大小。
             * len: 实读取的大小。
             * return:
             *     true:读成功
             *     false:读失败
            *****************************************************/
            bool read(uint8_t *buff, const size_t max_len, size_t &len);
            bool read(std::string &content);
            bool read(std::vector<uint8_t> &content);

            /****************************************************
             * 函数说明：从头读所有文件内容。(NOTE：pos随读随移)
             *
             * content:成功时，文件的所有内容。
             * return:
             *     true:读成功
             *     false:读失败
            *****************************************************/
           bool readAll(std::string &content){return head() && read(content);};
           bool readAll(std::vector<uint8_t> &content){return head() && read(content);};

        public:
            /****************************************************
             * 函数说明：获取文件的大小，单位为字节.（会把pos置于文件尾）
             *
             * size: 文件的大小
             * return:
             *     true: 获取成功
             *     false: 获取失败
            *****************************************************/
            bool size(size_t &size);

            /****************************************************
             * 函数说明：截断文件,必须有 写操作 打开。(不修改pos)
             *
             * size : 截断文件后的指定大小(byte)。
             *         如果size在[0, current size]之间，则截断多余文件内容。
             *         如果size > current size，则对文件扩容，且内容全为0的bit.
             * return:
             *     true: 截断成功
             *     false: 截断失败
            *****************************************************/
            bool truncate(const size_t size);

            /****************************************************
             * 函数说明：清空文件内容(NOTE: 设置pos为0)
            *****************************************************/
            bool clear() { return head() && truncate(0);};

            /****************************************************
             * 函数说明：获取文件的当前操作位置:pos
             *
             * pos: 文件读写位置,即本类所说的pos
             * return:
             *     true:获取成功
             *     false: 获取失败
            *****************************************************/
            bool getPos(size_t &pos);

            /****************************************************
             * 函数说明：设置文件的当前操作位置
             *
             * pos: 文件读写位置,即本类所说的pos
             * return:
             *     true: 设置成功
             *     false: 设置失败
            *****************************************************/
            bool setPos(const size_t pos);

            /****************************************************
             * 函数说明：基于当前位置pos并修改它，向前或向后偏移位置
             *
             * offset: 偏移量，可负数可正数
             * return:
             *     true: 移动成功
             *     false: 移动失败
            *****************************************************/
            bool move(const long offset);

            /****************************************************
             * 函数说明：设置位置pos为0
             *
             * return:
             *     true: 设置成功
             *     false: 设置失败
            *****************************************************/
            bool head() { return setPos(0);};

            /****************************************************
             * 函数说明：设置位置pos为尾
             *
             * return:
             *     true: 设置成功
             *     false: 设置失败
            *****************************************************/
            bool tail()
            {
                size_t pos;
                return size(pos) && setPos(pos);
            };

        private:
            int fd_;                         // 文件的描述符
            std::string file_name_;          // 文件名
    };
}


#endif // FILE_H
