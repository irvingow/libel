set (http_SRCS
        http_context.cpp
        http_response.cpp
        http_server.cpp)

add_library(libel_http ${http_SRCS})
target_link_libraries(libel_http libel_net)

install(TARGETS libel_http DESTINATION lib)
set(HEADERS
        http_context.h
        http_response.h
        http_server.h
        http_request.h)

add_executable(http_server_test tests/http_server_test.cpp)
target_link_libraries(http_server_test libel_http)
add_executable(http_request_test tests/http_request_test.cpp)
target_link_libraries(http_request_test libel_http)
