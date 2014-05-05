/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_BASIC_STREAM_HPP
#define CURVECP_ASIO_DETAIL_BASIC_STREAM_HPP

#include <curvecp/detail/session.hpp>
#include <curvecp/detail/io.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>

namespace curvecp {

namespace detail {

/**
 * Internal CurveCP stream handling implementation.
 */
class basic_stream {
public:
  /// The endpoint type
  typedef typename boost::asio::ip::udp::socket::endpoint_type endpoint_type;

  /**
   * Constructs an internal CurveCP client stream implementation.
   *
   * @param session Internal session reference
   */
  basic_stream(session &session)
    : ref_session_(session)
  {
  }

  basic_stream(const basic_stream&) = delete;
  basic_stream &operator=(const basic_stream&) = delete;

  /**
   * Returns the ASIO IO service associated with this stream.
   */
  virtual boost::asio::io_service &get_io_service() = 0;

  /**
   * Configures the local CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte local extension
   */
  virtual void set_local_extension(const std::string &extension)
  {}

  /**
   * Configures the local CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte local public key
   */
  virtual void set_local_public_key(const std::string &publicKey)
  {}

  /**
   * Configures the local CurveCP private key. Must be set before starting
   * the connection.
   *
   * @param privateKey A 32-byte local private key
   */
  virtual void set_local_private_key(const std::string &privateKey)
  {}

  /**
   * Configures the remote CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte remote extension
   */
  virtual void set_remote_extension(const std::string &extension)
  {}

  /**
   * Configures the remote CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte remote public key
   */
  virtual void set_remote_public_key(const std::string &publicKey)
  {}

  /**
   * Configures the remote domain name. Must be set before starting the
   * connection.
   *
   * @param domain A domain name
   */
  virtual void set_remote_domain_name(const std::string &domain)
  {}

  /**
   * Configures the secure nonce generator. Must be set before starting the
   * connection.
   *
   * @param generator A valid NonceGenerator
   */
  template <typename NonceGenerator>
  void set_nonce_generator(NonceGenerator generator)
  {
    nonce_generator_ = generator;
  }

  /**
   * Binds the underlying UDP socket to a specific local endpoint.
   *
   * @param endpoint Endpoint to bind to
   */
  virtual void bind(const endpoint_type &endpoint)
  {}

  /**
   * Connects the underlying UDP socket with a specific remote endpoint and
   * starts the CurveCP connection.
   *
   * @param endpoint Endpoint to connect with
   */
  virtual void connect(const endpoint_type &endpoint)
  {}

  /**
   * Starts an async IO operation on this stream.
   *
   * @param op IO operation to perform
   * @param handler Handler to be called after operation completes
   */
  template <typename Operation, typename Handler>
  void async_io_operation(const Operation &op, Handler &handler)
  {
    io_op<basic_stream, Operation, Handler>(ref_session_, *this, op, handler)(boost::system::error_code(), true);
  }
protected:
  /// Session
  session &ref_session_;
  /// Nonce generator
  std::function<void(unsigned char*, size_t)> nonce_generator_;
};

}

}

#endif
