/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_STREAM_HPP
#define CURVECP_ASIO_STREAM_HPP

#include <curvecp/detail/client_stream.hpp>
#include <curvecp/detail/read_op.hpp>
#include <curvecp/detail/write_op.hpp>
#include <curvecp/detail/close_op.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/async_result.hpp>

namespace curvecp {

namespace detail {
  class acceptor;
}

/**
 * CurveCP client stream.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
class stream {
public:
  friend class curvecp::detail::acceptor;

  /// CurveCP endpoint type
  typedef typename curvecp::detail::basic_stream::endpoint_type endpoint;

  /**
   * Constructs a CurveCP client stream.
   *
   * @param service ASIO IO service
   */
  stream(boost::asio::io_service &service)
    : stream_(boost::make_shared<detail::client_stream>(service))
  {
  }

  stream(const stream&) = delete;
  stream &operator=(const stream&) = delete;

  /**
   * Returns the ASIO IO service associated with this stream.
   */
  boost::asio::io_service &get_io_service() { return stream_->get_io_service(); }

  /**
   * Configures the local CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte local extension
   */
  void set_local_extension(const std::string &extension) { stream_->set_local_extension(extension); }

  /**
   * Configures the local CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte local public key
   */
  void set_local_public_key(const std::string &publicKey) { stream_->set_local_public_key(publicKey); }

  /**
   * Configures the local CurveCP private key. Must be set before starting
   * the connection.
   *
   * @param privateKey A 32-byte local private key
   */
  void set_local_private_key(const std::string &privateKey) { stream_->set_local_private_key(privateKey); }

  /**
   * Configures the remote CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte remote extension
   */
  void set_remote_extension(const std::string &extension) { stream_->set_remote_extension(extension); }

  /**
   * Configures the remote CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte remote public key
   */
  void set_remote_public_key(const std::string &publicKey) { stream_->set_remote_public_key(publicKey); }

  /**
   * Configures the remote domain name. Must be set before starting the
   * connection.
   *
   * @param domain A domain name
   */
  void set_remote_domain_name(const std::string &domain) { stream_->set_remote_domain_name(domain); }

  /**
   * Configures the secure nonce generator. Must be set before starting the
   * connection.
   *
   * @param generator A valid NonceGenerator
   */
  template <typename NonceGenerator>
  void set_nonce_generator(NonceGenerator generator) { stream_->set_nonce_generator(generator); }

  /**
   * Binds the underlying UDP socket to a specific local endpoint.
   *
   * @param endpoint Endpoint to bind to
   */
  void bind(const typename detail::basic_stream::endpoint_type &endpoint) { stream_->bind(endpoint); }

  /**
   * Connects the underlying UDP socket with a specific remote endpoint and
   * starts the CurveCP connection.
   *
   * @param endpoint Endpoint to connect with
   */
  void connect(const typename detail::basic_stream::endpoint_type &endpoint) { stream_->connect(endpoint); }

  /**
   * Performs a close operation on the stream.
   */
  template <typename CloseHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(CloseHandler, void())
  async_close(BOOST_ASIO_MOVE_ARG(CloseHandler) handler)
  {
    boost::asio::detail::async_result_init<
      CloseHandler, void()> init(
        BOOST_ASIO_MOVE_CAST(CloseHandler)(handler));

    stream_->async_io_operation(curvecp::detail::close_op(), init.handler);
    return init.result.get();
  }

  /**
   * Performs a read operation on the stream.
   */
  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void (boost::system::error_code, std::size_t))
  async_read_some(const MutableBufferSequence &buffers,
                  BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
  {
    // If you get an error on the following line it means that your handler does
    // not meet the documented type requirements for a ReadHandler.
    using boost::asio::handler_type;
    BOOST_ASIO_READ_HANDLER_CHECK(ReadHandler, handler) type_check;

    boost::asio::detail::async_result_init<
      ReadHandler, void (boost::system::error_code, std::size_t)> init(
        BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));

    stream_->async_io_operation(curvecp::detail::read_op<MutableBufferSequence>(buffers), init.handler);
    return init.result.get();
  }

  /**
   * Performs a write operation on the stream.
   */
  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (boost::system::error_code, std::size_t))
  async_write_some(const ConstBufferSequence &buffers,
                   BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
  {
    // If you get an error on the following line it means that your handler does
    // not meet the documented type requirements for a WriteHandler.
    using boost::asio::handler_type;
    BOOST_ASIO_WRITE_HANDLER_CHECK(WriteHandler, handler) type_check;

    boost::asio::detail::async_result_init<
      WriteHandler, void (boost::system::error_code, std::size_t)> init(
        BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));

    stream_->async_io_operation(curvecp::detail::write_op<ConstBufferSequence>(buffers), init.handler);
    return init.result.get();
  }
private:
  /// Private stream implementation
  boost::shared_ptr<detail::basic_stream> stream_;
};

}

#endif
