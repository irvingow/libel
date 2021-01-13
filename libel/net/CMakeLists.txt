
set(net_SRCS
        acceptor.cpp
        buffer.cpp
        channel.cpp
        connector.cpp
        eventloop.cpp
        eventloop_thread.cpp
        eventloop_threadpool.cpp
        inet_address.cpp
        poller.cpp
        poller/default_poller.cpp
        poller/epoll_poller.cpp
        poller/poll_poller.cpp
        socket.cpp
        sockets_ops.cpp
        tcp_client.cpp
        tcp_connection.cpp
        tcp_server.cpp
        timer.cpp
        timerqueue.cpp
        )

add_library(libel_net ${net_SRCS})
target_link_libraries(libel_net libel_base)

add_subdirectory(http)
add_subdirectory(tests)

if (PROTOBUF_FOUND)
    add_subdirectory(protobuf)
    add_subdirectory(protorpc)
else()
    add_subdirectory(protobuf EXCLUDE_FROM_ALL)
    add_subdirectory(protorpc EXCLUDE_FROM_ALL)
endif()
