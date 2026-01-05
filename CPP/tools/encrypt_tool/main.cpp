#include "argparse.hpp"

#include "Define.h"
#include "File.h"
#include "log.h"

#include "EncryptTool.h"

// OTA文件加密:
//   私钥加密 plain_key         -> 标准输出 base64_key
//    plain_key 加密input_file -> out_file
// ./encrypt_tool input_file  --output out_file --key  plain_key  --pem private.pem
// OTA文件解密:
//   公钥解密base64_key       -> plain_key
//   plain_key解密input_file -> out_file
// ./encrypt_tool input_file -d  --output out_file --key KEY_DECRYPTION_KEY --pem public.pem

// 导出日志文件加密:
//   公钥加密 plain_key         -> 标准输出 base64_key
//    plain_key 加密input_file -> out_file
// ./encrypt_tool input_file  --output out_file --key KEY_DECRYPTION_KEY --pem public.pem
// 日志文件解密:
//   私钥解密base64_key       -> plain_key
//   plain_key解密input_file -> out_file
// ./encrypt_tool input_file -d  --output out_file --key KEY_DECRYPTION_KEY --pem private.pem

namespace
{
    const std::string KEY_INPUT_FILE = "input";     // 必要的参数 - 输入的文件
    const std::string KEY_VERBOSE = "--verbose";    // 是否调试
    const std::string KEY_IS_DECRYPTION = "-d";     // 是否是解密
    const std::string KEY_OUTPUT_FILE = "--output"; // 处理后的文件(要与input不同)
    const std::string KEY_PEM_FILE = "--pem";       // 指定使用的非对称秘钥文件(pem文件)
    const std::string KEY_DECRYPTION_KEY = "--key"; // 对称加解密秘钥-加密(明文)-解密(秘钥被加密后的base64值)
} // namespace

int main(int argc, char *argv[])
{
    argparse::ArgumentParser program("encrypt_tool", "1.0.0");

    //(必须)位置参数:输入的文件
    program.add_argument(KEY_INPUT_FILE).help("Processed file.").required();

    // 调试信息，仅仅开发时使用(否则干扰正常的stdout)
    program.add_argument(KEY_VERBOSE)
      .default_value(false)
      .implicit_value(true)
      .help("Print debug info. Do not use in the release environment.");

    // 对称秘钥
    program.add_argument(KEY_DECRYPTION_KEY).required();

    // 是否是解密
    program.add_argument(KEY_IS_DECRYPTION).default_value(false).implicit_value(true).help("Declassified document.");

    // 输出文件
    program.add_argument(KEY_OUTPUT_FILE).required().help("Specify the output file.");

    // 非对称秘钥
    program.add_argument(KEY_PEM_FILE).help("PEM file.").required();

    try
    {
        program.parse_args(argc, argv);

        const std::string input = program.get<std::string>(KEY_INPUT_FILE);
        const std::string output = program.get<std::string>(KEY_OUTPUT_FILE);
        const std::string secret_key = program.get<std::string>(KEY_DECRYPTION_KEY);
        const std::string pem_file = program.get<std::string>(KEY_PEM_FILE);
        const bool verbose = program[KEY_VERBOSE] == true;
        const bool is_decryption = program[KEY_IS_DECRYPTION] == true;

        init_spdlog("encrypt_tool", verbose);

        EncryptTool encrypt_tool;
        if (is_decryption)
        {
            return encrypt_tool.decryption(input, output, secret_key, pem_file) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        else
        {
            return encrypt_tool.encrypt(input, output, secret_key, pem_file) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }
    catch (const std::exception &err)
    {
        SPDLOG_ERROR("Catch exception: {}.", err.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}