add_library(addr OBJECT addr.c addr.h)
add_library(tty OBJECT tty.c tty.h)


list(APPEND examples
    tcpclient
    tcpserver
    timer
    sleep
)
set(profilerules ${examples})
list(TRANSFORM profilerules PREPEND profile_)


foreach (t IN LISTS examples) 
    # Test help
    add_executable(${t} 
        ${t}.c
        $<TARGET_OBJECTS:carrow>
        $<TARGET_OBJECTS:addr>
        $<TARGET_OBJECTS:tty>
    )
    target_include_directories(${t} PUBLIC "${PROJECT_BINARY_DIR}")
    target_link_libraries(${t} PUBLIC clog mrb)
    add_custom_target(${t}_exec COMMAND ${t})
    add_custom_target(${t}_profile
        COMMAND "valgrind" ${VALGRIND_FLAGS} ${t}
    )
endforeach()
