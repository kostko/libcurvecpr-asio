/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_STREAM_HPP
#define CURVECP_ASIO_DETAIL_STREAM_HPP

#include <curvecp/detail/curvecpr.h>
#include <curvecp/detail/session.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/udp.hpp>

#include <deque>

namespace curvecp {

namespace detail {

/**
 * Internal CurveCP stream handling implementation.
 */
class stream {
public:
  /// The endpoint type
  typedef typename boost::asio::ip::udp::socket::endpoint_type endpoint_type;

  /**
   * Constructs an internal CurveCP client stream implementation.
   *
   * @param service ASIO IO service
   */
  stream(boost::asio::io_service &service);

  /**
   * Returns the ASIO IO service associated with this stream.
   */
  boost::asio::io_service &get_io_service() { return socket_.get_io_service(); }

  /**
   * Configures the local CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte local extension
   */
  void set_local_extension(const std::string &extension);

  /**
   * Configures the local CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte local public key
   */
  void set_local_public_key(const std::string &publicKey);

  /**
   * Configures the local CurveCP private key. Must be set before starting
   * the connection.
   *
   * @param privateKey A 32-byte local private key
   */
  void set_local_private_key(const std::string &privateKey);

  /**
   * Configures the remote CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte remote extension
   */
  void set_remote_extension(const std::string &extension);

  /**
   * Configures the remote CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte remote public key
   */
  void set_remote_public_key(const std::string &publicKey);

  /**
   * Configures the remote domain name. Must be set before starting the
   * connection.
   *
   * @param domain A domain name
   */
  void set_remote_domain_name(const std::string &domain);

  /**
   * Configures the secure nonce generator. Must be set before starting the
   * connection.
   *
   * @param generator A valid NonceGenerator
   */
  template <typename NonceGenerator>
  void set_nonce_generator(NonceGenerator generator);

  /**
   * Binds the underlying UDP socket to a specific local endpoint.
   *
   * @param endpoint Endpoint to bind to
   */
  void bind(const endpoint_type &endpoint);

  /**
   * Connects the underlying UDP socket with a specific remote endpoint and
   * starts the CurveCP connection.
   *
   * @param endpoint Endpoint to connect with
   */
  void connect(const endpoint_type &endpoint);

  /**
   * Closes the stream.
   */
  void close();

  /**
   * Starts an async IO operation on this stream.
   *
   * @param op IO operation to perform
   * @param handler Handler to be called after operation completes
   */
  template <typename Operation, typename Handler>
  void async_io_operation(const Operation &op, Handler &handler);
protected:
  bool handle_upper_send(const unsigned char *buffer, std::size_t length);

  void handle_hello_timeout(const boost::system::error_code &error);

  void handle_ready_read();

  void handle_ready_write();

  void transmit_pending();

  void handle_lower_write(const boost::system::error_code &error, std::size_t bytes);

  void handle_lower_read(const boost::system::error_code &error, std::size_t bytes);
protected:
  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_send(struct curvecpr_client *client,
                         const unsigned char *buf,
                         size_t num);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_recv(struct curvecpr_client *client,
                         const unsigned char *buf,
                         size_t num);

  /**
   * Internal handler for libcurvecpr.
   */
  static int handle_next_nonce(struct curvecpr_client *client,
                               unsigned char *destination,
                               size_t num);
private:
  /// Transport UDP socket
  boost::asio::ip::udp::socket socket_;
  /// Client packet processor
  curvecpr_client client_;
  /// Session
  session session_;
  /// Transmit queue
  std::deque<std::vector<unsigned char>> transmit_queue_;
  /// Maximum transmit queue size
  size_t transmit_queue_maximum_;
  /// Receive buffer space
  std::vector<unsigned char> lower_recv_buffer_;
  /// Nonce generator
  std::function<void(unsigned char*, size_t)> nonce_generator_;
  /// Timer to resend hello packets when no cookie received
  boost::asio::deadline_timer hello_timed_out_;
};

}

}

#include <curvecp/detail/impl/stream.ipp>

#endif
