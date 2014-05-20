/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_DETAIL_ACCEPTOR_HPP
#define CURVECP_ASIO_DETAIL_ACCEPTOR_HPP

#include <curvecp/stream.hpp>
#include <curvecp/detail/session.hpp>
#include <curvecp/detail/basic_stream.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/udp.hpp>

#include <unordered_map>
#include <deque>
#include <mutex>

namespace curvecp {

namespace detail {

/**
 * Internal CurveCP server acceptor implementation.
 */
class acceptor : public boost::enable_shared_from_this<acceptor> {
public:
  /**
   * Constructs a new CurveCP server acceptor.
   *
   * @param service ASIO IO service
   */
  inline acceptor(boost::asio::io_service &service);

  acceptor(const acceptor&) = delete;
  acceptor &operator=(const acceptor&) = delete;

  /**
   * Returns the ASIO IO service associated with this acceptor.
   */
  boost::asio::io_service &get_io_service() { return socket_.get_io_service(); }

   /**
   * Configures the local CurveCP extension. Must be set before listening.
   *
   * @param extension A 16-byte local extension
   */
  inline void set_local_extension(const std::string &extension);

  /**
   * Configures the local CurveCP public key. Must be set before listening.
   *
   * @param publicKey A 32-byte local public key
   */
  inline void set_local_public_key(const std::string &publicKey);

  /**
   * Configures the local CurveCP private key. Must be set before listening.
   *
   * @param privateKey A 32-byte local private key
   */
  inline void set_local_private_key(const std::string &privateKey);

  /**
   * Configures the secure nonce generator. Must be set before listening.
   *
   * @param generator A valid NonceGenerator
   */
  template <typename NonceGenerator>
  void set_nonce_generator(NonceGenerator generator) { nonce_generator_ = generator; }

  /**
   * Binds the underlying UDP socket to a specific local endpoint.
   *
   * @param endpoint Endpoint to bind to
   */
  inline void bind(const typename detail::basic_stream::endpoint_type &endpoint);

  /**
   * Starts to listen for new connections.
   */
  inline void listen();

  /**
   * Performs a stream accept operation.
   *
   * @param stream Destination instance that will contain the accepted stream
   * @param error Error code
   * @return True if a new stream has been accepted, false if accept needs retry
   */
  inline bool accept(curvecp::stream &stream, boost::system::error_code &error);

  /**
   * Returns the endpoint to which the local socket is bound.
   */
  inline typename detail::basic_stream::endpoint_type local_endpoint() const;

  /**
   * Schedules a handler to be executed after the acceptor is ready for
   * accepting a new stream.
   *
   * @param handler Handler that should be called when ready
   */
  template <typename Handler>
  inline void async_pending_accept_wait(BOOST_ASIO_MOVE_ARG(Handler) handler);
protected:
  inline void handle_session_close(const std::string &sessionKey);

  inline void handle_upper_send(boost::shared_ptr<session> session,
                                const unsigned char *buffer,
                                std::size_t length);

  inline void handle_lower_read(const boost::system::error_code &error, std::size_t bytes);
protected:
  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_put_session(struct curvecpr_server *server,
                                       const struct curvecpr_session *s,
                                       void *priv,
                                       struct curvecpr_session **s_stored);
  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_get_session(struct curvecpr_server *server,
                                       const unsigned char their_session_pk[32],
                                       struct curvecpr_session **s_stored);

  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_send(struct curvecpr_server *server,
                                struct curvecpr_session *s,
                                void *priv,
                                const unsigned char *buf,
                                size_t num);
  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_recv(struct curvecpr_server *server,
                                struct curvecpr_session *s,
                                void *priv,
                                const unsigned char *buf,
                                size_t num);

  /**
   * Internal handler for libcurvecpr.
   */
  inline static int handle_next_nonce(struct curvecpr_server *server,
                                      unsigned char *destination,
                                      size_t num);
private:
  /// Mutex
  std::recursive_mutex mutex_;
  /// Dispatch strand
  boost::asio::strand strand_;
  /// Underlying UDP socket
  boost::asio::ip::udp::socket socket_;
  /// Maximum number of allowed pending sessions
  std::size_t maximum_pending_sessions_;
  /// Pending sessions waiting an accept call
  std::deque<boost::shared_ptr<session>> pending_sessions_;
  /// Session storage
  std::unordered_map<std::string, boost::shared_ptr<session>> sessions_;
  /// Server packet processor
  curvecpr_server server_;
  /// Receive endpoint
  boost::asio::ip::udp::endpoint lower_recv_endpoint_;
  /// Receive buffer space
  std::vector<unsigned char> lower_recv_buffer_;
  /// Pending ready accept timer
  boost::asio::deadline_timer pending_ready_accept_;
  /// Nonce generator
  std::function<void(unsigned char*, size_t)> nonce_generator_;
};

}

}

#include <curvecp/detail/impl/acceptor.ipp>

#endif
