#include "EncryptTool.h"
#include "log.h"
#include "os.h"

bool EncryptTool::encrypt(const std::string &in_file, const std::string &out_file, const std::string &secret_key,
                          const std::string &in_key_file)
{
    SPDLOG_INFO("encrypted file. file is {}. key is {}.", in_file, in_key_file);
    bool ret = false;

    std::string sign_base64;
    if (signature(secret_key, sign_base64, in_key_file))
    {
        if (symmetrical_encryption(in_file, out_file, secret_key))
        {
            ret = true;
            // 标准输出:提供给调用方记录
            printf("%s", sign_base64.c_str());
        }
        else
        {
            SPDLOG_ERROR("Symmetric encryption file failed.");
        }
    }
    else
    {
        SPDLOG_ERROR("Signature failed. secret_key is {}. key is {}.", secret_key, in_key_file);
    }
    return ret;
}

bool EncryptTool::decryption(const std::string &in_file, const std::string &out_file, const std::string &base64_sign,
                             const std::string &in_key_file)
{
    SPDLOG_INFO("Decryption file. file is {}. base64_sign is {}. key is {}.", in_file, base64_sign,
             in_key_file);
    bool ret = false;
    std::string secret_key;
    if (unsignature(base64_sign, secret_key, in_key_file))
    {
        if (symmetrical_decryption(in_file, out_file, secret_key))
        {
            ret = true;
            printf("%s", secret_key.c_str());
        }
        else
        {
            SPDLOG_ERROR("Decryption of the file failed.");
        }
    }
    else
    {
        SPDLOG_ERROR("Decryption of signature failed.");
    }

    return ret;
}

bool EncryptTool::signature(const std::string &secret_key, std::string &sign_base64, const std::string &in_key_file)
{
    std::string shell_cmd;
    if (is_private_key(in_key_file))
    {
        shell_cmd =
          "echo \"" + secret_key + "\" | openssl pkeyutl -sign -inkey " + in_key_file + " | openssl base64 -A";
    }
    else
    {
        shell_cmd = "echo \"" + secret_key + "\" |  openssl pkeyutl -encrypt -pubin -inkey " + in_key_file +
                    " | openssl base64 -A";
    }

    auto reuslt = os::exec_shell(shell_cmd);
    if (reuslt.first)
    {
        sign_base64 = reuslt.second;
    }
    else
    {
        SPDLOG_ERROR("cmd: {}. stdout:{}.", shell_cmd, reuslt.second);
    }

    return reuslt.first;
}

bool EncryptTool::unsignature(const std::string &sign_base64, std::string &secret_key, const std::string &in_key_file)
{
    std::string shell_cmd;
    if (is_private_key(in_key_file))
    {
        shell_cmd =
          "echo \"" + sign_base64 + "\" | openssl base64 -d | openssl pkeyutl -decrypt  -inkey " + in_key_file;
    }
    else
    {
        shell_cmd = "echo \"" + sign_base64 + "\" | openssl base64 -d | openssl pkeyutl -verifyrecover -pubin -inkey " +
                    in_key_file;
    }

    auto reuslt = os::exec_shell(shell_cmd);
    if (reuslt.first)
    {
        secret_key = reuslt.second;
    }
    else
    {
        SPDLOG_ERROR("cmd: {}. stdout:{}.", shell_cmd, reuslt.second);
    }

    return reuslt.first;
}

bool EncryptTool::symmetrical_encryption(const std::string &in_file, const std::string &out_file,
                                         const std::string &key)
{
    const std::string shell_cmd =
      "openssl enc -aes-256-cbc -pbkdf2 -iter 10000 -in " + in_file + " -out " + out_file + " -pass pass:" + key;
    auto reuslt = os::exec_shell(shell_cmd);
    if (!reuslt.first)
    {
        SPDLOG_ERROR("cmd: {}. stdout:{}.", shell_cmd, reuslt.second);
    }

    return reuslt.first;
}

bool EncryptTool::symmetrical_decryption(const std::string &in_file, const std::string &out_file,
                                         const std::string &key)
{
    const std::string shell_cmd =
      "openssl enc -d -aes-256-cbc -pbkdf2 -iter 10000 -in " + in_file + " -out " + out_file + " -pass pass:" + key;
    auto reuslt = os::exec_shell(shell_cmd);
    if (!reuslt.first)
    {
        SPDLOG_ERROR("cmd: {}. stdout:{}.", shell_cmd, reuslt.second);
    }

    return reuslt.first;
}

bool EncryptTool::is_private_key(const std::string &in_key_file)
{
    const std::string shell_cmd = "openssl rsa -in " + in_key_file + " -noout 2> /dev/null";
    auto reuslt = os::exec_shell(shell_cmd);
    return reuslt.first;
}
