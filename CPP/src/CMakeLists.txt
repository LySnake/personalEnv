add_definitions(-DTASK_SCHEDULER_COUNT=1)

add_subdirectory(base)
add_subdirectory(monitor)


aux_source_directory(. CPP_MAIN)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/base)
include_directories(${PROJECT_SOURCE_DIR}/src/monitor)
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)

link_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(${PROJECT_SOURCE_DIR}/third_party/lib)

add_executable(zhengze ${CPP_MAIN})
target_link_libraries(zhengze PUBLIC 
    pthread 
    base
    monitor_manager
    spdlog
)
