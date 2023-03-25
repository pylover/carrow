cmake_minimum_required(VERSION 3.7)
project(carrow 
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fms-extensions -D_GNU_SOURCE=1")
set(CMAKE_BUILD_TYPE "debug")


configure_file(manifest.h.in manifest.h)
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)

add_library(carrow OBJECT carrow.c)
target_include_directories(carrow PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(carrow PUBLIC m clog)


# Testing
include(CTest)
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")


add_executable(test_carrow
    $<TARGET_OBJECTS:carrow> 
    test_carrow.c)
target_link_libraries(test_carrow PUBLIC m clog)
add_test(NAME test_carrow COMMAND test_carrow)
