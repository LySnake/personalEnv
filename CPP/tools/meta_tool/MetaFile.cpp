#include <arpa/inet.h>

#include "File.h"
#include "MetaFile.h"
#include "log.h"
#include "os.h"

namespace
{

    // | -- 任意长度     --|-- 存在最大长度--|--     2B       --|--   1B     --|--   2B     --|
    // | -- File原始内容 --|--  meta信息  --|-- meta信息长度  --|-- meta版本 --|-- Meta head --|

    //文件尾巴,meta头
    constexpr uint16_t META_FRAME_HEAD = 0x9527;

    // V1版本，基本信息
    using META_DATA_LEN_T = uint16_t;
    constexpr uint8_t META_VERSION_V1 = 0x01;
    constexpr META_DATA_LEN_T META_DATA_MAX_LEN = 1024 * 4;
    constexpr uint32_t META_BASE_INFO_LEN_V1 =
      sizeof(META_FRAME_HEAD) + sizeof(META_VERSION_V1) + sizeof(META_DATA_LEN_T);

} // namespace

int MetaFile::handleFile()
{
    const bool succeed = args_.have_meta() ? appendMetaFile() : parseFile();
    return succeed ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool MetaFile::appendMetaFile()
{
    bool result = false;

    do
    {
        if (args_.have_outfile() || args_.is_in_place())
        {
            if (File::isExist(args_.meta_file()))
            {
                std::string out_file_path;
                if (args_.is_in_place())
                {
                    out_file_path = args_.input_file();
                }
                else
                {
                    const std::string shell_cmd_cp = "cp " + args_.input_file() + " " + args_.output_file();
                    const auto ret = os::exec_shell(shell_cmd_cp);
                    if (ret.first)
                    {
                        out_file_path = args_.output_file();
                    }
                    else
                    {
                        SPDLOG_ERROR("Output file failed.");
                        break;
                    }
                }

                File out_file(out_file_path);
                File meta_file(args_.meta_file());

                std::vector<uint8_t> meta_content;
                if (meta_file.open(true, false) && meta_file.readAll(meta_content))
                {
                    if (meta_content.size() <= META_DATA_MAX_LEN)
                    {
                        if (out_file.open(false, true) && out_file.append(meta_content))
                        {
                            uint8_t meta_base[META_BASE_INFO_LEN_V1] = {0};

                            uint32_t index = 0;
                            *reinterpret_cast<META_DATA_LEN_T *>(meta_base + index) =
                              htons(static_cast<META_DATA_LEN_T>(meta_content.size()));
                            index += sizeof(META_DATA_LEN_T);

                            meta_base[index++] = META_VERSION_V1;

                            *reinterpret_cast<uint16_t *>(meta_base + index) = htons(META_FRAME_HEAD);
                            index += sizeof(META_FRAME_HEAD);

                            if (out_file.append(meta_base, sizeof(meta_base)))
                            {
                                result = true;
                            }
                            else
                            {
                                SPDLOG_ERROR("Failed to add META base info.");
                            }
                        }
                        else
                        {
                            SPDLOG_ERROR("Failed to add META data.");
                        }
                    }
                    else
                    {
                        SPDLOG_ERROR("The META data is too large.");
                    }
                }
                else
                {
                    SPDLOG_ERROR("Failed to read the META file.");
                }
            }
            else
            {
                SPDLOG_ERROR("The Meta file does not exist.");
            }
        }
        else
        {
            SPDLOG_ERROR("Required parameters. Parameters is {} or {}.", KEY_OUTPUT_FILE, KEY_IN_PLACE);
        }
    } while (false);

    return result;
}

bool MetaFile::parseFile()
{
    bool result = false;

    do
    {
        File input_file(args_.input_file());
        size_t input_file_size = 0;
        if (input_file.open(true, false) && input_file.size(input_file_size))
        {
            uint8_t meta_base[META_BASE_INFO_LEN_V1] = {};
            size_t read_len = 0;
            if (input_file.setPos(input_file_size - sizeof(meta_base)) &&
                input_file.read(meta_base, sizeof(meta_base), read_len) && read_len == sizeof(meta_base))
            {
                uint32_t index = 0;
                const META_DATA_LEN_T meta_data_len = ntohs(*reinterpret_cast<META_DATA_LEN_T *>(meta_base + index));
                index += sizeof(META_DATA_LEN_T);

                const auto meta_version = meta_base[index++];
                const uint16_t meta_head = ntohs(*reinterpret_cast<uint16_t *>(meta_base + index));
                index += sizeof(meta_head);

                if (meta_head == META_FRAME_HEAD)
                {
                    if (meta_version == META_VERSION_V1)
                    {
                        std::string meta_content;
                        size_t ori_file_size = input_file_size - sizeof(meta_base) - meta_data_len;
                        if (input_file.setPos(ori_file_size) && input_file.read(meta_content))
                        {
                            input_file.close();
                            meta_content.resize(meta_content.size() - sizeof(meta_base));

                            // 标准输出:输出meta信息
                            printf("%s", meta_content.c_str());

                            if (args_.have_outfile() || args_.is_in_place())
                            {
                                std::string out_file_path;
                                if (args_.is_in_place())
                                {
                                    out_file_path = args_.input_file();
                                }
                                else
                                {
                                    const std::string shell_cmd_cp =
                                      "cp " + args_.input_file() + " " + args_.output_file();
                                    const auto ret = os::exec_shell(shell_cmd_cp);
                                    if (ret.first)
                                    {
                                        out_file_path = args_.output_file();
                                    }
                                    else
                                    {
                                        SPDLOG_ERROR("Output file failed.");
                                        break;
                                    }
                                }

                                File output_file(out_file_path);
                                if (output_file.open(false, true) && output_file.truncate(ori_file_size))
                                {
                                    result = true;
                                }
                                else
                                {
                                    SPDLOG_ERROR("File restoration failed.");
                                    output_file.close();
                                    const std::string shell_cmd_rm = "rm " + args_.output_file();
                                    os::exec_shell(shell_cmd_rm);
                                }
                            }
                            else
                            {
                                result = true;
                            }
                        }
                        else
                        {
                            SPDLOG_ERROR("Failed to read META data.");
                        }
                    }
                    else
                    {
                        SPDLOG_ERROR("The META version does not match. There might be no META information.");
                    }
                }
                else
                {
                    SPDLOG_ERROR("Meta header does not match, possibly there is no META information.");
                }
            }
            else
            {
                SPDLOG_ERROR("Failed to read META base info.");
            }
        }
        else
        {
            SPDLOG_ERROR("Operation on the input file failed.");
        }
    } while (false);

    return result;
}