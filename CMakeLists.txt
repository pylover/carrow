cmake_minimum_required(VERSION 3.7)
project(carrow 
    VERSION 5.2.0
    LANGUAGES C
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fms-extensions -D_GNU_SOURCE=1")


configure_file(manifest.h.in manifest.h)
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)


add_library(carrow STATIC carrow.c carrow.h)


# Install
install(TARGETS carrow DESTINATION "lib")
install(FILES carrow.h DESTINATION "include")
install(FILES carrow_generic.h DESTINATION "include")
install(FILES carrow_generic.c DESTINATION "include")


# CPack
set(CPACK_PACKAGE_FILE_NAME "libcarrow-${PROJECT_VERSION}")
set(CPACK_SET_DESTDIR true)
set(CPACK_PACKAGE_NAME libcarrow)
set(CPACK_PACKAGE_CONTACT "Vahid Mardani <vahid.mardani@gmail.com>")
set(CPACK_GENERATOR DEB)
include(CPack)


# Testing
if (NOT DEFINED ENV{SKIP_TESTS})
include(CTest)
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")


# Valgrind, Profiling
add_custom_target(profile 
    DEPENDS 
    profile_test_carrow
)

# Test carrow
add_executable(test_carrow 
    $<TARGET_OBJECTS:carrow>
    test_carrow.c)
target_include_directories(test_carrow PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(test_carrow PUBLIC clog)
add_test(NAME test_carrow COMMAND test_carrow)
add_custom_target(profile_test_carrow
    COMMAND "valgrind" 
    ${VALGRIND_FLAGS}
    $<TARGET_FILE:test_carrow>
)
endif()


# Examples
if (NOT DEFINED ENV{SKIP_EXAMPLES})
add_subdirectory(examples)
endif()
