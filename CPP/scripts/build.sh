#!/bin/bash

### Please set your TOOLCHAIN variable in ~/.bashrc
# if [ -z "$TOOLCHAIN" ]; then
#     echo "Error:变量 TOOLCHAIN 为空"
#     exit -1
# fi

cd `dirname $0`

rm -rf ../build
mkdir ../build
cd ../build

# cmake -DTOOLCHAIN=${TOOLCHAIN} -j8 ..
cmake ..

make -j4