cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

set (PROTO_FILES Proto/svg.proto Proto/graph.proto Proto/transport_router.proto Proto/map_renderer.proto Proto/transport_catalogue.proto)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})


set(TRANSPORT_CATALOGUE_FILES transport_catalogue.cpp transport_catalogue.h main.cpp)
set(MAP_RENDERER_FILES map_renderer.h map_renderer.cpp )
set (TRANSPORT_ROUTER_FILES router.h ranges.h graph.h transport_router.cpp transport_router.h)
set(DOMAIN_FILES domain.cpp domain.h geo.h)
set(SERIALIZATION_FILES serialization.cpp serialization.h)
set(JSON_FILES json_reader.h json_reader.cpp JSONlib/json.h JSONlib/json.cpp JSONlib/json_builder.cpp JSONlib/json_builder.h) 
set(SVG_FILES SvgLib/svg.cpp SvgLib/svg.h)
set(REQUEST_HANDLER_FILES request_handler.cpp request_handler.h)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TRANSPORT_CATALOGUE_FILES} ${MAP_RENDERER_FILES} ${TRANSPORT_ROUTER_FILES} ${DOMAIN_FILES} ${SERIALIZATION_FILES} ${JSON_FILES} ${SVG_FILES} ${REQUEST_HANDLER_FILES})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})



string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)