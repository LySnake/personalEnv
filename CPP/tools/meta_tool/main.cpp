#include <argparse/argparse.hpp>

#include "Define.h"
#include "File.h"
#include "log.h"

#include "Args.h"
#include "MetaFile.h"

int main(int argc, char *argv[])
{
    int exit_code = EXIT_FAILURE;
    argparse::ArgumentParser program("meta_tool", "1.0.0");

    //(必须)位置参数:输入的文件
    program.add_argument(KEY_INPUT_FILE).help("Processed file.");

    // 调试信息，仅仅开发时使用(否则干扰正常的stdout)
    program.add_argument(KEY_VERBOSE)
      .default_value(false)
      .implicit_value(true)
      .help("Print debug info. Do not use in the release environment.");

    // 需要附加的meta文件
    // 有：标识附加meta信息。
    // 没有:表示去除/输出meta信息或者原文件
    program.add_argument(KEY_META_FILE)
      .default_value(std::string(""))
      .help("Specify to add meta file information to the end of the file.");
    program.add_argument(KEY_OUTPUT_FILE)
      .default_value(Args::def_output_file)
      .help("specify the output file."); // 输出文件

    program.add_argument(KEY_IN_PLACE)
      .default_value(false)
      .implicit_value(true)
      .help("modify the original file directly."); // 直接作用在 KEY_INPUT_FILE指定的文件上

    try
    {
        program.parse_args(argc, argv);

        const std::string input = program.get<std::string>(KEY_INPUT_FILE);
        const std::string meta = program.get<std::string>(KEY_META_FILE);
        const std::string output = program.get<std::string>(KEY_OUTPUT_FILE);
        const bool verbose = program[KEY_VERBOSE] == true;
        const bool in_place = program[KEY_IN_PLACE] == true;

        const Args args(input, output, meta, in_place);

        init_spdlog("meta_tool", verbose);

        MetaFile meta_file(args);

        exit_code = meta_file.handleFile();
    }
    catch (const std::exception &err)
    {
        SPDLOG_ERROR("Catch exception: {}.", err.what());
    }

    deinit_spdlog();
    return exit_code;
}