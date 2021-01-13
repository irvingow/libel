add_library(libel_protobuf_codec ProtobufCodecLite.cpp)
set_target_properties(libel_protobuf_codec PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(libel_protobuf_codec libel_net z protobuf)

install(TARGETS libel_protobuf_codec DESTINATION lib)