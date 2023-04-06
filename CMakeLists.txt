cmake_minimum_required(VERSION 3.7)
project(carrow 
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fms-extensions -D_GNU_SOURCE=1")


configure_file(manifest.h.in manifest.h)
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)


add_library(evloop OBJECT evloop.c evloop.h)


# Install
install(FILES carrow.h DESTINATION "include")
install(FILES carrow.c DESTINATION "include")


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


# Valgrind, Profiling
add_custom_target(profile 
    DEPENDS 
    profile_test_carrow
)
set(VALGRIND_FLAGS
    -s
    --tool=memcheck 
    --leak-check=yes 
    --show-reachable=yes 
    --num-callers=20 
    --track-fds=yes 
)


# Test carrow
add_executable(test_carrow 
    $<TARGET_OBJECTS:evloop>
    test_carrow.c)
target_include_directories(test_carrow PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(test_carrow PUBLIC clog)
add_test(NAME test_carrow COMMAND test_carrow)
add_custom_target(profile_test_carrow
    COMMAND "valgrind" 
    ${VALGRIND_FLAGS}
    $<TARGET_FILE:test_carrow>
)


# Examples
add_subdirectory(examples)
