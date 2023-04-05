
add_executable(tcpclient)
target_sources(tcpclient PRIVATE tcpclient.c)
target_include_directories(tcpclient PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(tcpclient PUBLIC clog)

