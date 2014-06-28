/*
 * Copyright (C) 2014 Jernej Kos (jernej@kos.mx)
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef CURVECP_ASIO_ACCEPTOR_HPP
#define CURVECP_ASIO_ACCEPTOR_HPP

#include <curvecp/detail/acceptor.hpp>
#include <curvecp/detail/accept_op.hpp>
#include <curvecp/stream.hpp>

namespace curvecp {

/**
 * CurveCP server acceptor.
 */
class acceptor {
public:
  /**
   * Constructs a new CurveCP server acceptor.
   *
   * @param service ASIO IO service
   */
  acceptor(boost::asio::io_service &service)
    : acceptor_(boost::make_shared<detail::acceptor>(service))
  {
  }

  acceptor(const acceptor&) = delete;
  acceptor &operator=(const acceptor&) = delete;

  /**
   * Returns the ASIO IO service associated with this acceptor.
   */
  boost::asio::io_service &get_io_service() { return acceptor_->get_io_service(); }

  /**
   * Configures the local CurveCP extension. Must be set before starting
   * the connection.
   *
   * @param extension A 16-byte local extension
   */
  void set_local_extension(const std::string &extension) { acceptor_->set_local_extension(extension); }

  /**
   * Configures the local CurveCP public key. Must be set before starting
   * the connection.
   *
   * @param publicKey A 32-byte local public key
   */
  void set_local_public_key(const std::string &publicKey) { acceptor_->set_local_public_key(publicKey); }

  /**
   * Configures the local CurveCP private key. Must be set before starting
   * the connection.
   *
   * @param privateKey A 32-byte local private key
   */
  void set_local_private_key(const std::string &privateKey) { acceptor_->set_local_private_key(privateKey); }

  /**
   * Configures the secure nonce generator. Must be set before listening.
   *
   * @param generator A valid NonceGenerator
   */
  template <typename NonceGenerator>
  void set_nonce_generator(NonceGenerator generator) { acceptor_->set_nonce_generator(generator); }

  /**
   * Binds the underlying UDP socket to a specific local endpoint.
   *
   * @param endpoint Endpoint to bind to
   */
  void bind(const detail::basic_stream::endpoint_type &endpoint) { acceptor_->bind(endpoint); }

  /**
   * Starts to listen for new connections.
   */
  void listen() { acceptor_->listen(); }

  /**
   * Returns the endpoint to which the local socket is bound.
   */
  detail::basic_stream::endpoint_type local_endpoint() const { return acceptor_->local_endpoint(); };

  /**
   * Performs an accept operation.
   */
  template <typename AcceptHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void (boost::system::error_code))
  async_accept(stream &peer,
               BOOST_ASIO_MOVE_ARG(AcceptHandler) handler)
  {
    boost::asio::detail::async_result_init<
      AcceptHandler, void (boost::system::error_code)> init(
        BOOST_ASIO_MOVE_CAST(AcceptHandler)(handler));

    detail::accept_op<curvecp::detail::acceptor, curvecp::stream, decltype(init.handler)>(
      *acceptor_, peer, init.handler)(boost::system::error_code(), true);
    return init.result.get();
  }
private:
  /// Private acceptor implementation
  boost::shared_ptr<detail::acceptor> acceptor_;
};

}

#endif
