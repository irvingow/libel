include_directories(${PROJECT_BINARY_DIR})

add_executable(WebServer webserver.cpp)
set_target_properties(WebServer PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(WebServer libel_net libel_http)