# add_definitions(-DTASK_SCHEDULER_COUNT=1)

aux_source_directory(. CPP_ENCRYPT_TOOL)
include_directories(${PROJECT_SOURCE_DIR}/src/base)
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)
include_directories(${PROJECT_SOURCE_DIR}/third_party/include/argparse)

link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(encrypt_tool ${CPP_ENCRYPT_TOOL})
target_link_libraries(encrypt_tool PUBLIC 
    pthread 
    base
)
