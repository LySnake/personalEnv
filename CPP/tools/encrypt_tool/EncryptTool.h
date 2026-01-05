#ifndef ENCRYPT_TOOL_H
#define ENCRYPT_TOOL_H

#include <string>

// NOTE:因为公钥、私钥使用上都是不公开发布的，所以场景上不区分[私钥签名、公钥验证签名]。
//      只是公钥在设备上，私钥在代码仓库中。
//
//      公私钥都能签名(小数据加密)，但是:
//         私钥签名 <--> 公钥解签
//         公钥签名 <--> 私钥解签

class EncryptTool
{
  public:
    // 对大文件进行对称加密.使用secret_key对in_file文件加密，再使用in_key_file对secret_key加密，再base64编码再stdout
    // in_file:需要加密的文件
    // out_file:加密后的文件
    // secret_key:对称加密秘钥
    // in_key_file:加密用的非对称秘钥(公私钥)
    bool encrypt(const std::string &in_file, const std::string &out_file, const std::string &secret_key,
                 const std::string &in_key_file);

    // 对大文件进行对称解密，现解码base64_sign得到密文，再通过in_key_file得到对称加密秘钥明文，再解密in_file文件到out_file。成功后，对称加密明文输出到stdout
    // in_file:需要解密的文件
    // out_file:解密后的文件
    // base64_sign: 加密的md5值(既使用非对称秘钥加密的md5值)
    // in_key_file:解密用的非对称秘钥(公私钥)
    bool decryption(const std::string &in_file, const std::string &out_file, const std::string &base64_sign,
                    const std::string &in_key_file);

  private:
    // 对文件进行签名(其实就是对指定小数据进行加解密)
    // secret_key:需要签名的文件的md5值信息
    // sign_base64:签名信息的base64编码
    // in_key_file:公私秘钥
    bool signature(const std::string &secret_key, std::string &sign_base64, const std::string &in_key_file);
    // 对文件进行反签名(其实就是对指定小数据进行加解密)
    // sign_base64:需要反签名的文件的签名信息的base64编码
    // secret_key:文件的md5值
    // in_key_file:公私秘钥
    bool unsignature(const std::string &sign_base64, std::string &secret_key, const std::string &in_key_file);

    // 对称加密
    bool symmetrical_encryption(const std::string &in_file, const std::string &out_file, const std::string &key);
    // 对称解密
    bool symmetrical_decryption(const std::string &in_file, const std::string &out_file, const std::string &key);

  public:
    // 判断秘钥是否是私钥
    bool is_private_key(const std::string &in_key_file);

  private:
};

#endif // ENCRYPT_TOOL_H