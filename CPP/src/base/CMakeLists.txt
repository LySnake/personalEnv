
aux_source_directory(. CPP_BASE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/third_party/include)

add_library(base ${CPP_BASE})
