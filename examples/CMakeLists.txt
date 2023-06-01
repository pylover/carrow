add_library(addr OBJECT addr.c addr.h)
add_library(tty OBJECT tty.c tty.h)


set(VALGRIND_FLAGS
    -s
    --tool=memcheck 
    --leak-check=yes 
    --show-reachable=yes 
    --num-callers=20 
    --track-fds=yes 
)

add_executable(tcpclient
    $<TARGET_OBJECTS:carrow>
    $<TARGET_OBJECTS:addr>
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


add_executable(tcpserver
    $<TARGET_OBJECTS:carrow>
    $<TARGET_OBJECTS:addr>
    $<TARGET_OBJECTS:tty>
    tcpserver.c
)
target_include_directories(tcpserver PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(tcpserver PUBLIC clog mrb)
add_custom_target(run_tcpserver $<TARGET_FILE:tcpserver>)
add_custom_target(profile_tcpserver
    COMMAND "valgrind" 
    ${VALGRIND_FLAGS}
    $<TARGET_FILE:tcpserver>
)


add_executable(timer
    $<TARGET_OBJECTS:carrow>
    timer.c
)
target_include_directories(timer PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(timer PUBLIC clog mrb)
add_custom_target(run_timer $<TARGET_FILE:timer>)
add_custom_target(profile_timer
    COMMAND "valgrind" 
    ${VALGRIND_FLAGS}
    $<TARGET_FILE:timer>
)
