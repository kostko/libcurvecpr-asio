macro(install_headers_with_directory header_list)

foreach(header ${${header_list}})
string(REGEX MATCH "(.*)/" dir ${header})
install(FILES ${header} DESTINATION include/${dir})
endforeach(header)

endmacro(install_headers_with_directory)

set(libcurvecpr_asio_includes
curvecp/acceptor.hpp
curvecp/curvecp.hpp
curvecp/stream.hpp
curvecp/detail/accept_op.hpp
curvecp/detail/acceptor.hpp
curvecp/detail/basic_stream.hpp
curvecp/detail/client_stream.hpp
curvecp/detail/close_op.hpp
curvecp/detail/connect_op.hpp
curvecp/detail/io.hpp
curvecp/detail/read_op.hpp
curvecp/detail/server_stream.hpp
curvecp/detail/session.hpp
curvecp/detail/write_op.hpp
curvecp/detail/impl/acceptor.ipp
curvecp/detail/impl/client_stream.ipp
curvecp/detail/impl/server_stream.ipp
curvecp/detail/impl/session.ipp
)

install_headers_with_directory(libcurvecpr_asio_includes)
