add_custom_command(OUTPUT rpc.pb.cc rpc.pb.h
        COMMAND protoc
        ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto -I${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS rpc.proto
        VERBATIM )

set_source_files_properties(rpc.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion")
include_directories(${PROJECT_BINARY_DIR})

add_library(libel_protorpc_wire rpc.pb.cc RpcCodec.cpp)
set_target_properties(libel_protorpc_wire PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")

 add_executable(protobuf_rpc_wire_test RpcCodec_test.cpp)
 target_link_libraries(protobuf_rpc_wire_test libel_protorpc_wire libel_protobuf_codec)
 set_target_properties(protobuf_rpc_wire_test PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")

add_library(libel_protorpc RpcChannel.cpp RpcServer.cpp)
set_target_properties(libel_protorpc PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(libel_protorpc libel_protorpc_wire libel_protobuf_codec libel_net protobuf z)

install(TARGETS libel_protorpc_wire libel_protorpc DESTINATION lib)