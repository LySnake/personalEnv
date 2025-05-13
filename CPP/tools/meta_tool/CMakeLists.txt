# add_definitions(-DTASK_SCHEDULER_COUNT=1)

aux_source_directory(. CPP_META_TOOL)
include_directories(${PROJECT_SOURCE_DIR}/src/base)
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)

link_directories(${PROJECT_SOURCE_DIR}/lib)

add_executable(meta_tool ${CPP_META_TOOL})
target_link_libraries(meta_tool PUBLIC 
    pthread 
    base
)
