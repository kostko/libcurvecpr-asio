
set(simple_example_src
simple_example.cpp
)

add_executable(simple_example ${simple_example_src})
target_link_libraries(simple_example ${libcurvecpr_asio_external_libraries})
