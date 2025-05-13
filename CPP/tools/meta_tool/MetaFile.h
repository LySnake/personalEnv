#ifndef META_FILE_H
#define META_FILE_H

#include "Args.h"
#include "Define.h"

class MetaFile
{
  public:
    MetaFile(const Args &args) : args_{args} {}
    DEFAULT_COPY_AND_MOVE_OPERATOR(MetaFile);

    int handleFile();

  private:
    // 对输入的文件，附加指定的meta文件信息
    bool appendMetaFile();
    // 对输入的文件，读meta信息到stdout，可选还原到原文件
    bool parseFile();

  private:
    const Args &args_;
};

#endif // META_FILE_H