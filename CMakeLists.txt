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


add_library(core OBJECT core.c)
add_library(addr OBJECT addr.c)
add_library(ev OBJECT ev.c)


# Static Library
add_library(carrow 
    STATIC 
    $<TARGET_OBJECTS:core> 
    $<TARGET_OBJECTS:ev> 
)
target_include_directories(carrow PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(carrow PUBLIC m clog)


# Install
install(TARGETS carrow DESTINATION "lib")
install(FILES carrow.h DESTINATION "include")


# CPack
set(CPACK_PACKAGE_FILE_NAME "libcarrow-${PROJECT_VERSION}")
set(CPACK_SET_DESTDIR true)
set(CPACK_PACKAGE_NAME libcarrow)
set(CPACK_PACKAGE_CONTACT "Vahid Mardani <vahid.mardani@gmail.com>")
set(CPACK_GENERATOR DEB)
include(CPack)


# Testing
include(CTest)
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")


# Valgrind
set(VALGRIND_FLAGS
    -s
    --tool=memcheck 
    --leak-check=yes 
    --show-reachable=yes 
    --num-callers=20 
    --track-fds=yes 
)


# Test carrow
add_executable(test_carrow test_carrow.c)
target_link_libraries(test_carrow PUBLIC carrow)
add_test(NAME test_carrow COMMAND test_carrow)
add_custom_target(profile_test_carrow
    COMMAND "valgrind" 
    ${VALGRIND_FLAGS}
    $<TARGET_FILE:test_carrow>
)


# Profiling
add_custom_target(profile 
    DEPENDS 
    profile_test_carrow
)


add_executable(tcpserver 
    tcpserver.c
    $<TARGET_OBJECTS:addr> 
)
target_link_libraries(tcpserver PUBLIC carrow)

