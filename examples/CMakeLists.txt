
set(VALGRIND_FLAGS
    -s
    --tool=memcheck 
    --leak-check=yes 
    --show-reachable=yes 
    --num-callers=20 
    --track-fds=yes 
)

add_executable(tcpclient
    $<TARGET_OBJECTS:evloop>
    $<TARGET_OBJECTS:tcp>
    $<TARGET_OBJECTS:tty>
    tcpclient.c
)
target_include_directories(tcpclient PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(tcpclient PUBLIC clog mrb)
add_custom_target(run_tcpclient $<TARGET_FILE:tcpclient>)
add_custom_target(profile_tcpclient
    COMMAND "valgrind" 
    ${VALGRIND_FLAGS}
    $<TARGET_FILE:tcpclient>
)

