
add_executable(tcpclient
    $<TARGET_OBJECTS:evloop>
    $<TARGET_OBJECTS:tcp>
    tcpclient.c
)
target_include_directories(tcpclient PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(tcpclient PUBLIC clog)
add_custom_target(run_tcpclient $<TARGET_FILE:tcpclient>)
add_custom_target(profile_tcpclient
    COMMAND "valgrind" 
    ${VALGRIND_FLAGS}
    $<TARGET_FILE:tcpclient>
)

