
set(simple_example_src
simple_example.cpp
)

add_executable(simple_example ${simple_example_src})
target_link_libraries(simple_example ${libcurvecpr_asio_external_libraries})

set(simple_server_src
simple_server.cpp
)

add_executable(simple_server ${simple_server_src})
target_link_libraries(simple_server ${libcurvecpr_asio_external_libraries})
