/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_CLIENT_STREAM_HPP
#define CURVECP_ASIO_DETAIL_CLIENT_STREAM_HPP

#include <curvecpr.h>

#include <curvecp/detail/session.hpp>
#include <curvecp/detail/basic_stream.hpp>

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
class client_stream : public basic_stream {
public:
  /**
   * Constructs an internal CurveCP client stream implementation.
   *
   * @param service ASIO IO service
   */
  inline client_stream(boost::asio::io_service &service);

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
  inline void set_local_extension(const std::string &extension);

  /**
   * Configures the local CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte local public key
   */
  inline void set_local_public_key(const std::string &publicKey);

  /**
   * Configures the local CurveCP private key. Must be set before starting
   * the connection.
   *
   * @param privateKey A 32-byte local private key
   */
  inline void set_local_private_key(const std::string &privateKey);

  /**
   * Configures the remote CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte remote extension
   */
  inline void set_remote_extension(const std::string &extension);

  /**
   * Configures the remote CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte remote public key
   */
  inline void set_remote_public_key(const std::string &publicKey);

  /**
   * Configures the remote domain name. Must be set before starting the
   * connection.
   *
   * @param domain A domain name
   */
  inline void set_remote_domain_name(const std::string &domain);

  /**
   * Binds the underlying UDP socket to a specific local endpoint.
   *
   * @param endpoint Endpoint to bind to
   */
  inline void bind(const endpoint_type &endpoint) override;

  /**
   * Connects the underlying UDP socket with a specific remote endpoint and
   * starts the CurveCP connection.
   *
   * @param endpoint Endpoint to connect with
   * @param ec Resulting error code
   * @return True when connect has been completed, false when it must be retried
   */
  inline bool connect(const endpoint_type &endpoint,
                      boost::system::error_code &ec) override;
protected:
  inline void handle_upper_send(const unsigned char *buffer, std::size_t length);

  inline void handle_hello_timeout(const boost::system::error_code &error);

  inline void handle_lower_read(const boost::system::error_code &error, std::size_t bytes);
protected:
  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_send(struct curvecpr_client *client,
                                const unsigned char *buf,
                                size_t num);

  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_recv(struct curvecpr_client *client,
                                const unsigned char *buf,
                                size_t num);

  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_next_nonce(struct curvecpr_client *client,
                                      unsigned char *destination,
                                      size_t num);
private:
  session session_;
  /// Transport UDP socket
  boost::asio::ip::udp::socket socket_;
  /// Client packet processor
  curvecpr_client client_;
  /// Receive buffer space
  std::vector<unsigned char> lower_recv_buffer_;
  /// Timer to resend hello packets when no cookie received
  boost::asio::deadline_timer hello_timed_out_;
  /// Hello retries
  int hello_retries_;
};

}

}

#include <curvecp/detail/impl/client_stream.ipp>

#endif
