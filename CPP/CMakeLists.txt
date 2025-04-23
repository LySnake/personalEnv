cmake_minimum_required(VERSION 3.10)

project(zhengze VERSION 0.1)

# 指定使用 C++17
set(CMAKE_CXX_STANDARD 17)
# 强制使用 C++17，不可回退，如果编译器不支持 C++ 17，直接报错
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# 格式化
find_program(CLANG_FORMAT clang-format)

if(CLANG_FORMAT)
# 添加一个 CMake 目标，用于格式化项目中的所有源文件
add_custom_target(format
    COMMAND ${CLANG_FORMAT} -i
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format on source files"
)
endif()

############################ GIT MESSAGE #######################################
# 查找Git工具（QUIET避免未安装时报错）
find_package(Git QUIET)

# 设置默认值
set(GIT_COMMIT_ID "unknown")
set(GIT_BRANCH_NAME "unknown")

# 如果Git存在且当前目录是仓库，获取真实信息
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET  # 忽略非Git仓库的错误
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH_NAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET  # 忽略非Git仓库的错误
    )
endif()

# 将信息传递给编译器
add_definitions(
    -DGIT_COMMIT_ID="${GIT_COMMIT_ID}"
    -DGIT_BRANCH_NAME="${GIT_BRANCH_NAME}"
)
################################################################################

############################ Toolchain settings #################################
if(DEFINED TOOLCHAIN) 
message("TOOLCHAIN: ${TOOLCHAIN}")

set(ZEROMQ_LIB libzmq_arm.a)

# 设置C编译器和C++编译器
set(CMAKE_C_COMPILER "${TOOLCHAIN}/bin/aarch64-none-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN}/bin/aarch64-none-linux-gnu-g++")

# 设置链接器
set(CMAKE_LINKER "${TOOLCHAIN}/bin/aarch64-none-linux-gnu-ld")

else()

set(ZEROMQ_LIB libzmq_x86.a)

endif()
################################################################################

# 设置不同模式下，编译后的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/lib")


add_compile_options(-Wall -Werror -g)

add_subdirectory(src)
