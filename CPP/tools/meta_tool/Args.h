#ifndef ARGS_H
#define ARGS_H

#include <string>

#include "Define.h"

inline const std::string KEY_INPUT_FILE = "input";     // 必要的参数 - 输入的文件
inline const std::string KEY_VERBOSE = "--verbose";    // 是否调试
inline const std::string KEY_META_FILE = "--metafile"; // 指定的meta文件
inline const std::string KEY_OUTPUT_FILE = "--output"; // 处理后的文件(要与input不同)
inline const std::string KEY_IN_PLACE = "--in-place";  // 不处理 --output文件，直接在input文件上操作

class Args
{
  public:
    static const std::string def_output_file;

  public:
    Args(std::string input, std::string output, std::string meta, const bool is_in_place = false)
        : is_in_place_{is_in_place}, input_file_{std::move(input)}, output_file_{std::move(output)}, meta_file_{
                                                                                                       std::move(meta)}
    {
    }
    DEFAULT_COPY_AND_MOVE_OPERATOR(Args);

  public:
    bool is_in_place() const { return is_in_place_; }
    bool have_outfile() const { return output_file_ != def_output_file; }
    bool have_meta() const { return !meta_file_.empty(); }

    const std::string &input_file() const { return input_file_; }
    const std::string &output_file() const { return output_file_; }
    const std::string &meta_file() const { return meta_file_; }

  private:
    bool is_in_place_;
    std::string input_file_;
    std::string output_file_;
    std::string meta_file_;
};

inline const std::string Args::def_output_file = "/dev/null";

#endif // ARGS_H