aux_source_directory(. CPP_MONITOR)
add_library(monitor_manager ${CPP_MONITOR})
include_directories(${PROJECT_SOURCE_DIR}/src/base)
target_link_libraries(monitor_manager
    PRIVATE
)